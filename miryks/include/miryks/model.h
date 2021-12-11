#pragma once

#include <miryks/miryks.h>

#include <opengl/renderer.h>
#include <opengl/scene.h>
#include <opengl/group.h>
#include <opengl/geometry.h>
#include <opengl/material.h>

#include "skeleton.h"

#define callback(x) void (x ## _callback) (RD, x ## _t *)

namespace miryks
{
	callback(ni_node);
	callback(bs_lighting_shader_property);
	callback(bs_effect_shader_property);
	callback(bs_effect_shader_property_float_controller);
	callback(ni_float_interpolator);
	callback(ni_float_data);
	callback(bs_shader_texture_set);
	callback(ni_alpha_property);
	callback(ni_skin_instance);
	callback(ni_skin_data);
	callback(ni_skin_partition);
	callback(bs_tri_shape);

	void matrix_from_common(group_type *, ni_common_layout_t *);

	class nifmodel
	{
	public:
		nifmodel();
		nifmodel(Res *);
		~nifmodel();
		Nif *model = nullptr;
		std::map<int, group_type *> groups;
		group_type *baseGroup, *lastGroup;
		group_type *MakeNewGroup(Rd *);
		void Construct();
		void Step();
		void Misty();
		virtual void v () {};
		
		std::vector<NiRef> shapes__;
	};

	class ModelSkinned : public nifmodel
	{
	public:
		NiRef lastShape;
		ModelSkinned(const char *);
		void Construct();
		void Step(Skel *);
		void Initial(Skel *);
	};

	class SkinnedMesh
	{
	public:
		ModelSkinned *modelSkinned;
		Skel *skel;
		Anim *animation;
		drawgroup *drawGroup;
		SkinnedMesh(const char *);
		~SkinnedMesh();
		void Step();
	};
}

#undef callback