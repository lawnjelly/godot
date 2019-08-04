#include "callbacks.h"

void Callbacks::set_callback(String callback_name, Object * pObj, String func_name)
{
	// test creating a reference to see if the function exists
	FuncRef fr;
	fr.set_instance(pObj);
	fr.set_function(func_name);

	if (!fr.is_valid())
	{
		// the function does not exist
		WARN_PRINTS("The function " + func_name + " or object is invalid, ignoring.");
		return;
	}

	Func f;
	f.obj_id = pObj->get_instance_id();
	f.func_name = func_name;

	m_Funcs.insert(callback_name, f);
}

bool Callbacks::get_callback(String callback_name, FuncRef &fr)
{
	Map<String, Callbacks::Func>::Element * pF = m_Funcs.find(callback_name);
	if (!pF)
		return false;

	Object * pObj = ObjectDB::get_instance(pF->value().obj_id);
	if (!pObj)
	{
		// object is no longer valid!
		// delete entry from the Map?
		m_Funcs.erase(pF);
		return false;
	}

	fr.set_instance(pObj);
	fr.set_function(pF->value().func_name);
	return true;
}




