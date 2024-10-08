#include "np_mesh.h"
#include "../source/navphysics_loader.h"
#include "../source/navphysics_log.h"
#include "../source/navphysics_map.h"
#include "../source/navphysics_pointf.h"
#include "../source/navphysics_pointi.h"
#include "../source/navphysics_vector.h"
#include "np_bake_params.h"

#include "modules/navigation/navigation_mesh_generator.h"
#include "scene/3d/navigation_mesh_instance.h"

#ifdef TOOLS_ENABLED
#include "editor/editor_node.h"
#include "editor/editor_settings.h"
#endif

NPMesh::NPMesh() {
	NavPhysics::set_log_callback(_nav_physics_log_callback);
	data.h_mesh = NavPhysics::g_world.safe_mesh_create();
}

NPMesh::~NPMesh() {
	if (data.h_mesh) {
		NavPhysics::g_world.safe_mesh_free(data.h_mesh);
		data.h_mesh = 0;
	}
}

void NPMesh::_nav_physics_log_callback(const char *p_string) {
	print_line(p_string);
}

void NPMesh::_bind_methods() {
	//	ClassDB::bind_method(D_METHOD("set_vertices", "vertices"), &NPMesh::set_vertices);
	//	ClassDB::bind_method(D_METHOD("get_vertices"), &NPMesh::get_vertices);

	//	ADD_PROPERTY(PropertyInfo(Variant::POOL_VECTOR3_ARRAY, "vertices", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NOEDITOR | PROPERTY_USAGE_INTERNAL), "set_vertices", "get_vertices");

	//	ClassDB::bind_method(D_METHOD("set_indices", "indices"), &NPMesh::set_indices);
	//	ClassDB::bind_method(D_METHOD("get_indices"), &NPMesh::get_indices);

	//	ADD_PROPERTY(PropertyInfo(Variant::POOL_INT_ARRAY, "indices", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NOEDITOR | PROPERTY_USAGE_INTERNAL), "set_indices", "get_indices");

	ClassDB::bind_method(D_METHOD("set_data", "data"), &NPMesh::set_data);
	ClassDB::bind_method(D_METHOD("get_data"), &NPMesh::get_data);

	ADD_PROPERTY(PropertyInfo(Variant::POOL_BYTE_ARRAY, "data", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NOEDITOR | PROPERTY_USAGE_INTERNAL), "set_data", "get_data");
}

void NPMesh::_update_mesh() {
	//	if (data.verts.size() && data.indices.size()) {
	//		bake_load(data.verts.ptr(), data.verts.size(), (const uint32_t *)data.indices.ptr(), data.indices.size());
	//	} else {
	//		NavPhysics::Mesh *mesh = NavPhysics::g_world.safe_get_mesh(data.h_mesh);
	//		NP_DEV_ASSERT(mesh);
	//		mesh->clear();
	//	}
}

//void NPMesh::set_data(const Vector<uint8_t> &p_data)
//{

//}

//Vector<uint8_t> NPMesh::get_data() const
//{

//}

void NPMesh::set_data(const Vector<uint8_t> &p_data) {
	NavPhysics::Loader loader;
	NavPhysics::Mesh *mesh = NavPhysics::g_world.safe_get_mesh(data.h_mesh);
	NP_DEV_ASSERT(mesh);
	mesh->clear();

	if (p_data.size()) {
		loader.load_raw_data(p_data.ptr(), p_data.size(), *mesh);
	}
}

Vector<uint8_t> NPMesh::get_data() const {
	NavPhysics::Loader loader;
	NavPhysics::Mesh *mesh = NavPhysics::g_world.safe_get_mesh(data.h_mesh);
	NP_DEV_ASSERT(mesh);

	Vector<uint8_t> data;
	data.resize(loader.prepare_raw_data(*mesh));
	loader.save_raw_data(data.ptrw(), data.size());

	return data;
}

//void NPMesh::set_ivertices(const Vector<int> &p_iverts) {
//	data.iverts = p_iverts;
//}

