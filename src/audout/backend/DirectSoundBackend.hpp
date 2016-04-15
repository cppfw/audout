/**
 * @author Ivan Gagis <igagis@gmail.com>
 */

#pragma once


#include <utki/config.hpp>


#if M_OS != M_OS_WINDOWS
#	error "compiling in non-Windows environment"
#endif


#include <cstring>

#include <initguid.h> //The header file initguid.h is required to avoid the error message "undefined reference to `IID_IDirectSoundBuffer8'".
#include <dsound.h>

#include <pogodi/WaitSet.hpp>
#include <nitki/MsgThread.hpp>

#include "../Player.hpp"
#include "../Exc.hpp"


namespace audout{

class WinEvent : public pogodi::Waitable{
	HANDLE eventForWaitable;

	std::uint32_t flagsMask;//flags to wait for


	virtual void setWaitingEvents(std::uint32_t flagsToWaitFor)override{
		//Only possible flag values are READ and 0 (NOT_READY)
		if(flagsToWaitFor != 0 && flagsToWaitFor != pogodi::Waitable::READ){
			ASSERT_INFO(false, "flagsToWaitFor = " << flagsToWaitFor)
			throw audout::Exc("WinEvent::SetWaitingEvents(): flagsToWaitFor should be ting::Waitable::READ or 0, other values are not allowed");
		}

		this->flagsMask = flagsToWaitFor;
	}

	bool checkSignaled()override{
		switch(WaitForSingleObject(this->eventForWaitable, 0)){
			case WAIT_OBJECT_0: //event is signaled
				this->setCanReadFlag();
				if(ResetEvent(this->eventForWaitable) == 0){
					ASSERT(false)
					throw audout::Exc("WinEvent::Reset(): ResetEvent() failed");
				}
				break;
			case WAIT_TIMEOUT: //event is not signalled
				this->clearCanReadFlag();
				break;
			default:
				throw audout::Exc("WinEvent: error when checking event state, WaitForSingleObject() failed");
		}
		
		return (this->readinessFlags & this->flagsMask) != 0;
	}
public:

	HANDLE getHandle()override{
		return this->eventForWaitable;
	}
	
	WinEvent(){
		this->eventForWaitable = CreateEvent(
			nullptr, //security attributes
			TRUE, //manual-reset
			FALSE, //not signaled initially
			nullptr //no name
		);
		if(this->eventForWaitable == 0){
			throw audout::Exc("WinEvent::WinEvent(): could not create event (Win32) for implementing Waitable");
		}
	}
	
	virtual ~WinEvent()noexcept{
		CloseHandle(this->eventForWaitable);
	}
};



class DirectSoundBackend : public nitki::MsgThread{
	Listener* listener;

	struct DirectSound{
		LPDIRECTSOUND8 ds;//LP prefix means long pointer
		
		DirectSound(){
			if(DirectSoundCreate8(nullptr, &this->ds, nullptr) != DS_OK){
				throw audout::Exc("DirectSound object creation failed");
			}
			
			try{
				HWND hwnd = GetDesktopWindow();
				if(!hwnd){
					throw audout::Exc("DirectSound: no foreground window found");
				}

				if(this->ds->SetCooperativeLevel(hwnd, DSSCL_PRIORITY) != DS_OK){
					throw audout::Exc("DirectSound: setting cooperative level failed");
				}
			}catch(...){
				this->ds->Release();
				throw;
			}
		}
		~DirectSound()throw(){
			this->ds->Release();
		}
	} ds;

	struct DirectSoundBuffer{
		LPDIRECTSOUNDBUFFER8 dsb; //LP stands for long pointer
		
		unsigned halfSize;
		
		DirectSoundBuffer(DirectSound& ds, unsigned bufferSizeFrames, AudioFormat format) :
				halfSize(format.bytesPerFrame() * bufferSizeFrames)
		{
			WAVEFORMATEX wf;
			memset(&wf, 0, sizeof(WAVEFORMATEX));

			wf.nChannels = format.numChannels();
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
				throw audout::Exc("DirectSound: requested buffer size is out of supported size range [DSBSIZE_MIN, DSBSIZE_MAX]");
			}
			
			{
				LPDIRECTSOUNDBUFFER dsb1;
				if(ds.ds->CreateSoundBuffer(&dsbdesc, &dsb1, nullptr) != DS_OK){
					throw audout::Exc("DirectSound: creating sound buffer failed");
				}
				if(dsb1->QueryInterface(IID_IDirectSoundBuffer8, (LPVOID*)&this->dsb) != DS_OK){
					dsb1->Release();
					throw audout::Exc("DirectSound: querying sound buffer interface failed");
				}
				dsb1->Release();
			}
			
