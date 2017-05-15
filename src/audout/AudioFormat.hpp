#pragma once

namespace audout{

enum class Frame_e{
	MONO = 1,
	STEREO = 2,
	QUADRO = 4,
	FIVE_DOT_ONE = 6,
	SEVEN_DOT_ONE = 8
};

enum class SamplingRate_e{
	HZ_11025 = 11025,
	HZ_22050 = 22050,
	HZ_44100 = 44100,
	HZ_48000 = 48000
};

class AudioFormat{
public:
	Frame_e frame;
	
	constexpr static unsigned numChannels(Frame_e frameType)noexcept{
		return unsigned(frameType);
	}
	
	SamplingRate_e samplingRate;
	
	AudioFormat(Frame_e frame, SamplingRate_e samplingRate) :
			frame(frame),
			samplingRate(samplingRate)
	{}
	
	unsigned numChannels()const noexcept{
		return numChannels(this->frame);
	}

	unsigned frequency()const noexcept{
		return unsigned(this->samplingRate);
	}
	
	unsigned bytesPerFrame()const noexcept{
		return 2 * this->numChannels();
	}
};



}