//void NPMesh::set_vertices(const Vector<Vector3> &p_verts) {
//	data.verts = p_verts;
//	_update_mesh();
//}

//void NPMesh::set_indices(const Vector<int> &p_indices) {
//	data.indices = p_indices;

//	data.polys.resize(p_indices.size() / 3);
//	for (uint32_t n = 0; n < data.polys.size(); n++) {
//		Poly p;
//		p.first_index = n * 3;
//		p.num_indices = 3;
//		data.polys.set(n, p);
//	}

//	_update_mesh();
//}

Vector<Vector3> NPMesh::get_vertices() const {
	NavPhysics::Loader loader;
	NavPhysics::Mesh *mesh = NavPhysics::g_world.safe_get_mesh(data.h_mesh);
	NP_DEV_ASSERT(mesh);
	NavPhysics::Loader::WorkingMeshData md;
	loader.extract_working_data(md, *mesh);

	Vector<Vector3> ret;
	if (md.num_verts) {
		ret.resize(md.num_verts);
		memcpy(ret.ptrw(), md.verts, md.num_verts * sizeof(Vector3));
	}
	return ret;
	//	return data.verts;
}
Vector<int> NPMesh::get_indices() const {
	NavPhysics::Loader loader;
	NavPhysics::Mesh *mesh = NavPhysics::g_world.safe_get_mesh(data.h_mesh);
	NP_DEV_ASSERT(mesh);
	NavPhysics::Loader::WorkingMeshData md;
	loader.extract_working_data(md, *mesh);

	Vector<int> ret;
	if (md.num_indices) {
		ret.resize(md.num_indices);
		static_assert(sizeof(int) == 4, "Expects 32 bit int.");
		memcpy(ret.ptrw(), md.indices, md.num_indices * sizeof(int32_t));
	}
	return ret;
	//	return data.indices;
}
Vector<NPMesh::Poly> NPMesh::get_polys() const {
	NavPhysics::Loader loader;
	NavPhysics::Mesh *mesh = NavPhysics::g_world.safe_get_mesh(data.h_mesh);
	NP_DEV_ASSERT(mesh);
	NavPhysics::Loader::WorkingMeshData md;
	loader.extract_working_data(md, *mesh);

	Vector<Poly> ret;
	if (md.num_polys) {
		ret.resize(md.num_polys);

		uint32_t index_count = 0;

		for (uint32_t n = 0; n < md.num_polys; n++) {
			Poly p;
			p.first_index = index_count;
			p.num_indices = md.poly_num_indices[n];
			index_count += p.num_indices;
			ret.set(n, p);
		}
	}
	return ret;
	//	return data.polys;
}

