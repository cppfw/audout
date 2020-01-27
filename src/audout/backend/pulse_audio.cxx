#pragma once

#include <utki/destructable.hpp>

#include <pulse/simple.h>
#include <pulse/error.h>

#include "write_based.cxx"


namespace{

class audio_backend : public write_based, public utki::destructable{
	pa_simple *handle;
	
	void write(const utki::span<std::int16_t> buf)override{
//		ASSERT(buf.Size() == this->BufferSizeInBytes())

		int error;
		
		if(pa_simple_write(
				this->handle,
				&*buf.begin(),
				size_t(buf.sizeInBytes()),
				&error
			) < 0)
		{
			TRACE(<< "pa_simple_write(): error (" << pa_strerror(error) << ")" << std::endl)
		}
	}
	
public:
	audio_backend(
			audout::format outputFormat,
			uint32_t bufferSizeFrames,
			audout::listener* listener
		) :
			write_based(listener, bufferSizeFrames * outputFormat.num_channels())
	{
		TRACE(<< "opening device" << std::endl)

		pa_sample_spec ss;
		ss.format = PA_SAMPLE_S16NE;//Native endian
		ss.channels = outputFormat.num_channels();
		ss.rate = outputFormat.frequency();

		unsigned bufferSizeInBytes = bufferSizeFrames * outputFormat.frame_size();
		pa_buffer_attr ba;
		ba.fragsize = bufferSizeInBytes;
		ba.tlength = bufferSizeInBytes;
		ba.minreq = std::uint32_t(-1);
		ba.maxlength = std::uint32_t(-1);
		ba.prebuf = std::uint32_t(-1);
		
		pa_channel_map cm;
		pa_channel_map_init_auto(&cm, ss.channels, PA_CHANNEL_MAP_WAVEEX);

		int error;

		this->handle = pa_simple_new(
				nullptr, // Use the default server.
				"audout", // Our application's name.
				PA_STREAM_PLAYBACK,
				nullptr, // Use the default device.
				"sound stream", // Description of our stream.
				&ss, // our sample format.
				&cm, // channel map
				&ba, // buffering attributes.
				&error
			);

		if(!this->handle){
			TRACE(<< "error opening PulseAudio connection (" << pa_strerror(error) << ")" << std::endl)
			std::stringstream ss;
			ss << "error opening PulseAudio connection: " << pa_strerror(error);
			throw std::runtime_error(ss.str());
		}
		
		this->startThread();
	}
	
	virtual ~audio_backend()noexcept{
		this->stopThread();
		
		ASSERT(this->handle)
		pa_simple_free(this->handle);
	}
};

}
