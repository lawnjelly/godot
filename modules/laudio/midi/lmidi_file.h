#pragma once

namespace File {
class FiFile;
}

#include "core/list.h"
#include "core/local_vector.h"
#include "core/os/file_access.h"
#include <stdint.h>
//#include <Core/CoLinkedList.h>
//#include <vector>
//#include <Core/CoTypes.h>

namespace Sound { // namespace start

#pragma pack(push, midi_pack)
#pragma pack(1)

class LMIDIFile {
public:
	// NOTE that the numbers in midi files are BIG endian, and must be converted to little endian for PC, and vice versa
	class LMIDIHeader {
	public:
		uint8_t m_ucMagic[4]; // always MThd
		uint32_t m_uiHeaderLength; // always 6
		uint16_t m_usFormat;
		// 0 = single track file format
		// 1 = multiple track file format
		// 2 = multiple song file format (i.e., a series of type 0 files)
		uint16_t m_usTracks; // number of track chunks that follow the header
		int16_t m_sDivision;
		// unit of time for delta timing. If the value is positive, then it represents the units per beat.
		// For example, +96 would mean 96 ticks per beat. If the value is negative, delta times
		// are in SMPTE compatible units.
	};

	class LMIDITrackHeader {
	public:
		uint8_t m_ucMagic[4]; // always MTrk
		uint32_t m_uiNumBytes; // number of bytes in the track following from here
	};

	class LTrackEvent {
	public:
		int32_t m_iDeltaTime; // since last event
	};

	class LMIDIEvent : public LTrackEvent {
	public:
	};

	class LMetaEvent : public LTrackEvent {
	public:
		int32_t m_iMetaType; // 1 byte
		int32_t m_iNumBytes; // num bytes of data
		// data follows
	};

	// unused as we are ignoring sysex events
	// sysex_event = 0xF0 + <data_bytes> 0xF7 or sysex_event = 0xF7 + <data_bytes> 0xF7
	// In the first case, the resultant MIDI data stream would include the 0xF0. In the second case the 0xF0 is omitted.
	class LSysExEvent : public LTrackEvent {
	public:
		int32_t m_iNumBytes;
		// data follows
	};

	class LNote {
	public:
		uint32_t m_uiTime;
		uint32_t m_uiLength;
		uint16_t m_Key;
		uint16_t m_Velocity;
	};

	class LTrack {
	public:
		LNote *RequestNewNote() {
			m_Notes.resize(m_Notes.size() + 1);
			return &m_Notes[m_Notes.size() - 1];
		}
		bool RegisterNoteOff(unsigned int uiTime, unsigned int uiKey); // set the length of any notes that this is the note off for

		// access
		unsigned int GetNumNotes() const { return m_Notes.size(); }
		const LNote *GetNote(unsigned int ui) const { return &m_Notes[ui]; }

		int32_t find_track_start_and_end(int32_t &r_end) const;

		String _name;

	private:
		LocalVector<LNote> m_Notes;
	};

	LMIDIFile();
	~LMIDIFile();

	// funcs
	bool Load(FileAccess *pFile);

	// access
	short GetDivision() const { return m_Header.m_sDivision; }
	unsigned int GetNumTracks() const { return m_Header.m_usTracks; }
	//	CoTrack * GetFirstTrack() {return m_Tracks.GetFirstMember();}
	//	CoTrack * GetNextTrack() {return m_Tracks.GetMember();}
	const LTrack &GetTrack(uint32_t n) { return m_Tracks[n]; }

private:
	void Out(const char *p_string);
	void OutInt(int i);
	void MessBox(const char *p_mess_a, const char *p_mess_b);
	void flush_log();

	// internal funcs
	bool LoadTrack(FileAccess *pFile);
	bool LoadEvent(FileAccess *pFile, unsigned int &uiTrackBytes);
	bool LoadSysEx(FileAccess *pFile, unsigned char ucEventType, unsigned int &uiTrackBytes);
	bool LoadMeta(FileAccess *pFile, unsigned int &uiTrackBytes);
	bool LoadMidi(FileAccess *pFile, unsigned char ucEventType, unsigned int &uiTrackBytes);

	bool LoadSystemMessage(FileAccess *pFile, unsigned char ucEventType, unsigned int &uiTrackBytes);
	bool LoadChannelMessage(FileAccess *pFile, unsigned char ucChannel, unsigned int &uiTrackBytes);

	// MIDI encoding tries to use minimum bytes to store values
	bool ReadVariableLengthValue(FileAccess *pFile, unsigned int &uiValue, unsigned int &uiTrackBytes);
	void WriteVariableLengthValue(FileAccess *pFile, int iValue);

	bool file_seek(FileAccess *p_file, int32_t p_offset);

	// mem vars
	LMIDIHeader m_Header;
	List<LTrack> m_Tracks;
	static uint32_t m_uiTrackTime; // running time through the track when loading / saving
	static LTrack *m_pCurrentTrack;
	String _log;

	void EndianSwapUS(uint16_t &x) {
		x = (x >> 8) |
				(x << 8);
	}

	void EndianSwapUI(uint32_t &x) {
		x = (x >> 24) |
				((x << 8) & 0x00FF0000) |
				((x >> 8) & 0x0000FF00) |
				(x << 24);
	}

	void EndianSwapU64(uint64_t &x) {
		x = (x >> 56) |
				((x << 40) & 0x00FF000000000000) |
				((x << 24) & 0x0000FF0000000000) |
				((x << 8) & 0x000000FF00000000) |
				((x >> 8) & 0x00000000FF000000) |
				((x >> 24) & 0x0000000000FF0000) |
				((x >> 40) & 0x000000000000FF00) |
				(x << 56);
	}
};

#pragma pack(pop, midi_pack)

} //namespace Sound
