/*************************************************************************/
/*  rasterizer_storage_gles2.cpp                                         */
/*************************************************************************/
/*                       This file is part of:                           */
/*                           GODOT ENGINE                                */
/*                      https://godotengine.org                          */
/*************************************************************************/
/* Copyright (c) 2007-2020 Juan Linietsky, Ariel Manzur.                 */
/* Copyright (c) 2014-2020 Godot Engine contributors (cf. AUTHORS.md).   */
/*                                                                       */
/* Permission is hereby granted, free of charge, to any person obtaining */
/* a copy of this software and associated documentation files (the       */
/* "Software"), to deal in the Software without restriction, including   */
/* without limitation the rights to use, copy, modify, merge, publish,   */
/* distribute, sublicense, and/or sell copies of the Software, and to    */
/* permit persons to whom the Software is furnished to do so, subject to */
/* the following conditions:                                             */
/*                                                                       */
/* The above copyright notice and this permission notice shall be        */
/* included in all copies or substantial portions of the Software.       */
/*                                                                       */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,       */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF    */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.*/
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY  */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,  */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE     */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                */
/*************************************************************************/

//#define GLES2_DISABLE_RENDER_TARGETS

#include "rasterizer_storage_gles2.h"

#include "core/math/transform.h"
#include "core/config/project_settings.h"
#include "rasterizer_canvas_gles2.h"
#include "rasterizer_scene_gles2.h"
#include "servers/rendering/shader_language.h"
#include "drivers/gles_common/rasterizer_storage_common.h"

GLuint RasterizerStorageGLES2::system_fbo = 0;

/* TEXTURE API */

#define _EXT_COMPRESSED_RGBA_S3TC_DXT1_EXT 0x83F1
#define _EXT_COMPRESSED_RGBA_S3TC_DXT3_EXT 0x83F2
#define _EXT_COMPRESSED_RGBA_S3TC_DXT5_EXT 0x83F3

#define _EXT_COMPRESSED_RED_RGTC1_EXT 0x8DBB
#define _EXT_COMPRESSED_RED_RGTC1 0x8DBB
#define _EXT_COMPRESSED_SIGNED_RED_RGTC1 0x8DBC
#define _EXT_COMPRESSED_RG_RGTC2 0x8DBD
#define _EXT_COMPRESSED_SIGNED_RG_RGTC2 0x8DBE
#define _EXT_COMPRESSED_SIGNED_RED_RGTC1_EXT 0x8DBC
#define _EXT_COMPRESSED_RED_GREEN_RGTC2_EXT 0x8DBD
#define _EXT_COMPRESSED_SIGNED_RED_GREEN_RGTC2_EXT 0x8DBE
#define _EXT_ETC1_RGB8_OES 0x8D64

#define _EXT_COMPRESSED_RGB_PVRTC_4BPPV1_IMG 0x8C00
#define _EXT_COMPRESSED_RGB_PVRTC_2BPPV1_IMG 0x8C01
#define _EXT_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG 0x8C02
#define _EXT_COMPRESSED_RGBA_PVRTC_2BPPV1_IMG 0x8C03

#define _EXT_COMPRESSED_SRGB_PVRTC_2BPPV1_EXT 0x8A54
#define _EXT_COMPRESSED_SRGB_PVRTC_4BPPV1_EXT 0x8A55
#define _EXT_COMPRESSED_SRGB_ALPHA_PVRTC_2BPPV1_EXT 0x8A56
#define _EXT_COMPRESSED_SRGB_ALPHA_PVRTC_4BPPV1_EXT 0x8A57

#define _EXT_COMPRESSED_RGBA_BPTC_UNORM 0x8E8C
#define _EXT_COMPRESSED_SRGB_ALPHA_BPTC_UNORM 0x8E8D
#define _EXT_COMPRESSED_RGB_BPTC_SIGNED_FLOAT 0x8E8E
#define _EXT_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT 0x8E8F

#define _GL_TEXTURE_EXTERNAL_OES 0x8D65

#ifdef GLES_OVER_GL
#define _GL_HALF_FLOAT_OES 0x140B
#else
#define _GL_HALF_FLOAT_OES 0x8D61
#endif

#define _EXT_TEXTURE_CUBE_MAP_SEAMLESS 0x884F

#define _RED_OES 0x1903

#define _DEPTH_COMPONENT24_OES 0x81A6

#ifndef GLES_OVER_GL
#define glClearDepth glClearDepthf

// enable extensions manually for android and ios
#ifndef UWP_ENABLED
#include <dlfcn.h> // needed to load extensions
#endif

#ifdef IPHONE_ENABLED

#include <OpenGLES/ES2/glext.h>
//void *glRenderbufferStorageMultisampleAPPLE;
//void *glResolveMultisampleFramebufferAPPLE;
#define glRenderbufferStorageMultisample glRenderbufferStorageMultisampleAPPLE
#elif defined(ANDROID_ENABLED)

#include <GLES2/gl2ext.h>
PFNGLRENDERBUFFERSTORAGEMULTISAMPLEEXTPROC glRenderbufferStorageMultisampleEXT;
PFNGLFRAMEBUFFERTEXTURE2DMULTISAMPLEEXTPROC glFramebufferTexture2DMultisampleEXT;
#define glRenderbufferStorageMultisample glRenderbufferStorageMultisampleEXT
#define glFramebufferTexture2DMultisample glFramebufferTexture2DMultisampleEXT

#elif defined(UWP_ENABLED)
#include <GLES2/gl2ext.h>
#define glRenderbufferStorageMultisample glRenderbufferStorageMultisampleANGLE
#define glFramebufferTexture2DMultisample glFramebufferTexture2DMultisampleANGLE
#endif

#define GL_TEXTURE_3D 0x806F
#define GL_MAX_SAMPLES 0x8D57
#endif //!GLES_OVER_GL

void RasterizerStorageGLES2::bind_quad_array() const {
	glBindBuffer(GL_ARRAY_BUFFER, resources.quadie);
	glVertexAttribPointer(GD_VS::ARRAY_VERTEX, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 4, 0);
	glVertexAttribPointer(GD_VS::ARRAY_TEX_UV, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 4, CAST_INT_TO_UCHAR_PTR(8));

	glEnableVertexAttribArray(GD_VS::ARRAY_VERTEX);
	glEnableVertexAttribArray(GD_VS::ARRAY_TEX_UV);
}

Ref<Image> RasterizerStorageGLES2::_get_gl_image_and_format(const Ref<Image> &p_image, Image::Format p_format, uint32_t p_flags, Image::Format &r_real_format, GLenum &r_gl_format, GLenum &r_gl_internal_format, GLenum &r_gl_type, bool &r_compressed, bool p_force_decompress) const {

	r_gl_format = 0;
	Ref<Image> image = p_image;
	r_compressed = false;
	r_real_format = p_format;

	bool need_decompress = false;

	switch (p_format) {

		case Image::FORMAT_L8: {

			r_gl_internal_format = GL_LUMINANCE;
			r_gl_format = GL_LUMINANCE;
			r_gl_type = GL_UNSIGNED_BYTE;
		} break;
		case Image::FORMAT_LA8: {
			r_gl_internal_format = GL_LUMINANCE_ALPHA;
			r_gl_format = GL_LUMINANCE_ALPHA;
			r_gl_type = GL_UNSIGNED_BYTE;
		} break;
		case Image::FORMAT_R8: {

			r_gl_internal_format = GL_ALPHA;
			r_gl_format = GL_ALPHA;
			r_gl_type = GL_UNSIGNED_BYTE;

		} break;
		case Image::FORMAT_RG8: {
			ERR_PRINT("RG texture not supported, converting to RGB8.");
			if (image.is_valid())
				image->convert(Image::FORMAT_RGB8);
			r_real_format = Image::FORMAT_RGB8;
			r_gl_internal_format = GL_RGB;
			r_gl_format = GL_RGB;
			r_gl_type = GL_UNSIGNED_BYTE;

		} break;
		case Image::FORMAT_RGB8: {

			r_gl_internal_format = GL_RGB;
			r_gl_format = GL_RGB;
			r_gl_type = GL_UNSIGNED_BYTE;

		} break;
		case Image::FORMAT_RGBA8: {

			r_gl_format = GL_RGBA;
			r_gl_internal_format = GL_RGBA;
			r_gl_type = GL_UNSIGNED_BYTE;

		} break;
		case Image::FORMAT_RGBA4444: {

			r_gl_internal_format = GL_RGBA;
			r_gl_format = GL_RGBA;
			r_gl_type = GL_UNSIGNED_SHORT_4_4_4_4;

		} break;
//		case Image::FORMAT_RGBA5551: {

//			r_gl_internal_format = GL_RGB5_A1;
//			r_gl_format = GL_RGBA;
//			r_gl_type = GL_UNSIGNED_SHORT_5_5_5_1;

//		} break;
		case Image::FORMAT_RF: {
			if (!config.float_texture_supported) {
				ERR_PRINT("R float texture not supported, converting to RGB8.");
				if (image.is_valid())
					image->convert(Image::FORMAT_RGB8);
				r_real_format = Image::FORMAT_RGB8;
				r_gl_internal_format = GL_RGB;
				r_gl_format = GL_RGB;
				r_gl_type = GL_UNSIGNED_BYTE;
			} else {
				r_gl_internal_format = GL_ALPHA;
				r_gl_format = GL_ALPHA;
				r_gl_type = GL_FLOAT;
			}
		} break;
		case Image::FORMAT_RGF: {
			ERR_PRINT("RG float texture not supported, converting to RGB8.");
			if (image.is_valid())
				image->convert(Image::FORMAT_RGB8);
			r_real_format = Image::FORMAT_RGB8;
			r_gl_internal_format = GL_RGB;
			r_gl_format = GL_RGB;
			r_gl_type = GL_UNSIGNED_BYTE;
		} break;
		case Image::FORMAT_RGBF: {
			if (!config.float_texture_supported) {
				ERR_PRINT("RGB float texture not supported, converting to RGB8.");
				if (image.is_valid())
					image->convert(Image::FORMAT_RGB8);
				r_real_format = Image::FORMAT_RGB8;
				r_gl_internal_format = GL_RGB;
				r_gl_format = GL_RGB;
				r_gl_type = GL_UNSIGNED_BYTE;
			} else {
				r_gl_internal_format = GL_RGB;
				r_gl_format = GL_RGB;
				r_gl_type = GL_FLOAT;
			}
		} break;
		case Image::FORMAT_RGBAF: {
			if (!config.float_texture_supported) {
				ERR_PRINT("RGBA float texture not supported, converting to RGBA8.");
				if (image.is_valid())
					image->convert(Image::FORMAT_RGBA8);
				r_real_format = Image::FORMAT_RGBA8;
				r_gl_internal_format = GL_RGBA;
				r_gl_format = GL_RGBA;
				r_gl_type = GL_UNSIGNED_BYTE;
			} else {
				r_gl_internal_format = GL_RGBA;
				r_gl_format = GL_RGBA;
				r_gl_type = GL_FLOAT;
			}
		} break;
		case Image::FORMAT_RH: {
			need_decompress = true;
		} break;
		case Image::FORMAT_RGH: {
			need_decompress = true;
		} break;
		case Image::FORMAT_RGBH: {
			need_decompress = true;
		} break;
		case Image::FORMAT_RGBAH: {
			need_decompress = true;
		} break;
		case Image::FORMAT_RGBE9995: {
			r_gl_internal_format = GL_RGB;
			r_gl_format = GL_RGB;
			r_gl_type = GL_UNSIGNED_BYTE;

			if (image.is_valid())

				image = image->rgbe_to_srgb();

			return image;

		} break;
		case Image::FORMAT_DXT1: {

			if (config.s3tc_supported) {
				r_gl_internal_format = _EXT_COMPRESSED_RGBA_S3TC_DXT1_EXT;
				r_gl_format = GL_RGBA;
				r_gl_type = GL_UNSIGNED_BYTE;
				r_compressed = true;
			} else {
				need_decompress = true;
			}

		} break;
		case Image::FORMAT_DXT3: {

			if (config.s3tc_supported) {
				r_gl_internal_format = _EXT_COMPRESSED_RGBA_S3TC_DXT3_EXT;
				r_gl_format = GL_RGBA;
				r_gl_type = GL_UNSIGNED_BYTE;
				r_compressed = true;
			} else {
				need_decompress = true;
			}

		} break;
		case Image::FORMAT_DXT5: {

			if (config.s3tc_supported) {
				r_gl_internal_format = _EXT_COMPRESSED_RGBA_S3TC_DXT5_EXT;
				r_gl_format = GL_RGBA;
				r_gl_type = GL_UNSIGNED_BYTE;
				r_compressed = true;
			} else {
				need_decompress = true;
			}

		} break;
		case Image::FORMAT_RGTC_R: {

			if (config.rgtc_supported) {

				r_gl_internal_format = _EXT_COMPRESSED_RED_RGTC1_EXT;
				r_gl_format = GL_RGBA;
				r_gl_type = GL_UNSIGNED_BYTE;
				r_compressed = true;

			} else {

				need_decompress = true;
			}

		} break;
		case Image::FORMAT_RGTC_RG: {

			if (config.rgtc_supported) {

				r_gl_internal_format = _EXT_COMPRESSED_RED_GREEN_RGTC2_EXT;
				r_gl_format = GL_RGBA;
				r_gl_type = GL_UNSIGNED_BYTE;
				r_compressed = true;
			} else {

				need_decompress = true;
			}

		} break;
		case Image::FORMAT_BPTC_RGBA: {

			if (config.bptc_supported) {

				r_gl_internal_format = _EXT_COMPRESSED_RGBA_BPTC_UNORM;
				r_gl_format = GL_RGBA;
				r_gl_type = GL_UNSIGNED_BYTE;
				r_compressed = true;

			} else {

				need_decompress = true;
			}
		} break;
		case Image::FORMAT_BPTC_RGBF: {

			if (config.bptc_supported) {

				r_gl_internal_format = _EXT_COMPRESSED_RGB_BPTC_SIGNED_FLOAT;
				r_gl_format = GL_RGB;
				r_gl_type = GL_FLOAT;
				r_compressed = true;
			} else {

				need_decompress = true;
			}
		} break;
		case Image::FORMAT_BPTC_RGBFU: {
			if (config.bptc_supported) {

				r_gl_internal_format = _EXT_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT;
				r_gl_format = GL_RGB;
				r_gl_type = GL_FLOAT;
				r_compressed = true;
			} else {

				need_decompress = true;
			}
		} break;
		case Image::FORMAT_PVRTC2: {

			if (config.pvrtc_supported) {

				r_gl_internal_format = _EXT_COMPRESSED_RGB_PVRTC_2BPPV1_IMG;
				r_gl_format = GL_RGBA;
				r_gl_type = GL_UNSIGNED_BYTE;
				r_compressed = true;

			} else {

				need_decompress = true;
			}
		} break;
		case Image::FORMAT_PVRTC2A: {

			if (config.pvrtc_supported) {

				r_gl_internal_format = _EXT_COMPRESSED_RGBA_PVRTC_2BPPV1_IMG;
				r_gl_format = GL_RGBA;
				r_gl_type = GL_UNSIGNED_BYTE;
				r_compressed = true;

			} else {

				need_decompress = true;
			}

		} break;
		case Image::FORMAT_PVRTC4: {

			if (config.pvrtc_supported) {

				r_gl_internal_format = _EXT_COMPRESSED_RGB_PVRTC_4BPPV1_IMG;
				r_gl_format = GL_RGBA;
				r_gl_type = GL_UNSIGNED_BYTE;
				r_compressed = true;

			} else {

				need_decompress = true;
			}

		} break;
		case Image::FORMAT_PVRTC4A: {

			if (config.pvrtc_supported) {

				r_gl_internal_format = _EXT_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG;
				r_gl_format = GL_RGBA;
				r_gl_type = GL_UNSIGNED_BYTE;
				r_compressed = true;

			} else {

				need_decompress = true;
			}

		} break;
		case Image::FORMAT_ETC: {

			if (config.etc1_supported) {
				r_gl_internal_format = _EXT_ETC1_RGB8_OES;
				r_gl_format = GL_RGBA;
				r_gl_type = GL_UNSIGNED_BYTE;
				r_compressed = true;
			} else {
				need_decompress = true;
			}
		} break;
		case Image::FORMAT_ETC2_R11: {

			need_decompress = true;
		} break;
		case Image::FORMAT_ETC2_R11S: {

			need_decompress = true;
		} break;
		case Image::FORMAT_ETC2_RG11: {

			need_decompress = true;
		} break;
		case Image::FORMAT_ETC2_RG11S: {

			need_decompress = true;
		} break;
		case Image::FORMAT_ETC2_RGB8: {

			need_decompress = true;
		} break;
		case Image::FORMAT_ETC2_RGBA8: {

			need_decompress = true;
		} break;
		case Image::FORMAT_ETC2_RGB8A1: {

			need_decompress = true;
		} break;
		default: {

			ERR_FAIL_V(Ref<Image>());
		}
	}

	if (need_decompress || p_force_decompress) {

		if (!image.is_null()) {

			image = image->duplicate();
			image->decompress();
			ERR_FAIL_COND_V(image->is_compressed(), image);
			switch (image->get_format()) {
				case Image::FORMAT_RGB8: {
					r_gl_format = GL_RGB;
					r_gl_internal_format = GL_RGB;
					r_gl_type = GL_UNSIGNED_BYTE;
					r_real_format = Image::FORMAT_RGB8;
					r_compressed = false;
				} break;
				case Image::FORMAT_RGBA8: {
					r_gl_format = GL_RGBA;
					r_gl_internal_format = GL_RGBA;
					r_gl_type = GL_UNSIGNED_BYTE;
					r_real_format = Image::FORMAT_RGBA8;
					r_compressed = false;
				} break;
				default: {
					image->convert(Image::FORMAT_RGBA8);
					r_gl_format = GL_RGBA;
					r_gl_internal_format = GL_RGBA;
					r_gl_type = GL_UNSIGNED_BYTE;
					r_real_format = Image::FORMAT_RGBA8;
					r_compressed = false;

				} break;
			}
		}

		return image;
	}

	return p_image;
}

