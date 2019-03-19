#pragma once

#include <glm/vec3.hpp>
#include <iostream>
#include <algorithm>
class ray;

class BoundingBox {
	bool bEmpty;
	bool dirty;
	glm::dvec3 bmin;
	glm::dvec3 bmax;
	double bArea   = 0.0;
	double bVolume = 0.0;

public:
	BoundingBox();
	BoundingBox(glm::dvec3 bMin, glm::dvec3 bMax);

	glm::dvec3 getMin() const { return bmin; }
	glm::dvec3 getMax() const { return bmax; }
	bool isEmpty() { return bEmpty; }
	void setEmpty() { bEmpty = true; }

	void setMin(glm::dvec3 bMin)
	{
		bmin   = bMin;
		dirty  = true;
		bEmpty = false;
	}
	void setMax(glm::dvec3 bMax)
	{
		bmax   = bMax;
		dirty  = true;
		bEmpty = false;
	}

	void setMin(int i, double val)
	{
		if (i >= 0 && i <= 2) {
			bmin[i] = val;
			bEmpty = false;
		}
	}

	void setMax(int i, double val)
	{
		if (i >= 0 && i <= 2) {
			bmax[i] = val;
			bEmpty = false;
		}
	}

	// Does this bounding box intersect the target?
	bool intersects(const BoundingBox& target) const;

	// does the box contain this point?
	bool intersects(const glm::dvec3& point) const;

	// if the ray hits the box, put the "t" value of the intersection
	// closest to the origin in tMin and the "t" value of the far
	// intersection
	// in tMax and return true, else return false.
	bool intersect(const ray& r, double& tMin, double& tMax) const;

	void operator=(const BoundingBox& target);
	double area();
	double volume();
	void merge(const BoundingBox& bBox);
	glm::dvec3 midPoint() const {return (bmin + bmax) / glm::dvec3(2.0, 2.0, 2.0);}
	std::vector<int> longestAxis() const 
	{
		auto diff = bmax - bmin;

	    std::vector<int> max_idx(diff.length());
	    std::size_t n(0);
	    std::generate(std::begin(max_idx), std::end(max_idx), [&]{ return n++; });

	    std::sort(  std::begin(max_idx), 
	                std::end(max_idx),
	                [&](int i1, int i2) { return diff[i1] > diff[i2]; } );

	    return max_idx;
	}
};