bool NPMesh::bake(Node *p_node) {
#ifdef TOOLS_ENABLED
	EditorProgress *ep(nullptr);
	// FIXME
#endif
#if 0
	// After discussion on devchat disabled EditorProgress for now as it is not thread-safe and uses hacks and Main::iteration() for steps.
	// EditorProgress randomly crashes the Engine when the bake function is used with a thread e.g. inside Editor with a tool script and procedural navigation
	// This was not a problem in older versions as previously Godot was unable to (re)bake NavigationMesh at runtime.
	// If EditorProgress is fixed and made thread-safe this should be enabled again.
	if (Engine::get_singleton()->is_editor_hint()) {
		ep = memnew(EditorProgress("bake", TTR("Navigation Mesh Generator Setup:"), 11));
	}

	if (ep)
		ep->step(TTR("Parsing Geometry..."), 0);
#endif

	NPBakeParams params;
	Vector<float> vertices;
	Vector<int> indices;

	Transform navmesh_xform = Object::cast_to<Spatial>(p_node)->get_global_transform().affine_inverse();

	List<Node *> parse_nodes;

	if (params.get_source_geometry_mode() == NPBakeParams::SOURCE_GEOMETRY_NAVMESH_CHILDREN) {
		parse_nodes.push_back(p_node);
	} else {
		p_node->get_tree()->get_nodes_in_group(params.get_source_group_name(), &parse_nodes);
	}

	for (const List<Node *>::Element *E = parse_nodes.front(); E; E = E->next()) {
		NPBakeParams::ParsedGeometryType geometry_type = params.get_parsed_geometry_type();
		uint32_t collision_mask = params.get_collision_mask();
		bool recurse_children = params.get_source_geometry_mode() != NPBakeParams::SOURCE_GEOMETRY_GROUPS_EXPLICIT;

		Node *node = E->get();
		print_line("Baking NPMesh for node " + node->get_name());
		NavigationMeshGenerator::_parse_geometry(navmesh_xform, node, vertices, indices, geometry_type, collision_mask, recurse_children);
	}

	print_line("Baked " + itos(vertices.size()) + " vertices, " + itos(indices.size()) + " indices.");

	if (vertices.size() > 0 && indices.size() > 0) {
		rcHeightfield *hf = nullptr;
		rcCompactHeightfield *chf = nullptr;
		rcContourSet *cset = nullptr;
		rcPolyMesh *poly_mesh = nullptr;
		rcPolyMeshDetail *detail_mesh = nullptr;

		_build_recast_navigation_mesh(
				params,
#ifdef TOOLS_ENABLED
				ep,
#endif
				hf,
				chf,
				cset,
				poly_mesh,
				detail_mesh,
				vertices,
				indices);

		rcFreeHeightField(hf);
		hf = 0;

		rcFreeCompactHeightfield(chf);
		chf = 0;

		rcFreeContourSet(cset);
		cset = 0;

		rcFreePolyMesh(poly_mesh);
		poly_mesh = 0;

		rcFreePolyMeshDetail(detail_mesh);
		detail_mesh = 0;
	}

#ifdef TOOLS_ENABLED
	if (ep)
		ep->step(TTR("Done!"), 11);

	if (ep)
		memdelete(ep);
#endif

	return true;
}

