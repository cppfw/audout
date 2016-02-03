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
	
	static OSStatus outputCallback(
			void *inRefCon,
			AudioUnitRenderActionFlags * ioActionFlags,
			const AudioTimeStamp * inTimeStamp,
			UInt32 inBusNumber, UInt32 inNumberFrames,
			AudioBufferList * ioData
		)
	{
		/*
		SDL_AudioDevice *this = (SDL_AudioDevice *) inRefCon;
		AudioBuffer *abuf;
		UInt32 remaining, len;
		void *ptr;
		UInt32 i;

		//Only do anything if audio is enabled and not paused
		if (!this->enabled || this->paused) {
			for (i = 0; i < ioData->mNumberBuffers; i++) {
				abuf = &ioData->mBuffers[i];
				SDL_memset(abuf->mData, this->spec.silence, abuf->mDataByteSize);
			}
			return 0;
		}

		// No SDL conversion should be needed here, ever, since we accept
		// any input format in OpenAudio, and leave the conversion to CoreAudio.
		
		//   SDL_assert(!this->convert.needed);
		//   SDL_assert(this->spec.channels == ioData->mNumberChannels);

		for (i = 0; i < ioData->mNumberBuffers; i++) {
			abuf = &ioData->mBuffers[i];
			remaining = abuf->mDataByteSize;
			ptr = abuf->mData;
			while (remaining > 0) {
				if (this->hidden->bufferOffset >= this->hidden->bufferSize) {
					// Generate the data
					SDL_LockMutex(this->mixer_lock);
					(*this->spec.callback)(this->spec.userdata,
								this->hidden->buffer, this->hidden->bufferSize);
					SDL_UnlockMutex(this->mixer_lock);
					this->hidden->bufferOffset = 0;
				}

				len = this->hidden->bufferSize - this->hidden->bufferOffset;
				if (len > remaining)
					len = remaining;
				SDL_memcpy(ptr, (char *)this->hidden->buffer +
						   this->hidden->bufferOffset, len);
				ptr = (char *)ptr + len;
				remaining -= len;
				this->hidden->bufferOffset += len;
			}
		}
*/
		return 0;
	}
	
public:
	AppleCoreaudioBackend(
			audout::AudioFormat outputFormat,
			std::uint32_t bufferSizeFrames,
			audout::Listener* listener
		)
	{
		if(AudioUnitInitialize(this->audioComponent.instance)){
			throw audout::Exc("Failed to initialize audio unit instance");
		}
		
		AudioStreamBasicDescription formatDesc;
		formatDesc.mSampleRate = outputFormat.frequency();
		formatDesc.mFormatID = kAudioFormatLinearPCM;
		formatDesc.mFormatFlags = kAudioFormatFlagIsSignedInteger;
		formatDesc.mFramesPerPacket = 1;
		formatDesc.mChannelsPerFrame = outputFormat.numChannels();
		formatDesc.mBitsPerChannel = 16;
		formatDesc.mBytesPerFrame = formatDesc.mChannelsPerFrame * 2;
		formatDesc.mBytesPerPacket = formatDesc.mBytesPerFrame;
		
		if(AudioUnitSetProperty(
				this->audioComponent.instance,
				kAudioUnitProperty_StreamFormat,
				kAudioUnitScope_Input,
				0,
				&formatDesc,
				sizeof(formatDesc)
			))
		{
			throw audout::Exc("Failed to set audio unit input property");
		}
		
		AURenderCallbackStruct callback;
		memset(&callback, 0, sizeof(callback));
		callback.inputProc = &outputCallback;
		callback.inputProcRefCon = listener;
		
		if(AudioUnitSetProperty(
				this->audioComponent.instance,
				kAudioUnitProperty_SetRenderCallback,
				kAudioUnitScope_Input,
				0,
				&callback,
				sizeof(callback)
			))
		{
			throw audout::Exc("Unable to attach an IOProc to the selected audio unit");
		}
		
		this->setPaused(false);
	}
	
	~AppleCoreaudioBackend()noexcept{
		this->setPaused(true);
	}
	
	void setPaused(bool paused){
		if(paused){
			AudioOutputUnitStop(this->audioComponent.instance);
		}else{
			if(AudioOutputUnitStart(this->audioComponent.instance)){
				throw audout::Exc("Unable to start audio unit");
			}
		}
	}
};

}
