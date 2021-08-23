#pragma once


#include <dark/dark.h>
#include <skyrim/skyrim.h>

#include <renderer/types.h>
#include <renderer/group.h>
#include <renderer/geometry.h>

#define SHOW 1

namespace skyrim
{
	class Bone;
	class Skeleton;
	class Keyframes;
	class Animation;

	class Skeleton
	{
	public:
		Skeleton();
		Skeleton(const char *);

		std::map<NiRef, Bone *> bones;
		std::map<const std::string, Bone *> bonesNamed;

		Nif *nif;
		Bone *baseBone, *lastBone;
		Animation *animation;

		Bone *make_new_group(Rd *, int);

		void load(const char *);
		void construct();
		void step();
	};

	class Bone
	{
	public:
		Group *group;
		//std::vector<Bone *> bones;
		Bone()
		{
			group = new Group;
#if SHOW
			group->geometry = new Geometry();
			group->geometry->SetupMesh();
#endif
		};
		mat4 rest, mod, diff;
	};

	class Keyframes
	{
	public:
		Keyframes(Nif *);
		Nif *model;
		struct ni_controller_sequence_t *csp;
		bool loop = true;
	};

	class Animation
	{
	public:
		Animation(Keyframes *keyframes) : keyframes(keyframes)
		{
		}
		void step();
		void simpleNonInterpolated();
		Skeleton *skeleton = nullptr;
		float time = 0;
		bool play = true;
		Keyframes *keyframes;
	};

}