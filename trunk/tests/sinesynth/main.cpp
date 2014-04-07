#include <ting/math.hpp>
#include <ting/mt/Thread.hpp>

#include "../../src/audout/Player.hpp"


struct SinePlayer : public audout::PlayerListener{
	ting::Inited<float, 0> time;

	audout::AudioFormat format;
	
	//override
	void FillPlayBuf(ting::Buffer<ting::s16>& buf)throw(){
//			TRACE_ALWAYS(<< "filling smp buf, freq = " << freq << std::endl)

		for(ting::s16* dst = buf.Begin(); dst != buf.End();){
			ting::s16 v = float(0x7fff) * ting::math::Sin<float>(this->time * ting::math::D2Pi<float>() * 440.0f);
			this->time += 1 / float(format.samplingRate.Frequency());
			for(unsigned i = 0; i != format.frame.NumChannels(); ++i){
				ASSERT(buf.Overlaps(dst))
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

void Play(audout::AudioFormat format){
	SinePlayer pl(format);
	ting::Ptr<audout::Player> p = audout::Player::CreatePlayer(
			format,
			1000,
			&pl);
	p->SetPaused(false);

	ting::mt::Thread::Sleep(1000);
}


int main(int argc, char *argv[]){
	
	{
		TRACE_ALWAYS(<< "Opening audio playback device: Mono 11025" << std::endl)
		Play(audout::AudioFormat(audout::AudioFormat::Frame::MONO, audout::AudioFormat::SamplingRate::HZ_11025));
		TRACE_ALWAYS(<< "finished playing" << std::endl)
	}
	
	{
		TRACE_ALWAYS(<< "Opening audio playback device: Stereo 11025" << std::endl)
		Play(audout::AudioFormat(audout::AudioFormat::Frame::STEREO, audout::AudioFormat::SamplingRate::HZ_11025));
	}
	//TODO:
	{
		TRACE_ALWAYS(<< "Opening audio playback device: Mono 22050" << std::endl)
		Play(audout::AudioFormat(audout::AudioFormat::Frame::MONO, audout::AudioFormat::SamplingRate::HZ_22050));
	}
	
	{
		TRACE_ALWAYS(<< "Opening audio playback device: Stereo 22050" << std::endl)
		Play(audout::AudioFormat(audout::AudioFormat::Frame::STEREO, audout::AudioFormat::SamplingRate::HZ_22050));
	}
	
	{
		TRACE_ALWAYS(<< "Opening audio playback device: Mono 44100" << std::endl)
		Play(audout::AudioFormat(audout::AudioFormat::Frame::MONO, audout::AudioFormat::SamplingRate::HZ_44100));
	}
	
	{
		TRACE_ALWAYS(<< "Opening audio playback device: Stereo 44100" << std::endl)
		Play(audout::AudioFormat(audout::AudioFormat::Frame::STEREO, audout::AudioFormat::SamplingRate::HZ_44100));
	}
	
	return 0;
}
