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

#include <vector>
#include <array>

#include <utki/util.hpp>
#include <utki/destructable.hpp>

#include <SLES/OpenSLES.h>

#if M_OS_NAME == M_OS_NAME_ANDROID
#	include <SLES/OpenSLES_Android.h>
#include <cstdint>

#endif

#include "../player.hpp"

namespace{

class audio_backend : public utki::destructable{

    audout::listener* listener;

	struct Engine{
		SLObjectItf object; // object
		SLEngineItf engine; // engine interface
		
		Engine(){
			// create engine object
			if(slCreateEngine(&this->object, 0, NULL, 0, NULL, NULL) != SL_RESULT_SUCCESS){
				throw std::runtime_error("OpenSLES: Creating engine object failed");
			}

            utki::scope_exit scopeExit([this](){
                this->destroy();
            });

			// realize the engine
			if((*this->object)->Realize(this->object, SL_BOOLEAN_FALSE) != SL_RESULT_SUCCESS){
				throw std::runtime_error("OpenSLES: Realizing engine object failed");
			}
			
			// get the engine interface, which is needed in order to create other objects
			if((*this->object)->GetInterface(this->object, SL_IID_ENGINE, &this->engine) != SL_RESULT_SUCCESS){
				throw std::runtime_error("OpenSLES: Obtaining Engine interface failed");
			}

			scopeExit.release();
		}
		
		~Engine()noexcept{
			this->destroy();
		}
		
		void destroy()noexcept{
			(*this->object)->Destroy(this->object);
		}
		
	private:
		Engine(const Engine&) = delete;
		Engine& operator=(const Engine&) = delete;
	} engine;
	
	struct OutputMix{
		SLObjectItf object;
		
		OutputMix(Engine& engine){
			if((*engine.engine)->CreateOutputMix(engine.engine, &this->object, 0, NULL, NULL) != SL_RESULT_SUCCESS){
				throw std::runtime_error("OpenSLES: Creating output mix object failed");
			}

            utki::scope_exit scopeExit([this](){
                this->destroy();
            });

			// realize the output mix
			if((*this->object)->Realize(this->object, SL_BOOLEAN_FALSE) != SL_RESULT_SUCCESS){
				throw std::runtime_error("OpenSLES: Realizing output mix object failed");
			}

			scopeExit.release();
		}
		
		~OutputMix()noexcept{
			this->destroy();
		}
		
		void destroy()noexcept{
			(*this->object)->Destroy(this->object);
		}
		
	private:
		OutputMix(const OutputMix&) = delete;
		OutputMix& operator=(const OutputMix&) = delete;
	} outputMix;
	
	struct Player{
		audio_backend& backend;
		
		SLObjectItf object;
		SLPlayItf play;
#if M_OS_NAME == M_OS_NAME_ANDROID
		SLAndroidSimpleBufferQueueItf
#else
		SLBufferQueueItf
#endif
				bufferQueue;

        std::array<std::vector<std::uint8_t>, 2> bufs;
	
		// this callback handler is called every time a buffer finishes playing
		static void Callback(
#if M_OS_NAME == M_OS_NAME_ANDROID
				SLAndroidSimpleBufferQueueItf queue,
				void *context
#else
				SLBufferQueueItf queue,
				SLuint32 eventFlags,
				const void *buffer,
				SLuint32 bufferSize,
				SLuint32 dataUsed,
				void *context
#endif
			)
		{
//			TRACE(<< "audio_backend::Player::Callback(): invoked" << std::endl)
			
			ASSERT(context)
			Player* player = static_cast<Player*>(context);
			
#if M_OS_NAME == M_OS_NAME_ANDROID
#else
			ASSERT(buffer == &*player->bufs[0].begin())
			ASSERT(bufferSize == player->bufs[0].size())
			ASSERT(dataUsed <= bufferSize)
#endif
			
			ASSERT(player->bufs.size() == 2)
			std::swap(player->bufs[0], player->bufs[1]); // swap buffers, the 0th one is the buffer which is currently playing
			
#if M_OS_NAME == M_OS_NAME_ANDROID
			SLresult res = (*queue)->Enqueue(queue, &*player->bufs[0].begin(), player->bufs[0].size());
#else
			SLresult res = (*queue)->Enqueue(queue, &*player->bufs[0].begin(), player->bufs[0].size(), SL_BOOLEAN_FALSE);
#endif
			ASSERT(res == SL_RESULT_SUCCESS)
			
			// fill the second buffer to be enqueued next time the callback is called
            ASSERT(player->bufs[1].size() % 2 == 0)
			player->backend.listener->fill(
                    utki::span<std::int16_t>(
                            reinterpret_cast<std::int16_t*>(&*player->bufs[1].begin()),
                            player->bufs[1].size() / 2
                        )
                );
		}
		
