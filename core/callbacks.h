#ifndef CALLBACKS_H
#define CALLBACKS_H

#include "core/func_ref.h"

class Callbacks
{
	struct Func
	{
		int obj_id;
		String func_name;
	};
public:

	void set_callback(String callback_name, Object * pObj, String func_name);
	bool get_callback(String callback_name, FuncRef &fr);
private:

	Map<String, Callbacks::Func> m_Funcs;
};


#endif
