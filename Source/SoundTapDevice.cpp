/*
  File:SoundTapDevice.cpp

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
	Movavi Sound Tap is derived from Soundflower.
 
    Soundflower is derived from Apple's 'PhantomAudioDriver'
    sample code.  It uses the same timer mechanism to simulate a hardware
    interrupt, with some additional code to compensate for the software
    timer's inconsistencies.  
    
    Soundflower basically copies the mixbuffer and presents it to clients
    as an input buffer, allowing applications to send audio one another.
*/

#include "SoundTapDevice.h"
#include "SoundTapEngine.h"
#include <IOKit/audio/IOAudioControl.h>
#include <IOKit/audio/IOAudioLevelControl.h>
#include <IOKit/audio/IOAudioToggleControl.h>
#include <IOKit/audio/IOAudioDefines.h>
#include <IOKit/IOLib.h>

#define super IOAudioDevice

OSDefineMetaClassAndStructors(SoundTapDevice, IOAudioDevice)

// There should probably only be one of these? This needs to be 
// set to the last valid position of the log lookup table. 
const SInt32 SoundTapDevice::kVolumeMax = 99;
const SInt32 SoundTapDevice::kGainMax = 99;




bool SoundTapDevice::initHardware(IOService *provider)
{
    bool result = false;
    
	//IOLog("SoundTapDevice[%p]::initHardware(%p)\n", this, provider);
    
    if (!super::initHardware(provider))
        goto Done;
    
    setDeviceName("Movavi Sound Tap");
    setDeviceShortName("MovaviSoundTap");
    setManufacturerName("Movavi");
    
    if (!createAudioEngines())
        goto Done;
    
    result = true;
    
Done:

    return result;
}


