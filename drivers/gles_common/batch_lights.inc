bool _light_scissor_begin(const Rect2 &p_item_rect, const Transform2D &p_light_xform, const Rect2 &p_light_rect) const {

	float area_item = p_item_rect.size.x * p_item_rect.size.y; // double check these are always positive

	// quick reject .. the area of pixels saved can never be more than the area of the item
	if (area_item < bdata.scissor_threshold_area) {
		return false;
	}

	Rect2 cliprect;
	if (!_light_find_intersection(p_item_rect, p_light_xform, p_light_rect, cliprect)) {
		// should not really occur .. but just in case
		cliprect = Rect2(0, 0, 0, 0);
	} else {
		// some conditions not to scissor
		// determine the area (fill rate) that will be saved
		float area_cliprect = cliprect.size.x * cliprect.size.y;
		float area_saved = area_item - area_cliprect;

		// if area saved is too small, don't scissor
		if (area_saved < bdata.scissor_threshold_area) {
			return false;
		}
	}

	int rh = get_storage()->frame.current_rt->height;

	int y = rh - (cliprect.position.y + cliprect.size.y);
	if (get_storage()->frame.current_rt->flags[RasterizerStorage::RENDER_TARGET_VFLIP])
		y = cliprect.position.y;
	get_this()->gl_enable_scissor(cliprect.position.x, y, cliprect.size.width, cliprect.size.height);

	return true;
}

bool _light_find_intersection(const Rect2 &p_item_rect, const Transform2D &p_light_xform, const Rect2 &p_light_rect, Rect2 &r_cliprect) const {
	// transform light to world space (note this is done in the earlier intersection test, so could
	// be made more efficient)
	Vector2 pts[4] = {
		p_light_xform.xform(p_light_rect.position),
		p_light_xform.xform(Vector2(p_light_rect.position.x + p_light_rect.size.x, p_light_rect.position.y)),
		p_light_xform.xform(Vector2(p_light_rect.position.x, p_light_rect.position.y + p_light_rect.size.y)),
		p_light_xform.xform(Vector2(p_light_rect.position.x + p_light_rect.size.x, p_light_rect.position.y + p_light_rect.size.y)),
	};

	// calculate the light bound rect in world space
	Rect2 lrect(pts[0].x, pts[0].y, 0, 0);
	for (int n = 1; n < 4; n++) {
		lrect.expand_to(pts[n]);
	}

	// intersection between the 2 rects
	// they should probably always intersect, because of earlier check, but just in case...
	if (!p_item_rect.intersects(lrect))
		return false;

	// note this does almost the same as Rect2.clip but slightly more efficient for our use case
	r_cliprect.position.x = MAX(p_item_rect.position.x, lrect.position.x);
	r_cliprect.position.y = MAX(p_item_rect.position.y, lrect.position.y);

	Point2 item_rect_end = p_item_rect.position + p_item_rect.size;
	Point2 lrect_end = lrect.position + lrect.size;

	r_cliprect.size.x = MIN(item_rect_end.x, lrect_end.x) - r_cliprect.position.x;
	r_cliprect.size.y = MIN(item_rect_end.y, lrect_end.y) - r_cliprect.position.y;

	return true;
}

void _calculate_scissor_threshold_area() {
	if (!bdata.settings_scissor_lights) {
		return;
	}

	// scissor area threshold is 0.0 to 1.0 in the settings for ease of use.
	// we need to translate to an absolute area to determine quickly whether
	// to scissor.
	if (bdata.settings_scissor_threshold < 0.0001f) {
		bdata.scissor_threshold_area = -1.0f; // will always pass
	} else {
		// in pixels
		int w = get_storage()->frame.current_rt->width;
		int h = get_storage()->frame.current_rt->height;

		int screen_area = w * h;

		bdata.scissor_threshold_area = bdata.settings_scissor_threshold * screen_area;
	}
}
