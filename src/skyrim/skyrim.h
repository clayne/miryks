#pragma once

#include <skyrim/grup.h>

namespace skyrim
{
	extern char *editme;
	
	class Record;
	class Grup;

	typedef Record Rcd;

	class Char;
	class Monster;
	class Skel;
	class Bone;
	class Keyf;
	class Anim;

	class Model;
	class ModelSkinned;
	class SkinnedMesh;

	class Interior;
	class Container;

	void ext_nif_save(const char *, NIF);
	NIF ext_nif_saved(const char *);
	std::map<const char *, NIF> &ext_nif_map();

	RES get_res(
		const char *,
		const char * = "meshes\\",
		unsigned long = 0x1);

	NIF get_nif(RES);
	NIF get_nif(const char *);

	ESP load_plugin(const char *, bool = false);
	BSA load_archive(const char *);

	void load_these_definitions(ESP);

	Keyf *load_keyframes_from_disk(const char *);

	struct CellCapture
	{
		Record wrcd;
		Grup persistent, temporary;
	};
	Record get_race(const char *);
	Record get_world_space(const char *);
	CellCapture get_interior_cell(const char *, int);

}