		Player(audio_backend& backend, Engine& engine, OutputMix& outputMix, unsigned bufferSizeFrames, audout::format format) :
				backend(backend)
		{
			// allocate play buffers of required size
			{
				size_t bufSize = bufferSizeFrames * format.frame_size();
                ASSERT(this->bufs.size() == 2)
                for(auto& b : this->bufs){
                    b.resize(bufSize);
                }
				// initialize the first buffer with 0's, since playing will start from the first buffer
				memset(&*this->bufs[0].begin(), 0, this->bufs[0].size());
			}
			
			//========================
			// configure audio source
#if M_OS_NAME == M_OS_NAME_ANDROID
			SLDataLocator_AndroidSimpleBufferQueue bufferQueueStruct = {SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE, 2}; //2 buffers in queue
#else
			SLDataLocator_BufferQueue bufferQueueStruct = {SL_DATALOCATOR_BUFFERQUEUE, 2}; // 2 buffers in queue
#endif
			
			unsigned num_channels = format.num_channels();
			SLuint32 channelMask;
			switch(num_channels){
				case 1:
					channelMask = SL_SPEAKER_FRONT_CENTER;
					break;
				case 2:
					channelMask = SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT;
					break;
				default:
					ASSERT(false)
					break;
			}
			SLDataFormat_PCM audioFormat = {
				SL_DATAFORMAT_PCM,
				num_channels, // number of channels
				format.frequency() * 1000, // milliHertz
				16, // bits per sample, if 16bits then sample is signed 16bit integer
				16, // container size for sample, it can be bigger than sample itself. E.g. 32bit container for 16bits sample
				channelMask, // which channels map to which speakers
				SL_BYTEORDER_LITTLEENDIAN // we want little endian byte order
			};
			
			SLDataSource audioSourceStruct = {&bufferQueueStruct, &audioFormat};

			//======================
			// configure audio sink
			SLDataLocator_OutputMix outputMixStruct = {SL_DATALOCATOR_OUTPUTMIX, outputMix.object};
			SLDataSink audioSinkStruct = {&outputMixStruct, NULL};

			//=====================
			//=====================
			// create audio player
			const SLInterfaceID ids[1] = {SL_IID_BUFFERQUEUE};
			const SLboolean req[1] = {SL_BOOLEAN_TRUE};
			if((*engine.engine)->CreateAudioPlayer(
					engine.engine,
					&this->object,
					&audioSourceStruct,
					&audioSinkStruct,
					1,
					ids,
					req
				) != SL_RESULT_SUCCESS)
			{
				throw std::runtime_error("OpenSLES: Creating player object failed");
			}

            utki::scope_exit scopeExit([this](){
                this->destroy();
            });

			// realize the player
			if((*this->object)->Realize(this->object, SL_BOOLEAN_FALSE) != SL_RESULT_SUCCESS){
				throw std::runtime_error("OpenSLES: Realizing player object failed");
			}

			// get the play interface
			if((*this->object)->GetInterface(this->object, SL_IID_PLAY, &this->play) != SL_RESULT_SUCCESS){
				throw std::runtime_error("OpenSLES: Obtaining Play interface failed");
			}

			// get the buffer queue interface
			if((*this->object)->GetInterface(this->object, SL_IID_BUFFERQUEUE, &this->bufferQueue) != SL_RESULT_SUCCESS){
				throw std::runtime_error("OpenSLES: Obtaining Play interface failed");
			}

			// register callback on the buffer queue
			if((*this->bufferQueue)->RegisterCallback(
					this->bufferQueue,
					&Callback,
					this // context to be passed to the callback
				) != SL_RESULT_SUCCESS)
			{
				throw std::runtime_error("OpenSLES: Registering callback on the buffer queue failed");
			}

			scopeExit.release();
		}
		
		~Player()noexcept{
			this->destroy();
		}
		
		void destroy()noexcept{
			(*this->object)->Destroy(this->object);
		}

        void setPaused(bool pause){
            SLresult res;
            if(pause){
                res = (*this->play)->SetPlayState(this->play, SL_PLAYSTATE_STOPPED);
            }else{
                res = (*this->play)->SetPlayState(this->play, SL_PLAYSTATE_PLAYING);
            }

            if(res != SL_RESULT_SUCCESS){
                throw std::runtime_error("OpenSLES: Setting player state failed");
            }
        }

	private:
		Player(const Player&);
		Player& operator=(const Player&);
	} player;

public:
    void setPaused(bool pause){
        this->player.setPaused(pause);
    }

	// create buffered queue player
	audio_backend(
            audout::format outputFormat,
            std::uint32_t bufferSizeFrames,
            audout::listener* listener
        ) :
            listener(listener),
			outputMix(this->engine),
			player(*this, this->engine, this->outputMix, bufferSizeFrames, outputFormat)
	{
//		TRACE(<< "audio_backend::audio_backend(): Starting player" << std::endl)
		this->setPaused(false);
		
		// enqueue the first buffer for playing, otherwise it will not start playing
#if M_OS_NAME == M_OS_NAME_ANDROID
		SLresult res = (*this->player.bufferQueue)->Enqueue(
                this->player.bufferQueue,
                &*this->player.bufs[0].begin(),
                this->player.bufs[0].size()
            );
#else
		SLresult res = (*this->player.bufferQueue)->Enqueue(
		        this->player.bufferQueue,
		        &*this->player.bufs[0].begin(),
		        this->player.bufs[0].size(),
		        SL_BOOLEAN_FALSE
		    );
#endif
		if(res != SL_RESULT_SUCCESS){
			throw std::runtime_error("OpenSLES: unable to enqueue");
		}
	}

	~audio_backend()noexcept{
		// stop player playing
		SLresult res = (*player.play)->SetPlayState(player.play, SL_PLAYSTATE_STOPPED);
		ASSERT(res == SL_RESULT_SUCCESS);
		
		// TODO: make sure somehow that the callback will not be called anymore
	}
};

}
