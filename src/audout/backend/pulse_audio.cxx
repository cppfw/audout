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

#include <pulse/error.h>
#include <pulse/simple.h>
#include <utki/destructable.hpp>

#include "write_based.cxx"

namespace {

class audio_backend : public write_based, public utki::destructable
{
	pa_simple* handle;

	void write(const utki::span<std::int16_t> buf) override
	{
		//		ASSERT(buf.Size() == this->BufferSizeInBytes())

		int error;

		if (pa_simple_write(this->handle, &*buf.begin(), size_t(buf.size_bytes()), &error) < 0) {
			LOG([&](auto& o) {
				o << "pa_simple_write(): error (" << pa_strerror(error) << ")" << std::endl;
			})
		}
	}

public:
	audio_backend(audout::format outputFormat, uint32_t bufferSizeFrames, audout::listener* listener) :
		write_based(listener, bufferSizeFrames * outputFormat.num_channels())
	{
		LOG([&](auto& o) {
			o << "opening device" << std::endl;
		})

		pa_sample_spec ss;
		ss.format = PA_SAMPLE_S16NE; //Native endian
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

		if (!this->handle) {
			LOG([&](auto& o) {
				o << "error opening PulseAudio connection (" << pa_strerror(error) << ")" << std::endl;
			})
			std::stringstream ss;
			ss << "error opening PulseAudio connection: " << pa_strerror(error);
			throw std::runtime_error(ss.str());
		}

		this->start();
	}

	virtual ~audio_backend() noexcept
	{
		this->quit();
		this->join();

		ASSERT(this->handle)
		pa_simple_free(this->handle);
	}
};

} // namespace
