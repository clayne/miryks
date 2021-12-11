extern "C"
{	
#include <half.h>
}

#include <miryks/model.h>

#include <opengl/shader.h>
#include <opengl/texture.h>
#include <opengl/renderer.h>

namespace miryks
{
	nifmodel::nifmodel()
	{
		baseGroup = new GroupBounded();
		groups[-1] = baseGroup;
		lastGroup = baseGroup;
	}
	
	nifmodel::nifmodel(Res *res) : nifmodel()
	{
		if (!res)
			return;
		model = get_nif(res);
		Construct();
	}

	nifmodel::~nifmodel()
	{
	}

	void nifmodel::Construct()
	{
		Rd *rd = calloc_nifprd();
		rd->nif = model;
		rd->data = this;
		rd->ni_node_callback = ni_node_callback;
		rd->bs_tri_shape_callback = bs_tri_shape_callback;
		rd->bs_lighting_shader_property_callback = bs_lighting_shader_property_callback;
		rd->bs_effect_shader_property_callback = bs_effect_shader_property_callback;
		rd->bs_shader_texture_set_callback = bs_shader_texture_set_callback;
		rd->ni_alpha_property_callback = ni_alpha_property_callback;
		nif_rd(rd);
		free_nifprd(&rd);
		baseGroup->Update();
	}

	void nifmodel::Step()
	{
		
	}

	group_type *nifmodel::MakeNewGroup(Rd *rd)
	{
		group_type *group = new GroupBounded();
		groups[rd->current] = group;
		groups[rd->parent]->Add(group);
		lastGroup = group;
		return group;
	}

	void matrix_from_common(group_type *group, ni_common_layout_t *common)
	{
		// todo doesnt fit two columns on laptop
		group->matrix = translate(group->matrix, cast_vec3(&common->A->translation));
		group->matrix *= inverse(mat4(cast_mat3(&common->A->rotation)));
		group->matrix = scale(group->matrix, vec3(common->A->scale));
	}

	void ni_node_callback(RD rd, NiNode *block)
	{
		// printf("ni node callback\n");
		nifmodel *model = (nifmodel *)rd->data;
		group_type *group = model->MakeNewGroup(rd);
		matrix_from_common(group, block->common);
	}

	vec3 bytestofloat(unsigned char *vec)
	{
		float xf, yf, zf;
		xf = (double( vec[0] ) / 255.0) * 2.0 - 1.0;
		yf = (double( vec[1] ) / 255.0) * 2.0 - 1.0;
		zf = (double( vec[2] ) / 255.0) * 2.0 - 1.0;
		return vec3(xf, yf, zf);
	}

	vec2 halftexcoord(unsigned short *uv)
	{
		union { float f; uint32_t i; } u, v;
		u.i = half_to_float(uv[0]);
		v.i = half_to_float(uv[1]);
		return vec2(u.f, v.f);
	}
	
	void bs_tri_shape_callback(RD rd, BSTriShape *block)
	{
		//if (dynamic_cast<MeshSkinned *>(model))
		// printf("model.cpp bs tri shape callback !!! ");
		nifmodel *model = (nifmodel *)rd->data;
		model->shapes__.push_back(rd->current);
		group_type *group = model->MakeNewGroup(rd);
		matrix_from_common(group, block->common);
		Geometry *geometry = new Geometry();
		group->geometry = geometry;
		geometry->material->src = &simple;
		const char *name = nif_get_string(rd->nif, block->common->F->name);
		if (strstr(name, "Marker"))
			return;
		if (!block->infos->num_vertices)
			return;
		if (!block->vertex_data_all && !block->vertex_data_no_clr)
			return;
		geometry->Clear(block->infos->num_vertices, block->infos->num_triangles * 3);
		if (!block->infos->num_vertices)
			return;
		if (block->infos->num_triangles)
		{
			for (unsigned short i = 0; i < block->infos->num_triangles; i++)
			{
				auto triangle = block->triangles[i];
				geometry->elements.insert(geometry->elements.end(), { triangle.a, triangle.b, triangle.c });
			}
		}
		if ( block->vertex_data_all )
		{
		for (unsigned short i = 0; i < block->infos->num_vertices; i++)
		{
			struct bs_vertex_data_sse_all *vertex_data = &block->vertex_data_all[i]; 
			geometry->vertices[i].position = cast_vec3(&vertex_data->vertex);
			geometry->vertices[i].uv = halftexcoord((unsigned short *)&vertex_data->uv);
			geometry->vertices[i].normal = bytestofloat((unsigned char *)&vertex_data->normal);
			geometry->material->tangents = true;
			geometry->vertices[i].tangent = bytestofloat((unsigned char *)&vertex_data->tangent);
			auto c = vertex_data->vertex_colors;
			geometry->vertices[i].color = vec4(c.x / 255.f, c.y / 255.f, c.z / 255.f, c.w / 255.f);
		}
		}
		if ( block->vertex_data_no_clr )
		{
		for (unsigned short i = 0; i < block->infos->num_vertices; i++)
		{
			struct bs_vertex_data_sse_no_clr *vertex_data = &block->vertex_data_no_clr[i]; 
			geometry->vertices[i].position = cast_vec3(&vertex_data->vertex);
			geometry->vertices[i].uv = halftexcoord((unsigned short *)&vertex_data->uv);
			geometry->vertices[i].normal = bytestofloat((unsigned char *)&vertex_data->normal);
			geometry->material->tangents = true;
			geometry->vertices[i].tangent = bytestofloat((unsigned char *)&vertex_data->tangent);
		}
		}
		geometry->SetupMesh();
	}

