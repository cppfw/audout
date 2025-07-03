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

#include <nitki/queue.hpp>
#include <opros/wait_set.hpp>
#include <utki/config.hpp>
#include <utki/destructable.hpp>
#include <utki/util.hpp>

#if CFG_OS != CFG_OS_WINDOWS
#	error "compiling in non-Windows environment"
#endif

#include <cstring>
#include <thread>

#include <dsound.h>
#include <initguid.h> // The header file initguid.h is required to avoid the error message "undefined reference to `IID_IDirectSoundBuffer8'".

#include "../player.hpp"

namespace {

class WinEvent : public opros::waitable
{
	void set_waiting_flags(utki::flags<opros::ready> wait_for) override
	{
		// Only possible flag values are 'read' or 0 (not ready)
		if (!(wait_for & (~utki::make_flags({opros::ready::read}))).is_clear()) {
			ASSERT(false, [&](auto& o) {
				o << "wait_for = " << wait_for;
			})
			throw std::invalid_argument("WinEvent::set_waiting_flags(): only 'read' or no flags are allowed");
		}
	}

	utki::flags<opros::ready> get_readiness_flags() override
	{
		utki::flags<opros::ready> flags{false};
		switch (WaitForSingleObject(this->handle, 0)) {
			case WAIT_OBJECT_0: // event is signaled
				flags.set(opros::ready::read);
				if (ResetEvent(this->handle) == 0) {
					ASSERT(false)
					throw std::system_error(GetLastError(), std::generic_category(), "ResetEvent() failed");
				}
				break;
			case WAIT_ABANDONED:
			case WAIT_TIMEOUT: // event is not signalled
				break;
			default:
			case WAIT_FAILED:
				throw std::system_error(GetLastError(), std::generic_category(), "WaitForSingleObject() failed");
		}

		return flags;
	}

public:
	WinEvent() :
		opros::waitable([]() {
			auto handle = CreateEvent(
				nullptr, // security attributes
				TRUE, // manual-reset
				FALSE, // not signaled initially
				nullptr // no name
			);
			if (handle == 0) {
				throw std::system_error(GetLastError(), std::generic_category(), "CreateEvent() failed");
			}
			return handle;
		}())
	{}

	virtual ~WinEvent() noexcept
	{
		CloseHandle(this->handle);
	}
};

class audio_backend : public utki::destructable
{
	audout::listener* listener;

	std::thread thread;

	nitki::queue queue;

	bool quitFlag = false;

	struct direct_sound {
		LPDIRECTSOUND8 ds; // LP prefix means long pointer

		direct_sound()
		{
			if (DirectSoundCreate8(nullptr, &this->ds, nullptr) != DS_OK) {
				throw std::runtime_error("DirectSoundCreate8() failed");
			}

			utki::scope_exit ds_scope_exit([this]() {
				this->ds->Release();
			});

			HWND hwnd = GetDesktopWindow();
			if (!hwnd) {
				throw std::runtime_error("no foreground window found");
			}

			if (this->ds->SetCooperativeLevel(hwnd, DSSCL_PRIORITY) != DS_OK) {
				throw std::runtime_error("SetCooperativeLevel() failed");
			}

			ds_scope_exit.release();
		}

		~direct_sound() noexcept
		{
			this->ds->Release();
		}
	} ds;

	struct direct_sound_buffer {
		LPDIRECTSOUNDBUFFER8 dsb; // LP stands for long pointer

		unsigned halfSize;

		direct_sound_buffer(direct_sound& ds, unsigned bufferSizeFrames, audout::format format) :
			halfSize(format.frame_size() * bufferSizeFrames)
		{
			WAVEFORMATEX wf;
			memset(&wf, 0, sizeof(WAVEFORMATEX));

			wf.nChannels = format.num_channels();
			wf.nSamplesPerSec = format.frequency();

			wf.wFormatTag = WAVE_FORMAT_PCM;
			wf.wBitsPerSample = 16;
			wf.nBlockAlign = wf.nChannels * (wf.wBitsPerSample / 8);
			wf.nAvgBytesPerSec = wf.nSamplesPerSec * wf.nBlockAlign;

			DSBUFFERDESC dsbdesc;
			memset(&dsbdesc, 0, sizeof(DSBUFFERDESC));

			dsbdesc.dwSize = sizeof(DSBUFFERDESC);
			dsbdesc.dwFlags = DSBCAPS_GETCURRENTPOSITION2 | DSBCAPS_CTRLPOSITIONNOTIFY | DSBCAPS_GLOBALFOCUS;
			dsbdesc.dwBufferBytes = 2 * this->halfSize;
			dsbdesc.lpwfxFormat = &wf;

			if (dsbdesc.dwBufferBytes < DSBSIZE_MIN || DSBSIZE_MAX < dsbdesc.dwBufferBytes) {
				throw std::invalid_argument(
					"DirectSound: requested buffer size is out of supported size range [DSBSIZE_MIN, DSBSIZE_MAX]"
				);
			}

			{
				LPDIRECTSOUNDBUFFER dsb1;
				if (ds.ds->CreateSoundBuffer(&dsbdesc, &dsb1, nullptr) != DS_OK) {
					throw std::runtime_error("DirectSound: CreateSoundBuffer() failed");
				}
				utki::scope_exit dsb1_scope_exit([&dsb1]() {
					dsb1->Release();
				});

				if (dsb1->QueryInterface(IID_IDirectSoundBuffer8, (LPVOID*)&this->dsb) != DS_OK) {
					throw std::runtime_error("DirectSound: QueryInterface() failed");
				}
			}

			utki::scope_exit dsb_scope_exit([this]() {
				this->dsb->Release();
			});

			// init buffer with silence, i.e. fill it with 0'es
			{
				LPVOID addr;
				DWORD size;

				// lock the entire buffer
				if (this->dsb->Lock(
						0,
						0, // ignored because of the DSBLOCK_ENTIREBUFFER flag
						&addr,
						&size,
						nullptr, // wraparound not needed
						0, // size of wraparound not needed
						DSBLOCK_ENTIREBUFFER
					) != DS_OK)
				{
					throw std::runtime_error("DirectSound: buffer Lock() failed");
				}

				ASSERT(addr != 0)
				ASSERT(size == 2 * this->halfSize)

				// set buffer to 0'es
				memset(addr, 0, size);

				// unlock the buffer
				if (this->dsb->Unlock(addr, size, nullptr, 0) != DS_OK) {
					throw std::runtime_error("DirectSound: buffer Unlock() failed");
				}
			}

			this->dsb->SetCurrentPosition(0);

			dsb_scope_exit.release();
		}

