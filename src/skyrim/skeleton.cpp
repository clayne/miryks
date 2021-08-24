#include <algorithm>

#include <core/files.h>

#include "skeleton.h"

#include <renderer/group.h>
#include <renderer/types.h>

#include <glm/gtc/quaternion.hpp>

using namespace dark;

namespace skyrim
{
	static void ni_node_callback(Rd *, NiNode *);

	Keyframes *loadAnimDisk(const char *path)
	{
		Nif *nif = calloc_nifp();
		nif->path = path;
		fbuf(path, (char **)&nif->buf);
		nif_read(nif);
		ext_nif_save(nif, nif);
		Keyframes *kf = new Keyframes(nif);
		return kf;
	}

	Skeleton::Skeleton()
	{
		baseBone = new Bone();
		bones[-1] = baseBone;
		lastBone = baseBone;
		nif = nullptr;
		animation = nullptr;
	}

	Skeleton::Skeleton(const char *anam) : Skeleton()
	{
		load(anam);
		construct();
		// baseBone->group->visible = false;
	}

	void Skeleton::load(const char *anam)
	{
		// printf("skeleton load anam %s\n", anam);
		Rc *rc = load_rc("meshes\\", anam, 0x1);
		nif = import_nif(rc, true);
		printf("num_blocks of skeleton %u\n", nif->hdr->num_blocks);
	}

	void Skeleton::construct()
	{
		Rd *rd = calloc_nifprd();
		rd->nif = nif;
		rd->data = this;
		//rd->other = other;
		rd->ni_node_callback = ni_node_callback;
		nif_rd(rd);
		free_nifprd(&rd);
		baseBone->group->Update();
	}

	Bone *Skeleton::make_new_group(Rd *rd, int name)
	{
		Bone *bone = new Bone();
		bones[rd->current] = bone;
		bones[rd->parent]->group->Add(bone->group);
		bonesNamed[nif_get_string(rd->nif, name)] = bone;
		lastBone = bone;
		return bone;
	}

	void matrix_from_common(Bone *bone, ni_common_layout_t *common)
	{
		bone->group->matrix = translate(bone->group->matrix, cast_vec3(&common->A->translation));
		bone->group->matrix *= inverse(mat4(cast_mat3(&common->A->rotation)));
		bone->group->matrix = scale(bone->group->matrix, vec3(common->A->scale));
		bone->group->Update();
		bone->rest = bone->group->matrixWorld;
	}

	void ni_node_callback(Rd *rd, NiNode *block)
	{
		//printf("skelly ni node callback\n");
		Skeleton *skeleton = (Skeleton *)rd->data;
		Bone *bone = skeleton->make_new_group(rd, block->common->F->name);
		matrix_from_common(bone, block->common);
	}

	void Skeleton::step()
	{
		if (animation)
			animation->step();
	}

	// keyframes
	Keyframes::Keyframes(Nif *nif) : model(nif)
	{
		assertm(strcmp(model->hdr->block_types[0], NiControllerSequenceS) == 0, "block 0 not a controller sequence");

		csp = (NiControllerSequence *)model->blocks[0];
	}

	void Animation::step()
	{
		//printf("animstep");
		float adv = delta;
		if (play)
			time += adv;
		if (time >= keyframes->csp->C->stop_time)
			time -= keyframes->csp->C->stop_time;
		//printf("time %f\n", time);
		simpleNonInterpolated();
		skeleton->baseBone->group->Update();
		//printf("cbp %i", cbp->controller);
	}

	void Animation::simpleNonInterpolated()
	{
		Nif *model = keyframes->model;
		struct controlled_block_t *cbp;
		for (unsigned int i = 0; i < keyframes->csp->A->num_controlled_blocks; i++)
		{
			// Match node_name to a skeleton bone
			cbp = &keyframes->csp->controlled_blocks[i];
			char *name = nif_get_string(model, cbp->node_name);
			auto has = skeleton->bonesNamed.find(name);
			if (has == skeleton->bonesNamed.end())
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
			bone->group->matrix = matrix;
			bone->diff = inverse(matrix) * bone->rest;
			bone->group->Update();
		}
	}


/*
int num = tdp->A->num_rotation_keys;
			if (num > 1)
			{
				auto res = interpolate<quaternion_key, vec4>(this, num, tdp->quaternion_keys);
				//printf(" res ratio %f\n", res.ratio);
				if (res.one && res.two)
				{
					vec4 q1 = gloomVec4(res.one->value);
					vec4 q2 = gloomVec4(res.two->value);
					// todo, what does this fix? yoyo fingrs?
					//if ((q1.asVec4() * q2.asVec4()) < 0)
					//	q1 = -q1;
					q = glm::slerp(quat(q1), quat(q2), res.ratio);
				}
			}
			// used for clutched hands
			else if (num == 1)
			{
				auto key = &tdp->quaternion_keys[0];
				if (key->time <= time)
					q = quat(gloomVec4(key->value));
			}

			m = glm::mat4_cast(q);

			vec3 v = gloomVec3(tip->transform->translation);

			num = tdp->translations->num_keys;
			if (num > 0)
			{
				auto res2 = interpolate<translation_key, vec3>(this, num, tdp->translation_keys);
				if (res2.one && res2.two)
				{
					const vec3 v1 = gloomVec3(res2.one->value) * (1.0f - res2.ratio);
					const vec3 v2 = gloomVec3(res2.two->value) * res2.ratio;
					v = v1 + v2;
				}
				else if (res2.one)
					v = gloomVec3(res2.one->value);
			}

			//if (_tween && bone->_v.length())
			//{
			// if (bone->_v.length())
			//v = (v * _t_tween) + (bone->_v_2 * (1.0f - _t_tween));
			//}
			//else
			//	bone->_v_2 = v;
			*/

	// Todo, This is a big dump from Messiah (before Dark (so long before Dark2))
	// Pasted this here to see if it would compile. It won't since it's templated >_< And I can't use it because it's dumb. 
	// animation

	template <typename T, typename Y>
	struct Lol
	{
		const T *one;
		const T *two;
		float ratio;
	};

	// If loop is true then also interpolate between
	// Last and first

	template <typename T, typename Y>
	Lol<T, Y> interpolate(Animation *animation, int num, const T *keys)
	{
		Nif *nif = animation->keyframes->model;

		auto csp = animation->keyframes->csp;

		Lol<T, Y> s;
		s.one = nullptr;
		s.two = nullptr;
		s.ratio = 1;

		const auto loop = animation->keyframes->loop;

		int i, j;
		//num = keys.size();
		if (num > 1)
		{
			i = num - (loop ? 1 : 2);

			for (; i >= 0; i--)
			{
				//loop:
				const T &key_a = keys[i];
				j = i + 1;
				const bool got_two = j <= num - 1;

				if (key_a.time <= animation->time /*&& got_two && keys[j]._time >= a._t*/)
				{
					s.one = &key_a;
					if (got_two)
						s.two = &keys[j];

					// todo, this needs work
					else if (animation->keyframes->loop)
					{
						s.two = &keys[0];
					}

					break;
				}
			}
		}

		if (s.one && s.two)
		{
			float t_1 = s.one->time;
			float t_2 = s.two->time;
			// todo, does t_2 need negation here
			if (loop && t_2 < t_1)
				t_2 = t_2 - csp->C->stop_time;
			float ratio = (animation->time - t_2) / (t_2 - t_1) + 1.0f;
			s.ratio = ratio <= 0 ? 0 : ratio >= 1 ? 1 : ratio;
		}

		return s;
	}
	
} // namespace dark
