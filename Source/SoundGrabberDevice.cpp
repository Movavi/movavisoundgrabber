/*
  File:SoundGrabberDevice.cpp

  Version:1.0.1
    ma++ ingalls  |  cycling '74  |  Copyright (C) 2004  |  soundflower.com
    
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License
    as published by the Free Software Foundation; either version 2
    of the License, or (at your option) any later version.
    
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    
    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

/*
	Movavi Sound Grabber is derived from Soundflower.
 
    Soundflower is derived from Apple's 'PhantomAudioDriver'
    sample code.  It uses the same timer mechanism to simulate a hardware
    interrupt, with some additional code to compensate for the software
    timer's inconsistencies.  
    
    Soundflower basically copies the mixbuffer and presents it to clients
    as an input buffer, allowing applications to send audio one another.
*/

#include "SoundGrabberDevice.h"
#include "SoundGrabberEngine.h"
#include <IOKit/audio/IOAudioControl.h>
#include <IOKit/audio/IOAudioLevelControl.h>
#include <IOKit/audio/IOAudioToggleControl.h>
#include <IOKit/audio/IOAudioDefines.h>
#include <IOKit/IOLib.h>

#define super IOAudioDevice

OSDefineMetaClassAndStructors(SoundGrabberDevice, IOAudioDevice)

// There should probably only be one of these? This needs to be 
// set to the last valid position of the log lookup table. 
const SInt32 SoundGrabberDevice::kVolumeMax = 99;
const SInt32 SoundGrabberDevice::kGainMax = 99;




bool SoundGrabberDevice::initHardware(IOService *provider)
{
    bool result = false;
    
	//IOLog("SoundGrabberDevice[%p]::initHardware(%p)\n", this, provider);
    
    if (!super::initHardware(provider))
        goto Done;
    
    setDeviceName("Movavi Sound Grabber");
    setDeviceShortName("MovaviSoundGrabber");
    setManufacturerName("Movavi");
    
    if (!createAudioEngines())
        goto Done;
    
    result = true;
    
Done:

    return result;
}


bool SoundGrabberDevice::createAudioEngines()
{
    OSArray*				audioEngineArray = OSDynamicCast(OSArray, getProperty(AUDIO_ENGINES_KEY));
    OSCollectionIterator*	audioEngineIterator;
    OSDictionary*			audioEngineDict;
	
    if (!audioEngineArray) {
        IOLog("SoundGrabberDevice[%p]::createAudioEngine() - Error: no AudioEngine array in personality.\n", this);
        return false;
    }
    
	audioEngineIterator = OSCollectionIterator::withCollection(audioEngineArray);
    if (!audioEngineIterator) {
		IOLog("SoundGrabberDevice: no audio engines available.\n");
		return true;
	}
    
    while (audioEngineDict = (OSDictionary*)audioEngineIterator->getNextObject()) {
		SoundGrabberEngine*	audioEngine = NULL;
		
        if (OSDynamicCast(OSDictionary, audioEngineDict) == NULL)
            continue;
        
		audioEngine = new SoundGrabberEngine;
        if (!audioEngine)
			continue;
        
        if (!audioEngine->init(audioEngineDict))
			continue;

		initControls(audioEngine);
        activateAudioEngine(audioEngine);	// increments refcount and manages the object
        audioEngine->release();				// decrement refcount so object is released when the manager eventually releases it
    }
	
    audioEngineIterator->release();
    return true;
}

bool SoundGrabberDevice::initControls(SoundGrabberEngine* audioEngine)
{
    for (UInt32 channel=0; channel <= NUM_CHANS; channel++) {
        mVolume[channel] = mGain[channel] = kVolumeMax;
        mMuteOut[channel] = mMuteIn[channel] = false;
    }
	
    return true;
}
