#pragma once

#include "thirdparty/bgfx/bgfx/include/bgfx/bgfx.h"

class BGFXTest {
	//	entry::MouseState m_mouseState;
	uint32_t m_width;
	uint32_t m_height;
	uint32_t m_debug;
	uint32_t m_reset;
	bgfx::VertexBufferHandle m_vbh;
	bgfx::IndexBufferHandle m_ibh[5];
	bgfx::ProgramHandle m_program;
	int64_t m_timeOffset;
	int32_t m_pt;

	bool m_r;
	bool m_g;
	bool m_b;
	bool m_a;

	bgfx::ProgramHandle loadProgram(const char *p_vert, const char *p_frag);

public:
	void run();
	void init(uint32_t _width, uint32_t _height);
	bool update();
};
