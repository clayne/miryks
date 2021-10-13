#include <skyrim/skyrim.h>
#include <skyrim/cell.h>
#include <skyrim/grup.h>

#include <algorithm>
#include <cctype>
#include <string>

#include <renderer/renderer.h>
#include <renderer/camera.h>
#include <renderer/scene.h>

namespace skyrim
{
	WorldSpace *worldSpace = nullptr;
	
	WorldSpace *get_world_space(const char *id, int num)
	{
		printf("get_world_space\n");
		WorldSpace *worldSpace = nullptr;
		wrld temp;
		bool ok = wrld::top().loop([&](wrld::iter &g) {
			return (temp = g.typesake()).self.editor_id(id);
		}, 0);
		if (ok)
			worldSpace = new WorldSpace(temp);
		return worldSpace;
	}

	WorldSpace::WorldSpace(wrld &e) : WorldSpace(e.self, e.childs) { }

	WorldSpace::WorldSpace(Grup<> &g) : WorldSpace(g.record(), g.grup()) {}

	WorldSpace::WorldSpace(Record wrld, Grup<> grup)
		: Record(wrld),
		grup(grup)
	{
		assertc(grup.hed().group_type == WorldChildren);
		sceneDef->ambient = vec3(127.f / 255.f);
		printf("new WorldSpace: %s\n", data<const char *>("FULL"));
		formId xlcn = data<formId>("XLCN");
		int16_t *wctr = data<int16_t *>("WCTR");
		int32_t *nam0 = data<int32_t *>("NAM0");
		int32_t *nam9 = data<int32_t *>("NAM9");
	}

	WorldSpace *WorldSpace::Init()
	{
		sceneDef->ambient = vec3(180.0 / 255.0);
		personCam->pos = vec3(0, 0, -2048.0);
		DiscoverAllCells();
		LoadExterior(0, 0);
		return this;
	}

	void WorldSpace::DiscoverAllCells()
	{
		printf("DiscoverAllCells\n");
		grup.index = 2;
		grup.dive([&](Grup<> &g) {
			Record cell = g.record();
			Grup<> grup = g.grup();
			Exterior *exterior = new Exterior(cell, grup);
			exterior->worldSpace = this;
			exteriors.push_back(exterior);
			return false;
		}, {
			WorldChildren,
			ExteriorCellBlock,
			ExteriorCellSubBlock
		});
	}

	void WorldSpace::LoadExterior(int x, int y)
	{
		for (Exterior *exterior : exteriors)
		{
			if (exterior->xclc->x == x && exterior->xclc->y == y)
			{
				printf("loading exterior %i %i\n", x, y);
				exterior->Init();
			}
		}
	}

	void Exterior::Init()
	{
		Subgroup(persistent, CellPersistentChildren);
		Subgroup(temporary, CellTemporaryChildren);
	}
	
	void Exterior::Subgroup(Grup<> subgroup, int group_type)
	{
		if (!subgroup.valid()) {
			return;
		}
		subgroup.loop([&](Grup<> &g) {
			Record refr = g.record();
			if (refr.is_type(REFR))
			{
				Reference *reference = new Reference(refr);
				reference->cell = (Cell *)this;
				worldSpace->references.push_back(reference);
				if (refr.editor_id("darkworldheading"))
				{
					float *loc = refr.data<float *>("DATA");
					personCam->pos = cast_vec3(loc);
					personCam->pos.z += EYE_HEIGHT;
					personCam->yaw = cast_vec3(loc + 3).z;
					cameraCur = personCam;
				}
			}
			else if (refr.is_type(LAND))
			{
				assertc(land == nullptr);
				land = new Land(refr);
				land->exterior = this;
			}
			return false;
		}, group_type);
	}

}