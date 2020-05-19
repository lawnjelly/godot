public:

void batch_constructor()
{
	bdata.settings_use_batching = false;

#ifdef GLES_OVER_GL
	use_nvidia_rect_workaround = GLOBAL_GET("rendering/quality/2d/use_nvidia_rect_flicker_workaround");
#else
	// Not needed (a priori) on GLES devices
	use_nvidia_rect_workaround = false;
#endif
}

void batch_initialize()
{
	bdata.settings_use_batching = GLOBAL_GET("rendering/gles2/batching/use_batching");
	bdata.settings_max_join_item_commands = GLOBAL_GET("rendering/gles2/batching/max_join_item_commands");
	bdata.settings_colored_vertex_format_threshold = GLOBAL_GET("rendering/gles2/batching/colored_vertex_format_threshold");
	bdata.settings_item_reordering_lookahead = GLOBAL_GET("rendering/gles2/batching/item_reordering_lookahead");
	bdata.settings_light_max_join_items = GLOBAL_GET("rendering/gles2/batching/light_max_join_items");
	bdata.settings_use_single_rect_fallback = GLOBAL_GET("rendering/gles2/batching/single_rect_fallback");

	// we can use the threshold to determine whether to turn scissoring off or on
	bdata.settings_scissor_threshold = GLOBAL_GET("rendering/gles2/batching/light_scissor_area_threshold");
	if (bdata.settings_scissor_threshold > 0.999f) {
		bdata.settings_scissor_lights = false;
	} else {
		bdata.settings_scissor_lights = true;
	}

	// The sweet spot on my desktop for cache is actually smaller than the max, and this
	// is the default. This saves memory too so we will use it for now, needs testing to see whether this varies according
	// to device / platform.
	bdata.settings_batch_buffer_num_verts = GLOBAL_GET("rendering/gles2/batching/batch_buffer_size");

	// override the use_batching setting in the editor
	// (note that if the editor can't start, you can't change the use_batching project setting!)
	if (Engine::get_singleton()->is_editor_hint()) {
		bool use_in_editor = GLOBAL_GET("rendering/gles2/debug/use_batching_in_editor");
		bdata.settings_use_batching = use_in_editor;

		// fix some settings in the editor, as the performance not worth the risk
		bdata.settings_use_single_rect_fallback = false;
	}

	// if we are using batching, we will purposefully disable the nvidia workaround.
	// This is because the only reason to use the single rect fallback is the approx 2x speed
	// of the uniform drawing technique. If we used nvidia workaround, speed would be
	// approx equal to the batcher drawing technique (indexed primitive + VB).
	if (bdata.settings_use_batching) {
		use_nvidia_rect_workaround = false;
	}

	// For debugging, if flash is set in project settings, it will flash on alternate frames
	// between the non-batched renderer and the batched renderer,
	// in order to find regressions.
	// This should not be used except during development.
	// make a note of the original choice in case we are flashing on and off the batching
	bdata.settings_use_batching_original_choice = bdata.settings_use_batching;
	bdata.settings_flash_batching = GLOBAL_GET("rendering/gles2/debug/flash_batching");
	if (!bdata.settings_use_batching) {
		// no flash when batching turned off
		bdata.settings_flash_batching = false;
	}

	// frame diagnosis. print out the batches every nth frame
	bdata.settings_diagnose_frame = false;
	if (!Engine::get_singleton()->is_editor_hint() && bdata.settings_use_batching) {
		bdata.settings_diagnose_frame = GLOBAL_GET("rendering/gles2/debug/diagnose_frame");
	}

	// the maximum num quads in a batch is limited by GLES2. We can have only 16 bit indices,
	// which means we can address a vertex buffer of max size 65535. 4 vertices are needed per quad.

	// Note this determines the memory use by the vertex buffer vector. max quads (65536/4)-1
	// but can be reduced to save memory if really required (will result in more batches though)
	const int max_possible_quads = (65536 / 4) - 1;
	const int min_possible_quads = 8; // some reasonable small value

	// value from project settings
	int max_quads = bdata.settings_batch_buffer_num_verts / 4;

	// sanity checks
	max_quads = CLAMP(max_quads, min_possible_quads, max_possible_quads);
	bdata.settings_max_join_item_commands = CLAMP(bdata.settings_max_join_item_commands, 0, 65535);
	bdata.settings_colored_vertex_format_threshold = CLAMP(bdata.settings_colored_vertex_format_threshold, 0.0f, 1.0f);
	bdata.settings_scissor_threshold = CLAMP(bdata.settings_scissor_threshold, 0.0f, 1.0f);
	bdata.settings_light_max_join_items = CLAMP(bdata.settings_light_max_join_items, 0, 65535);
	bdata.settings_item_reordering_lookahead = CLAMP(bdata.settings_item_reordering_lookahead, 0, 65535);

	// for debug purposes, output a string with the batching options
	String batching_options_string = "OpenGL ES 2.0 Batching: ";
	if (bdata.settings_use_batching) {
		batching_options_string += "ON";

		if (OS::get_singleton()->is_stdout_verbose()) {
			batching_options_string += "\n\tOPTIONS\n";
			batching_options_string += "\tmax_join_item_commands " + itos(bdata.settings_max_join_item_commands) + "\n";
			batching_options_string += "\tcolored_vertex_format_threshold " + String(Variant(bdata.settings_colored_vertex_format_threshold)) + "\n";
			batching_options_string += "\tbatch_buffer_size " + itos(bdata.settings_batch_buffer_num_verts) + "\n";
			batching_options_string += "\tlight_scissor_area_threshold " + String(Variant(bdata.settings_scissor_threshold)) + "\n";

			batching_options_string += "\titem_reordering_lookahead " + itos(bdata.settings_item_reordering_lookahead) + "\n";
			batching_options_string += "\tlight_max_join_items " + itos(bdata.settings_light_max_join_items) + "\n";
			batching_options_string += "\tsingle_rect_fallback " + String(Variant(bdata.settings_use_single_rect_fallback)) + "\n";

			batching_options_string += "\tdebug_flash " + String(Variant(bdata.settings_flash_batching)) + "\n";
			batching_options_string += "\tdiagnose_frame " + String(Variant(bdata.settings_diagnose_frame));
		}

		print_line(batching_options_string);
	}

	// special case, for colored vertex format threshold.
	// as the comparison is >=, we want to be able to totally turn on or off
	// conversion to colored vertex format at the extremes, so we will force
	// 1.0 to be just above 1.0
	if (bdata.settings_colored_vertex_format_threshold > 0.995f) {
		bdata.settings_colored_vertex_format_threshold = 1.01f;
	}

	// save memory when batching off
	if (!bdata.settings_use_batching) {
		max_quads = 0;
	}

	uint32_t sizeof_batch_vert = sizeof(BatchVertex);

	bdata.max_quads = max_quads;

	// 4 verts per quad
	bdata.vertex_buffer_size_units = max_quads * 4;

	// the index buffer can be longer than 65535, only the indices need to be within this range
	bdata.index_buffer_size_units = max_quads * 6;

	// this comes out at approx 64K for non-colored vertex buffer, and 128K for colored vertex buffer
	bdata.vertex_buffer_size_bytes = bdata.vertex_buffer_size_units * sizeof_batch_vert;
	bdata.index_buffer_size_bytes = bdata.index_buffer_size_units * 2; // 16 bit inds

	// create equal number of norma and colored verts (as the normal may need to be translated to colored)
	bdata.vertices.create(bdata.vertex_buffer_size_units); // 512k
	bdata.vertices_colored.create(bdata.vertices.max_size()); // 1024k

	// num batches will be auto increased dynamically if required
	bdata.batches.create(1024);
	bdata.batches_temp.create(bdata.batches.max_size());

	// batch textures can also be increased dynamically
	bdata.batch_textures.create(32);
}
