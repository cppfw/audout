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

#include <utki/config.hpp>
#include <utki/singleton.hpp>
#include <utki/destructable.hpp>
#include <utki/span.hpp>

#include "format.hpp"

namespace audout{

//TODO: doxygen
class listener{
public:
	virtual void fill(utki::span<int16_t> play_buffer)noexcept = 0;
	
	virtual ~listener()noexcept{}
};

//TODO: doxygen
class player : public utki::intrusive_singleton<player>{
	friend class utki::intrusive_singleton<player>;
	static utki::intrusive_singleton<player>::T_Instance instance;

	std::unique_ptr<utki::destructable> backend;

public:
	/**
	 * @brief Create a singleton player object.
	 * @param output_format - output format.
	 * @param num_buffer_frames - request for size of playing buffer. Note, that it is not guaranteed that
	 *                            the size of the resulting buffer will be equal to this requested value.
	 * @param listener - callback for filling playing buffer.
	 */
	player(format output_format, uint32_t num_buffer_frames, listener* listener);
	
public:
	virtual ~player()noexcept{}
	
	void set_paused(bool pause);
};

}
