#pragma once

#include <vector>
#include <thread>

#include <nitki/queue.hpp>

#include "../Player.hpp"
#include "../Listener.hpp"


namespace audout{

class WriteBasedBackend{
	audout::Listener* listener;
	
	std::vector<std::int16_t> playBuf;
	
	nitki::queue queue;

	bool quitFlag = false;

	std::thread thread;

protected:
	bool isPaused = true;
	
	WriteBasedBackend(
			audout::Listener* listener,
			size_t playBufSizeInSamples
		) :
			listener(listener),
			playBuf(playBufSizeInSamples)
	{}
	
	void stopThread()noexcept{
		if(this->thread.joinable()){
			this->queue.push_back([this](){this->quitFlag = true;});
			this->thread.join();
		}
	}
	
	void startThread(){
		this->thread = std::thread([this](){this->run();});
	}
	
	virtual void write(const utki::span<int16_t> buf) = 0;
	
public:
	virtual ~WriteBasedBackend()noexcept{}
	
private:
	
	void run(){
		opros::wait_set ws(1);
		
		ws.add(this->queue, {opros::ready::read});
		
		while(!this->quitFlag){
//			TRACE(<< "Backend loop" << std::endl)
			
			if(this->isPaused){
//				TRACE(<< "Backend loop paused" << std::endl)
				ws.wait();
				
				auto m = this->queue.peekMsg();
				ASSERT(m)
				m();
				continue;
			}
			
			while(auto m = this->queue.peekMsg()){
				m();
			}

			this->listener->fillPlayBuf(utki::make_span(this->playBuf));
			
			this->write(utki::make_span(this->playBuf));
		}
		
		ws.remove(this->queue);
	}
	
public:
	void setPaused(bool pause){
		this->queue.push_back([this, pause](){
			this->isPaused = pause;
		});
	}
};

}
