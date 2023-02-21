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

#include <vector>
#include <thread>

#include <nitki/queue.hpp>

#include "../player.hpp"

namespace{

class write_based{
	audout::listener* listener;
	
	std::vector<std::int16_t> playBuf;
	
	nitki::queue queue;

	bool quitFlag = false;

	std::thread thread;

protected:
	bool isPaused = true;
	
	write_based(
			audout::listener* listener,
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
	virtual ~write_based()noexcept{}
	
private:
	
	void run(){
		opros::wait_set ws(1);
		
		ws.add(this->queue, {opros::ready::read});
		
		while(!this->quitFlag){
//			TRACE(<< "Backend loop" << std::endl)
			
			if(this->isPaused){
//				TRACE(<< "Backend loop paused" << std::endl)
				ws.wait(nullptr);
				
				auto m = this->queue.pop_front();
				ASSERT(m)
				m();
				continue;
			}
			
			while(auto m = this->queue.pop_front()){
				m();
			}

			this->listener->fill(utki::make_span(this->playBuf));
			
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
