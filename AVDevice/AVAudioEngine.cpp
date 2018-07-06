////////////////////////////////////////////////////////////////////////////
//
// Copyright 2018 Nikita Vasilev.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
////////////////////////////////////////////////////////////////////////////

#include "AVAudioEngine.hpp"

#include <IOKit/IOTimerEventSource.h>

#define kAudioSampleRate 48000
#define kAudioNumChannels 2
#define kAudioSampleDepth 16
#define kAudioSampleWidth 16
#define kAudioBufferSampleFrames kAudioSampleRate / 2
#define kAudioSampleBufferSize (kAudioBufferSampleFrames * kAudioNumChannels * (kAudioSampleDepth / 8))

#define kAudioInterruptInterval 10000000
#define kAudioInterruptHZ 100

bool AVAudioEngine::initHardware(IOService *provider) {
    IOAudioSampleRate initialSampleRate;
    IOAudioStream* audioStream;
    IOWorkLoop* workLoop = NULL;
    
    IOLog("AVAudioEngine[%p]::initHardware(%p)\n", this, provider);
    
    if (!IOAudioEngine::initHardware(provider)) {
        return false;
    }

    fAudioInterruptSource = IOTimerEventSource::timerEventSource(this, interruptOccured);
    
    if (!fAudioInterruptSource) {
        return false;
    }
    
    workLoop = getWorkLoop();
    
    if (!workLoop) {
        return false;
    }
    
    if (workLoop->addEventSource(fAudioInterruptSource) != kIOReturnSuccess) {
        return false;
    }
    
    initialSampleRate.whole = kAudioSampleRate;
    initialSampleRate.fraction = 0;
    
    setNumSampleFramesPerBuffer(kAudioBufferSampleFrames);
    setInputSampleLatency(kAudioSampleRate / kAudioInterruptHZ);
    setOutputSampleOffset(kAudioSampleRate / kAudioInterruptHZ);
    
    fOutputBuffer = (SInt16*)IOMalloc(kAudioSampleBufferSize);
    
    if (!fOutputBuffer) {
        return false;
    }
    
    fInputBuffer = (SInt16*)IOMalloc(kAudioSampleBufferSize);
    
    if (!fInputBuffer) {
        return false;
    }
    
    audioStream = createNewAudioStream(kIOAudioStreamDirectionOutput,
                                       fOutputBuffer, kAudioSampleBufferSize);
    
    if (!audioStream) {
        return false;
    }
    
    addAudioStream(audioStream);
    audioStream->release();
    
    audioStream = createNewAudioStream(kIOAudioStreamDirectionOutput,
                                       fOutputBuffer, kAudioSampleBufferSize);
    
    if (!audioStream) {
        return false;
    }
    
    addAudioStream(audioStream);
    audioStream->release();
    
    return true;
}

IOAudioStream* AVAudioEngine::createNewAudioStream(IOAudioStreamDirection direction,
                                                   void *sampleBuffer, UInt32 sampleBufferSize) {
    IOAudioStream* audioStream = new IOAudioStream;
    
    if (audioStream) {
        if (!audioStream->initWithAudioEngine(this, direction, 1)) {
            audioStream->release();
        } else {
            IOAudioSampleRate rate;
            IOAudioStreamFormat format = {
                2,
                kIOAudioStreamSampleFormatLinearPCM,
                kIOAudioStreamNumericRepresentationSignedInt,
                kAudioSampleDepth,
                kAudioSampleWidth,
                kIOAudioStreamAlignmentHighByte,
                kIOAudioStreamByteOrderBigEndian,
                true,
                0
            };
            
            audioStream->setSampleBuffer(sampleBuffer, sampleBufferSize);
            rate.fraction = 0;
            rate.whole = kAudioSampleRate;
            audioStream->addAvailableFormat(&format, &rate, &rate);
            audioStream->setFormat(&format);
        }
    }
    
    return audioStream;
}
