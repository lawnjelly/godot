#pragma once

#include "core/ustring.h"
#include <stdint.h>

// make sure bytes are packed correctly
#pragma pack(1)

namespace Sound { // namespace start

class LWAVImporter {
public:
	struct Header {
		uint32_t four_cc_RIFF = 0; // 'RIFF'
		int32_t file_size = 0; // - 8 bytes
		uint32_t four_cc_WAVE = 0; // 'WAVE'
		uint32_t four_cc_FMT = 0; // 'fmt '
		int32_t pcm_header_len = 0; // varies...
		int16_t format = 1; // 1 is pcm
		int16_t num_channels = 0; // 1,2 for stereo data is (l,r) pairs
		int32_t sample_rate = 0;
		int32_t bytes_per_second = 0; // ( sample rate * bits per sample * channels ) / 8
		int16_t block_align = 0; // [often 4] (bits per sample * channels) / 8.1 - 8 bit mono2  - 8 bit stereo / 16 bit mono4 - 16 bit stereo
		int16_t bits_per_sample = 0; // 16
	};

	struct ChunkHeader {
		uint32_t four_cc_DATA = 0; // 'data' or 'fact'
		uint32_t data_len = 0;
	};

	LWAVImporter();
	~LWAVImporter();

	uint8_t *load(const String &p_filename, uint32_t &r_num_samples, uint32_t &r_num_channels, uint32_t &r_sample_rate, uint32_t &r_bytes_per_channel);
	bool save(const String &p_filename, const uint8_t *p_data, uint32_t p_num_samples, uint32_t p_num_channels, uint32_t p_sample_rate, uint32_t p_bytes_per_channel);
	//	bool SaveWAV(const char * pszFilename, short * pData, unsigned int uiNumSamples, unsigned int uiNumChannels = 2, unsigned int uiSampleRate = 44100);
};

// only use this packing for this structure
#pragma pack()

} //namespace Sound
