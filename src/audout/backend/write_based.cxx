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

#include <thread>
#include <vector>

#include <nitki/loop_thread.hpp>
#include <nitki/queue.hpp>

#include "../player.hpp"

namespace {

class write_based : public nitki::loop_thread
{
	audout::listener* listener;

	std::vector<std::int16_t> play_buf;

protected:
	bool is_paused = true;

	write_based(
		audout::listener* listener, //
		size_t play_buf_size_samples
	) :
		nitki::loop_thread(0),
		listener(listener),
		play_buf(play_buf_size_samples)
	{}

	virtual void write(const utki::span<int16_t> buf) = 0;

public:
	write_based(const write_based&) = delete;
	write_based& operator=(const write_based&) = delete;

	write_based(write_based&&) = delete;
	write_based& operator=(write_based&&) = delete;

	~write_based() override = default;

private:
	std::optional<uint32_t> on_loop() override
	{
		if (this->is_paused) {
			return {};
		}

		this->listener->fill(utki::make_span(this->play_buf));

		// this call will block if play buffer is full
		this->write(utki::make_span(this->play_buf));

		return 0;
	}

public:
	void set_paused(bool pause)
	{
		this->push_back([this, pause]() {
			this->is_paused = pause;
		});
	}
};

} // namespace
