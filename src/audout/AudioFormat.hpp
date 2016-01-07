/**
 * @author Ivan Gagis <igagis@gmail.com>
 */

#pragma once

namespace audout{


class AudioFormat {
public:
	enum class EFrame{
		MONO = 1,
		STEREO = 2,
		QUADRO = 4,
		FIVE_DOT_ONE = 6,
		SEVEN_DOT_ONE = 8
	} frame;
	
	enum class ESamplingRate{
		HZ_11025 = 11025,
		HZ_22050 = 22050,
		HZ_44100 = 44100,
		HZ_48000 = 48000
	} samplingRate;
	
	AudioFormat(EFrame frame, ESamplingRate samplingRate) :
			frame(frame),
			samplingRate(samplingRate)
	{}
	
	unsigned numChannels()const noexcept{
		return unsigned(this->frame);
	}

	unsigned frequency()const noexcept{
		return unsigned(this->samplingRate);
	}
	
	unsigned bytesPerFrame()const noexcept{
		return 2 * numChannels();
	}
};



}//~namespace