static const GLenum _cube_side_enum[6] = {

	GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
	GL_TEXTURE_CUBE_MAP_POSITIVE_X,
	GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
	GL_TEXTURE_CUBE_MAP_POSITIVE_Y,
	GL_TEXTURE_CUBE_MAP_NEGATIVE_Z,
	GL_TEXTURE_CUBE_MAP_POSITIVE_Z,

};

RID RasterizerStorageGLES2::texture_2d_create(const Ref<Image> &p_image)
{
	RID id = texture_create();
	ERR_FAIL_COND_V(id == RID(), id);
	
	int w = p_image->get_width();
	int h = p_image->get_height();
	
	texture_allocate(id, w, h, 1, p_image->get_format(), GD_RD::TEXTURE_TYPE_2D, 0 );
	
	texture_set_data(id, p_image);
	
	return id;
}

void RasterizerStorageGLES2::texture_2d_update_immediate(RID p_texture, const Ref<Image> &p_image, int p_layer)
{
	// only 1 layer so far
	texture_set_data(p_texture, p_image);
}
void RasterizerStorageGLES2::texture_2d_update(RID p_texture, const Ref<Image> &p_image, int p_layer)
{
	// only 1 layer so far
	texture_set_data(p_texture, p_image);
}

Ref<Image> RasterizerStorageGLES2::texture_2d_get(RID p_texture) const
{
	Texture *tex = texture_owner.getornull(p_texture);
	ERR_FAIL_COND_V(!tex, Ref<Image>());

/*
#ifdef TOOLS_ENABLED
	if (tex->image_cache_2d.is_valid()) {
		return tex->image_cache_2d;
	}
#endif
	Vector<uint8_t> data = RD::get_singleton()->texture_get_data(tex->rd_texture, 0);
	ERR_FAIL_COND_V(data.size() == 0, Ref<Image>());
	Ref<Image> image;
	image.instance();
	image->create(tex->width, tex->height, tex->mipmaps > 1, tex->validated_format, data);
	ERR_FAIL_COND_V(image->empty(), Ref<Image>());
	if (tex->format != tex->validated_format) {
		image->convert(tex->format);
	}

#ifdef TOOLS_ENABLED
	if (Engine::get_singleton()->is_editor_hint()) {
		tex->image_cache_2d = image;
	}
#endif
*/
	ERR_FAIL_COND_V(!tex->images.size(), Ref<Image>());
	
	return tex->images[0];
	
//	return image;
	
	
//	return Ref<Image>();
}


RID RasterizerStorageGLES2::texture_create() {

	Texture *texture = memnew(Texture);
	ERR_FAIL_COND_V(!texture, RID());
	glGenTextures(1, &texture->tex_id);
	texture->active = false;
	texture->total_data_size = 0;

	return texture_owner.make_rid(texture);
}

void RasterizerStorageGLES2::texture_allocate(RID p_texture, int p_width, int p_height, int p_depth_3d, Image::Format p_format, GD_RD::TextureType p_type, uint32_t p_flags) {
	GLenum format;
	GLenum internal_format;
	GLenum type;

	bool compressed = false;

	if (p_flags & TEXTURE_FLAG_USED_FOR_STREAMING) {
		p_flags &= ~TEXTURE_FLAG_MIPMAPS; // no mipies for video
	}

	Texture *texture = texture_owner.getornull(p_texture);
	ERR_FAIL_COND(!texture);
	texture->width = p_width;
	texture->height = p_height;
	texture->format = p_format;
	texture->flags = p_flags;
	texture->stored_cube_sides = 0;
	texture->type = p_type;

	switch (p_type) {
		case GD_RD::TEXTURE_TYPE_2D: {
			texture->target = GL_TEXTURE_2D;
			texture->images.resize(1);
		} break;
//		case GD_RD::TEXTURE_TYPE_EXTERNAL: {
//#ifdef ANDROID_ENABLED
//			texture->target = _GL_TEXTURE_EXTERNAL_OES;
//#else
//			texture->target = GL_TEXTURE_2D;
//#endif
//			texture->images.resize(0);
//		} break;
		case GD_RD::TEXTURE_TYPE_CUBE: {
			texture->target = GL_TEXTURE_CUBE_MAP;
			texture->images.resize(6);
		} break;
		case GD_RD::TEXTURE_TYPE_2D_ARRAY:
		case GD_RD::TEXTURE_TYPE_3D: {
			texture->target = GL_TEXTURE_3D;
			ERR_PRINT("3D textures and Texture Arrays are not supported in GLES2. Please switch to the GLES3 backend.");
			return;
		} break;
		default: {
			ERR_PRINT("Unknown texture type!");
			return;
		}
	}

//	if (p_type != GD_VS::TEXTURE_TYPE_EXTERNAL) {
//		texture->alloc_width = texture->width;
//		texture->alloc_height = texture->height;
//		texture->resize_to_po2 = false;
//		if (!config.support_npot_repeat_mipmap) {
//			int po2_width = next_power_of_2(p_width);
//			int po2_height = next_power_of_2(p_height);

//			bool is_po2 = p_width == po2_width && p_height == po2_height;

//			if (!is_po2 && (p_flags & TEXTURE_FLAG_REPEAT || p_flags & TEXTURE_FLAG_MIPMAPS)) {

//				if (p_flags & TEXTURE_FLAG_USED_FOR_STREAMING) {
//					//not supported
//					ERR_PRINT("Streaming texture for non power of 2 or has mipmaps on this hardware: " + texture->path + "'. Mipmaps and repeat disabled.");
//					texture->flags &= ~(TEXTURE_FLAG_REPEAT | TEXTURE_FLAG_MIPMAPS);
//				} else {
//					texture->alloc_height = po2_height;
//					texture->alloc_width = po2_width;
//					texture->resize_to_po2 = true;
//				}
//			}
//		}

//		Image::Format real_format;
//		_get_gl_image_and_format(Ref<Image>(),
//				texture->format,
//				texture->flags,
//				real_format,
//				format,
//				internal_format,
//				type,
//				compressed,
//				texture->resize_to_po2);

//		texture->gl_format_cache = format;
//		texture->gl_type_cache = type;
//		texture->gl_internal_format_cache = internal_format;
//		texture->data_size = 0;
//		texture->mipmaps = 1;

//		texture->compressed = compressed;
//	}
	
	
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(texture->target, texture->tex_id);

//	if (p_type == GD_VS::TEXTURE_TYPE_EXTERNAL) {
//		glTexParameteri(texture->target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
//		glTexParameteri(texture->target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
//		glTexParameteri(texture->target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
//		glTexParameteri(texture->target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
//	} else if (p_flags & TEXTURE_FLAG_USED_FOR_STREAMING) {
//		//prealloc if video
//		glTexImage2D(texture->target, 0, internal_format, texture->alloc_width, texture->alloc_height, 0, format, type, NULL);
//	}

	texture->active = true;
}

