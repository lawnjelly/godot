#pragma once

// most of the performance sensitive debug output will be compiled out in release builds
// you won't be able to get frame debugging of the visibility tree though.
#ifdef DEBUG_ENABLED

#pragma message ("LPortal DEBUG_ENABLED, verbose mode")
#define LPRINT_RUN(a, b) {if (!Lawn::PDebug::m_bRunning) {String sz;\
for (int n=0; n<Lawn::PDebug::m_iTabDepth; n++)\
sz += "\t";\
LPRINT(a, sz + b);}}

//#define LPRINT_RUN(a, b) ;

#else
#define LPRINT_RUN(a, b) ;
#endif

#ifdef LDEBUG_VERBOSE
#define LPRINT(a, b) {LPRINT_IMPL(a, b);}
#else
#define LPRINT(a, b) {if (!Lawn::PDebug::m_bRunning) {LPRINT_IMPL(a, b);}}
#endif

#define LPRINT_IMPL(a, b) {\
if (a >= Lawn::PDebug::m_iLoggingLevel)\
{\
Lawn::PDebug::print(b);\
}\
}


#define LWARN(a, b) if (a >= Lawn::PDebug::m_iWarningLevel)\
{\
Lawn::PDebug::print(String("\tWARNING : ") + b);\
}

String ftos(float f) {return String(Variant(f));}


namespace Lawn
{

class PDebug
{
public:
	static void print(String sz);
	static int m_iLoggingLevel;
	static int m_iWarningLevel;
	static bool m_bRunning;

	static int m_iTabDepth;
};

} // namespace

