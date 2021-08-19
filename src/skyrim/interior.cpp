#include <skyrim_units>

#include "skyrim.h"
#include "interior.h"
#include "mesh.h"
#include "grup.h"

#include <algorithm>
#include <cctype>
#include <string>

#include <renderer/types.h>
#include <renderer/texture.h>
#include <renderer/camera.h>
#include <renderer/lights.h>

namespace skyrim
{
	Interior::Interior(const char *editorId)
	{
		this->editorId = editorId;
		Group *group = new Group();
	}

	void Interior::loadcell()
	{
		cell = find_cell_loop(editorId);
		parsegrup(8, cell.persistent);
		parsegrup(9, cell.temporary);
	}

	static void PlaceCameraDud(Interior *);

	void Interior::parsegrup(int group_type, Grup wgrp)
	{
		if (!wgrp.valid())
			return;
		// printf("loop cell subgroup %i\n", group_type);
		wgrp.foreach(group_type, [&](unsigned int i) {
			Record wrcd = wgrp.get<record *>(i);
			if (wrcd.sig(REFR))
			{
				Ref *ref = new Ref(wrcd.rcd);
				refs.push_back(ref);
				const char *name = wrcd.editorId();
				if (name)
					editorIds.emplace(name, ref);
				if (ref->baseObject.valid())
					if(ref->baseObject.sigany( { WEAP, MISC } ))
						lootables.push_back(ref);
					else if (ref->baseObject.sig( MSTT ))
						mstts.push_back(ref);
			}
			return false;
		});
		placecamera();
	}

	void parsegrup_no_wrapper(cgrupp grp)
	{
		for (unsigned int i = 0; i < grp->mixed->size; i++)
		{
			crecordp rcd = (crecordp)grp->mixed->elements[i];
			if (rcd->hed->sgn == *(unsigned int *)REFR)
			{
				// make Ref
			}
		}
	}
	
	void Interior::placecamera()
	{
		// Place the camera at the first XMarker you find
		if (alreadyTeleported)
			return;
		Grup wgrp = cell.persistent;
		wgrp.foreach(8, [&](unsigned int i) {
			Record wrcd = wgrp.get<record *>(i);
			if (*(wrcd.base()) == 0x0000003B)
			{
				float *locationalData = wrcd.data<float *>("DATA");
				// printf("found random xmarker for camera\n");
				personCam->pos = *cast_vec_3(locationalData);
				personCam->pos.z += EYE_HEIGHT;
				personCam->yaw = cast_vec_3(locationalData + 3)->z;
				alreadyTeleported = true;
				return true;
			}
			return false;
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
		std::vector<Ref *> closest = lootables;
		std::sort(lootables.begin(), lootables.end(), myfunction);
		
		for (Ref *ref : closest)
			if (ref->displayAsItem())
				return;

		for (Ref *mstt : mstts)
			mstt->step();
	}

} // namespace dark