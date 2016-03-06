#include <utki/math.hpp>
#include <nitki/Thread.hpp>

#include "../../src/audout/Player.hpp"


struct SinePlayer : public audout::Listener{
	double time = 0;

	audout::AudioFormat format;
	
	void fillPlayBuf(utki::Buf<std::int16_t> buf)noexcept override{
//			TRACE_ALWAYS(<< "filling smp buf, freq = " << freq << std::endl)

		for(auto dst = buf.begin(); dst != buf.end();){
			std::int16_t v = std::int16_t(decltype(this->time)(0x7fff) * std::sin(this->time * utki::twoPi<decltype(this->time)>() * 220.0f));
			this->time += 1 / decltype(this->time)(format.frequency());
			for(unsigned i = 0; i != format.numChannels(); ++i){
				ASSERT(buf.overlaps(dst))
				*dst = v;
				++dst;
			}
		}

//			TRACE_ALWAYS(<< "time = " << this->time << std::endl)
//			TRACE(<< "this->smpBuf = " << buf << std::endl)
	}
	
	SinePlayer(audout::AudioFormat format) :
			format(format)
	{}
};

void play(audout::AudioFormat format){
	SinePlayer pl(format);
	audout::Player p(format, 1000, &pl);
	p.setPaused(false);

	nitki::Thread::sleep(3000);
}


int main(int argc, char *argv[]){
	/*
	{
		TRACE_ALWAYS(<< "Opening audio playback device: Mono 11025" << std::endl)
		play(audout::AudioFormat(audout::AudioFormat::EFrame::MONO, audout::AudioFormat::ESamplingRate::HZ_11025));
		TRACE_ALWAYS(<< "finished playing" << std::endl)
	}
	
	{
		TRACE_ALWAYS(<< "Opening audio playback device: Stereo 11025" << std::endl)
		play(audout::AudioFormat(audout::AudioFormat::EFrame::STEREO, audout::AudioFormat::ESamplingRate::HZ_11025));
	}
	
	{
		TRACE_ALWAYS(<< "Opening audio playback device: Mono 22050" << std::endl)
		play(audout::AudioFormat(audout::AudioFormat::EFrame::MONO, audout::AudioFormat::ESamplingRate::HZ_22050));
	}
	
	{
		TRACE_ALWAYS(<< "Opening audio playback device: Stereo 22050" << std::endl)
		play(audout::AudioFormat(audout::AudioFormat::EFrame::STEREO, audout::AudioFormat::ESamplingRate::HZ_22050));
	}
	
	{
		TRACE_ALWAYS(<< "Opening audio playback device: Mono 44100" << std::endl)
		play(audout::AudioFormat(audout::AudioFormat::EFrame::MONO, audout::AudioFormat::ESamplingRate::HZ_44100));
	}
	
	{
		TRACE_ALWAYS(<< "Opening audio playback device: Stereo 44100" << std::endl)
		play(audout::AudioFormat(audout::AudioFormat::EFrame::STEREO, audout::AudioFormat::ESamplingRate::HZ_44100));
	}
	*/
	{
		TRACE_ALWAYS(<< "Opening audio playback device: Mono 48000" << std::endl)
		play(audout::AudioFormat(audout::AudioFormat::EFrame::MONO, audout::AudioFormat::ESamplingRate::HZ_48000));
	}
	
	{
		TRACE_ALWAYS(<< "Opening audio playback device: Stereo 48000" << std::endl)
		play(audout::AudioFormat(audout::AudioFormat::EFrame::STEREO, audout::AudioFormat::ESamplingRate::HZ_48000));
	}
	
	return 0;
}
