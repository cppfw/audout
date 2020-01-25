#pragma once

#include <utki/span.hpp>

namespace audout{

//TODO: doxygen
class Listener{
public:
	virtual void fillPlayBuf(utki::span<int16_t> play_buffer)noexcept = 0;
	
	virtual ~Listener()noexcept{}
};

}
