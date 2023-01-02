#include "lwav_importer.h"

#include "core/os/file_access.h"
#include "core/print_string.h"

#define GODOT_FOURCC(a, b, c, d) ((uint32_t)(((d) << 24) | ((c) << 16) | ((b) << 8) | (a)))

namespace Sound { // namespace start

LWAVImporter::LWAVImporter() {
}

LWAVImporter::~LWAVImporter() {
}

bool LWAVImporter::save(const String &p_filename, const uint8_t *p_data, uint32_t p_num_samples, uint32_t p_num_channels, uint32_t p_sample_rate, uint32_t p_bytes_per_channel) {
	ERR_FAIL_COND_V_MSG((p_bytes_per_channel != 2) && (p_bytes_per_channel != 4), false, "Only 16 bit or float supported.");
	ERR_FAIL_COND_V(!p_num_samples, false);
	ERR_FAIL_COND_V(!p_num_channels, false);
	ERR_FAIL_COND_V(!p_bytes_per_channel, false);

	Error err;
	FileAccess *file = FileAccess::open(p_filename, FileAccess::WRITE, &err);

	ERR_FAIL_COND_V_MSG(err != OK, false, "Cannot open file for writing '" + p_filename + "'.");

#define IMWAV_ERROR(a)                                 \
	{                                                  \
		file->close();                                 \
		memdelete(file);                               \
		ERR_FAIL_V_MSG(false, p_filename + " : " + a); \
	}

	Header hdr;
	ChunkHeader chdr;

	hdr.four_cc_RIFF = GODOT_FOURCC('R', 'I', 'F', 'F');
	hdr.four_cc_WAVE = GODOT_FOURCC('W', 'A', 'V', 'E');
	hdr.four_cc_FMT = GODOT_FOURCC('f', 'm', 't', ' ');
	hdr.format = p_bytes_per_channel == 4 ? 3 : 1;
	hdr.num_channels = p_num_channels;
	hdr.sample_rate = p_sample_rate;
	hdr.pcm_header_len = 16;
	hdr.bits_per_sample = p_bytes_per_channel == 4 ? 32 : 16;
	hdr.block_align = (p_num_channels * hdr.bits_per_sample) / 8;
	hdr.bytes_per_second = p_sample_rate * p_num_channels * p_bytes_per_channel;

	uint32_t data_size = p_num_samples * p_num_channels * p_bytes_per_channel;

	hdr.file_size = data_size + sizeof(Header) + sizeof(ChunkHeader) - 8;

	chdr.four_cc_DATA = GODOT_FOURCC('d', 'a', 't', 'a');
	chdr.data_len = data_size;

	file->store_buffer((const uint8_t *)&hdr, sizeof(Header));
	file->store_buffer((const uint8_t *)&chdr, sizeof(ChunkHeader));
	file->store_buffer(p_data, data_size);

	file->flush();
	file->close();
	memdelete(file);

#undef IMWAV_ERROR
	return true;
}

uint8_t *LWAVImporter::load(const String &p_filename, uint32_t &r_num_samples, uint32_t &r_num_channels, uint32_t &r_sample_rate, uint32_t &r_bytes_per_channel) {
	r_bytes_per_channel = 2;

	Error err;
	FileAccess *file = FileAccess::open(p_filename, FileAccess::READ, &err);

	ERR_FAIL_COND_V_MSG(err != OK, nullptr, "Cannot open file '" + p_filename + "'.");

	Header hdr;
	ChunkHeader chdr;

	// cut down on the code needed
#define IMWAV_ERROR(a)                                   \
	{                                                    \
		file->close();                                   \
		memdelete(file);                                 \
		ERR_FAIL_V_MSG(nullptr, p_filename + " : " + a); \
	}

	// read wav header
	if (file->get_buffer((uint8_t *)&hdr, sizeof(Header)) != sizeof(Header))
		IMWAV_ERROR("File is not a WAV");

	// more sanity checks
	if (hdr.four_cc_RIFF != GODOT_FOURCC('R', 'I', 'F', 'F'))
		IMWAV_ERROR("Bad RIFF format");

	if (hdr.four_cc_WAVE != GODOT_FOURCC('W', 'A', 'V', 'E'))
		IMWAV_ERROR("Bad WAVE format");

	if (hdr.four_cc_FMT != GODOT_FOURCC('f', 'm', 't', ' '))
		IMWAV_ERROR("Bad fmt format");

	if (hdr.format != 1)
		IMWAV_ERROR("bad wav wFormatTag");

	if ((hdr.bits_per_sample != 32) && (hdr.bits_per_sample != 24) && (hdr.bits_per_sample != 16) && (hdr.bits_per_sample != 8))
		IMWAV_ERROR("bad wav nBitsPerSample");

	switch (hdr.bits_per_sample) {
		case 32: {
			r_bytes_per_channel = 4;
		} break;
		case 24: {
			r_bytes_per_channel = 3;
		} break;
		case 16: {
			r_bytes_per_channel = 2;
		} break;

		default: {
			IMWAV_ERROR("bad wav nBitsPerSample");
		} break;
	}

	// skip over any remaining portion of wav header
	long int rmore = hdr.pcm_header_len - (sizeof(Header) - 20);
	file->seek(file->get_position() + rmore);

	// read chunks until a 'data' chunk is found
	int sflag = 1;
	while (sflag != 0) {
		// check attempts
		if (sflag > 10)
			IMWAV_ERROR("too many chunks");

		// read chunk header
		if (file->get_buffer((uint8_t *)&chdr, sizeof(ChunkHeader)) != sizeof(ChunkHeader))
			IMWAV_ERROR("cant read chunk");

		// check chunk type
		if (chdr.four_cc_DATA == GODOT_FOURCC('d', 'a', 't', 'a'))
			break;

		// skip over chunk
		sflag++;
		file->seek(file->get_position() + chdr.data_len);
	}

	// not done 8 bit yet
	if (hdr.bits_per_sample == 8)
		IMWAV_ERROR("no support for 8bit yet");

	// num channels
	r_num_channels = hdr.num_channels;
	r_sample_rate = hdr.sample_rate;

	// num samples
	uint32_t num_channel_samples = chdr.data_len;
	uint32_t bytes_per_channel = hdr.bits_per_sample / 8;
	num_channel_samples /= bytes_per_channel;
	r_num_samples = num_channel_samples / r_num_channels;

	if (!r_num_samples)
		IMWAV_ERROR("WAV contains no samples");

	//	DEV_ASSERT(num_channel_samples == (chdr.data_len / 2));

	// allocate buffer
	uint8_t *output = nullptr;

	// TODO alignment
	//	output = (uint8_t *)memalloc(num_channel_samples * 2);
	output = (uint8_t *)memalloc(chdr.data_len);

	// read in the data
	if (file->get_buffer((uint8_t *)output, chdr.data_len) != chdr.data_len) {
		memfree(output);
		output = nullptr;
		IMWAV_ERROR("WAV data is incorrect length");
	}

	file->close();
	memdelete(file);

	// debug info
	String sz;
	sz = "Read WAV: ";
	sz += p_filename;
	sz += "\nNumChannels\t";
	sz += itos(r_num_channels);
	sz += "\nNumSamples\t";
	sz += itos(r_num_samples);
	sz += "\nSampleRate\t";
	sz += itos(r_sample_rate);
	sz += "\n\n";
	print_line(sz);

#undef IMWAV_ERROR
	return (uint8_t *)output;
}

#if 0
bool LWAVImporter::SaveWAV(const char * pszFilename, short * pData, unsigned int uiNumSamples, unsigned int uiNumChannels, unsigned int uiSampleRate)
{
//	dr_wav can also be used to output WAV files. This does not currently support compressed formats. To use this, look at `drwav_init_write()`,
//	`drwav_init_file_write()`, etc. Use `drwav_write_pcm_frames()` to write samples, or `drwav_write_raw()` to write raw data in the "data" chunk.
	
//	```c
//	drwav_data_format format;
//	format.container = drwav_container_riff;     // <-- drwav_container_riff = normal WAV files, drwav_container_w64 = Sony Wave64.
//	format.format = DR_WAVE_FORMAT_PCM;          // <-- Any of the DR_WAVE_FORMAT_* codes.
//	format.channels = 2;
//	format.sampleRate = 44100;
//	format.bitsPerSample = 16;
//	drwav_init_file_write(&wav, "data/recording.wav", &format, NULL);
	
//	...
	
//	drwav_uint64 framesWritten = drwav_write_pcm_frames(pWav, frameCount, pSamples);
//	```

	drwav_data_format format;
	format.container = drwav_container_riff;     // <-- drwav_container_riff = normal WAV files, drwav_container_w64 = Sony Wave64.
	format.format = DR_WAVE_FORMAT_PCM;          // <-- Any of the DR_WAVE_FORMAT_* codes.
	format.channels = uiNumChannels;
	format.sampleRate = uiSampleRate;
	format.bitsPerSample = 16;
	
	
	drwav drwav_data;
	drwav_init_file_write(&drwav_data, pszFilename, &format, NULL);
	
	drwav_uint64 framesWritten = drwav_write_pcm_frames(&drwav_data, uiNumSamples, pData);

    drwav_uninit(&drwav_data);
	return true;
}
#endif

} //namespace Sound
