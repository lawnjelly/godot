#include "legacy.h"

namespace Batch
{

void Legacy::state_set_final_modulate(const Color &col)
{
	state.uniforms.final_modulate = col;
}

void Legacy::state_set_model_view(const Transform2D &tr)
{
	state.uniforms.modelview_matrix = tr;
}

void Legacy::state_set_extra(const Transform2D &tr)
{
	state.uniforms.extra_matrix = tr;
}



void Legacy::GL_SetState_BlendMode(int blend_mode)
{
	switch (blend_mode)
	{

		case RasterizerStorageGLES2::Shader::CanvasItem::BLEND_MODE_MIX:
		{
			glBlendEquation(GL_FUNC_ADD);
			if (storage->frame.current_rt && storage->frame.current_rt->flags[RasterizerStorage::RENDER_TARGET_TRANSPARENT])
			{
				glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
			}
			else
			{
				glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ZERO, GL_ONE);
			}
		}
		break;
		case RasterizerStorageGLES2::Shader::CanvasItem::BLEND_MODE_ADD:
		{

			glBlendEquation(GL_FUNC_ADD);
			if (storage->frame.current_rt && storage->frame.current_rt->flags[RasterizerStorage::RENDER_TARGET_TRANSPARENT])
			{
				glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE, GL_SRC_ALPHA, GL_ONE);
			}
			else
			{
				glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE, GL_ZERO, GL_ONE);
			}
		}
		break;
		case RasterizerStorageGLES2::Shader::CanvasItem::BLEND_MODE_SUB:
		{

			glBlendEquation(GL_FUNC_REVERSE_SUBTRACT);
			if (storage->frame.current_rt && storage->frame.current_rt->flags[RasterizerStorage::RENDER_TARGET_TRANSPARENT])
			{
				glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE, GL_SRC_ALPHA, GL_ONE);
			}
			else
			{
				glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE, GL_ZERO, GL_ONE);
			}
		}
		break;
		case RasterizerStorageGLES2::Shader::CanvasItem::BLEND_MODE_MUL:
		{
			glBlendEquation(GL_FUNC_ADD);
			if (storage->frame.current_rt && storage->frame.current_rt->flags[RasterizerStorage::RENDER_TARGET_TRANSPARENT])
			{
				glBlendFuncSeparate(GL_DST_COLOR, GL_ZERO, GL_DST_ALPHA, GL_ZERO);
			}
			else
			{
				glBlendFuncSeparate(GL_DST_COLOR, GL_ZERO, GL_ZERO, GL_ONE);
			}
		}
		break;
		case RasterizerStorageGLES2::Shader::CanvasItem::BLEND_MODE_PMALPHA:
		{
			glBlendEquation(GL_FUNC_ADD);
			if (storage->frame.current_rt && storage->frame.current_rt->flags[RasterizerStorage::RENDER_TARGET_TRANSPARENT])
			{
				glBlendFuncSeparate(GL_ONE, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
			}
			else
			{
				glBlendFuncSeparate(GL_ONE, GL_ONE_MINUS_SRC_ALPHA, GL_ZERO, GL_ONE);
			}
		}
		break;
	}
}

void Legacy::GL_SetState_LightBlend(VS::CanvasLightMode mode)
{
	switch (mode)
	{

		case VS::CANVAS_LIGHT_MODE_ADD:
		{
			glBlendEquation(GL_FUNC_ADD);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE);
		}
		break;
		case VS::CANVAS_LIGHT_MODE_SUB:
		{
			glBlendEquation(GL_FUNC_REVERSE_SUBTRACT);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE);
		}
		break;
		case VS::CANVAS_LIGHT_MODE_MIX:
		case VS::CANVAS_LIGHT_MODE_MASK:
		{
			glBlendEquation(GL_FUNC_ADD);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		}
		break;
	}
}

void Legacy::CanvasShader_SetConditionals_Light(bool has_shadow, Light *light)
{
	if (light)
	{
		state.canvas_shader.set_conditional(CanvasShaderGLES2::USE_SHADOWS, has_shadow);
		if (has_shadow)
		{
			state.canvas_shader.set_conditional(CanvasShaderGLES2::SHADOW_USE_GRADIENT, light->shadow_gradient_length > 0);
			state.canvas_shader.set_conditional(CanvasShaderGLES2::SHADOW_FILTER_NEAREST, light->shadow_filter == VS::CANVAS_LIGHT_FILTER_NONE);
			state.canvas_shader.set_conditional(CanvasShaderGLES2::SHADOW_FILTER_PCF3, light->shadow_filter == VS::CANVAS_LIGHT_FILTER_PCF3);
			state.canvas_shader.set_conditional(CanvasShaderGLES2::SHADOW_FILTER_PCF5, light->shadow_filter == VS::CANVAS_LIGHT_FILTER_PCF5);
			state.canvas_shader.set_conditional(CanvasShaderGLES2::SHADOW_FILTER_PCF7, light->shadow_filter == VS::CANVAS_LIGHT_FILTER_PCF7);
			state.canvas_shader.set_conditional(CanvasShaderGLES2::SHADOW_FILTER_PCF9, light->shadow_filter == VS::CANVAS_LIGHT_FILTER_PCF9);
			state.canvas_shader.set_conditional(CanvasShaderGLES2::SHADOW_FILTER_PCF13, light->shadow_filter == VS::CANVAS_LIGHT_FILTER_PCF13);
		}
	}
	else
	{
		state.canvas_shader.set_conditional(CanvasShaderGLES2::USE_LIGHTING, false);
		state.canvas_shader.set_conditional(CanvasShaderGLES2::USE_SHADOWS, false);
		state.canvas_shader.set_conditional(CanvasShaderGLES2::SHADOW_FILTER_NEAREST, false);
		state.canvas_shader.set_conditional(CanvasShaderGLES2::SHADOW_FILTER_PCF3, false);
		state.canvas_shader.set_conditional(CanvasShaderGLES2::SHADOW_FILTER_PCF5, false);
		state.canvas_shader.set_conditional(CanvasShaderGLES2::SHADOW_FILTER_PCF7, false);
		state.canvas_shader.set_conditional(CanvasShaderGLES2::SHADOW_FILTER_PCF9, false);
		state.canvas_shader.set_conditional(CanvasShaderGLES2::SHADOW_FILTER_PCF13, false);
	}
}

} // namespace Batch
