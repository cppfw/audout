/*
The MIT License (MIT)

Copyright (c) 2016-2025 Ivan Gagis <igagis@gmail.com>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

/* ================ LICENSE END ================ */

#pragma once

namespace audout {

/**
 * @brief Audio frame layout.
 * review: number of audio channels
 */
enum class frame {
	mono = 1,
	stereo = 2
};

/**
 * @brief Get number of audio channels for the given frame layout.
 * @param frame_type - frame layout.
 * @return number of audio channels for the given frame layout.
 */
constexpr inline unsigned num_channels(frame frame_type) noexcept
{
	return unsigned(frame_type);
}

/**
 * @brief Audio sampling rate.
 * review: in Hz
 */
enum class rate {
	hz_11025 = 11025,
	hz_22050 = 22050,
	hz_44100 = 44100,
	hz_48000 = 48000
};

/**
 * @brief Audio output format.
 */
class format
{
public:
	/**
	 * @brief Frame layout.
	 */
	// TODO: rename to frame_kind or frame_layout
	frame frame_type;

	/**
	 * @brief Sampling rate.
	 */
	rate sampling_rate;

	/**
	 * @brief Constructor.
	 * @param frame_type - frame layout.
	 * @param sampling_rate - sampling rate.
	 */
	format(
		// TODO: rename to frame_kind or frame_layout
		frame frame_type,//
		rate sampling_rate) :
		frame_type(frame_type),
		sampling_rate(sampling_rate)
	{}

	/**
	 * @brief Get number of audio channels.
	 * @return number of audio channels for this format.
	 */
	unsigned num_channels() const noexcept
	{
		return audout::num_channels(this->frame_type);
	}

	/**
	 * @brief Get sampling frequency.
	 * @return sampling frequency in hertz.
	 */
	// TODO: rename to sampling_rate_hz
	unsigned frequency() const noexcept
	{
		return unsigned(this->sampling_rate);
	}

	/**
	 * @brief Get size of a single audio frame in bytes.
	 * @return size of a single audio frame in bytes.
	 */
	unsigned frame_size() const noexcept
	{
		return 2 * this->num_channels();
	}
};

} // namespace audout