			//init buffer with silence, i.e. fill it with 0'es
			{
				LPVOID addr;
				DWORD size;
				
				//lock the entire buffer
				if(this->dsb->Lock(
						0,
						0, //ignored because of the DSBLOCK_ENTIREBUFFER flag
						&addr,
						&size,
						nullptr, //wraparound not needed
						0, //size of wraparound not needed
						DSBLOCK_ENTIREBUFFER
					) != DS_OK)
				{
					this->dsb->Release();
					throw audout::Exc("DirectSound: locking buffer failed");
				}
				
				ASSERT(addr != 0)
				ASSERT(size == 2 * this->halfSize)
				
				//set buffer to 0'es
				memset(addr, 0, size);
				
				//unlock the buffer
				if(this->dsb->Unlock(addr, size, nullptr, 0) != DS_OK){
					this->dsb->Release();
					throw audout::Exc("DirectSound: unlocking buffer failed");
				}
			}
			
			this->dsb->SetCurrentPosition(0);
		}
		
		~DirectSoundBuffer()noexcept{
			this->dsb->Release();
		}
	} dsb;
	
	WinEvent event1, event2;
	
	void fillDSBuffer(unsigned partNum){
		ASSERT(partNum == 0 || partNum == 1)
		LPVOID addr;
		DWORD size;

		//lock the second part of buffer
		if(this->dsb.dsb->Lock(
				this->dsb.halfSize * partNum, //offset
				this->dsb.halfSize, //size
				&addr,
				&size,
				nullptr, //wraparound not needed
				0, //size of wraparound not needed
				0 //no flags
			) != DS_OK)
		{
			TRACE(<< "DirectSound thread: locking buffer failed" << std::endl)
			return;
		}

		ASSERT(addr != 0)
		ASSERT(size == this->dsb.halfSize)

		this->listener->fillPlayBuf(utki::wrapBuf(static_cast<std::int16_t*>(addr), size / 2));

		//unlock the buffer
		if(this->dsb.dsb->Unlock(addr, size, nullptr, 0) != DS_OK){
			TRACE(<< "DirectSound thread: unlocking buffer failed" << std::endl)
			ASSERT(false)
		}
	}
	
	void run()override{
		pogodi::WaitSet ws(3);
		
		ws.add(this->queue, pogodi::Waitable::READ);
		ws.add(this->event1, pogodi::Waitable::READ);
		ws.add(this->event2, pogodi::Waitable::READ);
		
		while(!this->quitFlag){
//			TRACE(<< "Backend loop" << std::endl)
			
			ws.wait();
			
			if(this->queue.canRead()){
				while(auto m = this->queue.peekMsg()){
					m();
				}
			}

			//if first buffer playing has started, then fill the second one
			if(this->event1.canRead()){
				this->fillDSBuffer(1);
			}
			
			//if second buffer playing has started, then fill the first one
			if(this->event2.canRead()){
				this->fillDSBuffer(0);
			}
		}//~while
		
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
					0, //reserved, must be 0
					0,
					DSBPLAY_LOOPING
				);
		}
	}

public:
	DirectSoundBackend(AudioFormat format, unsigned bufferSizeFrames, Listener* listener) :
			listener(listener),
			dsb(this->ds, bufferSizeFrames, format)
	{
		//Set notification points
		{
			LPDIRECTSOUNDNOTIFY8 notify;
			
			//Get IID_IDirectSoundNotify interface
			if(this->dsb.dsb->QueryInterface(
					IID_IDirectSoundNotify8,
					(LPVOID*)&notify
				) != DS_OK)
			{
				throw audout::Exc("DirectSound: obtaining IID_IDirectSoundNotify interface failed");
			}
			
			std::array<DSBPOSITIONNOTIFY, 2> pos;
			pos[0].dwOffset = 0;
			pos[0].hEventNotify = this->event1.getHandle();
			pos[1].dwOffset = this->dsb.halfSize;
			pos[1].hEventNotify = this->event2.getHandle();
			
			if(notify->SetNotificationPositions(DWORD(pos.size()), &*pos.begin()) != DS_OK){
				notify->Release();
				throw audout::Exc("DirectSound: setting notification positions failed");
			}
			
			//release IID_IDirectSoundNotify interface
			notify->Release();
		}
		
		//start playing thread
		this->start();
		
		//launch buffer playing
		this->setPaused(false);
	}
	
	~DirectSoundBackend()noexcept{
		//stop buffer playing
		if(this->dsb.dsb->Stop() != DS_OK){
			ASSERT(false)
		}
		
		//Stop playing thread
		this->pushQuitMessage();
		this->join();
	}
};

}//~namespace