void RasterizerStorageGLES2::texture_set_data(RID p_texture, const Ref<Image> &p_image, int p_layer) {
	Texture *texture = texture_owner.getornull(p_texture);

	ERR_FAIL_COND(!texture);
	if (texture->target == GL_TEXTURE_3D) {
		// Target is set to a 3D texture or array texture, exit early to avoid spamming errors
		return;
	}
	ERR_FAIL_COND(!texture->active);
	ERR_FAIL_COND(texture->render_target);
	ERR_FAIL_COND(texture->format != p_image->get_format());
	ERR_FAIL_COND(p_image.is_null());
//	ERR_FAIL_COND(texture->type == GD_VS::TEXTURE_TYPE_EXTERNAL);

	GLenum type;
	GLenum format;
	GLenum internal_format;
	bool compressed = false;

	if (config.keep_original_textures && !(texture->flags & TEXTURE_FLAG_USED_FOR_STREAMING)) {
		texture->images.write[p_layer] = p_image;
	}

	Image::Format real_format;
	Ref<Image> img = _get_gl_image_and_format(p_image, p_image->get_format(), texture->flags, real_format, format, internal_format, type, compressed, texture->resize_to_po2);

	if (texture->resize_to_po2) {
		if (p_image->is_compressed()) {
			ERR_PRINT("Texture '" + texture->path + "' is required to be a power of 2 because it uses either mipmaps or repeat, so it was decompressed. This will hurt performance and memory usage.");
		}

		if (img == p_image) {
			img = img->duplicate();
		}
		img->resize_to_po2(false);
	}

	if (config.shrink_textures_x2 && (p_image->has_mipmaps() || !p_image->is_compressed()) && !(texture->flags & TEXTURE_FLAG_USED_FOR_STREAMING)) {

		texture->alloc_height = MAX(1, texture->alloc_height / 2);
		texture->alloc_width = MAX(1, texture->alloc_width / 2);

		if (texture->alloc_width == img->get_width() / 2 && texture->alloc_height == img->get_height() / 2) {

			img->shrink_x2();
		} else if (img->get_format() <= Image::FORMAT_RGBA8) {

			img->resize(texture->alloc_width, texture->alloc_height, Image::INTERPOLATE_BILINEAR);
		}
	}

	GLenum blit_target = (texture->target == GL_TEXTURE_CUBE_MAP) ? _cube_side_enum[p_layer] : GL_TEXTURE_2D;

	texture->data_size = img->get_data().size();
	Vector<uint8_t> read = img->get_data();

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(texture->target, texture->tex_id);

	texture->ignore_mipmaps = compressed && !img->has_mipmaps();

	if ((texture->flags & TEXTURE_FLAG_MIPMAPS) && !texture->ignore_mipmaps)
		if (texture->flags & TEXTURE_FLAG_FILTER) {
			glTexParameteri(texture->target, GL_TEXTURE_MIN_FILTER, config.use_fast_texture_filter ? GL_LINEAR_MIPMAP_NEAREST : GL_LINEAR_MIPMAP_LINEAR);
		} else {
			glTexParameteri(texture->target, GL_TEXTURE_MIN_FILTER, config.use_fast_texture_filter ? GL_NEAREST_MIPMAP_NEAREST : GL_NEAREST_MIPMAP_LINEAR);
		}
	else {
		if (texture->flags & TEXTURE_FLAG_FILTER) {
			glTexParameteri(texture->target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		} else {
			glTexParameteri(texture->target, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		}
	}

	if (texture->flags & TEXTURE_FLAG_FILTER) {

		glTexParameteri(texture->target, GL_TEXTURE_MAG_FILTER, GL_LINEAR); // Linear Filtering

	} else {

		glTexParameteri(texture->target, GL_TEXTURE_MAG_FILTER, GL_NEAREST); // raw Filtering
	}

	if (((texture->flags & TEXTURE_FLAG_REPEAT) || (texture->flags & TEXTURE_FLAG_MIRRORED_REPEAT)) && texture->target != GL_TEXTURE_CUBE_MAP) {

		if (texture->flags & TEXTURE_FLAG_MIRRORED_REPEAT) {
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
		} else {
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		}
	} else {

		//glTexParameterf( texture->target, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE );
		glTexParameterf(texture->target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameterf(texture->target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	}

	int mipmaps = ((texture->flags & TEXTURE_FLAG_MIPMAPS) && img->has_mipmaps()) ? img->get_mipmap_count() + 1 : 1;

	int w = img->get_width();
	int h = img->get_height();

	int tsize = 0;

	for (int i = 0; i < mipmaps; i++) {

		int size, ofs;
		img->get_mipmap_offset_and_size(i, ofs, size);

		if (compressed) {
			glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

			int bw = w;
			int bh = h;

			glCompressedTexImage2D(blit_target, i, internal_format, bw, bh, 0, size, &read[ofs]);
		} else {

			glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
			if (texture->flags & TEXTURE_FLAG_USED_FOR_STREAMING) {
				glTexSubImage2D(blit_target, i, 0, 0, w, h, format, type, &read[ofs]);
			} else {
				glTexImage2D(blit_target, i, internal_format, w, h, 0, format, type, &read[ofs]);
			}
		}

		tsize += size;

		w = MAX(1, w >> 1);
		h = MAX(1, h >> 1);
	}

	info.texture_mem -= texture->total_data_size;
	texture->total_data_size = tsize;
	info.texture_mem += texture->total_data_size;

	// printf("texture: %i x %i - size: %i - total: %i\n", texture->width, texture->height, tsize, info.texture_mem);

	texture->stored_cube_sides |= (1 << p_layer);

	if ((texture->flags & TEXTURE_FLAG_MIPMAPS) && mipmaps == 1 && !texture->ignore_mipmaps && (texture->type != GD_RD::TEXTURE_TYPE_CUBE || texture->stored_cube_sides == (1 << 6) - 1)) {
		//generate mipmaps if they were requested and the image does not contain them
		glGenerateMipmap(texture->target);
	}

	texture->mipmaps = mipmaps;
}

void RasterizerStorageGLES2::texture_set_data_partial(RID p_texture, const Ref<Image> &p_image, int src_x, int src_y, int src_w, int src_h, int dst_x, int dst_y, int p_dst_mip, int p_layer) {
	// TODO
	ERR_PRINT("Not implemented (ask Karroffel to do it :p)");
}

/*
Ref<Image> RasterizerStorageGLES2::texture_get_data(RID p_texture, int p_layer) const {

	Texture *texture = texture_owner.getornull(p_texture);

	ERR_FAIL_COND_V(!texture, Ref<Image>());
	ERR_FAIL_COND_V(!texture->active, Ref<Image>());
	ERR_FAIL_COND_V(texture->data_size == 0 && !texture->render_target, Ref<Image>());

	if (texture->type == GD_VS::TEXTURE_TYPE_CUBEMAP && p_layer < 6 && p_layer >= 0 && !texture->images[p_layer].is_null()) {
		return texture->images[p_layer];
	}

#ifdef GLES_OVER_GL

	Image::Format real_format;
	GLenum gl_format;
	GLenum gl_internal_format;
	GLenum gl_type;
	bool compressed;
	_get_gl_image_and_format(Ref<Image>(), texture->format, texture->flags, real_format, gl_format, gl_internal_format, gl_type, compressed, false);

	PoolVector<uint8_t> data;

	int data_size = Image::get_image_data_size(texture->alloc_width, texture->alloc_height, real_format, texture->mipmaps > 1);

	data.resize(data_size * 2); //add some memory at the end, just in case for buggy drivers
	PoolVector<uint8_t>::Write wb = data.write();

	glActiveTexture(GL_TEXTURE0);

	glBindTexture(texture->target, texture->tex_id);

	glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);

	for (int i = 0; i < texture->mipmaps; i++) {

		int ofs = Image::get_image_mipmap_offset(texture->alloc_width, texture->alloc_height, real_format, i);

		if (texture->compressed) {
			glPixelStorei(GL_PACK_ALIGNMENT, 4);
			glGetCompressedTexImage(texture->target, i, &wb[ofs]);
		} else {
			glPixelStorei(GL_PACK_ALIGNMENT, 1);
			glGetTexImage(texture->target, i, texture->gl_format_cache, texture->gl_type_cache, &wb[ofs]);
		}
	}

	wb.release();

	data.resize(data_size);

	Image *img = memnew(Image(texture->alloc_width, texture->alloc_height, texture->mipmaps > 1, real_format, data));

	return Ref<Image>(img);
#else

	Image::Format real_format;
	GLenum gl_format;
	GLenum gl_internal_format;
	GLenum gl_type;
	bool compressed;
	_get_gl_image_and_format(Ref<Image>(), texture->format, texture->flags, real_format, gl_format, gl_internal_format, gl_type, compressed, texture->resize_to_po2);

	PoolVector<uint8_t> data;

	int data_size = Image::get_image_data_size(texture->alloc_width, texture->alloc_height, Image::FORMAT_RGBA8, false);

	data.resize(data_size * 2); //add some memory at the end, just in case for buggy drivers
	PoolVector<uint8_t>::Write wb = data.write();

	GLuint temp_framebuffer;
	glGenFramebuffers(1, &temp_framebuffer);

	GLuint temp_color_texture;
	glGenTextures(1, &temp_color_texture);

	glBindFramebuffer(GL_FRAMEBUFFER, temp_framebuffer);

	glBindTexture(GL_TEXTURE_2D, temp_color_texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texture->alloc_width, texture->alloc_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, temp_color_texture, 0);

	glDepthMask(GL_FALSE);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	glDisable(GL_BLEND);
	glDepthFunc(GL_LEQUAL);
	glColorMask(1, 1, 1, 1);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture->tex_id);

	glViewport(0, 0, texture->alloc_width, texture->alloc_height);

	shaders.copy.bind();

	glClearColor(0.0, 0.0, 0.0, 0.0);
	glClear(GL_COLOR_BUFFER_BIT);
	bind_quad_array();
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glReadPixels(0, 0, texture->alloc_width, texture->alloc_height, GL_RGBA, GL_UNSIGNED_BYTE, &wb[0]);

	glDeleteTextures(1, &temp_color_texture);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glDeleteFramebuffers(1, &temp_framebuffer);

	wb.release();

	data.resize(data_size);

	Image *img = memnew(Image(texture->alloc_width, texture->alloc_height, false, Image::FORMAT_RGBA8, data));
	if (!texture->compressed) {
		img->convert(real_format);
	}

	return Ref<Image>(img);

#endif
}
*/

void RasterizerStorageGLES2::texture_set_flags(RID p_texture, uint32_t p_flags) {

	Texture *texture = texture_owner.getornull(p_texture);
	ERR_FAIL_COND(!texture);

	bool had_mipmaps = texture->flags & TEXTURE_FLAG_MIPMAPS;

	texture->flags = p_flags;

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(texture->target, texture->tex_id);

	if (((texture->flags & TEXTURE_FLAG_REPEAT) || (texture->flags & TEXTURE_FLAG_MIRRORED_REPEAT)) && texture->target != GL_TEXTURE_CUBE_MAP) {

		if (texture->flags & TEXTURE_FLAG_MIRRORED_REPEAT) {
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
		} else {
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		}
	} else {
		//glTexParameterf( texture->target, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE );
		glTexParameterf(texture->target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameterf(texture->target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	}

	if ((texture->flags & TEXTURE_FLAG_MIPMAPS) && !texture->ignore_mipmaps) {
		if (!had_mipmaps && texture->mipmaps == 1) {
			glGenerateMipmap(texture->target);
		}
		if (texture->flags & TEXTURE_FLAG_FILTER) {
			glTexParameteri(texture->target, GL_TEXTURE_MIN_FILTER, config.use_fast_texture_filter ? GL_LINEAR_MIPMAP_NEAREST : GL_LINEAR_MIPMAP_LINEAR);
		} else {
			glTexParameteri(texture->target, GL_TEXTURE_MIN_FILTER, config.use_fast_texture_filter ? GL_NEAREST_MIPMAP_NEAREST : GL_NEAREST_MIPMAP_LINEAR);
		}

	} else {
		if (texture->flags & TEXTURE_FLAG_FILTER) {
			glTexParameteri(texture->target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		} else {
			glTexParameteri(texture->target, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		}
	}

	if (texture->flags & TEXTURE_FLAG_FILTER) {

		glTexParameteri(texture->target, GL_TEXTURE_MAG_FILTER, GL_LINEAR); // Linear Filtering

	} else {

		glTexParameteri(texture->target, GL_TEXTURE_MAG_FILTER, GL_NEAREST); // raw Filtering
	}
}

uint32_t RasterizerStorageGLES2::texture_get_flags(RID p_texture) const {
	Texture *texture = texture_owner.getornull(p_texture);

	ERR_FAIL_COND_V(!texture, 0);

	return texture->flags;
}

Image::Format RasterizerStorageGLES2::texture_get_format(RID p_texture) const {
	Texture *texture = texture_owner.getornull(p_texture);

	ERR_FAIL_COND_V(!texture, Image::FORMAT_L8);

	return texture->format;
}

GD_RD::TextureType RasterizerStorageGLES2::texture_get_type(RID p_texture) const {
	Texture *texture = texture_owner.getornull(p_texture);

	ERR_FAIL_COND_V(!texture, GD_RD::TEXTURE_TYPE_2D);

	return texture->type;
}

uint32_t RasterizerStorageGLES2::texture_get_texid(RID p_texture) const {
	Texture *texture = texture_owner.getornull(p_texture);

	ERR_FAIL_COND_V(!texture, 0);

	return texture->tex_id;
}

void RasterizerStorageGLES2::texture_bind(RID p_texture, uint32_t p_texture_no) {
	Texture *texture = texture_owner.getornull(p_texture);

	ERR_FAIL_COND(!texture);

	glActiveTexture(GL_TEXTURE0 + p_texture_no);
	glBindTexture(texture->target, texture->tex_id);
}

uint32_t RasterizerStorageGLES2::texture_get_width(RID p_texture) const {
	Texture *texture = texture_owner.getornull(p_texture);

	ERR_FAIL_COND_V(!texture, 0);

	return texture->width;
}

uint32_t RasterizerStorageGLES2::texture_get_height(RID p_texture) const {
	Texture *texture = texture_owner.getornull(p_texture);

	ERR_FAIL_COND_V(!texture, 0);

	return texture->height;
}

uint32_t RasterizerStorageGLES2::texture_get_depth(RID p_texture) const {
	Texture *texture = texture_owner.getornull(p_texture);

	ERR_FAIL_COND_V(!texture, 0);

	return texture->depth;
}

void RasterizerStorageGLES2::texture_set_size_override(RID p_texture, int p_width, int p_height) 
{
	Texture *texture = texture_owner.getornull(p_texture);
	
	ERR_FAIL_COND(!texture);
	ERR_FAIL_COND(texture->render_target);
	
	ERR_FAIL_COND(p_width <= 0 || p_width > 16384);
	ERR_FAIL_COND(p_height <= 0 || p_height > 16384);
	//real texture size is in alloc width and height
	texture->width = p_width;
	texture->height = p_height;
}

void RasterizerStorageGLES2::texture_set_path(RID p_texture, const String &p_path) {
	Texture *texture = texture_owner.getornull(p_texture);
	ERR_FAIL_COND(!texture);

	texture->path = p_path;
}

String RasterizerStorageGLES2::texture_get_path(RID p_texture) const {
	Texture *texture = texture_owner.getornull(p_texture);
	ERR_FAIL_COND_V(!texture, "");

	return texture->path;
}

void RasterizerStorageGLES2::texture_debug_usage(List<GD_VS::TextureInfo> *r_info) {
	List<RID> textures;
	texture_owner.get_owned_list(&textures);

	for (List<RID>::Element *E = textures.front(); E; E = E->next()) {

		Texture *t = texture_owner.getornull(E->get());
		if (!t)
			continue;
		GD_VS::TextureInfo tinfo;
		tinfo.path = t->path;
		tinfo.format = t->format;
		tinfo.width = t->alloc_width;
		tinfo.height = t->alloc_height;
		tinfo.depth = 0;
		tinfo.bytes = t->total_data_size;
		r_info->push_back(tinfo);
	}
}

void RasterizerStorageGLES2::texture_set_shrink_all_x2_on_set_data(bool p_enable) {
	config.shrink_textures_x2 = p_enable;
}

void RasterizerStorageGLES2::textures_keep_original(bool p_enable) {
	config.keep_original_textures = p_enable;
}

Size2 RasterizerStorageGLES2::texture_size_with_proxy(RID p_texture) {

	const Texture *texture = texture_owner.getornull(p_texture);
	ERR_FAIL_COND_V(!texture, Size2());
	if (texture->proxy) {
		return Size2(texture->proxy->width, texture->proxy->height);
	} else {
		return Size2(texture->width, texture->height);
	}
}

void RasterizerStorageGLES2::texture_set_proxy(RID p_texture, RID p_proxy) {
	Texture *texture = texture_owner.getornull(p_texture);
	ERR_FAIL_COND(!texture);

	if (texture->proxy) {
		texture->proxy->proxy_owners.erase(texture);
		texture->proxy = NULL;
	}

	if (p_proxy.is_valid()) {
		Texture *proxy = texture_owner.getornull(p_proxy);
		ERR_FAIL_COND(!proxy);
		ERR_FAIL_COND(proxy == texture);
		proxy->proxy_owners.insert(texture);
		texture->proxy = proxy;
	}
}

void RasterizerStorageGLES2::texture_set_force_redraw_if_visible(RID p_texture, bool p_enable) {

	Texture *texture = texture_owner.getornull(p_texture);
	ERR_FAIL_COND(!texture);

	texture->redraw_if_visible = p_enable;
}

void RasterizerStorageGLES2::texture_set_detect_3d_callback(RID p_texture, GD_VS::TextureDetectCallback p_callback, void *p_userdata) {
	Texture *texture = texture_owner.getornull(p_texture);
	ERR_FAIL_COND(!texture);

	texture->detect_3d = p_callback;
	texture->detect_3d_ud = p_userdata;
}

void RasterizerStorageGLES2::texture_set_detect_srgb_callback(RID p_texture, GD_VS::TextureDetectCallback p_callback, void *p_userdata) {
	Texture *texture = texture_owner.getornull(p_texture);
	ERR_FAIL_COND(!texture);

	texture->detect_srgb = p_callback;
	texture->detect_srgb_ud = p_userdata;
}

void RasterizerStorageGLES2::texture_set_detect_normal_callback(RID p_texture, GD_VS::TextureDetectCallback p_callback, void *p_userdata) {
	Texture *texture = texture_owner.getornull(p_texture);
	ERR_FAIL_COND(!texture);

	texture->detect_normal = p_callback;
	texture->detect_normal_ud = p_userdata;
}

RID RasterizerStorageGLES2::texture_create_radiance_cubemap(RID p_source, int p_resolution) const {

	return RID();
}

RID RasterizerStorageGLES2::sky_create() {
	Sky *sky = memnew(Sky);
	sky->radiance = 0;
	return sky_owner.make_rid(sky);
}

void RasterizerStorageGLES2::sky_set_texture(RID p_sky, RID p_panorama, int p_radiance_size) {
	Sky *sky = sky_owner.getornull(p_sky);
	ERR_FAIL_COND(!sky);

	if (sky->panorama.is_valid()) {
		sky->panorama = RID();
		glDeleteTextures(1, &sky->radiance);
		sky->radiance = 0;
	}

	sky->panorama = p_panorama;
	if (!sky->panorama.is_valid()) {
		return; // the panorama was cleared
	}

	Texture *texture = texture_owner.getornull(sky->panorama);
	if (!texture) {
		sky->panorama = RID();
		ERR_FAIL_COND(!texture);
	}

	// glBindVertexArray(0) and more
	{
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
		glDisable(GL_CULL_FACE);
		glDisable(GL_DEPTH_TEST);
		glDisable(GL_SCISSOR_TEST);
		glDisable(GL_BLEND);

		for (int i = 0; i < GD_VS::ARRAY_MAX - 1; i++) {
			glDisableVertexAttribArray(i);
		}
	}

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(texture->target, texture->tex_id);

	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); //need this for proper sampling

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, resources.radical_inverse_vdc_cache_tex);

	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	// New cubemap that will hold the mipmaps with different roughness values
	glActiveTexture(GL_TEXTURE2);
	glGenTextures(1, &sky->radiance);
	glBindTexture(GL_TEXTURE_CUBE_MAP, sky->radiance);

	int size = p_radiance_size / 2; //divide by two because its a cubemap (this is an approximation because GLES3 uses a dual paraboloid)

	GLenum internal_format = GL_RGB;
	GLenum format = GL_RGB;
	GLenum type = GL_UNSIGNED_BYTE;

	// Set the initial (empty) mipmaps
	// Mobile hardware (PowerVR specially) prefers this approach,
	// the previous approach with manual lod levels kills the game.
	for (int i = 0; i < 6; i++) {
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, internal_format, size, size, 0, format, type, NULL);
	}

	glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

	// No filters for now
	glTexParameterf(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	// Framebuffer
	
	bind_framebuffer(resources.mipmap_blur_fbo);

	int mipmaps = 6;
	int lod = 0;
	int mm_level = mipmaps;
	size = p_radiance_size / 2;
	shaders.cubemap_filter.set_conditional(CubemapFilterShaderGLES2::USE_SOURCE_PANORAMA, true);
	shaders.cubemap_filter.set_conditional(CubemapFilterShaderGLES2::USE_DIRECT_WRITE, true);
	shaders.cubemap_filter.bind();

	// third, render to the framebuffer using separate textures, then copy to mipmaps
	while (size >= 1) {

		//make framebuffer size the texture size, need to use a separate texture for compatibility
		glActiveTexture(GL_TEXTURE3);
		glBindTexture(GL_TEXTURE_2D, resources.mipmap_blur_color);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, size, size, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, resources.mipmap_blur_color, 0);

		if (lod == 1) {
			//bind panorama for smaller lods

			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_CUBE_MAP, sky->radiance);
			shaders.cubemap_filter.set_conditional(CubemapFilterShaderGLES2::USE_SOURCE_PANORAMA, false);
			shaders.cubemap_filter.set_conditional(CubemapFilterShaderGLES2::USE_DIRECT_WRITE, false);
			shaders.cubemap_filter.bind();
		}
		glViewport(0, 0, size, size);
		bind_quad_array();

		glActiveTexture(GL_TEXTURE2); //back to panorama

		for (int i = 0; i < 6; i++) {

			shaders.cubemap_filter.set_uniform(CubemapFilterShaderGLES2::FACE_ID, i);

			float roughness = mm_level >= 0 ? lod / (float)(mipmaps - 1) : 1;
			roughness = MIN(1.0, roughness); //keep max at 1
			shaders.cubemap_filter.set_uniform(CubemapFilterShaderGLES2::ROUGHNESS, roughness);
			shaders.cubemap_filter.set_uniform(CubemapFilterShaderGLES2::Z_FLIP, false);

			glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

			glCopyTexSubImage2D(_cube_side_enum[i], lod, 0, 0, 0, 0, size, size);
		}

		size >>= 1;

		mm_level--;

		lod++;
	}

	shaders.cubemap_filter.set_conditional(CubemapFilterShaderGLES2::USE_SOURCE_PANORAMA, false);
	shaders.cubemap_filter.set_conditional(CubemapFilterShaderGLES2::USE_DIRECT_WRITE, false);

	// restore ranges
	glActiveTexture(GL_TEXTURE2); //back to panorama

	glTexParameterf(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameterf(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glBindTexture(GL_TEXTURE_2D, 0);
	glActiveTexture(GL_TEXTURE3); //back to panorama
	glBindTexture(GL_TEXTURE_2D, 0);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, 0);

	//reset flags on Sky Texture that may have changed
	texture_set_flags(sky->panorama, texture->flags);

	// Framebuffer did its job. thank mr framebuffer
	glActiveTexture(GL_TEXTURE0); //back to panorama
	bind_framebuffer_system();
}

/* SHADER API */

RID RasterizerStorageGLES2::shader_create() {

	Shader *shader = memnew(Shader);
	shader->mode = GD_VS::SHADER_SPATIAL;
	shader->shader = &scene->state.scene_shader;
	RID rid = shader_owner.make_rid(shader);
	_shader_make_dirty(shader);
	shader->self = rid;

	return rid;
}

void RasterizerStorageGLES2::_shader_make_dirty(Shader *p_shader) {
	if (p_shader->dirty_list.in_list())
		return;

	_shader_dirty_list.add(&p_shader->dirty_list);
}

void RasterizerStorageGLES2::shader_set_code(RID p_shader, const String &p_code) {

	Shader *shader = shader_owner.getornull(p_shader);
	ERR_FAIL_COND(!shader);

	shader->code = p_code;

	String mode_string = ShaderLanguage::get_shader_type(p_code);
	GD_VS::ShaderMode mode;

	if (mode_string == "canvas_item")
		mode = GD_VS::SHADER_CANVAS_ITEM;
	else if (mode_string == "particles")
		mode = GD_VS::SHADER_PARTICLES;
	else
		mode = GD_VS::SHADER_SPATIAL;

	if (shader->custom_code_id && mode != shader->mode) {
		shader->shader->free_custom_shader(shader->custom_code_id);
		shader->custom_code_id = 0;
	}

	shader->mode = mode;

	// TODO handle all shader types
	if (mode == GD_VS::SHADER_CANVAS_ITEM) {
		shader->shader = &canvas->state.canvas_shader;

	} else if (mode == GD_VS::SHADER_SPATIAL) {
		shader->shader = &scene->state.scene_shader;
	} else {
		return;
	}

	if (shader->custom_code_id == 0) {
		shader->custom_code_id = shader->shader->create_custom_shader();
	}

	_shader_make_dirty(shader);
}

String RasterizerStorageGLES2::shader_get_code(RID p_shader) const {

	const Shader *shader = shader_owner.getornull(p_shader);
	ERR_FAIL_COND_V(!shader, "");

	return shader->code;
}

void RasterizerStorageGLES2::_update_shader(Shader *p_shader) const {

	_shader_dirty_list.remove(&p_shader->dirty_list);

	p_shader->valid = false;

	p_shader->uniforms.clear();

	if (p_shader->code == String()) {
		return; //just invalid, but no error
	}

	ShaderCompilerGLES2::GeneratedCode gen_code;
	ShaderCompilerGLES2::IdentifierActions *actions = NULL;

	switch (p_shader->mode) {

		case GD_VS::SHADER_CANVAS_ITEM: {

			p_shader->canvas_item.light_mode = Shader::CanvasItem::LIGHT_MODE_NORMAL;
			p_shader->canvas_item.blend_mode = Shader::CanvasItem::BLEND_MODE_MIX;

			p_shader->canvas_item.uses_screen_texture = false;
			p_shader->canvas_item.uses_screen_uv = false;
			p_shader->canvas_item.uses_time = false;
			p_shader->canvas_item.uses_modulate = false;
			p_shader->canvas_item.uses_color = false;
			p_shader->canvas_item.uses_vertex = false;
			p_shader->canvas_item.batch_flags = 0;

			p_shader->canvas_item.uses_world_matrix = false;
			p_shader->canvas_item.uses_extra_matrix = false;
			p_shader->canvas_item.uses_projection_matrix = false;
			p_shader->canvas_item.uses_instance_custom = false;

			shaders.actions_canvas.render_mode_values["blend_add"] = Pair<int *, int>(&p_shader->canvas_item.blend_mode, Shader::CanvasItem::BLEND_MODE_ADD);
			shaders.actions_canvas.render_mode_values["blend_mix"] = Pair<int *, int>(&p_shader->canvas_item.blend_mode, Shader::CanvasItem::BLEND_MODE_MIX);
			shaders.actions_canvas.render_mode_values["blend_sub"] = Pair<int *, int>(&p_shader->canvas_item.blend_mode, Shader::CanvasItem::BLEND_MODE_SUB);
			shaders.actions_canvas.render_mode_values["blend_mul"] = Pair<int *, int>(&p_shader->canvas_item.blend_mode, Shader::CanvasItem::BLEND_MODE_MUL);
			shaders.actions_canvas.render_mode_values["blend_premul_alpha"] = Pair<int *, int>(&p_shader->canvas_item.blend_mode, Shader::CanvasItem::BLEND_MODE_PMALPHA);

			shaders.actions_canvas.render_mode_values["unshaded"] = Pair<int *, int>(&p_shader->canvas_item.light_mode, Shader::CanvasItem::LIGHT_MODE_UNSHADED);
			shaders.actions_canvas.render_mode_values["light_only"] = Pair<int *, int>(&p_shader->canvas_item.light_mode, Shader::CanvasItem::LIGHT_MODE_LIGHT_ONLY);

			shaders.actions_canvas.usage_flag_pointers["SCREEN_UV"] = &p_shader->canvas_item.uses_screen_uv;
			shaders.actions_canvas.usage_flag_pointers["SCREEN_PIXEL_SIZE"] = &p_shader->canvas_item.uses_screen_uv;
			shaders.actions_canvas.usage_flag_pointers["SCREEN_TEXTURE"] = &p_shader->canvas_item.uses_screen_texture;
			shaders.actions_canvas.usage_flag_pointers["TIME"] = &p_shader->canvas_item.uses_time;
			shaders.actions_canvas.usage_flag_pointers["MODULATE"] = &p_shader->canvas_item.uses_modulate;
			shaders.actions_canvas.usage_flag_pointers["COLOR"] = &p_shader->canvas_item.uses_color;

			shaders.actions_canvas.usage_flag_pointers["VERTEX"] = &p_shader->canvas_item.uses_vertex;

			shaders.actions_canvas.usage_flag_pointers["WORLD_MATRIX"] = &p_shader->canvas_item.uses_world_matrix;
			shaders.actions_canvas.usage_flag_pointers["EXTRA_MATRIX"] = &p_shader->canvas_item.uses_extra_matrix;
			shaders.actions_canvas.usage_flag_pointers["PROJECTION_MATRIX"] = &p_shader->canvas_item.uses_projection_matrix;
			shaders.actions_canvas.usage_flag_pointers["INSTANCE_CUSTOM"] = &p_shader->canvas_item.uses_instance_custom;

			actions = &shaders.actions_canvas;
			actions->uniforms = &p_shader->uniforms;
		} break;

		case GD_VS::SHADER_SPATIAL: {
			p_shader->spatial.blend_mode = Shader::Spatial::BLEND_MODE_MIX;
			p_shader->spatial.depth_draw_mode = Shader::Spatial::DEPTH_DRAW_OPAQUE;
			p_shader->spatial.cull_mode = Shader::Spatial::CULL_MODE_BACK;
			p_shader->spatial.uses_alpha = false;
			p_shader->spatial.uses_alpha_scissor = false;
			p_shader->spatial.uses_discard = false;
			p_shader->spatial.unshaded = false;
			p_shader->spatial.no_depth_test = false;
			p_shader->spatial.uses_sss = false;
			p_shader->spatial.uses_time = false;
			p_shader->spatial.uses_vertex_lighting = false;
			p_shader->spatial.uses_screen_texture = false;
			p_shader->spatial.uses_depth_texture = false;
			p_shader->spatial.uses_vertex = false;
			p_shader->spatial.uses_tangent = false;
			p_shader->spatial.uses_ensure_correct_normals = false;
			p_shader->spatial.writes_modelview_or_projection = false;
			p_shader->spatial.uses_world_coordinates = false;

			shaders.actions_scene.render_mode_values["blend_add"] = Pair<int *, int>(&p_shader->spatial.blend_mode, Shader::Spatial::BLEND_MODE_ADD);
			shaders.actions_scene.render_mode_values["blend_mix"] = Pair<int *, int>(&p_shader->spatial.blend_mode, Shader::Spatial::BLEND_MODE_MIX);
			shaders.actions_scene.render_mode_values["blend_sub"] = Pair<int *, int>(&p_shader->spatial.blend_mode, Shader::Spatial::BLEND_MODE_SUB);
			shaders.actions_scene.render_mode_values["blend_mul"] = Pair<int *, int>(&p_shader->spatial.blend_mode, Shader::Spatial::BLEND_MODE_MUL);

			shaders.actions_scene.render_mode_values["depth_draw_opaque"] = Pair<int *, int>(&p_shader->spatial.depth_draw_mode, Shader::Spatial::DEPTH_DRAW_OPAQUE);
			shaders.actions_scene.render_mode_values["depth_draw_always"] = Pair<int *, int>(&p_shader->spatial.depth_draw_mode, Shader::Spatial::DEPTH_DRAW_ALWAYS);
			shaders.actions_scene.render_mode_values["depth_draw_never"] = Pair<int *, int>(&p_shader->spatial.depth_draw_mode, Shader::Spatial::DEPTH_DRAW_NEVER);
			shaders.actions_scene.render_mode_values["depth_draw_alpha_prepass"] = Pair<int *, int>(&p_shader->spatial.depth_draw_mode, Shader::Spatial::DEPTH_DRAW_ALPHA_PREPASS);

			shaders.actions_scene.render_mode_values["cull_front"] = Pair<int *, int>(&p_shader->spatial.cull_mode, Shader::Spatial::CULL_MODE_FRONT);
			shaders.actions_scene.render_mode_values["cull_back"] = Pair<int *, int>(&p_shader->spatial.cull_mode, Shader::Spatial::CULL_MODE_BACK);
			shaders.actions_scene.render_mode_values["cull_disabled"] = Pair<int *, int>(&p_shader->spatial.cull_mode, Shader::Spatial::CULL_MODE_DISABLED);

			shaders.actions_scene.render_mode_flags["unshaded"] = &p_shader->spatial.unshaded;
			shaders.actions_scene.render_mode_flags["depth_test_disable"] = &p_shader->spatial.no_depth_test;

			shaders.actions_scene.render_mode_flags["vertex_lighting"] = &p_shader->spatial.uses_vertex_lighting;

			shaders.actions_scene.render_mode_flags["world_vertex_coords"] = &p_shader->spatial.uses_world_coordinates;

			shaders.actions_scene.render_mode_flags["ensure_correct_normals"] = &p_shader->spatial.uses_ensure_correct_normals;

			shaders.actions_scene.usage_flag_pointers["ALPHA"] = &p_shader->spatial.uses_alpha;
			shaders.actions_scene.usage_flag_pointers["ALPHA_SCISSOR"] = &p_shader->spatial.uses_alpha_scissor;

			shaders.actions_scene.usage_flag_pointers["SSS_STRENGTH"] = &p_shader->spatial.uses_sss;
			shaders.actions_scene.usage_flag_pointers["DISCARD"] = &p_shader->spatial.uses_discard;
			shaders.actions_scene.usage_flag_pointers["SCREEN_TEXTURE"] = &p_shader->spatial.uses_screen_texture;
			shaders.actions_scene.usage_flag_pointers["DEPTH_TEXTURE"] = &p_shader->spatial.uses_depth_texture;
			shaders.actions_scene.usage_flag_pointers["TIME"] = &p_shader->spatial.uses_time;

			// Use of any of these BUILTINS indicate the need for transformed tangents.
			// This is needed to know when to transform tangents in software skinning.
			shaders.actions_scene.usage_flag_pointers["TANGENT"] = &p_shader->spatial.uses_tangent;
			shaders.actions_scene.usage_flag_pointers["NORMALMAP"] = &p_shader->spatial.uses_tangent;

			shaders.actions_scene.write_flag_pointers["MODELVIEW_MATRIX"] = &p_shader->spatial.writes_modelview_or_projection;
			shaders.actions_scene.write_flag_pointers["PROJECTION_MATRIX"] = &p_shader->spatial.writes_modelview_or_projection;
			shaders.actions_scene.write_flag_pointers["VERTEX"] = &p_shader->spatial.uses_vertex;

			actions = &shaders.actions_scene;
			actions->uniforms = &p_shader->uniforms;

			if (p_shader->spatial.uses_screen_texture && p_shader->spatial.uses_depth_texture) {
				ERR_PRINT_ONCE("Using both SCREEN_TEXTURE and DEPTH_TEXTURE is not supported in GLES2");
			}

			if (p_shader->spatial.uses_depth_texture && !config.support_depth_texture) {
				ERR_PRINT_ONCE("Using DEPTH_TEXTURE is not permitted on this hardware, operation will fail.");
			}
		} break;

		default: {
			return;
		} break;
	}

	Error err = shaders.compiler.compile(p_shader->mode, p_shader->code, actions, p_shader->path, gen_code);
	if (err != OK) {
		return;
	}

	p_shader->shader->set_custom_shader_code(p_shader->custom_code_id, gen_code.vertex, gen_code.vertex_global, gen_code.fragment, gen_code.light, gen_code.fragment_global, gen_code.uniforms, gen_code.texture_uniforms, gen_code.custom_defines);

	p_shader->texture_count = gen_code.texture_uniforms.size();
	p_shader->texture_hints = gen_code.texture_hints;

	p_shader->uses_vertex_time = gen_code.uses_vertex_time;
	p_shader->uses_fragment_time = gen_code.uses_fragment_time;

	// some logic for batching
	if (p_shader->mode == GD_VS::SHADER_CANVAS_ITEM) {
		if (p_shader->canvas_item.uses_modulate | p_shader->canvas_item.uses_color) {
			p_shader->canvas_item.batch_flags |= RasterizerStorageCommon::PREVENT_COLOR_BAKING;
		}
		if (p_shader->canvas_item.uses_vertex) {
			p_shader->canvas_item.batch_flags |= RasterizerStorageCommon::PREVENT_VERTEX_BAKING;
		}
		if (p_shader->canvas_item.uses_world_matrix | p_shader->canvas_item.uses_extra_matrix | p_shader->canvas_item.uses_projection_matrix | p_shader->canvas_item.uses_instance_custom) {
			p_shader->canvas_item.batch_flags |= RasterizerStorageCommon::PREVENT_ITEM_JOINING;
		}
	}

	p_shader->shader->set_custom_shader(p_shader->custom_code_id);
	p_shader->shader->bind();

	// cache uniform locations

	for (SelfList<Material> *E = p_shader->materials.first(); E; E = E->next()) {
		_material_make_dirty(E->self());
	}

	p_shader->valid = true;
	p_shader->version++;
}

void RasterizerStorageGLES2::update_dirty_shaders() {
	while (_shader_dirty_list.first()) {
		_update_shader(_shader_dirty_list.first()->self());
	}
}

void RasterizerStorageGLES2::shader_get_param_list(RID p_shader, List<PropertyInfo> *p_param_list) const {

	Shader *shader = shader_owner.getornull(p_shader);
	ERR_FAIL_COND(!shader);

	if (shader->dirty_list.in_list()) {
		_update_shader(shader);
	}

	Map<int, StringName> order;

	for (Map<StringName, ShaderLanguage::ShaderNode::Uniform>::Element *E = shader->uniforms.front(); E; E = E->next()) {

		if (E->get().texture_order >= 0) {
			order[E->get().texture_order + 100000] = E->key();
		} else {
			order[E->get().order] = E->key();
		}
	}

	for (Map<int, StringName>::Element *E = order.front(); E; E = E->next()) {

		PropertyInfo pi;
		ShaderLanguage::ShaderNode::Uniform &u = shader->uniforms[E->get()];

		pi.name = E->get();

		switch (u.type) {
			case ShaderLanguage::TYPE_VOID: {
				pi.type = Variant::NIL;
			} break;

			case ShaderLanguage::TYPE_BOOL: {
				pi.type = Variant::BOOL;
			} break;

			// bool vectors
			case ShaderLanguage::TYPE_BVEC2: {
				pi.type = Variant::INT;
				pi.hint = PROPERTY_HINT_FLAGS;
				pi.hint_string = "x,y";
			} break;
			case ShaderLanguage::TYPE_BVEC3: {
				pi.type = Variant::INT;
				pi.hint = PROPERTY_HINT_FLAGS;
				pi.hint_string = "x,y,z";
			} break;
			case ShaderLanguage::TYPE_BVEC4: {
				pi.type = Variant::INT;
				pi.hint = PROPERTY_HINT_FLAGS;
				pi.hint_string = "x,y,z,w";
			} break;

				// int stuff
			case ShaderLanguage::TYPE_UINT:
			case ShaderLanguage::TYPE_INT: {
				pi.type = Variant::INT;

				if (u.hint == ShaderLanguage::ShaderNode::Uniform::HINT_RANGE) {
					pi.hint = PROPERTY_HINT_RANGE;
					pi.hint_string = rtos(u.hint_range[0]) + "," + rtos(u.hint_range[1]) + "," + rtos(u.hint_range[2]);
				}
			} break;

			case ShaderLanguage::TYPE_IVEC2:
			case ShaderLanguage::TYPE_UVEC2:
			case ShaderLanguage::TYPE_IVEC3:
			case ShaderLanguage::TYPE_UVEC3:
			case ShaderLanguage::TYPE_IVEC4:
			case ShaderLanguage::TYPE_UVEC4: {
				// not sure what this should be in godot 4
//				pi.type = Variant::POOL_INT_ARRAY;
				pi.type = Variant::PACKED_INT32_ARRAY;
			} break;

			case ShaderLanguage::TYPE_FLOAT: {
				pi.type = Variant::FLOAT;
				if (u.hint == ShaderLanguage::ShaderNode::Uniform::HINT_RANGE) {
					pi.hint = PROPERTY_HINT_RANGE;
					pi.hint_string = rtos(u.hint_range[0]) + "," + rtos(u.hint_range[1]) + "," + rtos(u.hint_range[2]);
				}
			} break;

			case ShaderLanguage::TYPE_VEC2: {
				pi.type = Variant::VECTOR2;
			} break;
			case ShaderLanguage::TYPE_VEC3: {
				pi.type = Variant::VECTOR3;
			} break;

			case ShaderLanguage::TYPE_VEC4: {
				if (u.hint == ShaderLanguage::ShaderNode::Uniform::HINT_COLOR) {
					pi.type = Variant::COLOR;
				} else {
					pi.type = Variant::PLANE;
				}
			} break;

			case ShaderLanguage::TYPE_MAT2: {
				pi.type = Variant::TRANSFORM2D;
			} break;

			case ShaderLanguage::TYPE_MAT3: {
				pi.type = Variant::BASIS;
			} break;

			case ShaderLanguage::TYPE_MAT4: {
				pi.type = Variant::TRANSFORM;
			} break;

			case ShaderLanguage::TYPE_SAMPLER2D:
//			case ShaderLanguage::TYPE_SAMPLEREXT:
			case ShaderLanguage::TYPE_ISAMPLER2D:
			case ShaderLanguage::TYPE_USAMPLER2D: {
				pi.type = Variant::OBJECT;
				pi.hint = PROPERTY_HINT_RESOURCE_TYPE;
				pi.hint_string = "Texture";
			} break;

			case ShaderLanguage::TYPE_SAMPLERCUBE: {
				pi.type = Variant::OBJECT;
				pi.hint = PROPERTY_HINT_RESOURCE_TYPE;
				pi.hint_string = "CubeMap";
			} break;

			case ShaderLanguage::TYPE_SAMPLER2DARRAY:
			case ShaderLanguage::TYPE_ISAMPLER2DARRAY:
			case ShaderLanguage::TYPE_USAMPLER2DARRAY:
			case ShaderLanguage::TYPE_SAMPLER3D:
			case ShaderLanguage::TYPE_ISAMPLER3D:
			case ShaderLanguage::TYPE_USAMPLER3D: {
				// Not implemented in GLES2
			} break;
				// new for godot 4
			case ShaderLanguage::TYPE_SAMPLERCUBEARRAY:
			case ShaderLanguage::TYPE_STRUCT:
			case ShaderLanguage::TYPE_MAX:
			{
				
			} break;
				
		}

		p_param_list->push_back(pi);
	}
}

void RasterizerStorageGLES2::shader_set_default_texture_param(RID p_shader, const StringName &p_name, RID p_texture) {

	Shader *shader = shader_owner.getornull(p_shader);
	ERR_FAIL_COND(!shader);
	ERR_FAIL_COND(p_texture.is_valid() && !texture_owner.owns(p_texture));

	if (p_texture.is_valid()) {
		shader->default_textures[p_name] = p_texture;
	} else {
		shader->default_textures.erase(p_name);
	}

	_shader_make_dirty(shader);
}

RID RasterizerStorageGLES2::shader_get_default_texture_param(RID p_shader, const StringName &p_name) const {

	const Shader *shader = shader_owner.getornull(p_shader);
	ERR_FAIL_COND_V(!shader, RID());

	const Map<StringName, RID>::Element *E = shader->default_textures.find(p_name);

	if (!E) {
		return RID();
	}

	return E->get();
}

void RasterizerStorageGLES2::shader_add_custom_define(RID p_shader, const String &p_define) {

	Shader *shader = shader_owner.getornull(p_shader);
	ERR_FAIL_COND(!shader);

	shader->shader->add_custom_define(p_define);

	_shader_make_dirty(shader);
}

void RasterizerStorageGLES2::shader_get_custom_defines(RID p_shader, Vector<String> *p_defines) const {

	Shader *shader = shader_owner.getornull(p_shader);
	ERR_FAIL_COND(!shader);

	shader->shader->get_custom_defines(p_defines);
}

void RasterizerStorageGLES2::shader_remove_custom_define(RID p_shader, const String &p_define) {

	Shader *shader = shader_owner.getornull(p_shader);
	ERR_FAIL_COND(!shader);

	shader->shader->remove_custom_define(p_define);

	_shader_make_dirty(shader);
}

/* COMMON MATERIAL API */

void RasterizerStorageGLES2::_material_make_dirty(Material *p_material) const {

	if (p_material->dirty_list.in_list())
		return;

	_material_dirty_list.add(&p_material->dirty_list);
}

RID RasterizerStorageGLES2::material_create() {

	Material *material = memnew(Material);

	return material_owner.make_rid(material);
}

void RasterizerStorageGLES2::material_set_shader(RID p_material, RID p_shader) {

	Material *material = material_owner.getornull(p_material);
	ERR_FAIL_COND(!material);

	Shader *shader = shader_owner.getornull(p_shader);

	if (material->shader) {
		// if a shader is present, remove the old shader
		material->shader->materials.remove(&material->list);
	}

	material->shader = shader;

	if (shader) {
		shader->materials.add(&material->list);
	}

	_material_make_dirty(material);
}

RID RasterizerStorageGLES2::material_get_shader(RID p_material) const {

	const Material *material = material_owner.getornull(p_material);
	ERR_FAIL_COND_V(!material, RID());

	if (material->shader) {
		return material->shader->self;
	}

	return RID();
}

void RasterizerStorageGLES2::material_set_param(RID p_material, const StringName &p_param, const Variant &p_value) {

	Material *material = material_owner.getornull(p_material);
	ERR_FAIL_COND(!material);

	if (p_value.get_type() == Variant::NIL) {
		material->params.erase(p_param);
	} else {
		material->params[p_param] = p_value;
	}

	_material_make_dirty(material);
}

Variant RasterizerStorageGLES2::material_get_param(RID p_material, const StringName &p_param) const {

	const Material *material = material_owner.getornull(p_material);
	ERR_FAIL_COND_V(!material, RID());

	if (material->params.has(p_param)) {
		return material->params[p_param];
	}

	return material_get_param_default(p_material, p_param);
}

Variant RasterizerStorageGLES2::material_get_param_default(RID p_material, const StringName &p_param) const {
	const Material *material = material_owner.getornull(p_material);
	ERR_FAIL_COND_V(!material, Variant());

	if (material->shader) {
		if (material->shader->uniforms.has(p_param)) {
			ShaderLanguage::ShaderNode::Uniform uniform = material->shader->uniforms[p_param];
			Vector<ShaderLanguage::ConstantNode::Value> default_value = uniform.default_value;
			return ShaderLanguage::constant_value_to_variant(default_value, uniform.type, uniform.hint);
		}
	}
	return Variant();
}

void RasterizerStorageGLES2::material_set_line_width(RID p_material, float p_width) {
	Material *material = material_owner.getornull(p_material);
	ERR_FAIL_COND(!material);

	material->line_width = p_width;
}

void RasterizerStorageGLES2::material_set_next_pass(RID p_material, RID p_next_material) {
	Material *material = material_owner.getornull(p_material);
	ERR_FAIL_COND(!material);

	material->next_pass = p_next_material;
}

bool RasterizerStorageGLES2::material_is_animated(RID p_material) {
	Material *material = material_owner.getornull(p_material);
	ERR_FAIL_COND_V(!material, false);
	if (material->dirty_list.in_list()) {
		_update_material(material);
	}

	bool animated = material->is_animated_cache;
	if (!animated && material->next_pass.is_valid()) {
		animated = material_is_animated(material->next_pass);
	}
	return animated;
}

bool RasterizerStorageGLES2::material_casts_shadows(RID p_material) {
	Material *material = material_owner.getornull(p_material);
	ERR_FAIL_COND_V(!material, false);
	if (material->dirty_list.in_list()) {
		_update_material(material);
	}

	bool casts_shadows = material->can_cast_shadow_cache;

	if (!casts_shadows && material->next_pass.is_valid()) {
		casts_shadows = material_casts_shadows(material->next_pass);
	}

	return casts_shadows;
}

bool RasterizerStorageGLES2::material_uses_tangents(RID p_material) {
	Material *material = material_owner.getornull(p_material);
	ERR_FAIL_COND_V(!material, false);

	if (!material->shader) {
		return false;
	}

	if (material->shader->dirty_list.in_list()) {
		_update_shader(material->shader);
	}

	return material->shader->spatial.uses_tangent;
}

bool RasterizerStorageGLES2::material_uses_ensure_correct_normals(RID p_material) {
	Material *material = material_owner.getornull(p_material);
	ERR_FAIL_COND_V(!material, false);

	if (!material->shader) {
		return false;
	}

	if (material->shader->dirty_list.in_list()) {
		_update_shader(material->shader);
	}

	return material->shader->spatial.uses_ensure_correct_normals;
}

void RasterizerStorageGLES2::material_add_instance_owner(RID p_material, RasterizerScene::InstanceBase *p_instance) {
/*
	Material *material = material_owner.getornull(p_material);
	ERR_FAIL_COND(!material);

	Map<RasterizerScene::InstanceBase *, int>::Element *E = material->instance_owners.find(p_instance);
	if (E) {
		E->get()++;
	} else {
		material->instance_owners[p_instance] = 1;
	}
*/
}

void RasterizerStorageGLES2::material_remove_instance_owner(RID p_material, RasterizerScene::InstanceBase *p_instance) {
/*
	Material *material = material_owner.getornull(p_material);
	ERR_FAIL_COND(!material);

	Map<RasterizerScene::InstanceBase *, int>::Element *E = material->instance_owners.find(p_instance);
	ERR_FAIL_COND(!E);

	E->get()--;

	if (E->get() == 0) {
		material->instance_owners.erase(E);
	}
*/
}

void RasterizerStorageGLES2::material_set_render_priority(RID p_material, int priority) {
	ERR_FAIL_COND(priority < GD_VS::MATERIAL_RENDER_PRIORITY_MIN);
	ERR_FAIL_COND(priority > GD_VS::MATERIAL_RENDER_PRIORITY_MAX);

	Material *material = material_owner.getornull(p_material);
	ERR_FAIL_COND(!material);

	material->render_priority = priority;
}

void RasterizerStorageGLES2::_update_material(Material *p_material) {
	if (p_material->dirty_list.in_list()) {
		_material_dirty_list.remove(&p_material->dirty_list);
	}

	if (p_material->shader && p_material->shader->dirty_list.in_list()) {
		_update_shader(p_material->shader);
	}

	if (p_material->shader && !p_material->shader->valid) {
		return;
	}

	{
		bool can_cast_shadow = false;
		bool is_animated = false;

		if (p_material->shader && p_material->shader->mode == GD_VS::SHADER_SPATIAL) {

			if (p_material->shader->spatial.blend_mode == Shader::Spatial::BLEND_MODE_MIX &&
					(!p_material->shader->spatial.uses_alpha || p_material->shader->spatial.depth_draw_mode == Shader::Spatial::DEPTH_DRAW_ALPHA_PREPASS)) {
				can_cast_shadow = true;
			}

			if (p_material->shader->spatial.uses_discard && p_material->shader->uses_fragment_time) {
				is_animated = true;
			}

			if (p_material->shader->spatial.uses_vertex && p_material->shader->uses_vertex_time) {
				is_animated = true;
			}

			if (can_cast_shadow != p_material->can_cast_shadow_cache || is_animated != p_material->is_animated_cache) {
				p_material->can_cast_shadow_cache = can_cast_shadow;
				p_material->is_animated_cache = is_animated;
				
				/*
				for (Map<Geometry *, int>::Element *E = p_material->geometry_owners.front(); E; E = E->next()) {
					E->key()->material_changed_notify();
				}

				for (Map<RasterizerScene::InstanceBase *, int>::Element *E = p_material->instance_owners.front(); E; E = E->next()) {
					E->key()->base_changed(false, true);
				}
				*/
			}
		}
	}

	// uniforms and other things will be set in the use_material method in ShaderGLES2

	if (p_material->shader && p_material->shader->texture_count > 0) {

		p_material->textures.resize(p_material->shader->texture_count);

		for (Map<StringName, ShaderLanguage::ShaderNode::Uniform>::Element *E = p_material->shader->uniforms.front(); E; E = E->next()) {
			if (E->get().texture_order < 0)
				continue; // not a texture, does not go here

			RID texture;

			Map<StringName, Variant>::Element *V = p_material->params.find(E->key());

			if (V) {
				texture = V->get();
			}

			if (!texture.is_valid()) {
				Map<StringName, RID>::Element *W = p_material->shader->default_textures.find(E->key());

				if (W) {
					texture = W->get();
				}
			}

			p_material->textures.write[E->get().texture_order] = Pair<StringName, RID>(E->key(), texture);
		}
	} else {
		p_material->textures.clear();
	}
}
/*
void RasterizerStorageGLES2::_material_add_geometry(RID p_material, Geometry *p_geometry) {
	Material *material = material_owner.getornull(p_material);
	ERR_FAIL_COND(!material);

	Map<Geometry *, int>::Element *I = material->geometry_owners.find(p_geometry);

	if (I) {
		I->get()++;
	} else {
		material->geometry_owners[p_geometry] = 1;
	}
}

void RasterizerStorageGLES2::_material_remove_geometry(RID p_material, Geometry *p_geometry) {

	Material *material = material_owner.getornull(p_material);
	ERR_FAIL_COND(!material);

	Map<Geometry *, int>::Element *I = material->geometry_owners.find(p_geometry);
	ERR_FAIL_COND(!I);

	I->get()--;

	if (I->get() == 0) {
		material->geometry_owners.erase(I);
	}
}
*/
void RasterizerStorageGLES2::update_dirty_materials() {
	while (_material_dirty_list.first()) {

		Material *material = _material_dirty_list.first()->self();
		_update_material(material);
	}
}









/* RENDER TARGET */

void RasterizerStorageGLES2::_set_current_render_target(RID p_render_target)
{
#ifdef GLES2_DISABLE_RENDER_TARGETS
	return;
#endif

	
	RenderTarget * rt = render_target_owner.getornull(p_render_target);
	
	// FTODO	
//	if (!p_render_target.is_valid() && storage->frame.current_rt && storage->frame.clear_request) {
	//		// pending clear request. Do that first.
	//		glBindFramebuffer(GL_FRAMEBUFFER, storage->frame.current_rt->fbo);
	//		glClearColor(storage->frame.clear_request_color.r,
	//				storage->frame.clear_request_color.g,
	//				storage->frame.clear_request_color.b,
	//				storage->frame.clear_request_color.a);
	//		glClear(GL_COLOR_BUFFER_BIT);
	//	}
	
	if (rt) {
		//	if (p_render_target.is_valid()) {
		//		RasterizerStorageGLES2::RenderTarget *rt = storage.render_target_owner.getornull(p_render_target);
		frame.current_rt = rt;
		ERR_FAIL_COND(!rt);
		frame.clear_request = false;
		
		glViewport(0, 0, rt->width, rt->height);
		
//		print_line("_set_current_render_target w " + itos(rt->width) + " h " + itos(rt->height));
		
		_dims.rt_width = rt->width;
		_dims.rt_height = rt->height;
		_dims.win_width = rt->width;
		_dims.win_height = rt->height;
		
	} else {
		frame.current_rt = NULL;
		frame.clear_request = false;
		// FTODO
		//		glViewport(0, 0, OS::get_singleton()->get_window_size().width, OS::get_singleton()->get_window_size().height);
		bind_framebuffer_system();
	}	
}


void RasterizerStorageGLES2::_render_target_allocate(RenderTarget *rt) {
	
#ifdef GLES2_DISABLE_RENDER_TARGETS
	return;
#endif
	
	
	// do not allocate a render target with no size
	if (rt->width <= 0 || rt->height <= 0)
		return;

	// do not allocate a render target that is attached to the screen
	if (rt->flags[RENDER_TARGET_DIRECT_TO_SCREEN]) {
		rt->fbo = RasterizerStorageGLES2::system_fbo;
		return;
	}

	GLuint color_internal_format;
	GLuint color_format;
	GLuint color_type = GL_UNSIGNED_BYTE;
	Image::Format image_format;

	if (rt->flags[RasterizerStorage::RENDER_TARGET_TRANSPARENT]) {
#ifdef GLES_OVER_GL
		color_internal_format = GL_RGBA8;
#else
		color_internal_format = GL_RGBA;
#endif
		color_format = GL_RGBA;
		image_format = Image::FORMAT_RGBA8;
	} else {
#ifdef GLES_OVER_GL
		color_internal_format = GL_RGB8;
#else
		color_internal_format = GL_RGB;
#endif
		color_format = GL_RGB;
		image_format = Image::FORMAT_RGB8;
	}

	rt->used_dof_blur_near = false;
	rt->mip_maps_allocated = false;

	{

		/* Front FBO */

		Texture *texture = texture_owner.getornull(rt->texture);
		ERR_FAIL_COND(!texture);

		// framebuffer
		glGenFramebuffers(1, &rt->fbo);
		bind_framebuffer(rt->fbo);

		// color
		glGenTextures(1, &rt->color);
		glBindTexture(GL_TEXTURE_2D, rt->color);

		glTexImage2D(GL_TEXTURE_2D, 0, color_internal_format, rt->width, rt->height, 0, color_format, color_type, NULL);

		if (texture->flags & TEXTURE_FLAG_FILTER) {

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		} else {

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		}

		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, rt->color, 0);

		// depth

		if (config.support_depth_texture) {

			glGenTextures(1, &rt->depth);
			glBindTexture(GL_TEXTURE_2D, rt->depth);
			glTexImage2D(GL_TEXTURE_2D, 0, config.depth_internalformat, rt->width, rt->height, 0, GL_DEPTH_COMPONENT, config.depth_type, NULL);

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, rt->depth, 0);
		} else {

			glGenRenderbuffers(1, &rt->depth);
			glBindRenderbuffer(GL_RENDERBUFFER, rt->depth);

			glRenderbufferStorage(GL_RENDERBUFFER, config.depth_buffer_internalformat, rt->width, rt->height);

			glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rt->depth);
		}

		GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);

		if (status != GL_FRAMEBUFFER_COMPLETE) {

			glDeleteFramebuffers(1, &rt->fbo);
			if (config.support_depth_texture) {

				glDeleteTextures(1, &rt->depth);
			} else {

				glDeleteRenderbuffers(1, &rt->depth);
			}

			glDeleteTextures(1, &rt->color);
			rt->fbo = 0;
			rt->width = 0;
			rt->height = 0;
			rt->color = 0;
			rt->depth = 0;
			texture->tex_id = 0;
			texture->active = false;
			WARN_PRINT("Could not create framebuffer!!");
			return;
		}

		texture->format = image_format;
		texture->gl_format_cache = color_format;
		texture->gl_type_cache = GL_UNSIGNED_BYTE;
		texture->gl_internal_format_cache = color_internal_format;
		texture->tex_id = rt->color;
		texture->width = rt->width;
		texture->alloc_width = rt->width;
		texture->height = rt->height;
		texture->alloc_height = rt->height;
		texture->active = true;

		texture_set_flags(rt->texture, texture->flags);
	}

	/* BACK FBO */
	/* For MSAA */

#ifndef JAVASCRIPT_ENABLED
	if (rt->msaa >= GD_VS::VIEWPORT_MSAA_2X && rt->msaa <= GD_VS::VIEWPORT_MSAA_16X && config.multisample_supported) {

		rt->multisample_active = true;

		static const int msaa_value[] = { 0, 2, 4, 8, 16 };
		int msaa = msaa_value[rt->msaa];

		int max_samples = 0;
		glGetIntegerv(GL_MAX_SAMPLES, &max_samples);
		if (msaa > max_samples) {
			WARN_PRINT("MSAA must be <= GL_MAX_SAMPLES, falling-back to GL_MAX_SAMPLES = " + itos(max_samples));
			msaa = max_samples;
		}

		//regular fbo
		glGenFramebuffers(1, &rt->multisample_fbo);
		bind_framebuffer(rt->multisample_fbo);

		glGenRenderbuffers(1, &rt->multisample_depth);
		glBindRenderbuffer(GL_RENDERBUFFER, rt->multisample_depth);
		glRenderbufferStorageMultisample(GL_RENDERBUFFER, msaa, config.depth_buffer_internalformat, rt->width, rt->height);

		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rt->multisample_depth);

#if defined(GLES_OVER_GL) || defined(IPHONE_ENABLED)

		glGenRenderbuffers(1, &rt->multisample_color);
		glBindRenderbuffer(GL_RENDERBUFFER, rt->multisample_color);
		glRenderbufferStorageMultisample(GL_RENDERBUFFER, msaa, color_internal_format, rt->width, rt->height);

		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, rt->multisample_color);
#elif ANDROID_ENABLED
		// Render to a texture in android
		glGenTextures(1, &rt->multisample_color);
		glBindTexture(GL_TEXTURE_2D, rt->multisample_color);

		glTexImage2D(GL_TEXTURE_2D, 0, color_internal_format, rt->width, rt->height, 0, color_format, color_type, NULL);

		// multisample buffer is same size as front buffer, so just use nearest
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

		glFramebufferTexture2DMultisample(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, rt->multisample_color, 0, msaa);
#endif

		GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);

		if (status != GL_FRAMEBUFFER_COMPLETE) {
			// Delete allocated resources and default to no MSAA
			WARN_PRINT_ONCE("Cannot allocate back framebuffer for MSAA");
			printf("err status: %x\n", status);
			config.multisample_supported = false;
			rt->multisample_active = false;

			glDeleteFramebuffers(1, &rt->multisample_fbo);
			rt->multisample_fbo = 0;

			glDeleteRenderbuffers(1, &rt->multisample_depth);
			rt->multisample_depth = 0;
#ifdef ANDROID_ENABLED
			glDeleteTextures(1, &rt->multisample_color);
#else
			glDeleteRenderbuffers(1, &rt->multisample_color);
#endif
			rt->multisample_color = 0;
		}

		glBindRenderbuffer(GL_RENDERBUFFER, 0);
		bind_framebuffer(0);