void NPMesh::_build_recast_navigation_mesh(
		const NPBakeParams &p_params,
#ifdef TOOLS_ENABLED
		EditorProgress *ep,
#endif
		rcHeightfield *hf,
		rcCompactHeightfield *chf,
		rcContourSet *cset,
		rcPolyMesh *poly_mesh,
		rcPolyMeshDetail *detail_mesh,
		Vector<float> &vertices,
		Vector<int> &indices) {
	rcContext ctx;

#ifdef TOOLS_ENABLED
	if (ep)
		ep->step(TTR("Setting up Configuration..."), 1);
#endif

	const float *verts = vertices.ptr();
	const int nverts = vertices.size() / 3;
	const int *tris = indices.ptr();
	const int ntris = indices.size() / 3;

	float bmin[3], bmax[3];
	rcCalcBounds(verts, nverts, bmin, bmax);

	rcConfig cfg;
	memset(&cfg, 0, sizeof(cfg));

	cfg.cs = p_params.get_cell_size();
	cfg.ch = p_params.get_cell_height();
	cfg.walkableSlopeAngle = p_params.get_agent_max_slope();
	cfg.walkableHeight = (int)Math::ceil(p_params.get_agent_height() / cfg.ch);
	cfg.walkableClimb = (int)Math::floor(p_params.get_agent_max_climb() / cfg.ch);
	cfg.walkableRadius = (int)Math::ceil(p_params.get_agent_radius() / cfg.cs);
	cfg.maxEdgeLen = (int)(p_params.get_edge_max_length() / p_params.get_cell_size());
	cfg.maxSimplificationError = p_params.get_edge_max_error();
	cfg.minRegionArea = (int)(p_params.get_region_min_size() * p_params.get_region_min_size());
	cfg.mergeRegionArea = (int)(p_params.get_region_merge_size() * p_params.get_region_merge_size());
	cfg.maxVertsPerPoly = (int)p_params.get_verts_per_poly();
	cfg.detailSampleDist = MAX(p_params.get_cell_size() * p_params.get_detail_sample_distance(), 0.1f);
	cfg.detailSampleMaxError = p_params.get_cell_height() * p_params.get_detail_sample_max_error();

	cfg.bmin[0] = bmin[0];
	cfg.bmin[1] = bmin[1];
	cfg.bmin[2] = bmin[2];
	cfg.bmax[0] = bmax[0];
	cfg.bmax[1] = bmax[1];
	cfg.bmax[2] = bmax[2];

	/*
	AABB baking_aabb = p_nav_mesh->get_filter_baking_aabb();

	bool aabb_has_no_volume = baking_aabb.has_no_area();

	if (!aabb_has_no_volume) {
		Vector3 baking_aabb_offset = p_nav_mesh->get_filter_baking_aabb_offset();

		cfg.bmin[0] = baking_aabb.position[0] + baking_aabb_offset.x;
		cfg.bmin[1] = baking_aabb.position[1] + baking_aabb_offset.y;
		cfg.bmin[2] = baking_aabb.position[2] + baking_aabb_offset.z;
		cfg.bmax[0] = cfg.bmin[0] + baking_aabb.size[0];
		cfg.bmax[1] = cfg.bmin[1] + baking_aabb.size[1];
		cfg.bmax[2] = cfg.bmin[2] + baking_aabb.size[2];
	}
*/

#ifdef TOOLS_ENABLED
	if (ep)
		ep->step(TTR("Calculating grid size..."), 2);
#endif
	rcCalcGridSize(cfg.bmin, cfg.bmax, cfg.cs, &cfg.width, &cfg.height);

	// ~30000000 seems to be around sweetspot where Editor baking breaks
	if ((cfg.width * cfg.height) > 30000000) {
		WARN_PRINT("NavigationMesh baking process will likely fail."
				   "\nSource geometry is suspiciously big for the current Cell Size and Cell Height in the NavMesh Resource bake settings."
				   "\nIf baking does not fail, the resulting NavigationMesh will create serious pathfinding performance issues."
				   "\nIt is advised to increase Cell Size and/or Cell Height in the NavMesh Resource bake settings or reduce the size / scale of the source geometry.");
	}

#ifdef TOOLS_ENABLED
	if (ep)
		ep->step(TTR("Creating heightfield..."), 3);
#endif
	hf = rcAllocHeightfield();

	ERR_FAIL_COND(!hf);
	ERR_FAIL_COND(!rcCreateHeightfield(&ctx, *hf, cfg.width, cfg.height, cfg.bmin, cfg.bmax, cfg.cs, cfg.ch));

#ifdef TOOLS_ENABLED
	if (ep)
		ep->step(TTR("Marking walkable triangles..."), 4);
#endif
	{
		Vector<unsigned char> tri_areas;
		tri_areas.resize(ntris);

		ERR_FAIL_COND(tri_areas.size() == 0);

		memset(tri_areas.ptrw(), 0, ntris * sizeof(unsigned char));
		rcMarkWalkableTriangles(&ctx, cfg.walkableSlopeAngle, verts, nverts, tris, ntris, tri_areas.ptrw());

		ERR_FAIL_COND(!rcRasterizeTriangles(&ctx, verts, nverts, tris, tri_areas.ptr(), ntris, *hf, cfg.walkableClimb));
	}

	if (p_params.get_filter_low_hanging_obstacles()) {
		rcFilterLowHangingWalkableObstacles(&ctx, cfg.walkableClimb, *hf);
	}
	if (p_params.get_filter_ledge_spans()) {
		rcFilterLedgeSpans(&ctx, cfg.walkableHeight, cfg.walkableClimb, *hf);
	}
	if (p_params.get_filter_walkable_low_height_spans()) {
		rcFilterWalkableLowHeightSpans(&ctx, cfg.walkableHeight, *hf);
	}

#ifdef TOOLS_ENABLED
	if (ep)
		ep->step(TTR("Constructing compact heightfield..."), 5);
#endif

	chf = rcAllocCompactHeightfield();

	ERR_FAIL_COND(!chf);
	ERR_FAIL_COND(!rcBuildCompactHeightfield(&ctx, cfg.walkableHeight, cfg.walkableClimb, *hf, *chf));

	rcFreeHeightField(hf);
	hf = 0;

#ifdef TOOLS_ENABLED
	if (ep)
		ep->step(TTR("Eroding walkable area..."), 6);
#endif

	ERR_FAIL_COND(!rcErodeWalkableArea(&ctx, cfg.walkableRadius, *chf));

#ifdef TOOLS_ENABLED
	if (ep)
		ep->step(TTR("Partitioning..."), 7);
#endif

	if (p_params.get_sample_partition_type() == NPBakeParams::SAMPLE_PARTITION_WATERSHED) {
		ERR_FAIL_COND(!rcBuildDistanceField(&ctx, *chf));
		ERR_FAIL_COND(!rcBuildRegions(&ctx, *chf, 0, cfg.minRegionArea, cfg.mergeRegionArea));
	} else if (p_params.get_sample_partition_type() == NPBakeParams::SAMPLE_PARTITION_MONOTONE) {
		ERR_FAIL_COND(!rcBuildRegionsMonotone(&ctx, *chf, 0, cfg.minRegionArea, cfg.mergeRegionArea));
	} else {
		ERR_FAIL_COND(!rcBuildLayerRegions(&ctx, *chf, 0, cfg.minRegionArea));
	}

#ifdef TOOLS_ENABLED
	if (ep)
		ep->step(TTR("Creating contours..."), 8);
#endif

	cset = rcAllocContourSet();

	ERR_FAIL_COND(!cset);
	ERR_FAIL_COND(!rcBuildContours(&ctx, *chf, cfg.maxSimplificationError, cfg.maxEdgeLen, *cset));

#ifdef TOOLS_ENABLED
	if (ep)
		ep->step(TTR("Creating polymesh..."), 9);
#endif

	poly_mesh = rcAllocPolyMesh();
	ERR_FAIL_COND(!poly_mesh);
	ERR_FAIL_COND(!rcBuildPolyMesh(&ctx, *cset, cfg.maxVertsPerPoly, *poly_mesh));

	detail_mesh = rcAllocPolyMeshDetail();
	ERR_FAIL_COND(!detail_mesh);
	ERR_FAIL_COND(!rcBuildPolyMeshDetail(&ctx, *poly_mesh, *chf, cfg.detailSampleDist, cfg.detailSampleMaxError, *detail_mesh));

	rcFreeCompactHeightfield(chf);
	chf = 0;
	rcFreeContourSet(cset);
	cset = 0;

#ifdef TOOLS_ENABLED
	if (ep)
		ep->step(TTR("Converting to native navigation mesh..."), 10);
#endif

	_convert_detail_mesh_to_native_navigation_mesh(detail_mesh);

	rcFreePolyMesh(poly_mesh);
	poly_mesh = 0;
	rcFreePolyMeshDetail(detail_mesh);
	detail_mesh = 0;
}