bool SoundTapDevice::createAudioEngines()
{
    OSArray*				audioEngineArray = OSDynamicCast(OSArray, getProperty(AUDIO_ENGINES_KEY));
    OSCollectionIterator*	audioEngineIterator;
    OSDictionary*			audioEngineDict;
	
    if (!audioEngineArray) {
        IOLog("SoundTapDevice[%p]::createAudioEngine() - Error: no AudioEngine array in personality.\n", this);
        return false;
    }
    
	audioEngineIterator = OSCollectionIterator::withCollection(audioEngineArray);
    if (!audioEngineIterator) {
		IOLog("SoundTapDevice: no audio engines available.\n");
		return true;
	}
    
    while (audioEngineDict = (OSDictionary*)audioEngineIterator->getNextObject()) {
		SoundTapEngine*	audioEngine = NULL;
		
        if (OSDynamicCast(OSDictionary, audioEngineDict) == NULL)
            continue;
        
		audioEngine = new SoundTapEngine;
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


#define addControl(control, handler) \
    if (!control) {\
		IOLog("Movavi Sound Tap failed to add control.\n");	\
		return false; \
	} \
    control->setValueChangeHandler(handler, this); \
    audioEngine->addDefaultAudioControl(control); \
    control->release();

bool SoundTapDevice::initControls(SoundTapEngine* audioEngine)
{
    IOAudioControl*	control = NULL;
    
    for (UInt32 channel=0; channel <= NUM_CHANS; channel++) {
        mVolume[channel] = mGain[channel] = kVolumeMax;
        mMuteOut[channel] = mMuteIn[channel] = false;
    }
    
    const char *channelNameMap[NUM_CHANS+1] = {	kIOAudioControlChannelNameAll,
										kIOAudioControlChannelNameLeft,
										kIOAudioControlChannelNameRight,
										kIOAudioControlChannelNameCenter,
										kIOAudioControlChannelNameLeftRear,
										kIOAudioControlChannelNameRightRear,
										kIOAudioControlChannelNameSub };
	
    for (UInt32 channel=7; channel <= NUM_CHANS; channel++)
        channelNameMap[channel] = "Unknown Channel";
    
    for (unsigned channel=0; channel <= NUM_CHANS; channel++) {
		
        // Create an output volume control for each channel with an int range from 0 to 65535
        // and a db range from -72 to 0
        // Once each control is added to the audio engine, they should be released
        // so that when the audio engine is done with them, they get freed properly
		 
		 // Volume level notes.
		 //
		 // Previously, the minimum volume was actually 10*log10(1/65535) = -48.2 dB. In the new
		 // scheme, we use a size 100 lookup table to compute the correct log scaling. And set
		 // the minimum to -40 dB. Perhaps -50 dB would have been better, but this seems ok.
		 
        control = IOAudioLevelControl::createVolumeControl(SoundTapDevice::kVolumeMax,		// Initial value
                                                           0,									// min value
                                                           SoundTapDevice::kVolumeMax,		// max value
                                                           (-40 << 16) + (32768),				// -72 in IOFixed (16.16)
                                                           0,									// max 0.0 in IOFixed
                                                           channel,								// kIOAudioControlChannelIDDefaultLeft,
                                                           channelNameMap[channel],				// kIOAudioControlChannelNameLeft,
                                                           channel,								// control ID - driver-defined
                                                           kIOAudioControlUsageOutput);
        addControl(control, (IOAudioControl::IntValueChangeHandler)volumeChangeHandler);
        
        // Gain control for each channel
        control = IOAudioLevelControl::createVolumeControl(SoundTapDevice::kGainMax,			// Initial value
                                                           0,									// min value
                                                           SoundTapDevice::kGainMax,			// max value
                                                           0,									// min 0.0 in IOFixed
                                                           (40 << 16) + (32768),				// 72 in IOFixed (16.16)
                                                           channel,								// kIOAudioControlChannelIDDefaultLeft,
                                                           channelNameMap[channel],				// kIOAudioControlChannelNameLeft,
                                                           channel,								// control ID - driver-defined
                                                           kIOAudioControlUsageInput);
        addControl(control, (IOAudioControl::IntValueChangeHandler)gainChangeHandler);
    }
	
    // Create an output mute control
    control = IOAudioToggleControl::createMuteControl(false,									// initial state - unmuted
                                                      kIOAudioControlChannelIDAll,				// Affects all channels
                                                      kIOAudioControlChannelNameAll,
                                                      0,										// control ID - driver-defined
                                                      kIOAudioControlUsageOutput);
    addControl(control, (IOAudioControl::IntValueChangeHandler)outputMuteChangeHandler);
    
    // Create an input mute control
    control = IOAudioToggleControl::createMuteControl(false,									// initial state - unmuted
                                                      kIOAudioControlChannelIDAll,				// Affects all channels
                                                      kIOAudioControlChannelNameAll,
                                                      0,										// control ID - driver-defined
                                                      kIOAudioControlUsageInput);
    addControl(control, (IOAudioControl::IntValueChangeHandler)inputMuteChangeHandler);
    
    return true;
}


IOReturn SoundTapDevice::volumeChangeHandler(IOService *target, IOAudioControl *volumeControl, SInt32 oldValue, SInt32 newValue)
{
    IOReturn			result = kIOReturnBadArgument;
    SoundTapDevice*	audioDevice = (SoundTapDevice *)target;
	
    if (audioDevice)
        result = audioDevice->volumeChanged(volumeControl, oldValue, newValue);
    return result;
}


IOReturn SoundTapDevice::volumeChanged(IOAudioControl *volumeControl, SInt32 oldValue, SInt32 newValue)
{
    if (volumeControl)
         mVolume[volumeControl->getChannelID()] = newValue;
    return kIOReturnSuccess;
}


IOReturn SoundTapDevice::outputMuteChangeHandler(IOService *target, IOAudioControl *muteControl, SInt32 oldValue, SInt32 newValue)
{
    IOReturn			result = kIOReturnBadArgument;
    SoundTapDevice*	audioDevice = (SoundTapDevice*)target;
	
    if (audioDevice)
        result = audioDevice->outputMuteChanged(muteControl, oldValue, newValue);
    return result;
}


IOReturn SoundTapDevice::outputMuteChanged(IOAudioControl *muteControl, SInt32 oldValue, SInt32 newValue)
{
    if (muteControl)
         mMuteOut[muteControl->getChannelID()] = newValue;
    return kIOReturnSuccess;
}


IOReturn SoundTapDevice::gainChangeHandler(IOService *target, IOAudioControl *gainControl, SInt32 oldValue, SInt32 newValue)
{
    IOReturn			result = kIOReturnBadArgument;
    SoundTapDevice*	audioDevice = (SoundTapDevice *)target;
	
    if (audioDevice)
        result = audioDevice->gainChanged(gainControl, oldValue, newValue);
    return result;
}


IOReturn SoundTapDevice::gainChanged(IOAudioControl *gainControl, SInt32 oldValue, SInt32 newValue)
{
    if (gainControl)
		mGain[gainControl->getChannelID()] = newValue;
    return kIOReturnSuccess;
}


IOReturn SoundTapDevice::inputMuteChangeHandler(IOService *target, IOAudioControl *muteControl, SInt32 oldValue, SInt32 newValue)
{
    IOReturn			result = kIOReturnBadArgument;
    SoundTapDevice*	audioDevice = (SoundTapDevice*)target;

    if (audioDevice)
        result = audioDevice->inputMuteChanged(muteControl, oldValue, newValue);
    return result;
}


IOReturn SoundTapDevice::inputMuteChanged(IOAudioControl *muteControl, SInt32 oldValue, SInt32 newValue)
{
    if (muteControl)
         mMuteIn[muteControl->getChannelID()] = newValue;
    return kIOReturnSuccess;
}