#ifdef ANDROID_ENABLED
		glBindTexture(GL_TEXTURE_2D, 0);
#endif

	} else
#endif // JAVASCRIPT_ENABLED
	{
		rt->multisample_active = false;
	}

	glClearColor(0, 0, 0, 0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// copy texscreen buffers
//	if (!(rt->flags[RasterizerStorage::RENDER_TARGET_NO_SAMPLING])) {
	if (true) {

		glGenTextures(1, &rt->copy_screen_effect.color);
		glBindTexture(GL_TEXTURE_2D, rt->copy_screen_effect.color);

		if (rt->flags[RasterizerStorage::RENDER_TARGET_TRANSPARENT]) {
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, rt->width, rt->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
		} else {
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, rt->width, rt->height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
		}

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		glGenFramebuffers(1, &rt->copy_screen_effect.fbo);
		bind_framebuffer(rt->copy_screen_effect.fbo);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, rt->copy_screen_effect.color, 0);

		glClearColor(0, 0, 0, 0);
		glClear(GL_COLOR_BUFFER_BIT);

		GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
		if (status != GL_FRAMEBUFFER_COMPLETE) {
			_render_target_clear(rt);
			ERR_FAIL_COND(status != GL_FRAMEBUFFER_COMPLETE);
		}
	}

	// Allocate mipmap chains for post_process effects
