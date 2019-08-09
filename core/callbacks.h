#ifndef CALLBACKS_H
#define CALLBACKS_H

#include "core/func_ref.h"

class Callbacks
{
	struct Func
	{
		int obj_id; // if this is set to -1, the callback is unset
		String func_name;
		String callback_name;
	};
public:

	// returns the ID of the callback
	int create_callback(String callback_name);

	bool set_callback(String callback_name, Object * pObj, String func_name);
	bool get_callback(String callback_name, FuncRef &fr);
	bool get_callback(int callback_ID, FuncRef &fr);
private:
	int find_callback(String callback_name);

	//Map<String, Callbacks::Func> m_Funcs;
	Vector<Callbacks::Func> m_Funcs;
};


#endif
