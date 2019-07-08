#include <cmath>
#include <iostream>

#include "light.h"
#include <glm/glm.hpp>
#include <glm/gtx/io.hpp>


using namespace std;

glm::dvec3 DirectionalLight::dsaHelper(ray &r, const glm::dvec3& p) const{
	isect i;
	if(scene->intersect(r,i)) 
	{
		glm::dvec3 P = r.at(i.getT());
		glm::dvec3 N = i.getN();
		bool going_in = glm::dot(N, r.getDirection() ) < 0;
		if(going_in) 
		{
			// stop here if opject is opaque
			if(!i.getMaterial().Trans())
			{
				return glm::dvec3(0, 0, 0);
			}
			ray next (P, r.getDirection(), r.getAtten(), ray::SHADOW);
			return dsaHelper(next, P);

		}
		else 
		{
			auto curAtten = r.getAtten() * (glm::pow(i.getMaterial().kt(i), glm::dvec3(1,1,1) * glm::length(P - r.getPosition())));
			ray next (P, r.getDirection(), curAtten, ray::SHADOW);
			return dsaHelper(next, P);
		}
	}
	else
		return color * r.getAtten();
} 

glm::dvec3 PointLight::psaHelper(ray &r, const glm::dvec3& p) const{
	isect i;

	if(scene->intersect(r,i)) {
		glm::dvec3 P = r.at(i.getT());
		bool behindLight = glm::length(p - this->position) <= glm::length(p - P);
		if(behindLight) {
			return color * r.getAtten();
		}

		glm::dvec3 N = i.getN();
		bool going_in = glm::dot(N, r.getDirection() ) < 0;
		if(going_in) {
			// stop here if opject is opaque
			if(!i.getMaterial().Trans())
			{
				return glm::dvec3(0, 0, 0);
			}
			ray next (P, r.getDirection(), r.getAtten(), ray::SHADOW);
			return psaHelper(next, P);

		}
		else {
			auto curAtten = r.getAtten() * (glm::pow(i.getMaterial().kt(i), glm::dvec3(1,1,1) * glm::length(P - r.getPosition())));
			ray next (P, r.getDirection(), curAtten, ray::SHADOW);
			return psaHelper(next, P);
		}
	}
	else
		return color * r.getAtten();
} 


double DirectionalLight::distanceAttenuation(const glm::dvec3& P) const
{
	// distance to light is infinite, so f(di) goes to 0.  Return 1.
	return 1.0;
}


glm::dvec3 DirectionalLight::shadowAttenuation(const ray& r, const glm::dvec3& p) const
{

	ray ray_to_light (p, this->getDirection(p), glm::dvec3(1,1,1), ray::SHADOW);
	return dsaHelper(ray_to_light, p);

}

glm::dvec3 DirectionalLight::getColor() const
{
	return color;
}

glm::dvec3 DirectionalLight::getDirection(const glm::dvec3& P) const
{
	return -orientation;
}

double PointLight::distanceAttenuation(const glm::dvec3& P) const
{

0
	double d = glm::length(P - position);
	double I_att = 1.0 / (constantTerm + linearTerm * d + quadraticTerm * d * d);

	// Light intensity should only get weaker or stay the same
	// return attenuation;
	return glm::clamp(I_att, 0.0, 1.0);
}

glm::dvec3 PointLight::getColor() const
{
	return color;
}

glm::dvec3 PointLight::getDirection(const glm::dvec3& P) const
{
	return glm::normalize(position - P);
}


glm::dvec3 PointLight::shadowAttenuation(const ray& r, const glm::dvec3& p) const
{
	 ray ray_to_light (p,this->getDirection(p), glm::dvec3(1.0,1.0,1.0), ray::SHADOW);

	 return psaHelper(ray_to_light, p);

}


#define VERBOSE 0