//	if (!rt->flags[RasterizerStorage::RENDER_TARGET_NO_3D] && rt->width >= 2 && rt->height >= 2) {
	if (rt->width >= 2 && rt->height >= 2) {

		for (int i = 0; i < 2; i++) {

			ERR_FAIL_COND(rt->mip_maps[i].sizes.size());
			int w = rt->width;
			int h = rt->height;

			if (i > 0) {
				w >>= 1;
				h >>= 1;
			}

			int level = 0;
			int fb_w = w;
			int fb_h = h;

			while (true) {

				RenderTarget::MipMaps::Size mm;
				mm.width = w;
				mm.height = h;
				rt->mip_maps[i].sizes.push_back(mm);

				w >>= 1;
				h >>= 1;

				if (w < 2 || h < 2)
					break;

				level++;
			}

			GLsizei width = fb_w;
			GLsizei height = fb_h;

			if (config.render_to_mipmap_supported) {

				glGenTextures(1, &rt->mip_maps[i].color);
				glBindTexture(GL_TEXTURE_2D, rt->mip_maps[i].color);

				for (int l = 0; l < level + 1; l++) {
					glTexImage2D(GL_TEXTURE_2D, l, color_internal_format, width, height, 0, color_format, color_type, NULL);
					width = MAX(1, (width / 2));
					height = MAX(1, (height / 2));
				}
#ifdef GLES_OVER_GL
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, level);
#endif
			} else {

				// Can't render to specific levels of a mipmap in ES 2.0 or Webgl so create a texture for each level
				for (int l = 0; l < level + 1; l++) {
					glGenTextures(1, &rt->mip_maps[i].sizes.write[l].color);
					glBindTexture(GL_TEXTURE_2D, rt->mip_maps[i].sizes[l].color);
					glTexImage2D(GL_TEXTURE_2D, 0, color_internal_format, width, height, 0, color_format, color_type, NULL);
					width = MAX(1, (width / 2));
					height = MAX(1, (height / 2));
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
					glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
					glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
				}
			}

			glDisable(GL_SCISSOR_TEST);
			glColorMask(1, 1, 1, 1);
			glDepthMask(GL_TRUE);

			for (int j = 0; j < rt->mip_maps[i].sizes.size(); j++) {

				RenderTarget::MipMaps::Size &mm = rt->mip_maps[i].sizes.write[j];

				glGenFramebuffers(1, &mm.fbo);
				bind_framebuffer(mm.fbo);

				if (config.render_to_mipmap_supported) {

					glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, rt->mip_maps[i].color, j);
				} else {

					glBindTexture(GL_TEXTURE_2D, rt->mip_maps[i].sizes[j].color);
					glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, rt->mip_maps[i].sizes[j].color, 0);
				}

				bool used_depth = false;
				if (j == 0 && i == 0) { //use always
					if (config.support_depth_texture) {
						glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, rt->depth, 0);
					} else {
						glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rt->depth);
					}
					used_depth = true;
				}

				GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
				if (status != GL_FRAMEBUFFER_COMPLETE) {
					WARN_PRINT_ONCE("Cannot allocate mipmaps for 3D post processing effects");
					bind_framebuffer_system();
					return;
				}

				glClearColor(1.0, 0.0, 1.0, 0.0);
				glClear(GL_COLOR_BUFFER_BIT);
				if (used_depth) {
					glClearDepth(1.0);
					glClear(GL_DEPTH_BUFFER_BIT);
				}
			}

			rt->mip_maps[i].levels = level;

			if (config.render_to_mipmap_supported) {
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
				glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
				glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			}
		}
		rt->mip_maps_allocated = true;
	}
	
	bind_framebuffer_system();
}

