#pragma once

#include <utki/config.hpp>
#include <utki/singleton.hpp>

#include "AudioFormat.hpp"
#include "Listener.hpp"


#if M_OS == M_OS_WINDOWS
#	include "backend/DirectSoundBackend.hpp"
#elif M_OS == M_OS_LINUX
#	if M_OS_NAME == M_OS_NAME_ANDROID
#		include "backend/OpenSLESBackend.hpp"
#	else
#		include "backend/PulseAudioBackend.hpp"
//#		include "backend/ALSABackend.hpp"
#	endif
#elif M_OS == M_OS_MACOSX
#	include "backend/AppleCoreaudioBackend.hpp"
#else
#	error "Unknown OS"
#endif


namespace audout{

//TODO: doxygen
class Player : public utki::intrusive_singleton<Player>{
	friend class utki::intrusive_singleton<Player>;
	static utki::intrusive_singleton<Player>::T_Instance instance;
	
#if M_OS == M_OS_WINDOWS
	DirectSoundBackend backend;
#elif M_OS == M_OS_LINUX
#	if M_OS_NAME == M_OS_NAME_ANDROID
	OpenSLESBackend backend;
#	else
	PulseAudioBackend backend;
//	ALSABackend backend;
#	endif
#elif M_OS == M_OS_MACOSX
	AppleCoreaudioBackend backend;
#else
#	error "undefined OS"
#endif

	
public:
	/**
	 * @brief Create a singleton player object.
	 * @param outputFormat - output format.
	 * @param requestedBufSizeInFrames - request for size of playing buffer. Note, that it is not guaranteed that
	 *                                   the size of the resulting buffer will be equal to this requested value.
	 * @param listener - callback for filling playing buffer.
	 */
	Player(AudioFormat outputFormat, std::uint32_t requestedBufSizeInFrames, Listener* listener) :
			backend(outputFormat, requestedBufSizeInFrames, listener)
	{}
	
public:
	virtual ~Player()noexcept{}
	
	void setPaused(bool pause){
		this->backend.setPaused(pause);
	}
};

}//~namespace
