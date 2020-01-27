#pragma once

namespace audout{

enum class frame_type{
	mono = 1,
	stereo = 2,
	quadro = 4,
	five_dot_one = 6,
	seven_dot_one = 8
};

enum class sampling_rate{
	hz11025 = 11025,
	hz22050 = 22050,
	hz44100 = 44100,
	hz48000 = 48000
};

class format{
public:
	audout::frame_type frame_type;
	
	constexpr static unsigned num_channels(audout::frame_type frame_type)noexcept{
		return unsigned(frame_type);
	}
	
	audout::sampling_rate sampling_rate;
	
	format(audout::frame_type frame_type, audout::sampling_rate sampling_rate) :
			frame_type(frame_type),
			sampling_rate(sampling_rate)
	{}
	
	unsigned num_channels()const noexcept{
		return num_channels(this->frame_type);
	}

	unsigned frequency()const noexcept{
		return unsigned(this->sampling_rate);
	}
	
	unsigned frame_size()const noexcept{
		return 2 * this->num_channels();
	}
};



}
