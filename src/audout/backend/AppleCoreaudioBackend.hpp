/* 
 * File:   AppleCoreaudioBackend.hpp
 * Author: ivan
 *
 * Created on February 3, 2016, 9:36 PM
 */

#pragma once

#include "../AudioFormat.hpp"
#include "../Listener.hpp"

namespace audout{

class AppleCoreaudioBackend{
public:
	AppleCoreaudioBackend(
			audout::AudioFormat outputFormat,
			std::uint32_t bufferSizeFrames,
			audout::Listener* listener
		)
	{
		//TODO:
	}
	
	void setPaused(bool paused){
		//TODO:
	}
};

}
