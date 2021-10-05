#include <png.h>

#include <dark/dark.h>
#include <dark/files.h>

#include <dark/player.h>

#include <skyrim/cell.h>
#include <skyrim/actors.h>

#include <gooey/yagrum.h>

using namespace dark;
using namespace skyrim;

namespace dark
{
	Monster *someDraugr = nullptr, *meanSkelly = nullptr;
	Char *someHuman = nullptr;
}

namespace dark
{
	ESP Dark = NULL;
	Player *player1 = nullptr;
	std::map<const char *, int> keys;
}

void darkassert(bool e)
{
	assertc(e);
}

void load_bucket()
{
	in_place_viewer(get_res("clutter\\bucket02a.nif"));
	yagrum_queue("", 10, true);
}

void load_gloomgen()
{
	dungeon = GetInterior("GloomGen", 5);
	dungeon->Load();
	//gworldSpace = GetWorldSpace("DarkWorld", 5);
	//player1 = new Player();
}

void load_plugins_archives()
{
	get_plugins()[0] = load_plugin(PLUGIN_0);
	get_plugins()[1] = load_plugin(PLUGIN_1);
	get_plugins()[2] = load_plugin(PLUGIN_2);
	get_plugins()[3] = load_plugin(PLUGIN_3);
	get_plugins()[4] = load_plugin(PLUGIN_4);
	get_plugins()[5] = load_plugin(PLUGIN_5, true);
	get_archives()[0] = load_archive(ARCHIVE_0);
	//get_archives()[1] = load_archive(ARCHIVE_1);
	//get_archives()[2] = load_archive(ARCHIVE_2);
	get_archives()[3] = load_archive(ARCHIVE_3);
	get_archives()[4] = load_archive(ARCHIVE_4);
	get_archives()[5] = load_archive(ARCHIVE_5);
	//get_archives()[6] = load_archive(ARCHIVE_6);
	//get_archives()[7] = load_archive(ARCHIVE_7);
	get_archives()[8] = load_archive(ARCHIVE_8);
	get_archives()[9] = load_archive(ARCHIVE_9);
	get_archives()[10] = load_archive(ARCHIVE_10);
	get_archives()[11] = load_archive(ARCHIVE_11);
	get_archives()[12] = load_archive(ARCHIVE_12);
	get_archives()[13] = load_archive(ARCHIVE_13);
	get_archives()[14] = load_archive(ARCHIVE_14);
	get_archives()[15] = load_archive(ARCHIVE_15);
	get_archives()[16] = load_archive(ARCHIVE_16);
	get_archives()[17] = load_archive(ARCHIVE_17);
	assertc(get_plugins()[0]);
	assertc(get_plugins()[1]);
	assertc(get_plugins()[2]);
	assertc(get_plugins()[3]);
	assertc(get_plugins()[4]);
	assertc(get_plugins()[5]);
}

#include <renderer/camera.h>
#include <renderer/rendertarget.h>

int main()
{
	char **buf = &editme;
	fbuf("editme.txt", buf, true);
	goingrate();
	load_yagrum();
	load_plugins_archives();
	Dark = get_plugins()[5];
	nif_test();
	renderer_init();
	load_bucket();
	refs_init();
	put_it_fullscreen();
	load_gloomgen();
	someDraugr = new Monster("DraugrRace", "actors\\draugr\\character assets\\draugrmale06.nif");
	someDraugr->SetAnim("anims/draugr/alcove_wake.kf");
	someDraugr->Place("gloomgendraugr");
	//someDraugr = new Monster("DraugrRace", "actors\\dlc02\\hulkingdraugr\\hulkingdraugr.nif");
	//meanSkelly = new BodyPart("DraugrRace", "actors\\draugr\\character assets\\draugrskeleton.nif");
	//meanSkelly->PutDown("gloomgenskeleton");
	someHuman = new Char();
	someHuman->SetAnim("anims/character/idlewarmhands_crouched.kf");
	someHuman->Place("gloomgenman");
	//someHuman->SetAnim("anims/character/1hm_idle.kf");
	player1 = new Player();
	program_while();
	return 1;
}

void dark::reload_dark_esp()
{
	ESP *plugin = &get_plugins()[5];
	free_plugin(plugin);
	*plugin = load_plugin(PLUGIN_5, true);
}

void dark::reload_dungeon_in_place()
{
	if (dungeon)
	{
		const char *edId = dungeon->edId;
		delete dungeon;
		dungeon = GetInterior(edId, 5);
		dungeon->alreadyTeleported = true;
		dungeon->Load();
	}
}

#include <skyrim/model.h>

#include <renderer/scene.h>
#include <renderer/group.h>
#include <renderer/drawgroup.h>

void dark::in_place_viewer(RES res)
{
	static Model *model = nullptr;
	static DrawGroup *drawGroup = nullptr;
	if (model)
	{
		drawGroup->parent->Remove(drawGroup);
		delete model;
		delete drawGroup;
	}
	model = new Model(res);
	drawGroup = new DrawGroup(
		model->baseGroup, translate(mat4(1.0), personCam->pos));
	sceneDef->bigGroup->Add(drawGroup);
	hide_cursor();
	cameraCur = viewerCam;
	viewerCam->pos = drawGroup->aabb.center();
	//viewerCam->pos = personCam->pos;
	viewerCam->radius = drawGroup->aabb.radius2() * 2;
}