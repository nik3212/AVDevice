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

#include <IOKit/IOLib.h> // IOLog

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
    
    if (!createAudioEngine()) {
        return false;
    }
    
    return true;
}


