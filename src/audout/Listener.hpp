/**
 * @author Ivan Gagis <igagis@gmail.com>
 */

#pragma once

#include <utki/Buf.hpp>

namespace audout{

//TODO: doxygen
class Listener{
public:
	virtual void fillPlayBuf(utki::Buf<std::int16_t> playBuf)noexcept = 0;
	
	virtual ~Listener()noexcept{}
};

}
