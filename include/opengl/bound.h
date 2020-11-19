#ifndef OPENGL_BOUND_H
#define OPENGL_BOUND_H

#include <opengl/types.h>
#include <opengl/aabb.h>

struct Bound
{
	Bound(Group *);
	~Bound();
	
	void Update();
	void Draw(const mat4 &);

	Group * const group;

	AABB aabb, obb;
};

#endif