#include "player.hpp"

#if M_OS == M_OS_WINDOWS
#	include "backend/direct_sound.cxx"
#elif M_OS == M_OS_LINUX
#	if M_OS_NAME == M_OS_NAME_ANDROID
#		include "backend/opensl_es.cxx"
#	else
#		include "backend/pulse_audio.cxx"
// #		include "backend/alsa.cxx"
#	endif
#elif M_OS == M_OS_MACOSX
#	include "backend/apple_coreaudio.cxx"
#else
#	error "Unknown OS"
#endif


using namespace audout;


utki::intrusive_singleton<player>::T_Instance player::instance = nullptr;



player::player(format output_format, uint32_t num_buffer_frames, audout::listener* listener) :
			backend(new audio_backend(output_format, num_buffer_frames, listener))
{}


void player::set_paused(bool pause){
    static_cast<audio_backend*>(this->backend.get())->setPaused(pause);
}