	void bs_lighting_shader_property_callback(RD rd, BSLightingShaderProperty *block)
	{
		// printf("bs lighting shader property callback\n");
		nifmodel *model = (nifmodel *)rd->data;
		Geometry *geometry = model->lastGroup->geometry;
		if (geometry)
		{
			Material *material = geometry->material;
			material->name += "LightingShader";
			material->zwrite = false;
			material->color = vec3(1.0);
			material->emissive = cast_vec3(&block->B->emissive_color);
			material->specular = cast_vec3(&block->B->specular_color);
			material->specular *= block->B->specular_strength;
			material->opacity = block->B->alpha;
			material->glossiness = block->B->glossiness;
			if (!(block->B->shader_flags_1 & 0x00000002))
				material->dust = true;
			if (block->B->shader_flags_1 & 0x00001000)
				material->modelSpaceNormals = true;
			if (block->B->shader_flags_1 & 0x04000000)
				material->decal = true;
			if (block->B->shader_flags_1 & 0x08000000) // dynamic
				material->decal = true;
			if (block->B->shader_flags_2 & 0x00000001)
				material->zwrite = true;
			if (block->B->shader_flags_2 & 0x00000020)
				material->vertexColors = true;
			if (block->B->shader_flags_2 & 0x00000010)
				material->doubleSided = true;
			if (block->B->shader_flags_1 & 0x00000008)
				material->defines += "#define VERTEX_ALPHA\n";
			if (block->B->shader_flags_2 & 0x20000000)
				material->defines += "#define TREE_ANIM\n";
		}
	}

	void bs_effect_shader_property_callback(RD rd, BSEffectShaderProperty *block)
	{
		nifmodel *model = (nifmodel *)rd->data;
		Geometry *geometry = model->lastGroup->geometry;
		if (geometry)
		{
			Material *material = geometry->material;
			material->name += "EffectShader";
			material->zwrite = false;
			material->src = &fxs;
			material->transparent = true;
			material->color = cast_vec3(&block->D->base_color);
			material->opacity = block->D->base_color_scale;
			material->map = GetProduceTexture(block->source_texture);
			if (block->B->shader_flags_2 & 0x00000020)
				material->vertexColors = true;
			if (block->B->shader_flags_1 & 0x80000000)
				material->testing = true;
			if (block->B->shader_flags_2 & 0x00000001)
				material->zwrite = true;
			if (block->B->shader_flags_2 & 0x00000010)
				material->doubleSided = true;
		}
	}

	void bs_shader_texture_set_callback(RD rd, BSShaderTextureSet *block)
	{
		// printf("bs shader texture set callback\n");
		nifmodel *model = (nifmodel *)rd->data;
		group_type *group = model->lastGroup;
		Geometry *geometry = group->geometry;
		if (geometry)
		{
			Material *material = geometry->material;
			material->name += "Textured";
			for (int i = 0; i < block->A->num_textures; i++)
			{
				//if (!i && dynamic_cast<MeshSkinned *>(model)) printf("%s\n", block->textures[i]);
				std::string path = std::string(block->textures[i]);
				if (path.empty())
					continue;
				if (path.find("skyrimhd\\build\\pc\\data\\") != std::string::npos)
					path = path.substr(23, std::string::npos);
				if (i == 0)
					material->map = GetProduceTexture(path.c_str());
				if (i == 1)
					material->normalMap = GetProduceTexture(path.c_str());
				if (i == 2)
					material->glowMap = GetProduceTexture(path.c_str());
			}
		}
	}

