#include <dark/files.h>

#include <skyrim/skeleton.h>
#include <skyrim/record.h>

#include <renderer/group.h>
#include <renderer/renderer.h>

#include <glm/gtc/quaternion.hpp>
#include <algorithm>

using namespace dark;

namespace skyrim
{
	static void ni_node_callback(RD, NiNode *);

	Keyf *get_keyframes(const char *path)
	{
		return load_keyframes_from_disk(path);
	}

	Skel::Skel()
	{
		anim = nullptr;
		model = nullptr;
		bones[-1] = new Bone();
	}

	Skel::Skel(Record race) : Skel()
	{
		printf("skeleton anam %s\n", race.data<char *>("ANAM"));
		model = get_nif(race.data<char *>("ANAM"));
		Construct();
	}

	void Skel::Construct()
	{
		RD rd = calloc_nifprd();
		rd->nif = model;
		rd->data = this;
		rd->ni_node_callback = ni_node_callback;
		nif_rd(rd);
		free_nifprd(&rd);
		assertc(root);
		root->Update();
	}

	void matrix_from_common(Bone *bone, ni_common_layout_t *common)
	{
		bone->matrix = translate(bone->matrix, cast_vec3(&common->A->translation));
		bone->matrix *= inverse(mat4(cast_mat3(&common->A->rotation)));
		bone->matrix = scale(bone->matrix, vec3(common->A->scale));
		bone->Update();
		bone->rest = bone->matrixWorld;
	}

	Bone *Skel::MakeBoneHere(RD rd, NiNode *block)
	{
		//printf("bone name is %s\n", bone->name);
		Bone *bone = new Bone();
		bone->block = block;
		bone->name = nif_get_string(rd->nif, block->common->F->name);
		bones[rd->current] = bone;
		bones[rd->parent]->Add(bone);
		bonesNamed[bone->name] = bone;
		matrix_from_common(bone, block->common);
		if (strstr(bone->name, "[Root]")) {
			root = bone;
		}
		return bone;
	}

	void ni_node_callback(RD rd, NiNode *block)
	{
		Skel *skel = (Skel *)rd->data;
		Bone *bone = skel->MakeBoneHere(rd, block);
	}

	void Skel::Step()
	{
		if (anim)
			anim->Step();
	}

	Keyf::Keyf(NIF nif) : nif(nif)
	{
		loop = true;
		controllerSequence = nullptr;
		if (nif == nullptr)
			return;
		assertc(strcmp(nif->hdr->block_types[0], NiControllerSequenceS) == 0);
		controllerSequence = (NiControllerSequence *)nif->blocks[0];
	}

	Anim::Anim(Keyf *keyf) : keyf(keyf)
	{
		skel = nullptr;
		time = 0;
		play = true;
		if (keyf == nullptr)
			play = false;
	}

	void Anim::Step()
	{
		if (!play)
			return;
		time += delta;
		if (time >= keyf->controllerSequence->C->stop_time)
			time -= keyf->controllerSequence->C->stop_time;
		SimpleNonInterpolated();
		skel->root->Update();
	}

	void Anim::SimpleNonInterpolated()
	{
		NIF model = keyf->nif;
		struct controlled_block_t *cbp;
		for (unsigned int i = 0; i < keyf->controllerSequence->A->num_controlled_blocks; i++)
		{
			// Match node_name to a skeleton bone
			cbp = &keyf->controllerSequence->controlled_blocks[i];
			char *name = nif_get_string(model, cbp->node_name);
			auto has = skel->bonesNamed.find(name);
			if (has == skel->bonesNamed.end())
			{
				//printf("cant find bone %s\n", name);
				// cant find shield, weapon, quiver
				continue;
			}

			Bone *bone = has->second;
			auto tip = (NiTransformInterpolator *)nif_get_block(model, cbp->interpolator);
			auto tdp = (NiTransformData *)nif_get_block(model, tip->B->data);
			if (tip == NULL || tdp == NULL)
				continue;
			vec4 ro = cast_vec4(&tip->transform->rotation);
			int num = tdp->A->num_rotation_keys;
			if (num)
			{
				for (int i = num - 1; i >= 0; i--)
				{
					auto key = &tdp->quaternion_keys[i];
					//printf("qk %i time %f\n", i, key->time);
					if (key->time <= time || num == 1)
					{
						ro = cast_vec4(&key->value);
						//printf("ro.x%f\n", ro.x);
						break;
					}
				}
			}
			// else if (num == 1)
			// 	ro = *cast_vec_4((float *)(&tdp->quaternion_keys[0].value));

			vec3 tr = cast_vec3(&tip->transform->translation);
			num = tdp->translations->num_keys;
			if (num)
			{
				for (int i = num - 1; i >= 0; i--)
				{
					auto key = &tdp->translation_keys[i];
					if (key->time <= time || num == 1)
					{
						//printf("tr time %f\n", key->time);
						tr = cast_vec3(&key->value);
						break;
					}
				}
			}
			// else if (num == 1)
			// 	tr = gloomVec3(tdp->translation_keys[0].value);

			quat qu = quat(ro[0], ro[1], ro[2], ro[3]);

			mat4 matrix = mat4_cast(qu);
			matrix[3] = vec4(tr, 1);

			bone->mod = matrix;
			bone->matrix = matrix;
			bone->diff = inverse(matrix) * bone->rest;
			bone->Update();
		}
	}

	
} // namespace dark
