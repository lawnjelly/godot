
#include "lmidi_file.h"
#include "core/os/os.h"
//#include <File/FiFile.h>
//#include <Common/CoDebug.h>

#ifdef USE_MEMORY_MANAGER
#include "../memory/mememory.h"
#endif

//#define CO_MIDI_VERBOSE

namespace Sound { // namespace start

unsigned int LMIDIFile::m_uiTrackTime = 0;
LMIDIFile::LTrack *LMIDIFile::m_pCurrentTrack = 0;

void LMIDIFile::flush_log() {
#ifdef CO_MIDI_VERBOSE
	print_line(_log);
#endif
	_log = "";
}

void LMIDIFile::Out(const char *p_string) {
#ifdef CO_MIDI_VERBOSE
	_log += p_string;
	//print_line(p_string);
#endif
}

void LMIDIFile::OutInt(int i) {
#ifdef CO_MIDI_VERBOSE
	_log += itos(i);
	//print_line(itos(i));
#endif
}

void LMIDIFile::MessBox(const char *p_mess_a, const char *p_mess_b) {
	print_line(p_mess_a);
	print_line(p_mess_b);
}

bool LMIDIFile::LTrack::RegisterNoteOff(unsigned int uiTime, unsigned int uiKey) {
	for (uint32_t n = 0; n < m_Notes.size(); n++) {
		if (m_Notes[n].m_Key == uiKey) {
			LNote *pNote = &m_Notes[n];

			// note this doesn't allow zero length notes
			if ((pNote->m_uiLength == UINT32_MAX) && (pNote->m_uiTime < uiTime)) {
				pNote->m_uiLength = uiTime - pNote->m_uiTime;
				return true;
			}
		}
	}

#ifdef CO_MIDI_VERBOSE
	print_line("WARNING : Noteoff with no noteon\n");
#endif
	return false; // no note on found for this note off, probably an error in the file
}

int32_t LMIDIFile::LTrack::find_track_start_and_end(int32_t &r_end) const {
	r_end = 0;
	int32_t start = INT32_MAX;

	for (uint32_t n = 0; n < m_Notes.size(); n++) {
		const LNote &note = m_Notes[n];
		if (note.m_uiLength == UINT32_MAX) {
			continue;
		}
		if ((int32_t)note.m_uiTime < start) {
			start = note.m_uiTime;
		}
		int32_t end = note.m_uiTime + note.m_uiLength;
		if (end > r_end) {
			r_end = end;
		}
	}

	if (start == INT32_MAX) {
		return 0;
	}

	return start;
}

LMIDIFile::LMIDIFile() {
}

LMIDIFile::~LMIDIFile() {
}

bool LMIDIFile::Load(FileAccess *pFile) {
	// make sure tracks etc are blanked before loading
	m_Tracks.clear();

	LMIDIHeader h;
	pFile->get_buffer((uint8_t *)&h, sizeof(h));

	// endian swap
	EndianSwapUI(h.m_uiHeaderLength);
	EndianSwapUS(h.m_usFormat);
	EndianSwapUS(h.m_usTracks);
	EndianSwapUS((uint16_t &)h.m_sDivision);

	m_Header = h;

	// check the magic header value
	if ((h.m_ucMagic[0] != 'M') ||
			(h.m_ucMagic[1] != 'T') ||
			(h.m_ucMagic[2] != 'h') ||
			(h.m_ucMagic[3] != 'd')) {
		MessBox("LMIDIFile::Load", "not a midi file");
		return false;
	}

	if (h.m_usTracks > 512) {
		MessBox("LMIDIFile::Load", "too many tracks");
		return false;
	}

#ifdef CO_MIDI_VERBOSE
	Out("** Loading MIDI file **\nTracks\t");
	OutInt(h.m_usTracks);
	Out("\nDivision\t");
	OutInt(h.m_sDivision);
	Out("\n\n");
#endif

	// read in each track
	for (int t = 0; t < h.m_usTracks; t++) {
#ifdef CO_MIDI_VERBOSE
		Out("Loading track\t");
		OutInt(t);
		Out("\n");
#endif
		if (!LoadTrack(pFile))
			return false;
	}

	return true;
}

bool LMIDIFile::LoadTrack(FileAccess *pFile) {
	// load track header
	LMIDITrackHeader h;
	if (pFile->get_buffer((uint8_t *)&h, sizeof(h)) != sizeof(h))
		return false;

	EndianSwapUI(h.m_uiNumBytes);

	// check the magic header value
	if ((h.m_ucMagic[0] != 'M') ||
			(h.m_ucMagic[1] != 'T') ||
			(h.m_ucMagic[2] != 'r') ||
			(h.m_ucMagic[3] != 'k')) {
		MessBox("LMIDIFile::LoadTrack", "track header incorrect");
		return false;
	}

#ifdef CO_MIDI_VERBOSE
	Out("NumBytes\t");
	OutInt(h.m_uiNumBytes);
	Out("\n");
#endif

	// initialize the track time to zero
	m_uiTrackTime = 0;
	LTrack t;
	m_Tracks.push_back(t);
	m_pCurrentTrack = &m_Tracks.back()->get();

	//m_pCurrentTrack = m_Tracks.RequestNewMember();

	unsigned int uiTrackBytes = 0;

	// keep loading events until the trackbytes are all used up
	while (uiTrackBytes < h.m_uiNumBytes) {
		if (!LoadEvent(pFile, uiTrackBytes)) {
			flush_log();
			return false;
		}
	}

	flush_log();
	return true;
}

bool LMIDIFile::LoadEvent(FileAccess *pFile, unsigned int &uiTrackBytes) {
	unsigned int iTimeDelta = 0;
	if (!ReadVariableLengthValue(pFile, iTimeDelta, uiTrackBytes))
		return false;

	// running tracktime
	m_uiTrackTime += iTimeDelta;

#ifdef CO_MIDI_VERBOSE
	//CoSleep(1);
	//OS::get_singleton()->delay_usec(1000);
	/*
	String sz = "Event time\t(";
	sz += itos (iTimeDelta);
	sz += ")\t";
	sz += itos (m_uiTrackTime);
	sz += "\t\t";
	print_line (sz);
*/
	Out("Event time\t(");
	OutInt(iTimeDelta);
	Out(")\t");

	OutInt(m_uiTrackTime);
	Out("\t\t");
#endif

	static unsigned char sucPreviousEventType = 0;

	// what type of event is this? midi, meta or sysex?
	unsigned char ucEventType;
	if (!pFile->get_buffer((uint8_t *)&ucEventType, 1))
		return false;
	uiTrackBytes++;

	// special case, for if the 128 bit is not set, it is a FOLLOW of the last type of event
	// (cubase seems to export with this convention)
	if (!(ucEventType & 128)) {
		// go back one in the file, and substitute the PREVIOUS event type byte
		file_seek(pFile, -1);
		//pFile->SeekFromCurrent(-1);
		uiTrackBytes--;
		ucEventType = sucPreviousEventType;
	}

	// record this event type just in case there are multiples
	sucPreviousEventType = ucEventType;

	// system exclusive?
	if ((ucEventType == 0xF0) || (ucEventType == 0xF7))
		return LoadSysEx(pFile, ucEventType, uiTrackBytes);

	// meta?
	if (ucEventType == 0xFF)
		return LoadMeta(pFile, uiTrackBytes);

	// midi event
	return LoadMidi(pFile, ucEventType, uiTrackBytes);
}

bool LMIDIFile::LoadMidi(FileAccess *pFile, unsigned char ucEventType, unsigned int &uiTrackBytes) {
	DEV_ASSERT(m_pCurrentTrack);

	unsigned char n1, n2; // nibble 1 and 2 (nibble is 4 bits)
	n1 = ucEventType & 0xF0;
	n2 = ucEventType & 0x0F;

	if (n1 == 0xF0)
		return LoadSystemMessage(pFile, ucEventType, uiTrackBytes);

	if (n1 == 0xB0)
		return LoadChannelMessage(pFile, n2, uiTrackBytes);

	// if none of these, it is an ordinary channel voice message
	unsigned int uiExtra = 2; // bytes to read
	switch (n1) {
		case 0xC0: // program change
		case 0xD0: // channel after touch
			uiExtra = 1;
			break;
	}

	// read the extra 1 or 2 data bytes
	uint8_t d1, d2;
	uint8_t uc[2];
	uc[1] = 0;
	uc[0] = 0;

	if (pFile->get_buffer(uc, uiExtra) != uiExtra)
		return false;
	uiTrackBytes += uiExtra;

	d1 = uc[0];
	d2 = uc[1];

	switch (n1) {
		case 0x90: // note on
		{
#ifdef CO_MIDI_VERBOSE
			Out("NoteON\tkey\t");
			OutInt(d1);
			Out("\t vel\t");
			OutInt(d2);
			Out("\n");
#endif
			if (d2) {
				LNote *pNote = m_pCurrentTrack->RequestNewNote();
				pNote->m_uiTime = m_uiTrackTime;
				pNote->m_Key = d1;
				pNote->m_Velocity = d2;
				pNote->m_uiLength = -1; // unset until a noteoff found
			} else {
				// counts as note off in some MIDI files
				m_pCurrentTrack->RegisterNoteOff(m_uiTrackTime, d1);
			}
		} break;
		case 0x80: // note off
#ifdef CO_MIDI_VERBOSE
			Out("NoteOFF\tkey\t");
			OutInt(d1);
			Out("\t vel\t");
			OutInt(d2);
			Out("\n");
#endif
			m_pCurrentTrack->RegisterNoteOff(m_uiTrackTime, d1); // velocity ignored for now
			break;
		case 0xA0:
#ifdef CO_MIDI_VERBOSE
			Out("PolyTouch\n");
#endif
			break;
		case 0xB0:
#ifdef CO_MIDI_VERBOSE
			Out("ControlChange\n");
#endif
			break;
		case 0xC0:
#ifdef CO_MIDI_VERBOSE
			Out("ProgramChange\n");
#endif
			break;
		case 0xD0:
#ifdef CO_MIDI_VERBOSE
			Out("ChannelTouch\n");
#endif
			break;
		case 0xE0:
#ifdef CO_MIDI_VERBOSE
			Out("PitchWheel\n");
#endif
			break;
		default:
#ifdef CO_MIDI_VERBOSE
			Out("unrecognised\n");
#endif
			break;
	}

	return true;
}

bool LMIDIFile::LoadChannelMessage(FileAccess *pFile, unsigned char ucChannel, unsigned int &uiTrackBytes) {
	Out("...channmess...\n");

	uint8_t uc[2];
	if (pFile->get_buffer(uc, 2) != 2)
		return false;
	uiTrackBytes += 2;

	return true;
}

bool LMIDIFile::LoadSystemMessage(FileAccess *pFile, unsigned char ucEventType, unsigned int &uiTrackBytes) {
	// extra bytes needed
	unsigned int uiExtra = 0;

	Out("...sysmess...\t");

	switch (ucEventType) {
		// SYSTEM COMMON MESSAGES
		case 0xF1: // MIDI TIME CODE
#ifdef CO_MIDI_VERBOSE
			Out("MTC\n");
#endif
			uiExtra = 1;
			break;
		case 0xF2: // SONG POSITION POINTER
#ifdef CO_MIDI_VERBOSE
			Out("SongPositionPointer\n");
#endif
			uiExtra = 2;
			break;
		case 0xF3: // SONG SELECT
#ifdef CO_MIDI_VERBOSE
			Out("SongSelect\n");
#endif
			uiExtra = 1;
			break;
		case 0xF4: // UNDEFINED
#ifdef CO_MIDI_VERBOSE
			Out("Undefined (F4)\n");
#endif
			uiExtra = 0;
			break;
		case 0xF5: // UNDEFINED
#ifdef CO_MIDI_VERBOSE
			Out("Undefined (F5)\n");
#endif
			uiExtra = 0;
			break;
		case 0xF6: // TUNE REQUEST
#ifdef CO_MIDI_VERBOSE
			Out("TuneRequest\n");
#endif
			uiExtra = 0;
			break;

		// SYSTEM REALTIME MESSAGES
		case 0xF8: // TIMING CLOCK
#ifdef CO_MIDI_VERBOSE
			Out("TimingClock\n");
#endif
			uiExtra = 0;
			break;
		case 0xF9: // UNDEFINED
#ifdef CO_MIDI_VERBOSE
			Out("Undefined (F9)\n");
#endif
			uiExtra = 0;
			break;
		case 0xFA: // START CURRENT SEQUENCE
#ifdef CO_MIDI_VERBOSE
			Out("Start Sequence\n");
#endif
			uiExtra = 0;
			break;
		case 0xFB: // CONTINUE CURRENT SEQUENCE
#ifdef CO_MIDI_VERBOSE
			Out("Continue Sequence\n");
#endif
			uiExtra = 0;
			break;
		case 0xFC: // STOP CURRENT SEQUENCE
#ifdef CO_MIDI_VERBOSE
			Out("Stop Sequence\n");
#endif
			uiExtra = 0;
			break;
		case 0xFD: // UNDEFINED
#ifdef CO_MIDI_VERBOSE
			Out("Undefined (FD)\n");
#endif
			uiExtra = 0;
			break;
		case 0xFE: // ACTIVE SENSING
#ifdef CO_MIDI_VERBOSE
			Out("Active Sensing\n");
#endif
			uiExtra = 0;
			break;
		case 0xFF: // RESET
#ifdef CO_MIDI_VERBOSE
			Out("Reset\n");
#endif
			uiExtra = 0;
			break;
			/*
				case 0xF1: // midi time code quarter frame
					uiExtra = 1;
					break;
				case 0xF2: // song position pointer
					uiExtra = 2;
					break;
				case 0xF3: // song select
					uiExtra = 1;
					break;
					*/
		default:
#ifdef CO_MIDI_VERBOSE
			Out("Unrecognised\n");
#endif
			// all the rest have no extra bytes
			break;
	}

	// seek through extra bytes
	if (uiExtra) {
		if (!file_seek(pFile, uiExtra))
			return false;

		uiTrackBytes += uiExtra;
	}

	return true;
}

bool LMIDIFile::file_seek(FileAccess *p_file, int32_t p_offset) {
	p_file->seek(p_file->get_position() + p_offset);
	return !p_file->eof_reached();
}

bool LMIDIFile::LoadMeta(FileAccess *pFile, unsigned int &uiTrackBytes) {
	// read the meta type
	uint8_t ucType;
	if (!pFile->get_buffer(&ucType, 1))
		return false;
	uiTrackBytes++;

	// read the variable length data size
	unsigned int iDataSize;
	if (!ReadVariableLengthValue(pFile, iDataSize, uiTrackBytes))
		return false;

	// ignore the data
	if (iDataSize >= 1024) {
		if (!file_seek(pFile, iDataSize))
			return false;
	} else {
		uint8_t m_szData[1024];
		memset(m_szData, 0, sizeof(m_szData));
		if (pFile->get_buffer(m_szData, iDataSize) != iDataSize)
			return false;

		switch (ucType) {
			case 0x01:
			case 0x02:
			case 0x03:
			case 0x04:
			case 0x05:
			case 0x06:
#ifdef CO_MIDI_VERBOSE
				Out("meta\t===\t");
				Out((const char *)m_szData);
				if (m_pCurrentTrack->_name == "") {
					m_pCurrentTrack->_name = (const char *)m_szData;
				}
				Out("\t===\n");
#endif
				break;
			case 0x2F:
#ifdef CO_MIDI_VERBOSE
				Out("end of track\n");
#endif
				break;
			case 0x51:
#ifdef CO_MIDI_VERBOSE
				Out("tempo\n");
#endif
				break;
			case 0x58:
#ifdef CO_MIDI_VERBOSE
				Out("time signature\n");
#endif
				break;
			case 0x59:
#ifdef CO_MIDI_VERBOSE
				Out("key signature\n");
#endif
				break;
			default:
#ifdef CO_MIDI_VERBOSE
				Out("meta\n");
#endif
				break;
		}
	}

	uiTrackBytes += iDataSize;

	return true;
}

bool LMIDIFile::LoadSysEx(FileAccess *pFile, unsigned char ucEventType, unsigned int &uiTrackBytes) {
	unsigned char ucTerminator = 0xF0;
	if (ucEventType == 0xF0)
		ucTerminator = 0xF7;

	while (1) {
		// attempt to read a byte
		uint8_t uc;
		if (!pFile->get_buffer(&uc, 1))
			return false;
		uiTrackBytes++;

		// end of the sysex event
		if (uc == ucTerminator)
			break;
	}

#ifdef CO_MIDI_VERBOSE
	Out("SYSTEM EXCLUSIVE\n");
#endif

	return true;
}

// Taken from http://home.roadrunner.com/~jgglatt/tech/midifile/vari.htm
bool LMIDIFile::ReadVariableLengthValue(FileAccess *pFile, unsigned int &uiValue, unsigned int &uiTrackBytes) {
	uint32_t value;
	uint8_t c;

	// read first byte
	if (!pFile->get_buffer(&c, 1))
		return false;
	value = c;

	uiTrackBytes++; // keep count of how many bytes read in

	if (value & 0x80) {
		value &= 0x7F;
		do {
			if (!pFile->get_buffer(&c, 1))
				return false; // premature EOF
			uiTrackBytes++; // keep count of how many bytes read in
			value = (value << 7) + (c & 0x7F);
		} while (c & 0x80);
	}

	uiValue = value;
	return true;
}

void LMIDIFile::WriteVariableLengthValue(FileAccess *pFile, int iValue) {
	uint32_t value = iValue;
	uint32_t buffer;
	buffer = value & 0x7F;

	while ((value >>= 7)) {
		buffer <<= 8;
		buffer |= ((value & 0x7F) | 0x80);
	}

	while (true) {
		pFile->store_buffer((uint8_t *)&buffer, 1);
		if (buffer & 0x80)
			buffer >>= 8;
		else
			break;
	}
}

} //namespace Sound