void RasterizerStorageGLES2::_render_target_clear(RenderTarget *rt) {
	
#ifdef GLES2_DISABLE_RENDER_TARGETS
	return;
#endif
	
	// there is nothing to clear when DIRECT_TO_SCREEN is used
	if (rt->flags[RENDER_TARGET_DIRECT_TO_SCREEN])
		return;

	if (rt->fbo) {
		glDeleteFramebuffers(1, &rt->fbo);
		glDeleteTextures(1, &rt->color);
		rt->fbo = 0;
	}

	if (rt->external.fbo != 0) {
		// free this
		glDeleteFramebuffers(1, &rt->external.fbo);

		// clean up our texture
		Texture *t = texture_owner.getornull(rt->external.texture);
		t->alloc_height = 0;
		t->alloc_width = 0;
		t->width = 0;
		t->height = 0;
		t->active = false;
		texture_owner.free(rt->external.texture);
		memdelete(t);

		rt->external.fbo = 0;
	}

	if (rt->depth) {
		if (config.support_depth_texture) {
			glDeleteTextures(1, &rt->depth);
		} else {
			glDeleteRenderbuffers(1, &rt->depth);
		}

		rt->depth = 0;
	}

	Texture *tex = texture_owner.getornull(rt->texture);
	tex->alloc_height = 0;
	tex->alloc_width = 0;
	tex->width = 0;
	tex->height = 0;
	tex->active = false;

	if (rt->copy_screen_effect.color) {
		glDeleteFramebuffers(1, &rt->copy_screen_effect.fbo);
		rt->copy_screen_effect.fbo = 0;

		glDeleteTextures(1, &rt->copy_screen_effect.color);
		rt->copy_screen_effect.color = 0;
	}

	for (int i = 0; i < 2; i++) {
		if (rt->mip_maps[i].sizes.size()) {
			for (int j = 0; j < rt->mip_maps[i].sizes.size(); j++) {
				glDeleteFramebuffers(1, &rt->mip_maps[i].sizes[j].fbo);
				glDeleteTextures(1, &rt->mip_maps[i].sizes[j].color);
			}

			glDeleteTextures(1, &rt->mip_maps[i].color);
			rt->mip_maps[i].sizes.clear();
			rt->mip_maps[i].levels = 0;
			rt->mip_maps[i].color = 0;
		}
	}

	if (rt->multisample_active) {
		glDeleteFramebuffers(1, &rt->multisample_fbo);
		rt->multisample_fbo = 0;

		glDeleteRenderbuffers(1, &rt->multisample_depth);
		rt->multisample_depth = 0;
#ifdef ANDROID_ENABLED
		glDeleteTextures(1, &rt->multisample_color);
#else
		glDeleteRenderbuffers(1, &rt->multisample_color);
#endif
		rt->multisample_color = 0;
	}
}

RID RasterizerStorageGLES2::render_target_create() {
	
#ifdef GLES2_DISABLE_RENDER_TARGETS
//	return RID();
#endif
	
	RenderTarget *rt = memnew(RenderTarget);
	Texture *t = memnew(Texture);

	t->type = GD_RD::TEXTURE_TYPE_2D;
	t->flags = 0;
	t->width = 0;
	t->height = 0;
	t->alloc_height = 0;
	t->alloc_width = 0;
	t->format = Image::FORMAT_R8;
	t->target = GL_TEXTURE_2D;
	t->gl_format_cache = 0;
	t->gl_internal_format_cache = 0;
	t->gl_type_cache = 0;
	t->data_size = 0;
	t->total_data_size = 0;
	t->ignore_mipmaps = false;
	t->compressed = false;
	t->mipmaps = 1;
	t->active = true;
	t->tex_id = 0;
	t->render_target = rt;

	rt->texture = texture_owner.make_rid(t);
	return render_target_owner.make_rid(rt);
}

void RasterizerStorageGLES2::render_target_set_position(RID p_render_target, int p_x, int p_y) {
	
#ifdef GLES2_DISABLE_RENDER_TARGETS
	return;
#endif
	
	RenderTarget *rt = render_target_owner.getornull(p_render_target);
	ERR_FAIL_COND(!rt);

	rt->x = p_x;
	rt->y = p_y;
}

void RasterizerStorageGLES2::render_target_set_size(RID p_render_target, int p_width, int p_height) {
	
#ifdef GLES2_DISABLE_RENDER_TARGETS
	return;
#endif
	
	RenderTarget *rt = render_target_owner.getornull(p_render_target);
	ERR_FAIL_COND(!rt);

	if (p_width == rt->width && p_height == rt->height)
		return;

	_render_target_clear(rt);

	rt->width = p_width;
	rt->height = p_height;
	
	print_line("render_target_set_size " + itos(p_render_target.get_id()) + ", w " + itos(p_width) + " h " + itos(p_height) );	
	
	_render_target_allocate(rt);
}

RID RasterizerStorageGLES2::render_target_get_texture(RID p_render_target) {
	
#ifdef GLES2_DISABLE_RENDER_TARGETS
	return RID();
#endif
	
	RenderTarget *rt = render_target_owner.getornull(p_render_target);
	ERR_FAIL_COND_V(!rt, RID());

	if (rt->external.fbo == 0) {
		return rt->texture;
	} else {
		return rt->external.texture;
	}
}

void RasterizerStorageGLES2::render_target_set_external_texture(RID p_render_target, unsigned int p_texture_id) {
	
#ifdef GLES2_DISABLE_RENDER_TARGETS
	return;
#endif
	
	RenderTarget *rt = render_target_owner.getornull(p_render_target);
	ERR_FAIL_COND(!rt);

	if (p_texture_id == 0) {
		if (rt->external.fbo != 0) {
			// free this
			glDeleteFramebuffers(1, &rt->external.fbo);

			// and this
			if (rt->external.depth != 0) {
				glDeleteRenderbuffers(1, &rt->external.depth);
			}

			// clean up our texture
			Texture *t = texture_owner.getornull(rt->external.texture);
			t->alloc_height = 0;
			t->alloc_width = 0;
			t->width = 0;
			t->height = 0;
			t->active = false;
			texture_owner.free(rt->external.texture);
			memdelete(t);

			rt->external.fbo = 0;
			rt->external.color = 0;
			rt->external.depth = 0;
		}
	} else {
		Texture *t;

		if (rt->external.fbo == 0) {
			// create our fbo
			glGenFramebuffers(1, &rt->external.fbo);
			bind_framebuffer(rt->external.fbo);

			// allocate a texture
			t = memnew(Texture);

			t->type = GD_RD::TEXTURE_TYPE_2D;
			t->flags = 0;
			t->width = 0;
			t->height = 0;
			t->alloc_height = 0;
			t->alloc_width = 0;
			t->format = Image::FORMAT_RGBA8;
			t->target = GL_TEXTURE_2D;
			t->gl_format_cache = 0;
			t->gl_internal_format_cache = 0;
			t->gl_type_cache = 0;
			t->data_size = 0;
			t->compressed = false;
			t->srgb = false;
			t->total_data_size = 0;
			t->ignore_mipmaps = false;
			t->mipmaps = 1;
			t->active = true;
			t->tex_id = 0;
			t->render_target = rt;

			rt->external.texture = texture_owner.make_rid(t);

		} else {
			// bind our frame buffer
			bind_framebuffer(rt->external.fbo);

			// find our texture
			t = texture_owner.getornull(rt->external.texture);
		}

		// set our texture
		t->tex_id = p_texture_id;
		rt->external.color = p_texture_id;

		// size shouldn't be different
		t->width = rt->width;
		t->height = rt->height;
		t->alloc_height = rt->width;
		t->alloc_width = rt->height;

		// Switch our texture on our frame buffer
#if ANDROID_ENABLED
		if (rt->msaa >= GD_VS::VIEWPORT_MSAA_EXT_2X && rt->msaa <= GD_VS::VIEWPORT_MSAA_EXT_4X) {
			// This code only applies to the Oculus Go and Oculus Quest. Due to the the tiled nature
			// of the GPU we can do a single render pass by rendering directly into our texture chains
			// texture and apply MSAA as we render.

			// On any other hardware these two modes are ignored and we do not have any MSAA,
			// the normal MSAA modes need to be used to enable our two pass approach

			static const int msaa_value[] = { 2, 4 };
			int msaa = msaa_value[rt->msaa - GD_VS::VIEWPORT_MSAA_EXT_2X];

			if (rt->external.depth == 0) {
				// create a multisample depth buffer, we're not reusing Godots because Godot's didn't get created..
				glGenRenderbuffers(1, &rt->external.depth);
				glBindRenderbuffer(GL_RENDERBUFFER, rt->external.depth);
				glRenderbufferStorageMultisample(GL_RENDERBUFFER, msaa, config.depth_buffer_internalformat, rt->width, rt->height);
				glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rt->external.depth);
			}

			// and set our external texture as the texture...
			glFramebufferTexture2DMultisample(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, p_texture_id, 0, msaa);

		} else
#endif
		{
			// set our texture as the destination for our framebuffer
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, p_texture_id, 0);

			// seeing we're rendering into this directly, better also use our depth buffer, just use our existing one :)
			if (config.support_depth_texture) {
				glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, rt->depth, 0);
			} else {
				glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rt->depth);
			}
		}

		// check status and unbind
		GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
		bind_framebuffer_system();

		if (status != GL_FRAMEBUFFER_COMPLETE) {
			printf("framebuffer fail, status: %x\n", status);
		}

		ERR_FAIL_COND(status != GL_FRAMEBUFFER_COMPLETE);
	}
}

void RasterizerStorageGLES2::render_target_set_flag(RID p_render_target, RenderTargetFlags p_flag, bool p_value) {
	
#ifdef GLES2_DISABLE_RENDER_TARGETS
	return;
#endif
	
	RenderTarget *rt = render_target_owner.getornull(p_render_target);
	ERR_FAIL_COND(!rt);

	// When setting DIRECT_TO_SCREEN, you need to clear before the value is set, but allocate after as
	// those functions change how they operate depending on the value of DIRECT_TO_SCREEN
	if (p_flag == RENDER_TARGET_DIRECT_TO_SCREEN && p_value != rt->flags[RENDER_TARGET_DIRECT_TO_SCREEN]) {
		_render_target_clear(rt);
		rt->flags[p_flag] = p_value;
		_render_target_allocate(rt);
	}

	rt->flags[p_flag] = p_value;

	switch (p_flag) {
		case RENDER_TARGET_TRANSPARENT:
			/*
		case RENDER_TARGET_HDR:
		case RENDER_TARGET_NO_3D:
		case RENDER_TARGET_NO_SAMPLING:
		case RENDER_TARGET_NO_3D_EFFECTS: */{
			//must reset for these formats
			_render_target_clear(rt);
			_render_target_allocate(rt);

		} break;
		default: {
		}
	}
}

bool RasterizerStorageGLES2::render_target_was_used(RID p_render_target) {
	
#ifdef GLES2_DISABLE_RENDER_TARGETS
	return false;
#endif
	
	RenderTarget *rt = render_target_owner.getornull(p_render_target);
	ERR_FAIL_COND_V(!rt, false);

	return rt->used_in_frame;
}

void RasterizerStorageGLES2::render_target_clear_used(RID p_render_target) {
	
#ifdef GLES2_DISABLE_RENDER_TARGETS
	return;
#endif
	
	RenderTarget *rt = render_target_owner.getornull(p_render_target);
	ERR_FAIL_COND(!rt);

	rt->used_in_frame = false;
}

