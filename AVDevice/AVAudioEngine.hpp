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

#ifndef _AVAUDIODENGINE_H__
#define _AVAUDIODENGINE_H__

#include "AVDevice.hpp"

#include <IOKit/audio/IOAudioEngine.h>
#include <IOKit/IOLib.h> // IOLog

#define AVAudioEngine com_osxkernel_AVAudioEngine

class AVAudioEngine: public IOAudioEngine {
    OSDeclareDefaultStructors(AVAudioEngine)
public:
    virtual void free();
    
    virtual bool initHardware(IOService* provider);
    
    virtual IOAudioStream* createNewAudioStream(IOAudioStreamDirection direction,
                                                void* sampleBuffer, UInt32 sampleBufferSize);
    
    virtual void stop(IOService* provider);
    
    virtual IOReturn performAudioEngineStart();
    virtual IOReturn performAudioEngineStop();
    
    virtual UInt32 getCurrentSampleFrame();
    
    virtual IOReturn performFormatChange(IOAudioStream* audioStream, const IOAudioStreamFormat* newFormat,
                                         const IOAudioSampleRate* newSampleRate);
    
    virtual IOReturn clipOutputSamples(const void* mixBuf, void* sampleBuf, UInt32 firstSampleFrame,
                                       UInt32 numSamplesFrame, const IOAudioStreamFormat* streamFormat,
                                       IOAudioStream* audioStream);
    
    virtual IOReturn convertInputSamples(const void* sampleBuf, void* destBuf, UInt32 firstSampleFrame,
                                         UInt32 numSamplesFrame, const IOAudioStreamFormat* streamFormat,
                                         IOAudioStream* audioStream);
private:
    IOTimerEventSource* fAudioInterruptSource;
    SInt16* fOutputBuffer;
    SInt16* fInputBuffer;
    UInt32 fInterruptCount;
    SInt64 fNextTimeout;
    
    static void interruptOccured(OSObject* owner, IOTimerEventSource* sender);
    void handleAudioInterrupt();
};

#endif
