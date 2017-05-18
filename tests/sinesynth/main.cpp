#include <utki/math.hpp>
#include <utki/config.hpp>
#include <nitki/Thread.hpp>

#include "../../src/audout/Player.hpp"

#if M_OS_NAME == M_OS_NAME_ANDROID
#	include <jni.h>
#endif


struct SinePlayer : public audout::Listener{
	double time = 0;

	audout::AudioFormat format;
	
	void fillPlayBuf(utki::Buf<std::int16_t> buf)noexcept override{
//		TRACE_ALWAYS(<< "filling smp buf" << std::endl)

		for(auto dst = buf.begin(); dst != buf.end();){
			std::int16_t v = std::int16_t(
					decltype(this->time)(0x7fff) * std::sin(this->time * 2 * utki::pi<decltype(this->time)>() * 110.0f)
				);
			this->time += 1 / decltype(this->time)(format.frequency());
			for(unsigned i = 0; i != format.numChannels(); ++i){
				ASSERT(buf.overlaps(dst))
				*dst = v;
				++dst;
			}
		}

//		TRACE_ALWAYS(<< "time = " << this->time << std::endl)
//		TRACE(<< "this->smpBuf = " << buf << std::endl)
	}
	
	SinePlayer(audout::AudioFormat format) :
			format(format)
	{}
};

void play(audout::AudioFormat format){
	SinePlayer pl(format);
	audout::Player p(format, 1000, &pl);
	p.setPaused(false);

	nitki::Thread::sleep(2000);
}


void test(){
	{
		TRACE_ALWAYS(<< "Opening audio playback device: Mono 11025" << std::endl)
		play(audout::AudioFormat(audout::Frame_e::MONO, audout::SamplingRate_e::HZ_11025));
		TRACE_ALWAYS(<< "finished playing" << std::endl)
	}

	{
		TRACE_ALWAYS(<< "Opening audio playback device: Stereo 11025" << std::endl)
		play(audout::AudioFormat(audout::Frame_e::STEREO, audout::SamplingRate_e::HZ_11025));
	}

	{
		TRACE_ALWAYS(<< "Opening audio playback device: Mono 22050" << std::endl)
		play(audout::AudioFormat(audout::Frame_e::MONO, audout::SamplingRate_e::HZ_22050));
	}

	{
		TRACE_ALWAYS(<< "Opening audio playback device: Stereo 22050" << std::endl)
		play(audout::AudioFormat(audout::Frame_e::STEREO, audout::SamplingRate_e::HZ_22050));
	}

	{
		TRACE_ALWAYS(<< "Opening audio playback device: Mono 44100" << std::endl)
		play(audout::AudioFormat(audout::Frame_e::MONO, audout::SamplingRate_e::HZ_44100));
	}

	{
		TRACE_ALWAYS(<< "Opening audio playback device: Stereo 44100" << std::endl)
		play(audout::AudioFormat(audout::Frame_e::STEREO, audout::SamplingRate_e::HZ_44100));
	}

	{
		TRACE_ALWAYS(<< "Opening audio playback device: Mono 48000" << std::endl)
		play(audout::AudioFormat(audout::Frame_e::MONO, audout::SamplingRate_e::HZ_48000));
	}

	{
		TRACE_ALWAYS(<< "Opening audio playback device: Stereo 48000" << std::endl)
		play(audout::AudioFormat(audout::Frame_e::STEREO, audout::SamplingRate_e::HZ_48000));
	}
}

#if M_OS_NAME == M_OS_NAME_ANDROID

JNIEXPORT void JNICALL Java_igagis_github_io_audouttests_MainActivity_test(
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
            {"test", "()V", (void*)&Java_igagis_github_io_audouttests_MainActivity_test},
    };
    jclass clazz = env->FindClass("igagis/github/io/audouttests/MainActivity");
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
