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
};

class State_Item
{
public:
	State_Item() { Reset(); }
	void Reset();
	RasterizerStorageGLES2::Skeleton *m_pSkeleton;
	bool m_bUseSkeleton;

	bool m_bUnshaded;
	int m_iBlendMode;
	bool m_bReclip;
	RasterizerStorageGLES2::Material *m_pMaterial;
};

} // namespace Batch
