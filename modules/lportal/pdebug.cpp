
#include "pdebug.h"

namespace Lawn
{

int PDebug::m_iLoggingLevel = 0; // 2
int PDebug::m_iWarningLevel = 0;
int PDebug::m_iTabDepth = 0;
bool PDebug::m_bRunning = true;


void PDebug::print(String sz)
{
	print_line(sz);
}


} // namespace