void NPMesh::_convert_detail_mesh_to_native_navigation_mesh(const rcPolyMeshDetail *p_detail_mesh) {
	LocalVector<Vector3> nav_vertices;

	for (int i = 0; i < p_detail_mesh->nverts; i++) {
		const float *v = &p_detail_mesh->verts[i * 3];
		nav_vertices.push_back(Vector3(v[0], v[1], v[2]));
	}
	//p_nav_mesh->set_vertices(nav_vertices);

	LocalVector<i32> inds;

	for (int i = 0; i < p_detail_mesh->nmeshes; i++) {
		const unsigned int *m = &p_detail_mesh->meshes[i * 4];
		const unsigned int bverts = m[0];
		const unsigned int btris = m[2];
		const unsigned int ntris = m[3];
		const unsigned char *tris = &p_detail_mesh->tris[btris * 4];
		for (unsigned int j = 0; j < ntris; j++) {
			//			Vector<int> nav_indices;
			//			nav_indices.resize(3);
			// Polygon order in recast is opposite than godot's
			//			nav_indices.write[0] = ((int)(bverts + tris[j * 4 + 0]));
			//			nav_indices.write[1] = ((int)(bverts + tris[j * 4 + 2]));
			//			nav_indices.write[2] = ((int)(bverts + tris[j * 4 + 1]));

			inds.push_back(bverts + tris[j * 4 + 0]);
			inds.push_back(bverts + tris[j * 4 + 2]);
			inds.push_back(bverts + tris[j * 4 + 1]);

			//			p_nav_mesh->add_polygon(nav_indices);
		}
	}

	bake_load(nav_vertices.ptr(), nav_vertices.size(), (const u32 *)inds.ptr(), inds.size());

	//data.verts = nav_vertices;
	//data.indices = inds;
}

