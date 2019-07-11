// where smoothclass is defined as the class
// and smoothnode is defined as the node

public:
	enum eMode
	{
		MODE_LOCAL,
		MODE_GLOBAL,
	};
private:
	enum eSmoothFlags
	{
		SF_ENABLED = 1,
		SF_DIRTY = 2,
		SF_TRANSLATE = 4,
		SF_ROTATE = 8,
		SF_SCALE = 16,
		SF_GLOBAL_IN = 32,
		SF_GLOBAL_OUT = 64,
		SF_LERP = 128,
	};


	STransform m_Curr;
	STransform m_Prev;


	// defined by eSmoothFlags
	int m_Flags;
//	eMode m_Mode;

	//WeakRef m_refTarget;
	ObjectID m_ID_target;
	NodePath m_path_target;

protected:
	static void _bind_methods();
	void _notification(int p_what);

public:
	void set_input_mode(eMode p_mode);
	eMode get_input_mode() const;
	void set_output_mode(eMode p_mode);
	eMode get_output_mode() const;

	void set_enabled(bool p_enabled);
	bool is_enabled() const;

	void set_interpolate_translation(bool bTranslate);
	bool get_interpolate_translation() const;

	void set_interpolate_rotation(bool bRotate);
	bool get_interpolate_rotation() const;

	void set_interpolate_scale(bool bScale);
	bool get_interpolate_scale() const;


	void set_target(const Object *p_target);
	void set_target_path(const NodePath &p_path);
	NodePath get_target_path() const;

	void teleport();


private:
	void _set_target(const Object *p_target);
	void RemoveTarget();
	void FixedUpdate();
	void FrameUpdate();
	void RefreshTransform(SMOOTHNODE * pTarget, bool bDebug = false);
	SMOOTHNODE * GetTarget() const;
	void SetProcessing(bool bEnable);

	void ChangeFlags(int f, bool bSet) {if (bSet) {SetFlags(f);} else {ClearFlags(f);}}
	void SetFlags(int f) {m_Flags |= f;}
	void ClearFlags(int f) {m_Flags &= ~f;}
	bool TestFlags(int f) const {return (m_Flags & f) == f;}

	void ResolveTargetPath();
	void smooth_print_line(Variant sz);
