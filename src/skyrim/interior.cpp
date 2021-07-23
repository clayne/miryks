#include <skyrim_units>
#include <lib.h>

#include "interior.h"
#include "mesh.h"
#include "record.h"
#include "recordarray.h"

#include <algorithm>
#include <cctype>
#include <string>

// #include <renderer/renderable.h>
#include <renderer/texture.h>
#include <renderer/camera.h>
#include <renderer/lights.h>
#include <renderer/types.h>

namespace dark
{
	Interior::Interior(const char *edid)
	{
		editorId = edid;
		Group *group = new Group();
	}

	void Interior::loadCell()
	{
		loadedCell = getCell(editorId);
		parseGrup(loadedCell, loadedCell.persistent);
		parseGrup(loadedCell, loadedCell.non_persistent);
	}

	Cell Interior::getCell(const char *name)
	{
		Cell cell;
		ObjectArray A, B, C;
		Grup *top = esp_top_grup(get_plugins()[1], "CELL");
		A(top).foreach([&](unsigned int i) {
			B(A.getgrup(i)).foreach([&](unsigned int j) {
				C(B.getgrup(j)).foreach([&](unsigned int &k) {
					Object object(C.getrecord(k));
					Grup *grup = C.getgrup(k + 1);
					const char *editorId = getEditorId(object);
					if (0 == strcmp(name, editorId))
					{
						ObjectArray D(grup);
						cell.record = object.record;
						// printf("foreach found your interior `%s`\n", editorId);
						cell.persistent = D.size() >= 1 ? D.getgrup(0) : 0;
						cell.non_persistent = D.size() >= 2 ? D.getgrup(1) : 0;
						A.stop = B.stop = C.stop = true;
					}
					k += 1;
				});
			});
		});
		return cell;
	}

	static void PlaceCameraDud(Interior *);

	void Interior::parseGrup(Cell &cell, Grup *grup)
	{
		if (grup == nullptr)
			return;
		ObjectArray array;
		array(grup).foreach([&](unsigned int i) {
			Record *record = array.getrecord(i);
			Object object(record);
			if (object.isType("REFR"))
			{
				Ref *ref = new Ref(record);
				refs.push_back(ref);
				const char *editorId = getEditorId(object);
				if (editorId)
					editorIds.emplace(editorId, ref);
				if (ref->baseObject.valid() && ref->baseObject.isTypeAny({"WEAP", "MISC"}))
				{
					iterables.push_back(ref);
				}
			}
		});
		placeCamera();
	}

	void Interior::placeCamera()
	{
		if (alreadyTeleported)
			return;
		ObjectArray array;
		array(loadedCell.persistent).foreach([&](unsigned int i) {
			Object object(array.getrecord(i));
			if (*getBaseId(object) == 0x0000003B) //  "Marker"
			{
				// Place at any XMarker
				float *locationalData = object.data<float *>("DATA");
				// printf(" xmarker ! \n");
				first_person_camera->pos = *cast_vec_3(locationalData);
				first_person_camera->pos.z += EYE_HEIGHT;
				first_person_camera->yaw = cast_vec_3(locationalData + 3)->z;
				alreadyTeleported = true;
				array.stop = true;
			}
		});
	}

	Interior::~Interior()
	{
		unload();
	}

	void Interior::unload()
	{
		for (auto it = refs.begin(); it != refs.end(); ++it)
		{
			Ref *ref = *it;
			delete ref;
		}
	}

	bool myfunction(Ref *l, Ref *r)
	{
		return l->getDistance() < r->getDistance();
	}

	void Interior::update()
	{
		std::vector<Ref *> closest = iterables;
		std::sort(iterables.begin(), iterables.end(), myfunction);

		for (auto it = closest.begin(); it != closest.end(); ++it)
		{
			Ref *ref = *it;

			if (ref->displayAsItem())
				return;
		}
	}

} // namespace dark