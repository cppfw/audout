#pragma once

#include <utki/config.hpp>

#include <opros/wait_set.hpp>

#include <nitki/queue.hpp>

#if M_OS != M_OS_WINDOWS
#	error "compiling in non-Windows environment"
#endif

#include <cstring>
#include <thread>

#include <initguid.h> // The header file initguid.h is required to avoid the error message "undefined reference to `IID_IDirectSoundBuffer8'".
#include <dsound.h>

#include "../player.hpp"

namespace{

class WinEvent : public opros::waitable{
	HANDLE eventForWaitable;

	utki::flags<opros::ready> waiting_flags;

	virtual void set_waiting_flags(utki::flags<opros::ready> wait_for)override{
		// Only possible flag values are 'read' or 0 (not ready)
		if(!(wait_for & (~utki::make_flags({opros::ready::read}))).is_clear()){
			ASSERT_INFO(false, "wait_for = " << wait_for)
			throw std::invalid_argument("WinEvent::set_waiting_flags(): only 'read' or no flags are allowed");
		}

		this->waiting_flags = wait_for;
	}

	bool check_signaled()override{
		switch(WaitForSingleObject(this->eventForWaitable, 0)){
			case WAIT_OBJECT_0: // event is signaled
				this->readiness_flags.set(opros::ready::read);
				if(ResetEvent(this->eventForWaitable) == 0){
					ASSERT(false)
					throw std::system_error(GetLastError(), std::generic_category(), "ResetEvent() failed");
				}
				break;
			case WAIT_ABANDONED:
			case WAIT_TIMEOUT: // event is not signalled
				this->readiness_flags.clear(opros::ready::read);
				break;
			default:
			case WAIT_FAILED:
				throw std::system_error(GetLastError(), std::generic_category(), "WaitForSingleObject() failed");
		}
		
		return !(this->waiting_flags & this->flags()).is_clear();
	}
public:

	HANDLE get_handle()override{
		return this->eventForWaitable;
	}
	
	WinEvent(){
		this->eventForWaitable = CreateEvent(
			nullptr, // security attributes
			TRUE, // manual-reset
			FALSE, // not signaled initially
			nullptr // no name
		);
		if(this->eventForWaitable == 0){
			throw std::system_error(GetLastError(), std::generic_category(), "CreateEvent() failed");
		}
	}
	
	virtual ~WinEvent()noexcept{
		CloseHandle(this->eventForWaitable);
	}
};



class audio_backend{
	listener* listener;

	std::thread thread;

	nitki::queue queue;

	bool quitFlag = false;

	struct direct_sound{
		LPDIRECTSOUND8 ds; // LP prefix means long pointer
		
		direct_sound(){
			if(DirectSoundCreate8(nullptr, &this->ds, nullptr) != DS_OK){
				throw std::runtime_error("DirectSoundCreate8() failed");
			}
			
			utki::scope_exit ds_scope_exit([this](){
				this->ds->Release();
			});

			HWND hwnd = GetDesktopWindow();
			if(!hwnd){
				throw std::runtime_error("no foreground window found");
			}

			if(this->ds->SetCooperativeLevel(hwnd, DSSCL_PRIORITY) != DS_OK){
				throw std::runtime_error("SetCooperativeLevel() failed");
			}

			ds_scope_exit.reset();
		}
		~direct_sound()noexcept{
			this->ds->Release();
		}
	} ds;

	struct direct_sound_buffer{
		LPDIRECTSOUNDBUFFER8 dsb; // LP stands for long pointer
		
		unsigned halfSize;
		
		direct_sound_buffer(direct_sound& ds, unsigned bufferSizeFrames, audout::format format) :
				halfSize(format.bytesPerFrame() * bufferSizeFrames)
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
			
			if(dsbdesc.dwBufferBytes < DSBSIZE_MIN || DSBSIZE_MAX < dsbdesc.dwBufferBytes){
				throw std::invalid_argument("DirectSound: requested buffer size is out of supported size range [DSBSIZE_MIN, DSBSIZE_MAX]");
			}
			
			{
				LPDIRECTSOUNDBUFFER dsb1;
				if(ds.ds->CreateSoundBuffer(&dsbdesc, &dsb1, nullptr) != DS_OK){
					throw std::runtime_error("DirectSound: CreateSoundBuffer() failed");
				}
				utki::scope_exit dsb1_scope_exit([&dsb1](){dsb1->Release();});

				if(dsb1->QueryInterface(IID_IDirectSoundBuffer8, (LPVOID*)&this->dsb) != DS_OK){
					throw std::runtime_error("DirectSound: QueryInterface() failed");
				}
			}

