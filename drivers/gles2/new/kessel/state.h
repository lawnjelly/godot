#pragma once

#include "batch.h"
#include "drivers/gles2/rasterizer_storage_gles2.h"
#include "servers/visual/rasterizer.h"

namespace Batch
{

class BState
{
};

class State_Fill
{
public:
	State_Fill() { Reset(); }
	void Reset();
	Batch *m_pBatch_ItemGroup;
	Batch *m_pBatch_Item;
};

class State_ItemGroup
{
public:
	State_ItemGroup() { Reset(); }
	void Reset();
	BItemGroup *m_pItemGroup;
	RasterizerCanvas::Item *m_pScissorItem;
	RasterizerStorageGLES2::Shader *m_pCachedShader;
	bool m_bRebindShader;
	bool m_bPrevUseSkeleton;
	int m_iLastBlendMode;
	RID m_RID_CanvasLastMaterial;

	bool m_bScissorActive;

	int m_iBlendMode;
	RasterizerStorageGLES2::Material *m_pMaterial;
};

class State_Item
{
public:

	State_Item() { Reset(); }
	void Reset();
	void Reset_Item() {m_ChangeFlags = 0;}
	String ChangeFlagsToString() const; // for debugging

	RasterizerStorageGLES2::Skeleton *m_pSkeleton;
	bool m_bUseSkeleton;

	bool m_bUnshaded;
	bool m_bReclip;


	uint32_t m_ChangeFlags;
};

} // namespace Batch
