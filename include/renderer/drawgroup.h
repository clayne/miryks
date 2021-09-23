#ifndef OPENGL_DRAWGROUP_H
#define OPENGL_DRAWGROUP_H

#include <functional>

#include <renderer/renderer.h>
#include <renderer/aabb.h>

struct DrawGroup : Group
{
	static int num, masks;
	int mask;
	Group *const target;
	AABB aabb, obb;
	DrawGroup(Group *, mat4);
	virtual ~DrawGroup();
	virtual void Draw(const mat4 &) override;
	bool Invisible();
	void DrawBounds();
	void Cubify();
	virtual void Reset();
};

struct DrawGroupFlatSorted : DrawGroup
{
	bool hasTransparency = false;
	DrawGroupFlatSorted(Group *, mat4);
	virtual ~DrawGroupFlatSorted(){};
	virtual void Draw(const mat4 &) override;
	virtual void Reset() override;
	void SortWith(std::function<bool(const Group *, const Group *)>);
	void SortTransparency();
};

#endif