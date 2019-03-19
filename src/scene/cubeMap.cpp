#include "cubeMap.h"
#include "ray.h"
#include "../ui/TraceUI.h"
#include "../scene/material.h"
#include <iostream>
extern TraceUI* traceUI;

glm::dvec3 CubeMap::getColor(ray r) const
{
	// YOUR CODE HERE
	// FIXME: Implement Cube Map here
	glm::dvec3 dir = r.getDirection();
	double x = dir[0], y = dir[1], z = dir[2];

	double absX = std::abs(x);
	double absY = std::abs(y);
	double absZ = std::abs(z);

	bool x_positive = x > 0, y_positive = y > 0, z_positive = z > 0;

	double max_axis, uc, vc;
	int index = -1;
	if(x_positive && absX >= absY && absX >= absZ)
	{
		max_axis = absX;
		uc = z;
		vc = y;
		index = 0;
	}
	else if (!x_positive && absX >= absY && absX >= absZ)
	{
		max_axis = absX;
		uc = -z;
		vc = y;
		index = 1;
	}
	else if (y_positive && absY >= absX && absY >= absZ) {
		max_axis = absY;
		uc = x;
		vc = -z;
		index = 2;
	}

	else if (!y_positive && absY >= absX && absY >= absZ) {
		max_axis = absY;
		uc = x;
		vc = z;
		index = 3;
	}

	else if (z_positive && absZ >= absX && absZ >= absY) {
		max_axis = absZ;
		uc = -x;
		vc = y;
		index = 5;
	}

	else if (!z_positive && absZ >= absX && absZ >= absY) {
		max_axis = absZ;
		uc = x;
		vc = y;
		index = 4;
	}
	double u = 0.5 * (uc / max_axis + 1.0);
	double v = 0.5 * (vc / max_axis + 1.0);
	return tMap[index]->getMappedValue(glm::dvec2(u, v));
	// return glm::dvec3();
}

CubeMap::CubeMap()
{
}

CubeMap::~CubeMap()
{
}

void CubeMap::setNthMap(int n, TextureMap* m)
{
	if (m != tMap[n].get())
		tMap[n].reset(m);
}
