#pragma once

#include <utki/config.hpp>
#include <utki/singleton.hpp>
#include <utki/destructable.hpp>
#include <utki/span.hpp>

#include "format.hpp"

namespace audout{

//TODO: doxygen
class listener{
public:
	virtual void fill(utki::span<int16_t> play_buffer)noexcept = 0;
	
	virtual ~listener()noexcept{}
};

//TODO: doxygen
class player : public utki::intrusive_singleton<player>{
	friend class utki::intrusive_singleton<player>;
	static utki::intrusive_singleton<player>::T_Instance instance;

	std::unique_ptr<utki::destructable> backend;

public:
	/**
	 * @brief Create a singleton player object.
	 * @param output_format - output format.
	 * @param num_buffer_frames - request for size of playing buffer. Note, that it is not guaranteed that
	 *                            the size of the resulting buffer will be equal to this requested value.
	 * @param listener - callback for filling playing buffer.
	 */
	player(format output_format, uint32_t num_buffer_frames, listener* listener);
	
public:
	virtual ~player()noexcept{}
	
	void set_paused(bool pause);
};

}
