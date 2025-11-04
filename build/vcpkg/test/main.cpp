#include <iostream>

#include <audout/player.hpp>

struct dummy_player : public audout::listener {
	void fill(utki::span<int16_t> buf) noexcept override
	{}

	dummy_player()
	{}
};

int main(int argc, const char** argv){
    dummy_player pl;
	audout::player p(
		audout::format(audout::frame::mono, audout::rate::hz_11025), // dummy values
		1, // play buffer size in frames, 1 as a dummy value
		&pl
	);
	p.set_paused(false);

    std::cout << "Hello audout!" << std::endl;

    return 0;
}