void RasterizerStorageGLES2::render_target_set_msaa(RID p_render_target, GD_VS::ViewportMSAA p_msaa) {
	
#ifdef GLES2_DISABLE_RENDER_TARGETS
	return;
#endif
	
	RenderTarget *rt = render_target_owner.getornull(p_render_target);
	ERR_FAIL_COND(!rt);

	if (rt->msaa == p_msaa)
		return;

	if (!config.multisample_supported) {
		ERR_PRINT("MSAA not supported on this hardware.");
		return;
	}

	_render_target_clear(rt);
	rt->msaa = p_msaa;
	_render_target_allocate(rt);
}


//RasterizerStorageGLES2::RenderTarget * RasterizerStorageGLES2::render_target_get(RID p_render_target)
//{
//	return render_target_owner.getornull(p_render_target);
//}


void RasterizerStorageGLES2::render_target_set_use_fxaa(RID p_render_target, bool p_fxaa) {
#ifdef GLES2_DISABLE_RENDER_TARGETS
	return;
#endif
	
	RenderTarget *rt = render_target_owner.getornull(p_render_target);
	ERR_FAIL_COND(!rt);

	rt->use_fxaa = p_fxaa;
}

void RasterizerStorageGLES2::render_target_set_use_debanding(RID p_render_target, bool p_debanding) {
#ifdef GLES2_DISABLE_RENDER_TARGETS
	return;
#endif
	
	RenderTarget *rt = render_target_owner.getornull(p_render_target);
	ERR_FAIL_COND(!rt);

	if (p_debanding) {
		WARN_PRINT_ONCE("Debanding is not supported in the GLES2 backend. Switch to the GLES3 backend and make sure HDR is enabled.");
	}

	rt->use_debanding = p_debanding;
}

void RasterizerStorageGLES2::render_target_request_clear(RID p_render_target, const Color &p_clear_color)
{
#ifdef GLES2_DISABLE_RENDER_TARGETS
	return;
#endif
	
	RenderTarget *rt = render_target_owner.getornull(p_render_target);
	ERR_FAIL_COND(!rt);
	rt->clear_requested = true;
}

bool RasterizerStorageGLES2::render_target_is_clear_requested(RID p_render_target)
{
#ifdef GLES2_DISABLE_RENDER_TARGETS
	return false;
#endif
	RenderTarget *rt = render_target_owner.getornull(p_render_target);
	ERR_FAIL_COND_V(!rt, false);
	return rt->clear_requested;
}
Color RasterizerStorageGLES2::render_target_get_clear_request_color(RID p_render_target)
{
#ifdef GLES2_DISABLE_RENDER_TARGETS
	return Color();
#endif
	
	RenderTarget *rt = render_target_owner.getornull(p_render_target);
	ERR_FAIL_COND_V(!rt, Color());
	return rt->clear_color;
}

void RasterizerStorageGLES2::render_target_disable_clear_request(RID p_render_target)
{
#ifdef GLES2_DISABLE_RENDER_TARGETS
	return;
#endif
	RenderTarget *rt = render_target_owner.getornull(p_render_target);
	ERR_FAIL_COND(!rt);
	rt->clear_requested = false;
}

void RasterizerStorageGLES2::render_target_do_clear_request(RID p_render_target)
{
#ifdef GLES2_DISABLE_RENDER_TARGETS
	return;
#endif
	RenderTarget *rt = render_target_owner.getornull(p_render_target);
	ERR_FAIL_COND(!rt);
	if (!rt->clear_requested) {
		return;
	}
	
	const Color &c = rt->clear_color;
	
	glClearColor(c.r, c.g, c.b, c.a);
	// more bits?
	glClear(GL_COLOR_BUFFER_BIT);	
}


/* CANVAS SHADOW */

RID RasterizerStorageGLES2::canvas_light_shadow_buffer_create(int p_width) {

	CanvasLightShadow *cls = memnew(CanvasLightShadow);

	if (p_width > config.max_texture_size)
		p_width = config.max_texture_size;

	cls->size = p_width;
	cls->height = 16;

	glActiveTexture(GL_TEXTURE0);

	glGenFramebuffers(1, &cls->fbo);
	bind_framebuffer(cls->fbo);

	glGenRenderbuffers(1, &cls->depth);
	glBindRenderbuffer(GL_RENDERBUFFER, cls->depth);
	glRenderbufferStorage(GL_RENDERBUFFER, config.depth_buffer_internalformat, cls->size, cls->height);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, cls->depth);

	glGenTextures(1, &cls->distance);
	glBindTexture(GL_TEXTURE_2D, cls->distance);
	if (config.use_rgba_2d_shadows) {
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, cls->size, cls->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	} else {
#ifdef GLES_OVER_GL
		glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, cls->size, cls->height, 0, _RED_OES, GL_FLOAT, NULL);
#else
		glTexImage2D(GL_TEXTURE_2D, 0, GL_FLOAT, cls->size, cls->height, 0, _RED_OES, GL_FLOAT, NULL);
#endif
	}

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, cls->distance, 0);

	GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	//printf("errnum: %x\n",status);
	bind_framebuffer_system();

	if (status != GL_FRAMEBUFFER_COMPLETE) {
		memdelete(cls);
		ERR_FAIL_COND_V(status != GL_FRAMEBUFFER_COMPLETE, RID());
	}

	return canvas_light_shadow_owner.make_rid(cls);
}

/* LIGHT SHADOW MAPPING */
/*

RID RasterizerStorageGLES2::canvas_light_occluder_create() {

	CanvasOccluder *co = memnew(CanvasOccluder);
	co->index_id = 0;
	co->vertex_id = 0;
	co->len = 0;

	return canvas_occluder_owner.make_rid(co);
}

void RasterizerStorageGLES2::canvas_light_occluder_set_polylines(RID p_occluder, const PoolVector<Vector2> &p_lines) {
	
	CanvasOccluder *co = canvas_occluder_owner.get(p_occluder);
	ERR_FAIL_COND(!co);

	co->lines = p_lines;

	if (p_lines.size() != co->len) {

		if (co->index_id)
			glDeleteBuffers(1, &co->index_id);
		if (co->vertex_id)
			glDeleteBuffers(1, &co->vertex_id);

		co->index_id = 0;
		co->vertex_id = 0;
		co->len = 0;
	}

	if (p_lines.size()) {

		PoolVector<float> geometry;
		PoolVector<uint16_t> indices;
		int lc = p_lines.size();

		geometry.resize(lc * 6);
		indices.resize(lc * 3);

		PoolVector<float>::Write vw = geometry.write();
		PoolVector<uint16_t>::Write iw = indices.write();

		PoolVector<Vector2>::Read lr = p_lines.read();

		const int POLY_HEIGHT = 16384;

		for (int i = 0; i < lc / 2; i++) {

			vw[i * 12 + 0] = lr[i * 2 + 0].x;
			vw[i * 12 + 1] = lr[i * 2 + 0].y;
			vw[i * 12 + 2] = POLY_HEIGHT;

			vw[i * 12 + 3] = lr[i * 2 + 1].x;
			vw[i * 12 + 4] = lr[i * 2 + 1].y;
			vw[i * 12 + 5] = POLY_HEIGHT;

			vw[i * 12 + 6] = lr[i * 2 + 1].x;
			vw[i * 12 + 7] = lr[i * 2 + 1].y;
			vw[i * 12 + 8] = -POLY_HEIGHT;

			vw[i * 12 + 9] = lr[i * 2 + 0].x;
			vw[i * 12 + 10] = lr[i * 2 + 0].y;
			vw[i * 12 + 11] = -POLY_HEIGHT;

			iw[i * 6 + 0] = i * 4 + 0;
			iw[i * 6 + 1] = i * 4 + 1;
			iw[i * 6 + 2] = i * 4 + 2;

			iw[i * 6 + 3] = i * 4 + 2;
			iw[i * 6 + 4] = i * 4 + 3;
			iw[i * 6 + 5] = i * 4 + 0;
		}

		//if same buffer len is being set, just use BufferSubData to avoid a pipeline flush

		if (!co->vertex_id) {
			glGenBuffers(1, &co->vertex_id);
			glBindBuffer(GL_ARRAY_BUFFER, co->vertex_id);
			glBufferData(GL_ARRAY_BUFFER, lc * 6 * sizeof(real_t), vw.ptr(), GL_STATIC_DRAW);
		} else {

			glBindBuffer(GL_ARRAY_BUFFER, co->vertex_id);
			glBufferSubData(GL_ARRAY_BUFFER, 0, lc * 6 * sizeof(real_t), vw.ptr());
		}

		glBindBuffer(GL_ARRAY_BUFFER, 0); //unbind

		if (!co->index_id) {

			glGenBuffers(1, &co->index_id);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, co->index_id);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, lc * 3 * sizeof(uint16_t), iw.ptr(), GL_DYNAMIC_DRAW);
		} else {

			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, co->index_id);
			glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, lc * 3 * sizeof(uint16_t), iw.ptr());
		}

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0); //unbind

		co->len = lc;
	}
}
*/

GD_VS::InstanceType RasterizerStorageGLES2::get_base_type(RID p_rid) const {
	return GD_VS::INSTANCE_NONE;
	
	/*
	if (mesh_owner.owns(p_rid)) {
		return GD_VS::INSTANCE_MESH;
	} else if (light_owner.owns(p_rid)) {
		return GD_VS::INSTANCE_LIGHT;
	} else if (multimesh_owner.owns(p_rid)) {
		return GD_VS::INSTANCE_MULTIMESH;
	} else if (immediate_owner.owns(p_rid)) {
		return GD_VS::INSTANCE_IMMEDIATE;
	} else if (reflection_probe_owner.owns(p_rid)) {
		return GD_VS::INSTANCE_REFLECTION_PROBE;
	} else if (lightmap_capture_data_owner.owns(p_rid)) {
		return GD_VS::INSTANCE_LIGHTMAP_CAPTURE;
	} else {
		return GD_VS::INSTANCE_NONE;
	}
*/
}

bool RasterizerStorageGLES2::free(RID p_rid) {

	if (render_target_owner.owns(p_rid)) {
		
		RenderTarget *rt = render_target_owner.getornull(p_rid);
		_render_target_clear(rt);

		Texture *t = texture_owner.getornull(rt->texture);
		if (t)
		{
		texture_owner.free(rt->texture);
		memdelete(t);
		}
		render_target_owner.free(p_rid);
		memdelete(rt);

		return true;
	} else if (texture_owner.owns(p_rid)) {

		Texture *t = texture_owner.getornull(p_rid);
		// can't free a render target texture
		ERR_FAIL_COND_V(t->render_target, true);

		info.texture_mem -= t->total_data_size;
		texture_owner.free(p_rid);
		memdelete(t);

		return true;
	} else if (sky_owner.owns(p_rid)) {

		Sky *sky = sky_owner.getornull(p_rid);
		sky_set_texture(p_rid, RID(), 256);
		sky_owner.free(p_rid);
		memdelete(sky);

		return true;
	} else if (shader_owner.owns(p_rid)) {

		Shader *shader = shader_owner.getornull(p_rid);

		if (shader->shader && shader->custom_code_id) {
			shader->shader->free_custom_shader(shader->custom_code_id);
		}

		if (shader->dirty_list.in_list()) {
			_shader_dirty_list.remove(&shader->dirty_list);
		}

		while (shader->materials.first()) {
			Material *m = shader->materials.first()->self();

			m->shader = NULL;
			_material_make_dirty(m);

			shader->materials.remove(shader->materials.first());
		}

		shader_owner.free(p_rid);
		memdelete(shader);

		return true;
	} else if (material_owner.owns(p_rid)) {

		Material *m = material_owner.getornull(p_rid);

		if (m->shader) {
			m->shader->materials.remove(&m->list);
		}
		
		/*
		for (Map<Geometry *, int>::Element *E = m->geometry_owners.front(); E; E = E->next()) {
			Geometry *g = E->key();
			g->material = RID();
		}

		for (Map<RasterizerScene::InstanceBase *, int>::Element *E = m->instance_owners.front(); E; E = E->next()) {

			RasterizerScene::InstanceBase *ins = E->key();

			if (ins->material_override == p_rid) {
				ins->material_override = RID();
			}

			for (int i = 0; i < ins->materials.size(); i++) {
				if (ins->materials[i] == p_rid) {
					ins->materials.write[i] = RID();
				}
			}
		}
*/

		material_owner.free(p_rid);
		memdelete(m);

		return true;
		
	} else {
		return false;
	}
		/*
	} else if (skeleton_owner.owns(p_rid)) {

		Skeleton *s = skeleton_owner.getornull(p_rid);

		if (s->update_list.in_list()) {
			skeleton_update_list.remove(&s->update_list);
		}

		for (Set<RasterizerScene::InstanceBase *>::Element *E = s->instances.front(); E; E = E->next()) {
			E->get()->skeleton = RID();
		}

		skeleton_allocate(p_rid, 0, false);

		if (s->tex_id) {
			glDeleteTextures(1, &s->tex_id);
		}

		skeleton_owner.free(p_rid);
		memdelete(s);

		return true;
	} else if (mesh_owner.owns(p_rid)) {

		Mesh *mesh = mesh_owner.getornull(p_rid);

		mesh->instance_remove_deps();
		mesh_clear(p_rid);

		while (mesh->multimeshes.first()) {
			MultiMesh *multimesh = mesh->multimeshes.first()->self();
			multimesh->mesh = RID();
			multimesh->dirty_aabb = true;

			mesh->multimeshes.remove(mesh->multimeshes.first());

			if (!multimesh->update_list.in_list()) {
				multimesh_update_list.add(&multimesh->update_list);
			}
		}

		mesh_owner.free(p_rid);
		memdelete(mesh);

		return true;
	} else if (multimesh_owner.owns(p_rid)) {

		MultiMesh *multimesh = multimesh_owner.getornull(p_rid);
		multimesh->instance_remove_deps();

		if (multimesh->mesh.is_valid()) {
			Mesh *mesh = mesh_owner.getornull(multimesh->mesh);
			if (mesh) {
				mesh->multimeshes.remove(&multimesh->mesh_list);
			}
		}

		multimesh_allocate(p_rid, 0, GD_VS::MULTIMESH_TRANSFORM_3D, GD_VS::MULTIMESH_COLOR_NONE);

		update_dirty_multimeshes();

		multimesh_owner.free(p_rid);
		memdelete(multimesh);

		return true;
	} else if (immediate_owner.owns(p_rid)) {
		Immediate *im = immediate_owner.getornull(p_rid);
		im->instance_remove_deps();

		immediate_owner.free(p_rid);
		memdelete(im);

		return true;
	} else if (light_owner.owns(p_rid)) {

		Light *light = light_owner.getornull(p_rid);
		light->instance_remove_deps();

		light_owner.free(p_rid);
		memdelete(light);

		return true;
	} else if (reflection_probe_owner.owns(p_rid)) {

		// delete the texture
		ReflectionProbe *reflection_probe = reflection_probe_owner.getornull(p_rid);
		reflection_probe->instance_remove_deps();

		reflection_probe_owner.free(p_rid);
		memdelete(reflection_probe);

		return true;
	} else if (lightmap_capture_data_owner.owns(p_rid)) {

		// delete the texture
		LightmapCapture *lightmap_capture = lightmap_capture_data_owner.getornull(p_rid);
		lightmap_capture->instance_remove_deps();

		lightmap_capture_data_owner.free(p_rid);
		memdelete(lightmap_capture);
		return true;

	} else if (canvas_occluder_owner.owns(p_rid)) {

		CanvasOccluder *co = canvas_occluder_owner.getornull(p_rid);
		if (co->index_id)
			glDeleteBuffers(1, &co->index_id);
		if (co->vertex_id)
			glDeleteBuffers(1, &co->vertex_id);

		canvas_occluder_owner.free(p_rid);
		memdelete(co);

		return true;

	} else if (canvas_light_shadow_owner.owns(p_rid)) {

		CanvasLightShadow *cls = canvas_light_shadow_owner.getornull(p_rid);
		glDeleteFramebuffers(1, &cls->fbo);
		glDeleteRenderbuffers(1, &cls->depth);
		glDeleteTextures(1, &cls->distance);
		canvas_light_shadow_owner.free(p_rid);
		memdelete(cls);

		return true;
		*/
}

bool RasterizerStorageGLES2::has_os_feature(const String &p_feature) const {

	if (p_feature == "pvrtc")
		return config.pvrtc_supported;

	if (p_feature == "s3tc")
		return config.s3tc_supported;

	if (p_feature == "etc")
		return config.etc1_supported;

	if (p_feature == "skinning_fallback")
		return config.use_skeleton_software;

	return false;
}

////////////////////////////////////////////

void RasterizerStorageGLES2::set_debug_generate_wireframes(bool p_generate) {
}

void RasterizerStorageGLES2::render_info_begin_capture() {

	info.snap = info.render;
}

void RasterizerStorageGLES2::render_info_end_capture() {

	info.snap.object_count = info.render.object_count - info.snap.object_count;
	info.snap.draw_call_count = info.render.draw_call_count - info.snap.draw_call_count;
	info.snap.material_switch_count = info.render.material_switch_count - info.snap.material_switch_count;
	info.snap.surface_switch_count = info.render.surface_switch_count - info.snap.surface_switch_count;
	info.snap.shader_rebind_count = info.render.shader_rebind_count - info.snap.shader_rebind_count;
	info.snap.vertices_count = info.render.vertices_count - info.snap.vertices_count;
	info.snap._2d_item_count = info.render._2d_item_count - info.snap._2d_item_count;
	info.snap._2d_draw_call_count = info.render._2d_draw_call_count - info.snap._2d_draw_call_count;
}