bool NPMesh::clear() {
	return true;
}

bool NPMesh::bake_load2(const NavigationMeshInstance &p_nav_mesh_instance) {
	const Ref<NavigationMesh> nmesh = p_nav_mesh_instance.get_navigation_mesh();
	if (!nmesh.is_valid()) {
		return false;
	}

	int num_verts = nmesh->get_vertices().size();
	if (!num_verts) {
		return false;
	}
	int num_polys = nmesh->get_polygon_count();
	if (!num_polys) {
		return false;
	}

	PoolVector<Vector3>::Read verts_read = nmesh->get_vertices().read();
	const Vector3 *pverts = verts_read.ptr();

	LocalVector<uint32_t> inds;

	for (int n = 0; n < num_polys; n++) {
		Vector<int> poly = nmesh->get_polygon(n);
		if (poly.size() == 3) {
			inds.push_back(poly[0]);
			inds.push_back(poly[1]);
			inds.push_back(poly[2]);
		}
	}

	if (!inds.size()) {
		return false;
	}

	return bake_load(pverts, num_verts, inds.ptr(), inds.size());
}

//bool NPMesh::working_load() {
//	NavPhysics::Loader loader;
//	NavPhysics::Loader::WorkingMeshData source;

//	source.num_verts = data.verts.size();

//	if (data.iverts.size() != (source.num_verts * 2)) {
//		return false;
//	}

//	source.verts = (const NavPhysics::FPoint3 *)data.verts.ptr();
//	source.iverts = (const NavPhysics::IPoint2 *)data.iverts.ptr();

//	return true;
//}

bool NPMesh::bake_load(const Vector3 *p_verts, uint32_t p_num_verts, const uint32_t *p_indices, uint32_t p_num_indices) {
	NavPhysics::Loader loader;
	NavPhysics::Loader::SourceMeshData source;

	NavPhysics::Vector<NavPhysics::FPoint3> np_verts;
	NavPhysics::Vector<u32> np_inds;

	for (uint32_t n = 0; n < p_num_verts; n++) {
		const Vector3 &pt = p_verts[n];
		np_verts.push_back(NavPhysics::FPoint3::make(pt.x, pt.y, pt.z));
	}

	for (uint32_t n = 0; n < p_num_indices; n++) {
		np_inds.push_back(p_indices[n]);
	}

	NavPhysics::Vector<u32> poly_num_inds;
	poly_num_inds.resize(p_num_indices / 3);

	for (u32 n = 0; n < poly_num_inds.size(); n++) {
		poly_num_inds[n] = 3;
	}

	source.num_verts = np_verts.size();
	source.verts = np_verts.ptr();
	source.num_indices = np_inds.size();
	source.indices = np_inds.ptr();
	source.num_polys = poly_num_inds.size();
	source.poly_num_indices = poly_num_inds.ptr();

	NavPhysics::Mesh *mesh = NavPhysics::g_world.safe_get_mesh(data.h_mesh);
	NP_DEV_ASSERT(mesh);

	loader.load_mesh(source, *mesh);

	return true;
}
