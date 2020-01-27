#pragma once

namespace audout{

enum class frame{
	mono = 1,
	stereo = 2
};

constexpr inline unsigned num_channels(frame frame_type)noexcept{
	return unsigned(frame_type);
}

enum class rate{
	hz11025 = 11025,
	hz22050 = 22050,
	hz44100 = 44100,
	hz48000 = 48000
};

class format{
public:
	frame frame_type;
	
	rate sampling_rate;
	
	format(frame frame_type, rate sampling_rate) :
			frame_type(frame_type),
			sampling_rate(sampling_rate)
	{}
	
	unsigned num_channels()const noexcept{
		return audout::num_channels(this->frame_type);
	}

	unsigned frequency()const noexcept{
		return unsigned(this->sampling_rate);
	}
	
	unsigned frame_size()const noexcept{
		return 2 * this->num_channels();
	}
};



}
