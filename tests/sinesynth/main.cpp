#include <utki/math.hpp>
#include <utki/config.hpp>
#include <nitki/thread.hpp>

#include "../../src/audout/player.hpp"

#if M_OS_NAME == M_OS_NAME_ANDROID
#	include <jni.h>
#endif


struct SinePlayer : public audout::listener{
	double time = 0;

	audout::format format;
	
	void fill(utki::span<std::int16_t> buf)noexcept override{
//		TRACE_ALWAYS(<< "filling smp buf" << std::endl)

		for(auto dst = buf.begin(); dst != buf.end();){
			std::int16_t v = std::int16_t(
					decltype(this->time)(0x7fff) * std::sin(this->time * 2 * utki::pi<decltype(this->time)>() * 220.0f)
				);
			this->time += 1 / decltype(this->time)(format.frequency());
			for(unsigned i = 0; i != format.num_channels(); ++i){
				ASSERT(buf.overlaps(dst))
				*dst = v;
				++dst;
			}
		}

//		TRACE_ALWAYS(<< "time = " << this->time << std::endl)
//		TRACE(<< "this->smpBuf = " << buf << std::endl)
	}
	
	SinePlayer(audout::format format) :
			format(format)
	{}
};

void play(audout::format format){
	SinePlayer pl(format);
	audout::player p(format, 1000, &pl);
	p.set_paused(false);

	std::this_thread::sleep_for(std::chrono::milliseconds(2000));
}


void test(){
	{
		TRACE_ALWAYS(<< "Opening audio playback device: Mono 11025" << std::endl)
		play(audout::format(audout::frame::mono, audout::rate::hz11025));
		TRACE_ALWAYS(<< "finished playing" << std::endl)
	}

	{
		TRACE_ALWAYS(<< "Opening audio playback device: Stereo 11025" << std::endl)
		play(audout::format(audout::frame::stereo, audout::rate::hz11025));
	}

	{
		TRACE_ALWAYS(<< "Opening audio playback device: Mono 22050" << std::endl)
		play(audout::format(audout::frame::mono, audout::rate::hz22050));
	}

	{
		TRACE_ALWAYS(<< "Opening audio playback device: Stereo 22050" << std::endl)
		play(audout::format(audout::frame::stereo, audout::rate::hz22050));
	}

	{
		TRACE_ALWAYS(<< "Opening audio playback device: Mono 44100" << std::endl)
		play(audout::format(audout::frame::mono, audout::rate::hz44100));
	}

	{
		TRACE_ALWAYS(<< "Opening audio playback device: Stereo 44100" << std::endl)
		play(audout::format(audout::frame::stereo, audout::rate::hz44100));
	}

	{
		TRACE_ALWAYS(<< "Opening audio playback device: Mono 48000" << std::endl)
		play(audout::format(audout::frame::mono, audout::rate::hz48000));
	}

	{
		TRACE_ALWAYS(<< "Opening audio playback device: Stereo 48000" << std::endl)
		play(audout::format(audout::frame::stereo, audout::rate::hz48000));
	}
}

#if M_OS_NAME == M_OS_NAME_ANDROID

JNIEXPORT void JNICALL Java_cppfw_github_io_audouttests_MainActivity_test(
		JNIEnv *env,
		jclass clazz,
		jstring chars
)
{
	test();
}

jint JNI_OnLoad(JavaVM* vm, void* reserved){
    TRACE_ALWAYS(<< "JNI_OnLoad(): invoked" << std::endl)

    JNIEnv* env;
    if (vm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6) != JNI_OK) {
        return -1;
    }

    static JNINativeMethod methods[] = {
            {"test", "()V", (void*)&Java_cppfw_github_io_audouttests_MainActivity_test},
    };
    jclass clazz = env->FindClass("cppfw/github/io/audouttests/MainActivity");
    ASSERT_ALWAYS(clazz)
    if(env->RegisterNatives(clazz, methods, 1) < 0){
        ASSERT_ALWAYS(false)
    }

    return JNI_VERSION_1_6;
}

#else
int main(int argc, char *argv[]){
	test();
	
	return 0;
}
#endif