		~direct_sound_buffer() noexcept
		{
			this->dsb->Release();
		}
	} dsb;

	WinEvent event1, event2;

	void fillDSBuffer(unsigned partNum)
	{
		ASSERT(partNum == 0 || partNum == 1)
		LPVOID addr;
		DWORD size;

		// lock the second part of buffer
		if (this->dsb.dsb->Lock(
				this->dsb.halfSize * partNum, // offset
				this->dsb.halfSize, // size
				&addr,
				&size,
				nullptr, // wraparound not needed
				0, // size of wraparound not needed
				0 // no flags
			) != DS_OK)
		{
			LOG([&](auto& o) {
				o << "DirectSound thread: locking buffer failed" << std::endl;
			})
			return;
		}

		ASSERT(addr != 0)
		ASSERT(size == this->dsb.halfSize)

		this->listener->fill(utki::make_span(static_cast<std::int16_t*>(addr), size / 2));

		// unlock the buffer
		if (this->dsb.dsb->Unlock(addr, size, nullptr, 0) != DS_OK) {
			LOG([&](auto& o) {
				o << "DirectSound thread: unlocking buffer failed" << std::endl;
			})
			ASSERT(false)
		}
	}

	// TODO: rewrite using nitki::loop_thread?
	void run()
	{
		opros::wait_set ws(3);

		ws.add(this->queue, {opros::ready::read}, &this->queue);
		ws.add(this->event1, {opros::ready::read}, &this->event1);
		ws.add(this->event2, {opros::ready::read}, &this->event2);

		while (!this->quitFlag) {
			//			TRACE(<< "audio_backend loop" << std::endl)

			ws.wait();

			for (const auto& t : ws.get_triggered()) {
				if (t.user_data == &this->queue) {
					while (auto m = this->queue.pop_front()) {
						m();
					}
				} else if (t.user_data == &this->event1) {
					// if first buffer playing has started, then fill the second one
					this->fillDSBuffer(1);
				} else if (t.user_data == &this->event2) {
					// if second buffer playing has started, then fill the first one
					this->fillDSBuffer(0);
				}
			}
		}

		ws.remove(this->event2);
		ws.remove(this->event1);
		ws.remove(this->queue);
	}

public:
	void setPaused(bool pause)
	{
		if (pause) {
			this->dsb.dsb->Stop();
		} else {
			this->dsb.dsb->Play(
				0, // reserved, must be 0
				0,
				DSBPLAY_LOOPING
			);
		}
	}

public:
	audio_backend(audout::format format, unsigned bufferSizeFrames, audout::listener* listener) :
		listener(listener),
		dsb(this->ds, bufferSizeFrames, format)
	{
		// set notification points
		{
			LPDIRECTSOUNDNOTIFY notify;

			// get IID_IDirectSoundNotify interface
			if (this->dsb.dsb->QueryInterface(IID_IDirectSoundNotify8, (LPVOID*)&notify) != DS_OK) {
				throw std::runtime_error("DirectSound: QueryInterface(IID_IDirectSoundNotify8) failed");
			}

			utki::scope_exit notify_scope_exit([&notify]() {
				notify->Release();
			});

			std::array<DSBPOSITIONNOTIFY, 2> pos;
			pos[0].dwOffset = 0;
			pos[0].hEventNotify = this->event1.get_handle();
			pos[1].dwOffset = this->dsb.halfSize;
			pos[1].hEventNotify = this->event2.get_handle();

			if (notify->SetNotificationPositions(DWORD(pos.size()), pos.data()) != DS_OK) {
				throw std::runtime_error("DirectSound: SetNotificationPositions() failed");
			}
		}

		// start playing thread
		this->thread = std::thread([this]() {
			this->run();
		});

		// launch buffer playing
		this->setPaused(false);
	}

	~audio_backend() noexcept
	{
		// stop buffer playing
		if (this->dsb.dsb->Stop() != DS_OK) {
			ASSERT(false)
		}

		// stop playing thread
		ASSERT(this->thread.joinable())
		this->queue.push_back([this]() {
			this->quitFlag = true;
		});
		this->thread.join();
	}
};

} // namespace
