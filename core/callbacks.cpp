#include "callbacks.h"

// add a new callback to the callback vector
int Callbacks::create_callback(String callback_name)
{
	Func f;
	f.obj_id = -1;
	f.callback_name = callback_name;

	m_Funcs.push_back(f);
	return m_Funcs.size()-1;
}


int Callbacks::find_callback(String callback_name)
{
	for (int n=0; n<m_Funcs.size(); n++)
	{
		if (m_Funcs[n].callback_name == callback_name)
			return n;
	}

	// not found
	return -1;
}


bool Callbacks::set_callback(String callback_name, Object * pObj, String func_name)
{
	// test creating a reference to see if the function exists
	FuncRef fr;
	fr.set_instance(pObj);
	fr.set_function(func_name);

	if (!fr.is_valid())
	{
		// the function does not exist
		WARN_PRINTS("The function " + func_name + " or object is invalid, ignoring.");
		return false;
	}

	// find the callback
	int id = find_callback(callback_name);

	if (id == -1)
	{
		WARN_PRINTS("The callback " + callback_name + " does not exist, ignoring.");
		return false;
	}


	// get the func by reference
	Func f = m_Funcs[id];

	f.obj_id = pObj->get_instance_id();
	f.func_name = func_name;

	m_Funcs.set(id, f);
//	m_Funcs.insert(callback_name, f);

	return true;
}

bool Callbacks::get_callback(int callback_ID, FuncRef &fr)
{
	// unset
	if (callback_ID < 0)
		return false;

	// error condition
	if (callback_ID >= m_Funcs.size())
	{
		WARN_PRINTS("Callback ID " + itos(callback_ID) + " not found.");
		return false;
	}

	// get the func by reference
	const Func &f = m_Funcs[callback_ID];

	// callback not set
	if (f.obj_id == -1)
	{
		return false;
	}


	Object * pObj = ObjectDB::get_instance(f.obj_id);
	if (!pObj)
	{
		// object is no longer valid!
		// delete entry
		Func temp = f;
		temp.obj_id = -1;
		m_Funcs.set(callback_ID, temp);
		return false;
	}

	fr.set_instance(pObj);
	fr.set_function(f.func_name);
	return true;
}


bool Callbacks::get_callback(String callback_name, FuncRef &fr)
{
	int id = find_callback(callback_name);
	if (id == -1)
		return false;

	return get_callback(id, fr);

	/*
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
	*/
}




