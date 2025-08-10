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

// NOLINTNEXTLINE(bugprone-suspicious-include, "not a suspicious include")
#include "write_based.cxx"

namespace {

class audio_backend :
	public write_based, //
	public utki::destructable
{
	pa_simple* handle;

	void write(const utki::span<std::int16_t> buf) override
	{
		int error{};

		if (pa_simple_write(this->handle, &*buf.begin(), size_t(buf.size_bytes()), &error) < 0) {
			LOG([&](auto& o) {
				o << "pa_simple_write(): error (" << pa_strerror(error) << ")" << std::endl;
			})
		}
	}

public:
	audio_backend(audout::format output_format, uint32_t buffer_size_frames, audout::listener* listener) :
		write_based(
			listener, //
			size_t(buffer_size_frames * output_format.num_channels())
		)
	{
		LOG([&](auto& o) {
			o << "opening device" << std::endl;
		})

		pa_sample_spec ss;
		ss.format = PA_SAMPLE_S16NE; // native endian
		ss.channels = output_format.num_channels();
		ss.rate = output_format.frequency();

		unsigned buffer_size_bytes = buffer_size_frames * output_format.frame_size();
		pa_buffer_attr ba;
		ba.fragsize = buffer_size_bytes;
		ba.tlength = buffer_size_bytes;
		ba.minreq = std::uint32_t(-1);
		ba.maxlength = std::uint32_t(-1);
		ba.prebuf = std::uint32_t(-1);

		pa_channel_map cm;
		pa_channel_map_init_auto(&cm, ss.channels, PA_CHANNEL_MAP_WAVEEX);

		int error{};

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
			utki::log_debug([&](auto& o) {
				o << "error opening PulseAudio connection (" << pa_strerror(error) << ")" << std::endl;
			});
			std::stringstream ss;
			ss << "error opening PulseAudio connection: " << pa_strerror(error);
			throw std::runtime_error(ss.str());
		}

		this->start();
	}

	audio_backend(const audio_backend&) = delete;
	audio_backend& operator=(const audio_backend&) = delete;

	audio_backend(audio_backend&&) = delete;
	audio_backend& operator=(audio_backend&&) = delete;

	~audio_backend() override
	{
		this->quit();
		this->join();

		utki::assert(this->handle, SL);
		pa_simple_free(this->handle);
	}
};

} // namespace
