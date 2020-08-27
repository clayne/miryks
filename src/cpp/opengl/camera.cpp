#include "camera"

#include "dark2.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/string_cast.hpp>

Camera::Camera()
{
	view = mat4(1);
	pos = vec3(0);
	disabled = false;
}

FirstPersonCamera::FirstPersonCamera() : Camera()
{
	eye = vec3(0);
	w = a = s = d = r = f = false;
	shift = false;
}

void FirstPersonCamera::Mouse(float x, float y)
{
	const float sensitivity = .001f;
	fyaw += x * sensitivity;
	fpitch -= y * sensitivity;
}

void FirstPersonCamera::Update(float time)
{
	if (disabled)
		return;

	Move(time);

	while (fyaw > 2 * pif)
		fyaw -= 2 * pif;
	while (fyaw < 0)
		fyaw += 2 * pif;

	fpitch = fmaxf(-pif, fminf(0, fpitch));

	view = mat4(1.0f);
	view = rotate(view, fpitch, vec3(1, 0, 0));
	view = rotate(view, fyaw, vec3(0, 0, 1));
	view = translate(view, -pos - eye);

	float aspect = (float)dark2::width / (float)dark2::height;

	projection = perspective(
		radians(fzoom),
		aspect,
		0.1f,
		10000.0f);
}

void FirstPersonCamera::Move(float time)
{
	auto forward = [&](float n) {
		pos.x += n * sin(fyaw);
		pos.y += n * cos(fyaw);
	};

	auto strafe = [&](float n) {
		pos.x += n * cos(-fyaw);
		pos.y += n * sin(-fyaw);
	};

	float speed = 500 * time;

	if (shift)
		speed /= 10;

	if (w && !s)
		forward(speed);
	if (s && !w)
		forward(-speed / 2);

	if (a && !d)
		strafe(-speed);
	if (d && !a)
		strafe(speed);

	if (r)
		pos.z += speed / 2;
	if (f)
		pos.z -= speed / 2;
}