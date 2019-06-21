#include "GoGameLogger.h"

namespace Pel
{

GoGameLogger g_GameLogger;

void GoGameLogger::Log(String p_string)
{
	m_Lines.push_back(p_string);
	print_line(p_string);
}
	
	
	
}
