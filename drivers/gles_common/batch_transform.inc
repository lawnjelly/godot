void _software_transform_vertex(BatchVector2 &r_v, const Transform2D &p_tr) const {
	Vector2 vc(r_v.x, r_v.y);
	vc = p_tr.xform(vc);
	r_v.set(vc);
}

void _software_transform_vertex(Vector2 &r_v, const Transform2D &p_tr) const {
	r_v = p_tr.xform(r_v);
}

TransformMode _find_transform_mode(const Transform2D &p_tr) const {
	// decided whether to do translate only for software transform
	if ((p_tr.elements[0].x == 1.0) &&
			(p_tr.elements[0].y == 0.0) &&
			(p_tr.elements[1].x == 0.0) &&
			(p_tr.elements[1].y == 1.0)) {
		return TM_TRANSLATE;
	}

	return TM_ALL;
}
