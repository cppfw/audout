/* 
 * File:   AppleCoreaudioBackend.hpp
 * Author: ivan
 *
 * Created on February 3, 2016, 9:36 PM
 */

#pragma once

#include <utki/config.hpp>

#include "../Exc.hpp"
#include "../AudioFormat.hpp"
#include "../Listener.hpp"

#include <AudioUnit/AudioUnit.h>

namespace audout{

class AppleCoreaudioBackend{

	struct AudioComponent{
		AudioComponentInstance instance;

		AudioComponent(){
			//open the default audio device
			AudioComponentDescription desc;
			desc.componentType = kAudioUnitType_Output;
#if M_OS_NAME == M_OS_NAME_IOS
			desc.componentSubType = kAudioUnitSubType_GenericOutput;
#else
			desc.componentSubType = kAudioUnitSubType_DefaultOutput;
#endif
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
			AudioUnitRenderActionFlags *ioActionFlags,
			const AudioTimeStamp *inTimeStamp,
			UInt32 inBusNumber,
			UInt32 inNumberFrames,
			AudioBufferList *ioData
		)
	{
		auto listener = reinterpret_cast<Listener*>(inRefCon);

		for(unsigned i = 0; i != ioData->mNumberBuffers; ++i){
			auto& buf = ioData->mBuffers[i];
//			TRACE(<< "num channels = " << buf.mNumberChannels << std::endl)
			ASSERT(buf.mDataByteSize % sizeof(std::int16_t) == 0)
			listener->fillPlayBuf(utki::wrapBuf(
					reinterpret_cast<std::int16_t*>(buf.mData),
					buf.mDataByteSize / sizeof(std::int16_t)
				));
		}

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
		formatDesc.mBytesPerPacket = formatDesc.mBytesPerFrame * formatDesc.mFramesPerPacket;

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
