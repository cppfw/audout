#include <chrono>
#include <ratio>

#include <nitki/thread.hpp>
#include <utki/config.hpp>
#include <utki/math.hpp>

#include "../../src/audout/player.hpp"

#if CFG_OS_NAME == CFG_OS_NAME_ANDROID
#	include <jni.h>
#endif

struct sine_player : public audout::listener {
	double time = 0;

	audout::format format;

	void fill(utki::span<std::int16_t> buf) noexcept override
	{
		constexpr auto sine_freq = 220.0f;

		for (auto dst = buf.begin(); dst != buf.end();) {
			auto v = int16_t(
				decltype(this->time)(std::numeric_limits<int16_t>::max()) *
				std::sin(this->time * 2 * utki::pi * sine_freq)
			);
			this->time += 1 / decltype(this->time)(format.frequency());
			for (unsigned i = 0; i != format.num_channels(); ++i) {
				ASSERT(utki::overlaps(buf, dst))
				*dst = v;
				++dst;
			}
		}
	}

	sine_player(audout::format format) :
		format(format)
	{}
};

constexpr auto play_buffer_size_frames = 1000;

void play(audout::format format)
{
	sine_player pl(format);
	audout::player p(
		format, //
		play_buffer_size_frames,
		&pl
	);
	p.set_paused(false);

	std::this_thread::sleep_for(std::chrono::milliseconds(2 * std::milli::den));
}

void test()
{
	{
		utki::log([&](auto& o) {
			o << "Opening audio playback device: Mono 11025" << std::endl;
		});
		play(audout::format(audout::frame::mono, audout::rate::hz_11025));
		utki::log([&](auto& o) {
			o << "finished playing" << std::endl;
		});
	}

	{
		utki::log([&](auto& o) {
			o << "Opening audio playback device: Stereo 11025" << std::endl;
		});
		play(audout::format(audout::frame::stereo, audout::rate::hz_11025));
	}

	{
		utki::log([&](auto& o) {
			o << "Opening audio playback device: Mono 22050" << std::endl;
		});
		play(audout::format(audout::frame::mono, audout::rate::hz_22050));
	}

	{
		utki::log([&](auto& o) {
			o << "Opening audio playback device: Stereo 22050" << std::endl;
		});
		play(audout::format(audout::frame::stereo, audout::rate::hz_22050));
	}

	{
		utki::log([&](auto& o) {
			o << "Opening audio playback device: Mono 44100" << std::endl;
		});
		play(audout::format(audout::frame::mono, audout::rate::hz_44100));
	}

	{
		utki::log([&](auto& o) {
			o << "Opening audio playback device: Stereo 44100" << std::endl;
		});
		play(audout::format(audout::frame::stereo, audout::rate::hz_44100));
	}

	{
		utki::log([&](auto& o) {
			o << "Opening audio playback device: Mono 48000" << std::endl;
		});
		play(audout::format(audout::frame::mono, audout::rate::hz_48000));
	}

	{
		utki::log([&](auto& o) {
			o << "Opening audio playback device: Stereo 48000" << std::endl;
		});
		play(audout::format(audout::frame::stereo, audout::rate::hz_48000));
	}
}

#if CFG_OS_NAME == CFG_OS_NAME_ANDROID

JNIEXPORT void JNICALL Java_cppfw_github_io_audouttests_MainActivity_test(JNIEnv* env, jclass clazz, jstring chars)
{
	test();
}

jint JNI_OnLoad(JavaVM* vm, void* reserved)
{
	utki::log([&](auto& o) {
		o << "JNI_OnLoad(): invoked" << std::endl;
	});

	JNIEnv* env;
	if (vm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6) != JNI_OK) {
		return -1;
	}

	static JNINativeMethod methods[] = {
		{"test", "()V", (void*)&Java_cppfw_github_io_audouttests_MainActivity_test},
	};
	jclass clazz = env->FindClass("cppfw/github/io/audouttests/MainActivity");
	utki::assert(clazz, SL);
	if (env->RegisterNatives(clazz, methods, 1) < 0) {
		utki::assert(false, SL);
	}

	return JNI_VERSION_1_6;
}

#else
int main(int argc, char* argv[])
{
	test();

	return 0;
}
#endif
