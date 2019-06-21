#ifndef GAMELOGGER_H
#define GAMELOGGER_H

namespace Pel
{

class GoGameLogger
{
public:
	void Log(String p_string);

	int Size() {return m_Lines.size();}
	String Get(int i) {return m_Lines[i];}

	void Clear() {m_Lines.clear();}
	
	
private:
	Vector<String> m_Lines;
};


extern GoGameLogger g_GameLogger;

}
#endif
