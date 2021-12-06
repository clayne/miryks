#include <templates.hpp>

#include <opengl/group.h>
#include <opengl/drawgroup.h>
#include <opengl/geometry.h>
#include <opengl/material.h>
#include <opengl/camera.h>

#include <algorithm>

int DrawGroup::num = 0;
int DrawGroup::masks = ~0;

DrawGroup::DrawGroup(Group *group, mat4 matrix)
	: target(group)
{
	this->matrix = matrix;
	mask = 1 << 0;
	parent = nullptr;
	num++;
	Reset();
	Update();
}

DrawGroup::~DrawGroup()
{
	num--;
}

void DrawGroup::Reset()
{
	Cubify();
}

bool DrawGroup::Invisible()
{
	if (visible && (DrawGroup::masks & mask) == mask)
		return false;
	return true;
}

void DrawGroup::Draw(const mat4 &left)
{
	if (Invisible())
		return;
	mat4 right = left * matrixWorld;
	if (childGroups.size())
		for (Group *child : childGroups)
			if (dynamic_cast<DrawGroup *>(child))
				child->Draw(right);
	if (target)
		target->DrawChilds(right);
	DrawBounds();
}

void DrawGroup::Cubify()
{
	GroupBounded *bounded = dynamic_cast<GroupBounded *>(target);
	if (bounded)
	{
		obb = aabb = bounded->aabb;
		aabb = Aabb::mult(aabb, matrix);
		obb.geometrize();
		aabb.geometrize();
	}
}

void DrawGroup::DrawBounds()
{
	if (!target)
		return;
	mat4 place = matrix * target->matrix;
	bool notTooLarge = aabb.volume() <= renderSettings.maximumBoundingVolume;
	if (renderSettings.AABBS && aabb.geometry && notTooLarge)
		aabb.geometry->Draw(mat4(1.0));
	if (renderSettings.OBBS && obb.geometry && notTooLarge)
		obb.geometry->Draw(place);
}