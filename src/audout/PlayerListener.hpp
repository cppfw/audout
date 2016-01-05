/**
 * @author Ivan Gagis <igagis@gmail.com>
 */

#pragma once

namespace audout{

//TODO: doxygen
class PlayerListener{
public:
	virtual void FillPlayBuf(const ting::Buffer<ting::s16>& playBuf)noexcept = 0;
	
	virtual ~PlayerListener()noexcept{}
};

}
