/* 
 * File:   AppleCoreaudioBackend.hpp
 * Author: ivan
 *
 * Created on February 3, 2016, 9:36 PM
 */

#pragma once

#include "../Exc.hpp"
#include "../AudioFormat.hpp"
#include "../Listener.hpp"

//#include <CoreAudio/CoreAudio.h>
#include <AudioUnit/AudioUnit.h>

namespace audout{

class AppleCoreaudioBackend{
	
	struct AudioComponent{
		AudioComponentInstance instance;
		
		AudioComponent(){
			//open the default audio device
			AudioComponentDescription desc;
			desc.componentType = kAudioUnitType_Output;
			desc.componentSubType = kAudioUnitSubType_DefaultOutput;
			desc.componentFlags = 0;
			desc.componentFlagsMask = 0;
			desc.componentManufacturer = kAudioUnitManufacturer_Apple;

			auto component = AudioComponentFindNext(nullptr, &desc);
			if(!component){
				throw audout::Exc("Failed to find default audio device");
			}

			if(AudioComponentInstanceNew(component, &this->instance)){
				throw audout::Exc("Failed to open default audio device");
			}
		}
		
		~AudioComponent()noexcept{
			AudioComponentInstanceDispose(this->instance);
		}
	} audioComponent;
	
public:
	AppleCoreaudioBackend(
			audout::AudioFormat outputFormat,
			std::uint32_t bufferSizeFrames,
			audout::Listener* listener
		)
	{
		//TODO:
	}
	
	void setPaused(bool paused){
		//TODO:
	}
};

}
