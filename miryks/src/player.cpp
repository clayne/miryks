#include <miryks/player.h>
#include <miryks/collision.h>

#include <opengl/renderer.h>
#include <opengl/camera.h>
#include <opengl/scene_graph.hpp>

namespace miryks
{
	Player::Player()
	{
		yaw = 0;
		thirdPerson = false;
		idle = false;
		race = dig_race("ImperialRace", 0);
		skel = new skeleton(race.data<char *>("ANAM"));
		orb = nullptr;
		// skel->mixer = new animation_mixer(skel);
		printf("player skel anam %s\n", race.data<char *>("ANAM"));
		// skel = new skeleton("actors\\character\\_1stperson\\skeleton.nif");
		// thirdPerson = false;
		// groupDrawer->visible = false;
		// Place("gloomgenman");
		anim = nullptr;
		head = nullptr;
		body = nullptr;
		body2 = nullptr;
		hands = nullptr;
		thirdPersonCamera = new ViewerCamera;

		// SetAnim("anims/character/1hm_1stp_run.kf");
		//  SetAnim("anims/character/1hm_walkforward.kf");

		groupDrawer = new GroupDrawer(nullptr, mat4(1.0));
		groupDrawer->name = "Player";
		// body2 = new SkinnedMesh("clothes\\prisoner\\prisonerclothes_0.nif");
		body = new SkinnedMesh("actors\\character\\character assets\\1stpersonmalebody_0.nif");
		hands = new SkinnedMesh("actors\\character\\character assets\\1stpersonmalehands_0.nif");
		body->skel = skel;
		// body2->skel = skel;
		hands->skel = skel;
		// if (body)
		//	groupDrawer->Add(body->groupDrawer);
		if (body)
			groupDrawer->Add(body->groupDrawer);
		if (hands)
			groupDrawer->Add(hands->groupDrawer);
		groupDrawer->Add(new GroupDrawer(skel->root, mat4(1.0)));
		groupDrawer->UpdateSideways();
		Place("gloomgenman");
	}

	void Player::Place(const char *q)
	{
		if (!ginterior)
			return;
		auto ref = ginterior->ids.find(q);
		if (ref == ginterior->ids.end())
			return;
		printf("place at %s\n", q);
		groupDrawer->matrix = ref->second->matrix;
		// groupDrawer->matrix = glm::translate(groupDrawer->matrix, vec3(50, 0, 0));
		groupDrawer->UpdateSideways();
		sceneDef->bigGroup->Add(groupDrawer);
		// Create an offsetted mirror of Man
		/*GroupDrawer *mirror = new GroupDrawer(groupDrawer, mat4());
		mirror->matrix = groupDrawer->matrix;
		mirror->matrix = glm::translate(mirror->matrix, vec3(50, 0, 0));
		sceneDef->bigGroup->Add(groupDrawer);*/
	}

	void Player::SetAnim(const char *path)
	{
		// if (anim)
		//	delete anim;
		keyframes *keyf = get_keyframes(path);
		skel->SetFreeze();
		anim = new animation(keyf);
		anim->skel = skel;
		skel->anim = anim;
		// skel->mixer->SetFront(anim);
	}

	void Player::Step()
	{
		if (skel)
			skel->Step();
		if (body)
			body->Step();
		if (body2)
			body2->Step();
		if (hands)
			hands->Step();

		if (pressing_key("v"))
		{
			Toggle();
		}

		if (pressing_key("w"))
		{
			idle = false;
			// printf("set anim\n");
			SetAnim("anims/character/1st/h2h_1stp_walk.kf");
		}
		else if (pressing_key("s"))
		{
			idle = false;
			SetAnim("anims/character/1st/h2h_1stp_walk.kf");
		}
		else if (pressing_key("a"))
		{
			idle = false;
			SetAnim("anims/character/1st/h2h_1stp_walk.kf");
		}
		else if (pressing_key("d"))
		{
			idle = false;
			SetAnim("anims/character/1st/h2h_1stp_walk.kf");
		}
		else if (!holding_key("w") && !holding_key("s") && !holding_key("a") && !holding_key("d"))
		{
			if (!idle)
			{
				idle = true;
				SetAnim("anims/character/1st/h2h_idle.kf");
			}
		}

		if (holding_key("w"))
		{
			if (orb)
			{
				btVector3 dir = btVector3(0, -5, 0);
				orb->rigidBody->applyCentralImpulse(dir);
			}
		}
		if (holding_key("s"))
		{
			if (orb)
			{
				btVector3 dir = btVector3(0, 5, 0);
				orb->rigidBody->applyCentralImpulse(dir);
			}
		}
		if (holding_key("d"))
		{
			if (orb)
			{
				btVector3 dir = btVector3(-5, 0, 0);
				orb->rigidBody->applyCentralImpulse(dir);
			}
		}
		if (holding_key("a"))
		{
			if (orb)
			{
				btVector3 dir = btVector3(5, 0, 0);
				orb->rigidBody->applyCentralImpulse(dir);
			}
		}
		vec3 down = vec3(0, 0, -60);
		groupDrawer->matrix = glm::translate(mat4(1.0), down + cameraCur->pos);
		if (!orb)
			orb = new collision::orb(groupDrawer);
		else
		{
			cameraCur->pos = collision::bt_to_glm(orb->get_world_transform().getOrigin());
			groupDrawer->matrix = glm::translate(mat4(1.0), down + cameraCur->pos);
		}
		// groupDrawer->matrix[3] = vec4(collision::bt_to_glm(orb->get_world_transform().getOrigin()), 1);
		// groupDrawer->matrix = glm::translate(groupDrawer->matrix, forward);
		groupDrawer->matrix = rotate(groupDrawer->matrix, -cameraCur->yaw, vec3(0, 0, 1));
		groupDrawer->UpdateSideways();
		/*groupDrawer->matrix = glm::translate(mat4(1.0), cameraCur->pos);
		groupDrawer->UpdateSideways();*/
		// groupDrawer->Reset();
	}

	void Player::Move()
	{
	}

	void Player::Toggle()
	{
		thirdPerson = !thirdPerson;

		if (thirdPerson)
		{
			// groupDrawer->visible = true;
			thirdPersonCamera->pos = cameraCur->pos;
			thirdPersonCamera->yaw = personCam->yaw;
			thirdPersonCamera->pitch = personCam->pitch;
			thirdPersonCamera->radius = 200;
			cameraCur = thirdPersonCamera;
		}
		else
		{
			// groupDrawer->visible = false;
			personCam->pos = cameraCur->pos;
			personCam->yaw = thirdPersonCamera->yaw;
			personCam->pitch = thirdPersonCamera->pitch;
			cameraCur = personCam;
		}
	}
}