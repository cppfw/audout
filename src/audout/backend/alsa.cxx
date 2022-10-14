/*
The MIT License (MIT)

Copyright (c) 2016-2021 Ivan Gagis <igagis@gmail.com>

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

// use the newer ALSA API
#define ALSA_PCM_NEW_HW_PARAMS_API
#include <alsa/asoundlib.h>

#include <utki/destructable.hpp>

#include "write_based.cxx"

#include "../player.hpp"

namespace{

class audio_backend :
		public write_based,
		public utki::destructable
{
	struct Device{
		snd_pcm_t *handle;
		
		Device(){
			// open PCM device for playback
			if(snd_pcm_open(&this->handle, "default" /*"hw:0,0"*/, SND_PCM_STREAM_PLAYBACK, 0) < 0){
//				TRACE(<< "ALSA: unable to open pcm device" << std::endl)
				throw std::runtime_error("ALSA: unable to open pcm device");
			}
		}
		
		~Device(){
			snd_pcm_close(this->handle);
		}
	} device;

	unsigned frame_size;
	
public:
	audio_backend(audout::format format, unsigned bufferSizeFrames, audout::listener* listener) :
			write_based(listener, bufferSizeFrames * format.num_channels()),
			frame_size(format.frame_size())
	{
//		TRACE(<< "setting HW params" << std::endl)

		this->SetHWParams(bufferSizeFrames, format);

//		TRACE(<< "setting SW params" << std::endl)

		this->SetSwParams(bufferSizeFrames); // must be called after this->SetHWParams()

		if(snd_pcm_prepare(this->device.handle) < 0){
//			TRACE(<< "cannot prepare audio interface for use" << std::endl)
			throw std::runtime_error("cannot set parameters");
		}
		
		this->startThread();
	}

	virtual ~audio_backend()throw(){
		this->stopThread();
	}

	int RecoverALSAFromXrun(int err){
		LOG([&](auto&o){o << "stream recovery" << std::endl;})
		if(err == -EPIPE){ // underrun
			err = snd_pcm_prepare(this->device.handle);
			if (err < 0){
				LOG(
						[&](auto&o){o << "Can't recovery from underrun, prepare failed, error code ="
						<< snd_strerror(err) << std::endl;}
					)
			}
			return 0;
		}else if(err == -ESTRPIPE){
			while((err = snd_pcm_resume(this->device.handle)) == -EAGAIN)
				std::this_thread::sleep_for(std::chrono::milliseconds(100)); // wait until the suspend flag is released
			if(err < 0){
				err = snd_pcm_prepare(this->device.handle);
				if (err < 0){
					LOG(
							[&](auto&o){o << "Can't recovery from suspend, prepare failed, error code ="
							<< snd_strerror(err) << std::endl;}
						)
				}
			}
			return 0;
		}
		return err;
	}

	void write(const utki::span<std::int16_t> buf)override{
		ASSERT(buf.size() % this->frame_size == 0)
		
		unsigned bufferSizeFrames = buf.size() / this->frame_size;
		
		unsigned numFramesWritten = 0;
		while(numFramesWritten < bufferSizeFrames){
			// write interleaved samples
			int ret = snd_pcm_writei(
					this->device.handle,
					reinterpret_cast<const void*>(&buf[numFramesWritten * this->frame_size]),
					bufferSizeFrames - numFramesWritten
				);
			if(ret < 0){
				if(ret == -EAGAIN)
					continue;

				int err = this->RecoverALSAFromXrun(ret);
				if(err < 0){
//					LOG(<< "write to audio interface failed, err = " << snd_strerror(err) << std::endl)
					throw std::runtime_error("write to audio interface failed");
				}
			}
			numFramesWritten += ret;
		}
	}

	void SetHWParams(unsigned bufferSizeFrames, audout::format format){
		struct HwParams{
			snd_pcm_hw_params_t* params;
			
			HwParams(){
				if(snd_pcm_hw_params_malloc(&this->params) < 0){
					LOG([&](auto&o){o << "cannot allocate hardware parameter structure" << std::endl;})
					throw std::runtime_error("cannot allocate hardware parameter structure");
				}
			}
			
			~HwParams(){
				snd_pcm_hw_params_free(this->params);
			}
		} hw;

		if(snd_pcm_hw_params_any(this->device.handle, hw.params) < 0){
			LOG([&](auto&o){o << "cannot initialize hardware parameter structure" << std::endl;})
			throw std::runtime_error("cannot initialize hardware parameter structure");
		}

		if(snd_pcm_hw_params_set_access(this->device.handle, hw.params, SND_PCM_ACCESS_RW_INTERLEAVED) < 0){
			LOG([&](auto&o){o << "cannot set access type" << std::endl;})
			throw std::runtime_error("cannot set access type");
		}

		if(snd_pcm_hw_params_set_format(this->device.handle, hw.params, SND_PCM_FORMAT_S16_LE) < 0){
			LOG([&](auto&o){o << "cannot set sample format" << std::endl;})
			throw std::runtime_error("cannot set sample format");
		}

		{
			unsigned val = format.frequency();
			if(snd_pcm_hw_params_set_rate_near(this->device.handle, hw.params, &val, 0) < 0){
				LOG([&](auto&o){o << "cannot set sample rate" << std::endl;})
				throw std::runtime_error("cannot set sample rate");
			}
		}

		if(snd_pcm_hw_params_set_channels(this->device.handle, hw.params, format.num_channels()) < 0){
			LOG([&](auto&o){o << "cannot set channel count" << std::endl;})
			throw std::runtime_error("cannot set channel count");
		}

		// set period size
		{
			snd_pcm_uframes_t frames = snd_pcm_uframes_t(bufferSizeFrames);
			int dir = 0;
			if(snd_pcm_hw_params_set_period_size_near(
					this->device.handle,
					hw.params,
					&frames,
					&dir
				) < 0
			)
			{
				LOG([&](auto&o){o << "could not set period size" << std::endl;})
				throw std::runtime_error("could not set period size");
			}

//			TRACE(<< "buffer size in samples = " << this->BufferSizeInSamples() << std::endl)
		}

		// Set number of periods. Periods used to be called fragments.
		{
			unsigned int numPeriods = 2;
			int err = snd_pcm_hw_params_set_periods_near(this->device.handle, hw.params, &numPeriods, NULL);
			if(err < 0){
				LOG([&](auto&o){o << "could not set number of periods, err = " << err << std::endl;})
				throw std::runtime_error("could not set number of periods");
			}
			LOG([&](auto&o){o << "numPeriods = " << numPeriods << std::endl;})
		}


		// set hw params
		if(snd_pcm_hw_params(this->device.handle, hw.params) < 0){
			LOG([&](auto&o){o << "cannot set parameters" << std::endl;})
			throw std::runtime_error("cannot set parameters");
		}
	}

	void SetSwParams(unsigned bufferSizeFrames){
		struct SwParams{
			snd_pcm_sw_params_t *params;
			SwParams(){
				if(snd_pcm_sw_params_malloc(&this->params) < 0){
					LOG([&](auto&o){o << "cannot allocate software parameters structure" << std::endl;})
					throw std::runtime_error("cannot allocate software parameters structure");
				}
			}
			~SwParams(){
				snd_pcm_sw_params_free(this->params);
			}
		} sw;

		if(snd_pcm_sw_params_current(this->device.handle, sw.params) < 0){
			LOG([&](auto&o){o << "cannot initialize software parameters structure" << std::endl;})
			throw std::runtime_error("cannot initialize software parameters structure");
		}

		// tell ALSA to wake us up whenever 'buffer size' frames of playback data can be delivered
		if(snd_pcm_sw_params_set_avail_min(this->device.handle, sw.params, bufferSizeFrames) < 0){
			LOG([&](auto&o){o << "cannot set minimum available count" << std::endl;})
			throw std::runtime_error("cannot set minimum available count");
		}

		// tell ALSA to start playing on first data write
		if(snd_pcm_sw_params_set_start_threshold(this->device.handle, sw.params, 0) < 0){
			LOG([&](auto&o){o << "cannot set start mode" << std::endl;})
			throw std::runtime_error("cannot set start mode");
		}

		if(snd_pcm_sw_params(this->device.handle, sw.params) < 0){
			LOG([&](auto&o){o << "cannot set software parameters" << std::endl;})
			throw std::runtime_error("cannot set software parameters");
		}
	}
	
};

}