int RasterizerStorageGLES2::get_captured_render_info(GD_VS::RenderInfo p_info) {

	switch (p_info) {
		case GD_VS::INFO_OBJECTS_IN_FRAME: {
			return info.snap.object_count;
		} break;
		case GD_VS::INFO_VERTICES_IN_FRAME: {
			return info.snap.vertices_count;
		} break;
		case GD_VS::INFO_MATERIAL_CHANGES_IN_FRAME: {
			return info.snap.material_switch_count;
		} break;
		case GD_VS::INFO_SHADER_CHANGES_IN_FRAME: {
			return info.snap.shader_rebind_count;
		} break;
		case GD_VS::INFO_SURFACE_CHANGES_IN_FRAME: {
			return info.snap.surface_switch_count;
		} break;
		case GD_VS::INFO_DRAW_CALLS_IN_FRAME: {
			return info.snap.draw_call_count;
		} break;
			/*
		case GD_VS::INFO_2D_ITEMS_IN_FRAME: {
			return info.snap._2d_item_count;
		} break;
		case GD_VS::INFO_2D_DRAW_CALLS_IN_FRAME: {
			return info.snap._2d_draw_call_count;
		} break;
			*/
		default: {
			return get_render_info(p_info);
		}
	}
}

int RasterizerStorageGLES2::get_render_info(GD_VS::RenderInfo p_info) {
	switch (p_info) {
		case GD_VS::INFO_OBJECTS_IN_FRAME:
			return info.render_final.object_count;
		case GD_VS::INFO_VERTICES_IN_FRAME:
			return info.render_final.vertices_count;
		case GD_VS::INFO_MATERIAL_CHANGES_IN_FRAME:
			return info.render_final.material_switch_count;
		case GD_VS::INFO_SHADER_CHANGES_IN_FRAME:
			return info.render_final.shader_rebind_count;
		case GD_VS::INFO_SURFACE_CHANGES_IN_FRAME:
			return info.render_final.surface_switch_count;
		case GD_VS::INFO_DRAW_CALLS_IN_FRAME:
			return info.render_final.draw_call_count;
			/*
		case GD_VS::INFO_2D_ITEMS_IN_FRAME:
			return info.render_final._2d_item_count;
		case GD_VS::INFO_2D_DRAW_CALLS_IN_FRAME:
			return info.render_final._2d_draw_call_count;
*/
		case GD_VS::INFO_USAGE_VIDEO_MEM_TOTAL:
			return 0; //no idea
		case GD_VS::INFO_VIDEO_MEM_USED:
			return info.vertex_mem + info.texture_mem;
		case GD_VS::INFO_TEXTURE_MEM_USED:
			return info.texture_mem;
		case GD_VS::INFO_VERTEX_MEM_USED:
			return info.vertex_mem;
		default:
			return 0; //no idea either
	}
}

String RasterizerStorageGLES2::get_video_adapter_name() const {

	return (const char *)glGetString(GL_RENDERER);
}

String RasterizerStorageGLES2::get_video_adapter_vendor() const {

	return (const char *)glGetString(GL_VENDOR);
}

void RasterizerStorageGLES2::initialize() {
	RasterizerStorageGLES2::system_fbo = 0;

	{

		const GLubyte *extension_string = glGetString(GL_EXTENSIONS);

		Vector<String> extensions = String((const char *)extension_string).split(" ");

		for (int i = 0; i < extensions.size(); i++) {
			config.extensions.insert(extensions[i]);
		}
	}
	
	// FTODO
	config.keep_original_textures = true; // false
	config.shrink_textures_x2 = false;
	config.depth_internalformat = GL_DEPTH_COMPONENT;
	config.depth_type = GL_UNSIGNED_INT;

#ifdef GLES_OVER_GL
	config.float_texture_supported = true;
	config.s3tc_supported = true;
	config.pvrtc_supported = false;
	config.etc1_supported = false;
	config.support_npot_repeat_mipmap = true;
	config.depth_buffer_internalformat = GL_DEPTH_COMPONENT24;
#else
	config.float_texture_supported = config.extensions.has("GL_ARB_texture_float") || config.extensions.has("GL_OES_texture_float");
	config.s3tc_supported = config.extensions.has("GL_EXT_texture_compression_s3tc") || config.extensions.has("WEBGL_compressed_texture_s3tc");
	config.etc1_supported = config.extensions.has("GL_OES_compressed_ETC1_RGB8_texture") || config.extensions.has("WEBGL_compressed_texture_etc1");
	config.pvrtc_supported = config.extensions.has("GL_IMG_texture_compression_pvrtc") || config.extensions.has("WEBGL_compressed_texture_pvrtc");
	config.support_npot_repeat_mipmap = config.extensions.has("GL_OES_texture_npot");

#ifdef JAVASCRIPT_ENABLED
	// RenderBuffer internal format must be 16 bits in WebGL,
	// but depth_texture should default to 32 always
	// if the implementation doesn't support 32, it should just quietly use 16 instead
	// https://www.khronos.org/registry/webgl/extensions/WEBGL_depth_texture/
	config.depth_buffer_internalformat = GL_DEPTH_COMPONENT16;
	config.depth_type = GL_UNSIGNED_INT;
#else
	// on mobile check for 24 bit depth support for RenderBufferStorage
	if (config.extensions.has("GL_OES_depth24")) {
		config.depth_buffer_internalformat = _DEPTH_COMPONENT24_OES;
		config.depth_type = GL_UNSIGNED_INT;
	} else {
		config.depth_buffer_internalformat = GL_DEPTH_COMPONENT16;
		config.depth_type = GL_UNSIGNED_SHORT;
	}
#endif
#endif

#ifndef GLES_OVER_GL
	//Manually load extensions for android and ios

#ifdef IPHONE_ENABLED
	// appears that IPhone doesn't need to dlopen TODO: test this rigorously before removing
	//void *gles2_lib = dlopen(NULL, RTLD_LAZY);
	//glRenderbufferStorageMultisampleAPPLE = dlsym(gles2_lib, "glRenderbufferStorageMultisampleAPPLE");
	//glResolveMultisampleFramebufferAPPLE = dlsym(gles2_lib, "glResolveMultisampleFramebufferAPPLE");
#elif ANDROID_ENABLED

	void *gles2_lib = dlopen("libGLESv2.so", RTLD_LAZY);
	glRenderbufferStorageMultisampleEXT = (PFNGLRENDERBUFFERSTORAGEMULTISAMPLEEXTPROC)dlsym(gles2_lib, "glRenderbufferStorageMultisampleEXT");
	glFramebufferTexture2DMultisampleEXT = (PFNGLFRAMEBUFFERTEXTURE2DMULTISAMPLEEXTPROC)dlsym(gles2_lib, "glFramebufferTexture2DMultisampleEXT");
#endif
#endif

	// Check for multisample support
	config.multisample_supported = config.extensions.has("GL_EXT_framebuffer_multisample") || config.extensions.has("GL_EXT_multisampled_render_to_texture") || config.extensions.has("GL_APPLE_framebuffer_multisample");

#ifdef GLES_OVER_GL
	//TODO: causes huge problems with desktop video drivers. Making false for now, needs to be true to render SCREEN_TEXTURE mipmaps
	config.render_to_mipmap_supported = false;
#else
	//check if mipmaps can be used for SCREEN_TEXTURE and Glow on Mobile and web platforms
	config.render_to_mipmap_supported = config.extensions.has("GL_OES_fbo_render_mipmap") && config.extensions.has("GL_EXT_texture_lod");
#endif

#ifdef GLES_OVER_GL
	config.use_rgba_2d_shadows = false;
	config.support_depth_texture = true;
	config.use_rgba_3d_shadows = false;
	config.support_depth_cubemaps = true;
#else
	config.use_rgba_2d_shadows = !(config.float_texture_supported && config.extensions.has("GL_EXT_texture_rg"));
	config.support_depth_texture = config.extensions.has("GL_OES_depth_texture") || config.extensions.has("WEBGL_depth_texture");
	config.use_rgba_3d_shadows = !config.support_depth_texture;
	config.support_depth_cubemaps = config.extensions.has("GL_OES_depth_texture_cube_map");
#endif

#ifdef GLES_OVER_GL
	config.support_32_bits_indices = true;
#else
	config.support_32_bits_indices = config.extensions.has("GL_OES_element_index_uint");
#endif

#ifdef GLES_OVER_GL
	config.support_write_depth = true;
#elif defined(JAVASCRIPT_ENABLED)
	config.support_write_depth = false;
#else
	config.support_write_depth = config.extensions.has("GL_EXT_frag_depth");
#endif

	config.support_half_float_vertices = true;
//every platform should support this except web, iOS has issues with their support, so add option to disable
#ifdef JAVASCRIPT_ENABLED
	config.support_half_float_vertices = false;
#endif
	bool disable_half_float = GLOBAL_GET("rendering/gles2/compatibility/disable_half_float");
	if (disable_half_float) {
		config.support_half_float_vertices = false;
	}

	config.rgtc_supported = config.extensions.has("GL_EXT_texture_compression_rgtc") || config.extensions.has("GL_ARB_texture_compression_rgtc") || config.extensions.has("EXT_texture_compression_rgtc");
	config.bptc_supported = config.extensions.has("GL_ARB_texture_compression_bptc") || config.extensions.has("EXT_texture_compression_bptc");

	//determine formats for depth textures (or renderbuffers)
	if (config.support_depth_texture) {
		// Will use texture for depth
		// have to manually see if we can create a valid framebuffer texture using UNSIGNED_INT,
		// as there is no extension to test for this.
		GLuint fbo;
		glGenFramebuffers(1, &fbo);
		bind_framebuffer(fbo);
		GLuint depth;
		glGenTextures(1, &depth);
		glBindTexture(GL_TEXTURE_2D, depth);
		glTexImage2D(GL_TEXTURE_2D, 0, config.depth_internalformat, 32, 32, 0, GL_DEPTH_COMPONENT, config.depth_type, NULL);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depth, 0);

		GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
		
		bind_framebuffer_system();
		glDeleteFramebuffers(1, &fbo);
		glBindTexture(GL_TEXTURE_2D, 0);
		glDeleteTextures(1, &depth);

		if (status != GL_FRAMEBUFFER_COMPLETE) {
			// If it fails, test to see if it supports a framebuffer texture using UNSIGNED_SHORT
			// This is needed because many OSX devices don't support either UNSIGNED_INT or UNSIGNED_SHORT
#ifdef GLES_OVER_GL
			config.depth_internalformat = GL_DEPTH_COMPONENT16;
#else
			// OES_depth_texture extension only specifies GL_DEPTH_COMPONENT.
			config.depth_internalformat = GL_DEPTH_COMPONENT;
#endif
			config.depth_type = GL_UNSIGNED_SHORT;

			glGenFramebuffers(1, &fbo);
			bind_framebuffer(fbo);

			glGenTextures(1, &depth);
			glBindTexture(GL_TEXTURE_2D, depth);
			glTexImage2D(GL_TEXTURE_2D, 0, config.depth_internalformat, 32, 32, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_SHORT, NULL);

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depth, 0);

			status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
			if (status != GL_FRAMEBUFFER_COMPLETE) {
				//if it fails again depth textures aren't supported, use rgba shadows and renderbuffer for depth
				config.support_depth_texture = false;
				config.use_rgba_3d_shadows = true;
			}
			
			bind_framebuffer_system();
			glDeleteFramebuffers(1, &fbo);
			glBindTexture(GL_TEXTURE_2D, 0);
			glDeleteTextures(1, &depth);
		}
	}

	//picky requirements for these
	config.support_shadow_cubemaps = config.support_depth_texture && config.support_write_depth && config.support_depth_cubemaps;

	frame.count = 0;
	frame.delta = 0;
	frame.current_rt = NULL;
	frame.clear_request = false;

	glGetIntegerv(GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS, &config.max_vertex_texture_image_units);
	glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &config.max_texture_image_units);
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &config.max_texture_size);

	// the use skeleton software path should be used if either float texture is not supported,
	// OR max_vertex_texture_image_units is zero
	config.use_skeleton_software = (config.float_texture_supported == false) || (config.max_vertex_texture_image_units == 0);

	shaders.copy.init();
	shaders.cubemap_filter.init();
	bool ggx_hq = GLOBAL_GET("rendering/quality/reflections/high_quality_ggx");
	shaders.cubemap_filter.set_conditional(CubemapFilterShaderGLES2::LOW_QUALITY, !ggx_hq);

	{
		// quad for copying stuff

		glGenBuffers(1, &resources.quadie);
		glBindBuffer(GL_ARRAY_BUFFER, resources.quadie);
		{
			const float qv[16] = {
				-1,
				-1,
				0,
				0,
				-1,
				1,
				0,
				1,
				1,
				1,
				1,
				1,
				1,
				-1,
				1,
				0,
			};

			glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 16, qv, GL_STATIC_DRAW);
		}

		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}

	{
		//default textures

		glGenTextures(1, &resources.white_tex);
		unsigned char whitetexdata[8 * 8 * 3];
		for (int i = 0; i < 8 * 8 * 3; i++) {
			whitetexdata[i] = 255;
		}

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, resources.white_tex);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 8, 8, 0, GL_RGB, GL_UNSIGNED_BYTE, whitetexdata);
		glGenerateMipmap(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, 0);

		glGenTextures(1, &resources.black_tex);
		unsigned char blacktexdata[8 * 8 * 3];
		for (int i = 0; i < 8 * 8 * 3; i++) {
			blacktexdata[i] = 0;
		}

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, resources.black_tex);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 8, 8, 0, GL_RGB, GL_UNSIGNED_BYTE, blacktexdata);
		glGenerateMipmap(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, 0);

		glGenTextures(1, &resources.normal_tex);
		unsigned char normaltexdata[8 * 8 * 3];
		for (int i = 0; i < 8 * 8 * 3; i += 3) {
			normaltexdata[i + 0] = 128;
			normaltexdata[i + 1] = 128;
			normaltexdata[i + 2] = 255;
		}

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, resources.normal_tex);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 8, 8, 0, GL_RGB, GL_UNSIGNED_BYTE, normaltexdata);
		glGenerateMipmap(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, 0);

		glGenTextures(1, &resources.aniso_tex);
		unsigned char anisotexdata[8 * 8 * 3];
		for (int i = 0; i < 8 * 8 * 3; i += 3) {
			anisotexdata[i + 0] = 255;
			anisotexdata[i + 1] = 128;
			anisotexdata[i + 2] = 0;
		}

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, resources.aniso_tex);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 8, 8, 0, GL_RGB, GL_UNSIGNED_BYTE, anisotexdata);
		glGenerateMipmap(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, 0);
	}

	// skeleton buffer
	{
		resources.skeleton_transform_buffer_size = 0;
		glGenBuffers(1, &resources.skeleton_transform_buffer);
	}

	// radical inverse vdc cache texture
	// used for cubemap filtering
	if (true /*||config.float_texture_supported*/) { //uint8 is similar and works everywhere
		glGenTextures(1, &resources.radical_inverse_vdc_cache_tex);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, resources.radical_inverse_vdc_cache_tex);

		uint8_t radical_inverse[512];

		for (uint32_t i = 0; i < 512; i++) {
			uint32_t bits = i;

			bits = (bits << 16) | (bits >> 16);
			bits = ((bits & 0x55555555) << 1) | ((bits & 0xAAAAAAAA) >> 1);
			bits = ((bits & 0x33333333) << 2) | ((bits & 0xCCCCCCCC) >> 2);
			bits = ((bits & 0x0F0F0F0F) << 4) | ((bits & 0xF0F0F0F0) >> 4);
			bits = ((bits & 0x00FF00FF) << 8) | ((bits & 0xFF00FF00) >> 8);

			float value = float(bits) * 2.3283064365386963e-10;
			radical_inverse[i] = uint8_t(CLAMP(value * 255.0, 0, 255));
		}

		glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, 512, 1, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, radical_inverse);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST); //need this for proper sampling

		glBindTexture(GL_TEXTURE_2D, 0);
	}

	{

		glGenFramebuffers(1, &resources.mipmap_blur_fbo);
		glGenTextures(1, &resources.mipmap_blur_color);
	}

#ifdef GLES_OVER_GL
	//this needs to be enabled manually in OpenGL 2.1

	if (config.extensions.has("GL_ARB_seamless_cube_map")) {
		glEnable(_EXT_TEXTURE_CUBE_MAP_SEAMLESS);
	}
	glEnable(GL_POINT_SPRITE);
	glEnable(GL_VERTEX_PROGRAM_POINT_SIZE);
#endif

	config.force_vertex_shading = GLOBAL_GET("rendering/quality/shading/force_vertex_shading");
	config.use_fast_texture_filter = GLOBAL_GET("rendering/quality/filters/use_nearest_mipmap_filter");
	config.should_orphan = GLOBAL_GET("rendering/options/api_usage_legacy/orphan_buffers");
}

void RasterizerStorageGLES2::finalize() {
}

void RasterizerStorageGLES2::_copy_screen() {
	bind_quad_array();
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
}

void RasterizerStorageGLES2::update_dirty_resources() {
	update_dirty_shaders();
	update_dirty_materials();
//	update_dirty_skeletons();
//	update_dirty_multimeshes();
}

RasterizerStorageGLES2::RasterizerStorageGLES2() {
	RasterizerStorageGLES2::system_fbo = 0;
	config.should_orphan = true;
}
