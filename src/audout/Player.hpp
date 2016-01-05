/**
 * @author Ivan Gagis <igagis@gmail.com>
 */

#pragma once

#include <utki/config.hpp>
#include <utki/Singleton.hpp>

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
#else
#	error "Unknown OS"
#endif


namespace audout{

//TODO: doxygen
class Player : public utki::IntrusiveSingleton<Player>{
	friend class utki::IntrusiveSingleton<Player>;
	static utki::IntrusiveSingleton<Player>::T_Instance instance;
	
#if M_OS == M_OS_WINDOWS
	DirectSoundBackend backend;
#elif M_OS == M_OS_LINUX
#	if M_OS_NAME == M_OS_NAME_ANDROID
	OpenSLESBackend backend;
#	else
	PulseAudioBackend backend;
//	ALSABackend backend;
#	endif
#else
#	error "undefined OS"
#endif

	
public:
	Player(AudioFormat outputFormat, std::uint32_t bufSizeInFrames, Listener* listener) :
			backend(outputFormat, bufSizeInFrames, listener)
	{}
	
public:
	virtual ~Player()noexcept{}
	
	void setPaused(bool pause){
		this->backend.setPaused(pause);
	}
};

}//~namespace
