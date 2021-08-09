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

#include "player.hpp"

#if M_OS == M_OS_WINDOWS
#	include "backend/direct_sound.cxx"
#elif M_OS == M_OS_LINUX
#	if M_OS_NAME == M_OS_NAME_ANDROID
#		include "backend/opensl_es.cxx"
#	else
#		include "backend/pulse_audio.cxx"
// #		include "backend/alsa.cxx"
#	endif
#elif M_OS == M_OS_MACOSX
#	include "backend/apple_coreaudio.cxx"
#else
#	error "Unknown OS"
#endif


using namespace audout;


utki::intrusive_singleton<player>::T_Instance player::instance = nullptr;



player::player(format output_format, uint32_t num_buffer_frames, audout::listener* listener) :
			backend(new audio_backend(output_format, num_buffer_frames, listener))
{}


void player::set_paused(bool pause){
    static_cast<audio_backend*>(this->backend.get())->setPaused(pause);
}