	void ni_alpha_property_callback(RD rd, NiAlphaProperty *block)
	{
		nifmodel *model = (nifmodel *)rd->data;
		group_type *group = model->lastGroup;
		Geometry *geometry = group->geometry;
		if (geometry)
		{
			Material *material = geometry->material;
			const int blendModes[] = {
				GL_ONE,
				GL_ZERO,
				GL_SRC_COLOR,
				GL_ONE_MINUS_SRC_COLOR,
				GL_DST_COLOR,
				GL_ONE_MINUS_DST_COLOR,
				GL_SRC_ALPHA,
				GL_ONE_MINUS_SRC_ALPHA,
				GL_DST_ALPHA,
				GL_ONE_MINUS_DST_ALPHA,
				GL_SRC_ALPHA_SATURATE};
			const int testModes[] = {
				GL_ALWAYS,
				GL_LESS,
				GL_EQUAL,
				GL_LEQUAL,
				GL_GREATER,
				GL_NOTEQUAL,
				GL_GEQUAL,
				GL_NEVER};
			unsigned short flags = block->C->flags;
			material->blending = (bool)(flags & 1);
			material->testing = (bool)(flags & (1 << 9));
			int src = blendModes[flags >> 1 & 0x0f];
			int dst = blendModes[flags >> 5 & 0x0f];
			material->blendFunc = {src, dst};
			material->treshold = block->C->treshold / 255.f;
			material->testFunc = testModes[flags >> 10 & 0x07];
		}
	}
	double easeInOutQuad( double t ) {
		return t < 0.5 ? 2 * t * t : t * (4 - 2 * t) - 1;
	}

	void nifmodel::Misty()
	{
		// mists
		auto callback = [](RD rd, BSEffectShaderPropertyFloatController *block) {
			nifmodel *model = (nifmodel *)rd->data;
			auto target = (BSEffectShaderProperty *)nif_get_block(rd->nif, block->A->target);
			//auto shape = (bs_tri_shape *)nif_get_block(rd->nif, target->meta.parent);
			group_type *group = nullptr;
			auto next_controller = block;
			while(next_controller)
			{
				auto controller = next_controller;
				next_controller = nullptr;
				if (controller->A->next_controller > -1)
					next_controller = (BSEffectShaderPropertyFloatController *)nif_get_block(rd->nif, controller->A->next_controller);
				if (target)
					group = model->groups[target->meta.parent];
				if (!group || !group->geometry)
					return;
				controller->meta.time += delta * 1.0;
				if (controller->meta.time > controller->A->stop_time)
					controller->meta.time -= controller->A->stop_time;
				float *uv = &target->meta.u;
				if (controller->A->controlled_variable == 6)
					uv = &target->meta.u;
				else if (controller->A->controlled_variable == 8)
					uv = &target->meta.v;
				auto interpolator = (NiFloatInterpolator *)nif_get_block(rd->nif, controller->A->interpolator);
				if (controller->A->interpolator)
				{
					if (interpolator->A->data)
					{
						auto data = (NiFloatData *)nif_get_block(rd->nif, interpolator->A->data);
						for (unsigned int i = data->A->num_keys; i-- > 0;)
						{
							int j = (i + 1 >= data->A->num_keys) ? 0 : i + 1;
							if (data->linear_keys)
							{
								auto one = &data->linear_keys[i];
								auto two = &data->linear_keys[j];
								if (one->time > controller->meta.time)
									continue;
								float ratio = (controller->meta.time - two->time) / (two->time - one->time) + 1.0f;
								ratio = ratio < 0 ? 0 : ratio > 1 ? 1 : ratio;

								float a = one->value * (1.0f - ratio);
								float b = two->value * ratio;
								*uv = a + b;
								break;
							}

							else if (data->quadratic_keys)
							{
								auto one = &data->quadratic_keys[i];
								auto two = &data->quadratic_keys[j];
								if (one->time > controller->meta.time)
									continue;
								float ratio = (controller->meta.time - two->time) / (two->time - one->time) + 1.0f;
								ratio = ratio < 0 ? 0 : ratio > 1 ? 1 : ratio;

								float x2 = ratio * ratio;
								float x3 = x2 * ratio;

								float x = easeInOutQuad(ratio);
								*uv = (one->value * x) + (two->value * (1.0 - x));
								break;
							}
						}
					}
				}

				Material *material = group->geometry->material;
				
				float u, v, s, t;
				u = target->meta.u + target->B->uv_offset.x;
				v = target->meta.v + target->B->uv_offset.y;
				s = 1.0f / target->B->uv_scale.x;
				t = 1.0f / target->B->uv_scale.y;
				
				material->setUvTransformDirectly(u, v, s, t, 0, 0, 0);
			}
		};
		Rd *rd = calloc_nifprd();
		rd->nif = model;
		rd->data = this;
		rd->bs_effect_shader_property_float_controller_callback = callback;
		nif_rd(rd);
		free_nifprd(&rd);
	}
}