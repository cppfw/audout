/*
The MIT License (MIT)

Copyright (c) 2016-2023 Ivan Gagis <igagis@gmail.com>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

/* ================ LICENSE END ================ */

#pragma once

#include <utki/config.hpp>
#include <utki/destructable.hpp>

#include "../format.hpp"

#include <AudioUnit/AudioUnit.h>

namespace{

class audio_backend : public utki::destructable{

	struct AudioComponent{
		AudioComponentInstance instance;

		AudioComponent(){
			//open the default audio device
			AudioComponentDescription desc;
			desc.componentType = kAudioUnitType_Output;
#if CFG_OS_NAME == CFG_OS_NAME_IOS
			desc.componentSubType = kAudioUnitSubType_GenericOutput;
#else
			desc.componentSubType = kAudioUnitSubType_DefaultOutput;
#endif
			desc.componentFlags = 0;
			desc.componentFlagsMask = 0;
			desc.componentManufacturer = kAudioUnitManufacturer_Apple;

			auto component = AudioComponentFindNext(nullptr, &desc);
			if(!component){
				throw std::runtime_error("Failed to find default audio device");
			}

			if(AudioComponentInstanceNew(component, &this->instance)){
				throw std::runtime_error("Failed to open default audio device");
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
		auto listener = reinterpret_cast<audout::listener*>(inRefCon);

		for(unsigned i = 0; i != ioData->mNumberBuffers; ++i){
			auto& buf = ioData->mBuffers[i];
//			TRACE(<< "num channels = " << buf.mNumberChannels << std::endl)
			ASSERT(buf.mDataByteSize % sizeof(std::int16_t) == 0)
			listener->fill(utki::make_span(
					reinterpret_cast<std::int16_t*>(buf.mData),
					buf.mDataByteSize / sizeof(std::int16_t)
				));
		}

		return 0;
	}

public:
	audio_backend(
			audout::format outputFormat,
			std::uint32_t bufferSizeFrames,
			audout::listener* listener
		)
	{
		if(AudioUnitInitialize(this->audioComponent.instance)){
			throw std::runtime_error("Failed to initialize audio unit instance");
		}

		AudioStreamBasicDescription formatDesc;
		formatDesc.mSampleRate = outputFormat.frequency();
		formatDesc.mFormatID = kAudioFormatLinearPCM;
		formatDesc.mFormatFlags = kAudioFormatFlagIsSignedInteger;
		formatDesc.mFramesPerPacket = 1;
		formatDesc.mChannelsPerFrame = outputFormat.num_channels();
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
			throw std::runtime_error("Failed to set audio unit input property");
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
			throw std::runtime_error("Unable to attach an IOProc to the selected audio unit");
		}

		this->setPaused(false);
	}

	~audio_backend()noexcept{
		this->setPaused(true);
	}

	void setPaused(bool paused){
		if(paused){
			AudioOutputUnitStop(this->audioComponent.instance);
		}else{
			if(AudioOutputUnitStart(this->audioComponent.instance)){
				throw std::runtime_error("Unable to start audio unit");
			}
		}
	}
};

}
