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

void AVAudioEngine::free() {
    if (fOutputBuffer) {
        IOFree(fOutputBuffer, kAudioSampleBufferSize);
        fOutputBuffer = NULL;
    }
    
    if (fInputBuffer) {
        IOFree(fInputBuffer, kAudioSampleBufferSize);
        fInputBuffer = NULL;
    }
    
    if (fAudioInterruptSource) {
        fAudioInterruptSource->release();
        fAudioInterruptSource = NULL;
    }
    
    IOAudioEngine::free();
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

IOReturn AVAudioEngine::performFormatChange(IOAudioStream *audioStream,
                                            const IOAudioStreamFormat *newFormat,
                                            const IOAudioSampleRate *newSampleRate) {
    IOLog("AVAudioEngine[%p]::performFormatChange(%p, %p, %p)\n", this, audioStream, newFormat, newSampleRate);
    
    if (newSampleRate) {
        switch (newSampleRate->whole) {
            case 44100:
                IOLog("/t-> 44.1kHz selected\n");
                break;
            case 48000:
                IOLog("/t-> 48kHz selected\n");
                break;
            default:
                IOLog("/t Internal Error - unknown sample rate selected.\n");
                break;
        }
    }
    
    return kIOReturnSuccess;
}

IOReturn AVAudioEngine::clipOutputSamples(const void *mixBuf,
                                          void *sampleBuf,
                                          UInt32 firstSampleFrame,
                                          UInt32 numSamplesFrame,
                                          const IOAudioStreamFormat *streamFormat,
                                          IOAudioStream *audioStream) {
    UInt32 sampleIndex, maxSampleIndex;
    float* floatMixBuf;
    SInt16* outputBuf;
    
    floatMixBuf = (float*)mixBuf;
    outputBuf = (SInt16*)sampleBuf;
    
    maxSampleIndex = (firstSampleFrame + numSamplesFrame) * streamFormat->fNumChannels;
    
    for (sampleIndex = (firstSampleFrame * streamFormat->fNumChannels); sampleIndex < maxSampleIndex; sampleIndex++) {
        float inSample;
        
        inSample = floatMixBuf[sampleIndex];
        
        if (inSample > 1.0) {
            inSample = 1.0;
        } else if (inSample < -1.0) {
            inSample = -1.0;
        }
        
        if (inSample >= 0) {
            outputBuf[sampleIndex] = (SInt16)(inSample * 32767.0);
        } else {
            outputBuf[sampleIndex] = (SInt16)(inSample * 32768.0);
        }
    }
    
    return kIOReturnSuccess;
}

IOReturn AVAudioEngine::convertInputSamples(const void *sampleBuf, void *destBuf, UInt32 firstSampleFrame, UInt32 numSampleFrames, const IOAudioStreamFormat *streamFormat, IOAudioStream *audioStream) {
    UInt32 numSamplesLeft;
    float *floatDestBuf;
    SInt16 *inputBuf;

    floatDestBuf = (float *)destBuf;

    inputBuf = &(((SInt16 *)sampleBuf)[firstSampleFrame * streamFormat->fNumChannels]);
    
    numSamplesLeft = numSampleFrames * streamFormat->fNumChannels;

    while (numSamplesLeft > 0) {
        SInt16 inputSample;

        inputSample = *inputBuf;

        if (inputSample >= 0) {
            *floatDestBuf = inputSample / 32767.0f;
        } else {
            *floatDestBuf = inputSample / 32768.0f;
        }

        ++inputBuf;
        ++floatDestBuf;
        --numSamplesLeft;
    }
    
    return kIOReturnSuccess;
}

IOReturn AVAudioEngine::performAudioEngineStart() {
    UInt64 time, timeNS;
    
    IOLog("AVAudioEngine[%p]::performAudioEngineStart()\n", this);
    
    fInterruptCount = 0;
    takeTimeStamp(false);
    
    fAudioInterruptSource->setTimeoutUS(kAudioInterruptInterval / 1000);
    
    clock_get_uptime(&time);
    absolutetime_to_nanoseconds(time, &timeNS);
    
    fNextTimeout = timeNS + kAudioInterruptInterval;
    
    return kIOReturnSuccess;
}

IOReturn AVAudioEngine::performAudioEngineStop() {
    IOLog("AVAudioEngine[%p]::performAudioEngineStop()\n", this);
    fAudioInterruptSource->cancelTimeout();
    return kIOReturnSuccess;
}

void AVAudioEngine::stop(IOService* provider) {
    IOLog("AVAudioEngine[%p]::stop()\n", this);
    
    if (fAudioInterruptSource) {
        fAudioInterruptSource->cancelTimeout();
        getWorkLoop()->removeEventSource(fAudioInterruptSource);
    }
    
    IOAudioEngine::stop(provider);
}

void AVAudioEngine::interruptOccured(OSObject *owner, IOTimerEventSource *sender) {
    UInt64 thisTimeNS;
    uint64_t time;
    SInt64 diff;
    
    AVAudioEngine* audioEngine = (AVAudioEngine*)owner;
    
    if (audioEngine) {
        audioEngine->handleAudioInterrupt();
    }
    
    if (!sender) {
        return;
    }
    
    clock_get_uptime(&time);
    absolutetime_to_nanoseconds(time, &thisTimeNS);
    diff = ((SInt64)audioEngine->fNextTimeout - (SInt64)thisTimeNS);
    
    sender->setTimeoutUS((UInt32)(((SInt64)kAudioInterruptInterval + diff) / 1000));
    audioEngine->fNextTimeout += kAudioInterruptInterval;
}

void AVAudioEngine::handleAudioInterrupt() {
    UInt32 bufferPosition = fInterruptCount % (kAudioInterruptHZ / 2);
    UInt32 sampleBytesPerInterrupt = (kAudioSampleRate / kAudioInterruptHZ) * (kAudioSampleWidth / 8) * kAudioNumChannels;
    UInt32 byteOffsetInBuffer = bufferPosition * sampleBytesPerInterrupt;
    
    UInt8* inputBuf = (UInt8*)fInputBuffer + byteOffsetInBuffer;
    UInt8* outputBuf = (UInt8*)fOutputBuffer + byteOffsetInBuffer;
    
    bcopy(outputBuf, inputBuf, sampleBytesPerInterrupt);
    
    if (bufferPosition == 0) {
        takeTimeStamp();
    }
    
    fInterruptCount++;
}

UInt32 AVAudioEngine::getCurrentSampleFrame() {
    UInt32 periodCount = (UInt32)fInterruptCount % (kAudioInterruptHZ / 2);
    UInt32 sampleFrame = periodCount * (kAudioSampleRate / kAudioInterruptHZ);
    return sampleFrame;
}
