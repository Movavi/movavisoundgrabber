/*
  File:SoundGrabberDevice.h

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

#pragma once

#include <IOKit/audio/IOAudioDevice.h>

#define AUDIO_ENGINES_KEY				"AudioEngines"
#define DESCRIPTION_KEY					"Description"
#define BLOCK_SIZE_KEY					"BlockSize"
#define NUM_BLOCKS_KEY					"NumBlocks"
#define NUM_STREAMS_KEY					"NumStreams"
#define FORMATS_KEY						"Formats"
#define SAMPLE_RATES_KEY				"SampleRates"
#define SEPARATE_STREAM_BUFFERS_KEY		"SeparateStreamBuffers"
#define SEPARATE_INPUT_BUFFERS_KEY		"SeparateInputBuffers"
#define SoundGrabberDevice				com_movavi_driver_SoundGrabberDevice
#define SoundGrabberEngine				com_movavi_driver_SoundGrabberEngine

class SoundGrabberEngine;

class SoundGrabberDevice : public IOAudioDevice
{
    OSDeclareDefaultStructors(SoundGrabberDevice)
    friend class SoundGrabberEngine;
    
	// class members
	
    static const SInt32 kVolumeMax;
    static const SInt32 kGainMax;

	
	// instance members

    SInt32 mVolume[NUM_CHANS+1];
    SInt32 mMuteOut[NUM_CHANS+1];
    SInt32 mMuteIn[NUM_CHANS+1];
    SInt32 mGain[NUM_CHANS+1];

	
	// methods
	
    virtual bool initHardware(IOService *provider);
    virtual bool createAudioEngines();
    virtual bool initControls(SoundGrabberEngine *audioEngine);
    
};
