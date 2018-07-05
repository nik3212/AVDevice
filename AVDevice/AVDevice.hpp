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

#ifndef _AVDEVICE_H__
#define _AVDEVICE_H__

#include <IOKit/audio/IOAudioDevice.h>

#define AVDevice com_osxkernel_AVDevice

class AVDevice: public IOAudioDevice {
    OSDeclareDefaultStructors(AVDevice)
    
    virtual bool initHardware(IOService* provider);
    bool createAudioEngine();
};

#endif
