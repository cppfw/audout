/**
 * @author Ivan Gagis <igagis@gmail.com>
 */

#pragma once

#include <nitki/MsgThread.hpp>

#include "../Player.hpp"
#include "../Listener.hpp"


namespace audout{

class WriteBasedBackend : private nitki::MsgThread{
	audout::Listener* listener;
	
	std::vector<std::int16_t> playBuf;
	
protected:
	bool isPaused = true;
	
	WriteBasedBackend(
			audout::Listener* listener,
			size_t playBufSizeInSamples
		) :
			listener(listener),
			playBuf(playBufSizeInSamples)
	{}
	
	void stopThread()throw(){
		this->pushPreallocatedQuitMessage();
		this->join();
	}
	
	void startThread(){
		this->Thread::start();
	}
	
	virtual void write(const utki::Buf<std::int16_t> buf) = 0;
	
public:
	virtual ~WriteBasedBackend()noexcept{}
	
private:
	
	void run()override{
		pogodi::WaitSet ws(1);
		
		ws.add(this->queue, pogodi::Waitable::READ);
		
		while(!this->quitFlag){
//			TRACE(<< "Backend loop" << std::endl)
			
			if(this->isPaused){
				ws.wait();
				
				auto m = this->queue.peekMsg();
				ASSERT(m)
				m();
				continue;
			}
			
			while(auto m = this->queue.peekMsg()){
				m();
			}

			this->listener->fillPlayBuf(utki::wrapBuf(this->playBuf));
			
			this->write(utki::wrapBuf(this->playBuf));
		}//~while
		
		ws.remove(this->queue);
	}
	
public:
	void setPaused(bool pause){
		this->pushMessage([this, pause](){
			this->isPaused = pause;
		});
	}
};

}//~namespace
