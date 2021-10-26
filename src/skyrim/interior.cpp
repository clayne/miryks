#include <skyrim/skyrim.h>
#include <skyrim/cell.h>
#include <skyrim/model.h>
#include <skyrim/grup.h>

#include <algorithm>
#include <cctype>
#include <string>
#include <ranges>

#include <renderer/renderer.h>
#include <renderer/texture.h>
#include <renderer/camera.h>
#include <renderer/lights.h>

namespace skyrim
{
	Interior *dungeon = nullptr;

	Interior *get_interior(const char *id, int num)
	{
		closure<record_and_grup> target;
		(grup<grup<grup<any,
			3>,2>,0> (cells)) <= (target, id);
		return new Interior(target.match);
	}

	Interior::Interior(record_and_grup &rng) : Cell(rng.one, rng.two)
	{
		printf("persistent n: %i\n", persistent.mixed().size);
		printf("temporary n: %i\n", temporary.mixed().size);
		sceneDef->ambient = vec3(50.f / 255.f);
	}

	Interior::~Interior()
	{
		Unload();
	}

	Interior *Interior::Init()
	{
		//Sift(persistent, CellPersistentChildren);
		Sift(temporary, CellTemporaryChildren);
		PutCam();
		return this;
	}

	void Interior::Unload()
	{
		for (Reference *ref : refs)
			delete ref;
	}

	auto thingsWithLabels = {Doors, Furniture, Books, Containers, Armor, Weapons, Ammo, Misc, Alchemy, Ingredients};

	struct maker : record
	{
		bool operator<=(
			closure<maker> &rhs)
		{
			Interior *interior = (Interior *)rhs.pointer;
			if (this->is_type(REFR))
			{
				Reference *reference = new Reference(*this);
				interior->refs.push_back(reference);
				if (this->editor_id())
					interior->edIds.emplace(this->editor_id(), reference);
				/*if (reference->baseObject.valid())
				{
					if (reference->baseObject.is_types( thingsWithLabels ))
						Refs::labelled.push_back(reference);
					else if (reference->baseObject.is_type( MSTT ))
						interior->mstts.push_back(reference);
				}*/
			}
			return false;
		}
		using record::record; // needed for template inheritance
	};

	void Interior::Sift(grup<> &subgroup, int group_type)
	{
		closure<maker> make_records((void *)this);
		subgroup.rewind();
		subgroup <= make_records;
	}

	void Interior::PutCam()
	{
		for (auto ref : refs | std::views::reverse)
		{
			if (*ref->base() == 0x00000032) // coc marker heading
			{
				float *locationalData = ref->data<float *>("DATA");
				personCam->pos = cast_vec3(locationalData);
				personCam->pos.z += EYE_HEIGHT;
				personCam->yaw = cast_vec3(locationalData + 3).z;
				dontTeleport = true;
				break;
			}
		}
#if 0
		if (dontTeleport)
			return;
		if (!persistent.valid())
			return;
		closure<visitor> me;
		me.pointer = (void*)this;
		subgroup.rewind();
		subgroup(me);
		persistent.loop([&](any &temp) {
			Record record = temp.u.r;
			if (*(record.base()) == 0x0000003B)
			{
				// printf("found random xmarker for camera\n");
				float *locationalData = record.data<float *>("DATA");
				personCam->pos = cast_vec3(locationalData);
				personCam->pos.z += EYE_HEIGHT;
				personCam->yaw = cast_vec3(locationalData + 3).z;
				dontTeleport = true;
				return true;
			}
			return false;
		});
#endif
	}

	void Interior::Update()
	{
		Refs::Nearby();

		for (Reference *mstt : mstts)
			mstt->model->Misty();
	}
}