#pragma once

#include <pulse/simple.h>
#include <pulse/error.h>

#include "../Exc.hpp"
#include "WriteBasedBackend.hpp"


namespace audout{

class PulseAudioBackend : public WriteBasedBackend{
	pa_simple *handle;
	
	void write(const utki::Buf<std::int16_t> buf)override{
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
	PulseAudioBackend(
			audout::AudioFormat outputFormat,
			std::uint32_t bufferSizeFrames,
			audout::Listener* listener
		) :
			WriteBasedBackend(listener, bufferSizeFrames * outputFormat.numChannels())
	{
		TRACE(<< "opening device" << std::endl)

		pa_sample_spec ss;
		ss.format = PA_SAMPLE_S16NE;//Native endian
		ss.channels = outputFormat.numChannels();
		ss.rate = outputFormat.frequency();

		unsigned bufferSizeInBytes = bufferSizeFrames * outputFormat.bytesPerFrame();
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
			throw audout::Exc("error opening PulseAudio connection");
		}
		
		this->startThread();
	}
	
	virtual ~PulseAudioBackend()noexcept{
		this->stopThread();
		
		ASSERT(this->handle)
		pa_simple_free(this->handle);
	}
};

}