			utki::scope_exit dsb_scope_exit([this](){this->dsb->Release();});
			
			// init buffer with silence, i.e. fill it with 0'es
			{
				LPVOID addr;
				DWORD size;
				
				// lock the entire buffer
				if(this->dsb->Lock(
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
				if(this->dsb->Unlock(addr, size, nullptr, 0) != DS_OK){
					throw std::runtime_error("DirectSound: buffer Unlock() failed");
				}
			}
			
			this->dsb->SetCurrentPosition(0);

			dsb_scope_exit.reset();
		}
		
		~direct_sound_buffer()noexcept{
			this->dsb->Release();
		}
	} dsb;
	
	WinEvent event1, event2;
	
	void fillDSBuffer(unsigned partNum){
		ASSERT(partNum == 0 || partNum == 1)
		LPVOID addr;
		DWORD size;

		// lock the second part of buffer
		if(this->dsb.dsb->Lock(
				this->dsb.halfSize * partNum, // offset
				this->dsb.halfSize, // size
				&addr,
				&size,
				nullptr, // wraparound not needed
				0, // size of wraparound not needed
				0 // no flags
			) != DS_OK)
		{
			TRACE(<< "DirectSound thread: locking buffer failed" << std::endl)
			return;
		}

		ASSERT(addr != 0)
		ASSERT(size == this->dsb.halfSize)

		this->listener->fill(utki::make_span(static_cast<std::int16_t*>(addr), size / 2));

		// unlock the buffer
		if(this->dsb.dsb->Unlock(addr, size, nullptr, 0) != DS_OK){
			TRACE(<< "DirectSound thread: unlocking buffer failed" << std::endl)
			ASSERT(false)
		}
	}
	
	void run(){
		opros::wait_set ws(3);
		
		ws.add(this->queue, {opros::ready::read});
		ws.add(this->event1, {opros::ready::read});
		ws.add(this->event2, {opros::ready::read});
		
		while(!this->quitFlag){
//			TRACE(<< "audio_backend loop" << std::endl)
			
			ws.wait();
			
			if(this->queue.flags().get(opros::ready::read)){
				while(auto m = this->queue.pop_front()){
					m();
				}
			}

			// if first buffer playing has started, then fill the second one
			if(this->event1.flags().get(opros::ready::read)){
				this->fillDSBuffer(1);
			}
			
			// if second buffer playing has started, then fill the first one
			if(this->event2.flags().get(opros::ready::read)){
				this->fillDSBuffer(0);
			}
		}
		
		ws.remove(this->event2);
		ws.remove(this->event1);
		ws.remove(this->queue);
	}
	
public:
	void setPaused(bool pause){
		if(pause){
			this->dsb.dsb->Stop();
		}else{
			this->dsb.dsb->Play(
					0, // reserved, must be 0
					0,
					DSBPLAY_LOOPING
				);
		}
	}

public:
	audio_backend(audout::format format, unsigned bufferSizeFrames, listener* listener) :
			listener(listener),
			dsb(this->ds, bufferSizeFrames, format)
	{
		// set notification points
		{
			LPDIRECTSOUNDNOTIFY notify;
			
			// get IID_IDirectSoundNotify interface
			if(this->dsb.dsb->QueryInterface(
					IID_IDirectSoundNotify8,
					(LPVOID*)&notify
				) != DS_OK)
			{
				throw std::runtime_error("DirectSound: QueryInterface(IID_IDirectSoundNotify8) failed");
			}
			
			utki::scope_exit notify_scope_exit([&notify](){notify->Release();});

			std::array<DSBPOSITIONNOTIFY, 2> pos;
			pos[0].dwOffset = 0;
			pos[0].hEventNotify = this->event1.get_handle();
			pos[1].dwOffset = this->dsb.halfSize;
			pos[1].hEventNotify = this->event2.get_handle();
			
			if(notify->SetNotificationPositions(DWORD(pos.size()), pos.data()) != DS_OK){
				throw std::runtime_error("DirectSound: SetNotificationPositions() failed");
			}
		}
		
		// start playing thread
		this->thread = std::thread([this](){this->run();});
		
		// launch buffer playing
		this->setPaused(false);
	}
	
	~audio_backend()noexcept{
		// stop buffer playing
		if(this->dsb.dsb->Stop() != DS_OK){
			ASSERT(false)
		}
		
		// stop playing thread
		ASSERT(this->thread.joinable())
		this->queue.push_back([this](){this->quitFlag = true;});
		this->thread.join();
	}
};
}
