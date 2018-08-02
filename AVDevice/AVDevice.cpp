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

#include "AVDevice.hpp"
#include "AVAudioEngine.hpp"

#include <IOKit/IOLib.h> // IOLog

#include <IOKit/audio/IOAudioControl.h>
#include <IOKit/audio/IOAudioLevelControl.h>
#include <IOKit/audio/IOAudioToggleControl.h>
#include <IOKit/audio/IOAudioDefines.h>

/// This function is called by start() to provide a convenient place
/// for the subclass to perform its hardware initialization.
bool AVDevice::initHardware(IOService* provider) {
    IOLog("AVAudioDevice[%p]::initHardware(%p)\n", this, provider);
    
    if (!IOAudioDevice::initHardware(provider)) {
        return false;
    }
    
    setDeviceName("AVAudioDevice");
    setDeviceShortName("AVDevice");
    setManufacturerName("osxkernel.com");
    
    if (!createAudioEngine("AVAudio")) {
        return false;
    }
    
    return true;
}

bool AVDevice::createAudioEngine(const char* name) {
    AVAudioEngine* audioEngine = new AVAudioEngine();

    if (!audioEngine) {
        return false;
    }
    
    if (!audioEngine->init(NULL)) {
        return false;
    }
    
    audioEngine->setDescription(name);
    activateAudioEngine(audioEngine);
    audioEngine->release();
    
    return true;
}
