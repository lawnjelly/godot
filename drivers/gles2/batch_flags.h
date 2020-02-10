#pragma once

namespace Batch {

template <class T> class BFlags
{
public:
	void set_flag(T s) { m_Flags |= s; }
	void clear_flag(T s) { m_Flags &= ~s; }
	void clear_flags() { m_Flags = 0; }
	bool get_flag(T s) { return m_Flags & s; }
	bool set_flag_if_false(T s) {
		if (!get_flag(s)) {
			set_flag(s);
			return true;
		}
		return false;
	}
	bool clear_flag_if_true(T s) {
		if (get_flag(s)) {
			clear_flag(s);
			return true;
		}
		return false;
	}
private:
	T m_Flags;
};

} // namespace
