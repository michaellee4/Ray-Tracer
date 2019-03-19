#include "material.h"
#include "../ui/TraceUI.h"
#include "light.h"
#include "ray.h"
extern TraceUI* traceUI;

#include <glm/gtx/io.hpp>
#include <iostream>
#include "../fileio/images.h"

using namespace std;
extern bool debugMode;

Material::~Material()
{
}

// Apply the phong model to this point on the surface of the object, returning
// the color of that point.
glm::dvec3 Material::shade(Scene* scene, const ray& r, const isect& i) const
{
	// YOUR CODE HERE*

	// For now, this method just returns the diffuse color of the object.
	// This gives a single matte color for every distinct surface in the
	// scene, and that's it.  Simple, but enough to get you started.
	// (It's also inconsistent with the phong model...)

	// Your mission is to fill in this method with the rest of the phong
	// shading model, including the contributions of all the light sources.
	// You will need to call both distanceAttenuation() and
	// shadowAttenuation()
	// somewhere in your code in order to compute shadows and light falloff.
	//	if( debugMode )
	//		std::cout << "Debugging Phong code..." << std::endl;

	// When you're iterating through the lights,
	// you'll want to use code that looks something
	// like this:
	//
	// for ( const auto& pLight : scene->getAllLights() )
	// {
	//              // pLight has type unique_ptr<Light>
	// 		.
	// 		.
	// 		.
	// }

	// get point of intersction
	auto p = r.at(i.getT());

	// from slides, base color
	glm::dvec3 color = ke(i) + ka(i) * scene->ambient();
	
	for(auto& light: scene->getAllLights()) {
		// Direction from intersection to light
		glm::dvec3 L = glm::normalize(light->getDirection(p));
		glm::dvec3 light_color = light->shadowAttenuation(r, p);
		// The normal of intersection surface 
		glm::dvec3 N = glm::normalize(i.getN());

		bool going_in = glm::dot(r.getDirection(), N) < 0;
		if (!going_in) { N = -N; }

		// Direction towards the camera
		glm::dvec3 V = glm::normalize(-r.getDirection());
		// Direction that a reflection would take
		// glm::dvec3 R = 2.0 * glm::dot(L, N) * N - L;
		glm::dvec3 R = -glm::reflect(L, N);
 		auto tmp = glm::dot(N, L);
 		if (this->Trans()) { tmp = abs(tmp); }
 		else tmp = max(tmp, 0.0);

		glm::dvec3 diffuse = kd(i)* tmp;

		// check if object is transparent?
		glm::dvec3 spec = ks(i) * pow(max(glm::dot(V,R),0.0), shininess(i));
		// glm::dvec3 spec = !going_in ? glm::dvec3(0, 0, 0) : ks(i) * pow(max(glm::dot(V,R),0.0), shininess(i));
		color += light_color * (diffuse + spec) * light->distanceAttenuation(p);
	}

	return color;
}


TextureMap::TextureMap(string filename)
{
	data = readImage(filename.c_str(), width, height);
	if (data.empty()) {
		width = 0;
		height = 0;
		string error("Unable to load texture map '");
		error.append(filename);
		error.append("'.");
		throw TextureMapException(error);
	}
}

glm::dvec3 TextureMap::getMappedValue(const glm::dvec2& coord) const
{
	// YOUR CODE HERE
	//
	// In order to add texture mapping support to the
	// raytracer, you need to implement this function.
	// What this function should do is convert from
	// parametric space which is the unit square
	// [0, 1] x [0, 1] in 2-space to bitmap coordinates,
	// and use these to perform bilinear interpolation
	// of the values.
	if (data.empty())
	{
		return glm::dvec3(1.0,1.0,1.0);
	}
	double x = coord[0] * (width -1 ), y = coord[1] * (height-1);

	double x_f = std::floor(x);
	double y_f = std::floor(y);
	double x_c = std::ceil(x);
	double y_c = std::ceil(y);
	if(x_f == x && y_f == y)
	{
		return getPixelAt(x, y);
	}
	else if(x_f != x && y_f != y)
	{


		double coefficient = 1.0 / ((x_c - x_f) * (y_c - y_f));
		glm::dvec2 x_diff(x_c - x, x - x_f);
		glm::dvec2 y_diff(y_c - y, y - y_f);

		auto c_11 = getPixelAt(x_f, y_f);
		auto c_12 = getPixelAt(x_f, y_c);
		auto c_21 = getPixelAt(x_c, y_f);
		auto c_22 = getPixelAt(x_c, y_c);

		auto cy_11 = c_11 * y_diff[0] + c_12 * y_diff[1];
		auto cy_21 = c_21 * y_diff[0] + c_22 * y_diff[1];

		glm::dvec3 color = x_diff[0] * cy_11 + x_diff[1] * cy_21;

		return coefficient * color;
	}
	else
	{
		// x is the integer
		if (x_f == x)
		{
			auto lower_color = getPixelAt(x, y_f);
			auto upper_color = getPixelAt(x, y_c);
			return glm::mix(lower_color, upper_color, y - y_f);
		}
		// y is the integer
		else
		{
			auto lower_color = getPixelAt(x_f, y);
			auto upper_color = getPixelAt(x_c, y);
			return glm::mix(lower_color, upper_color, x - x_f);
		}
	}

}

glm::dvec3 TextureMap::getPixelAt(int x, int y) const
{
	// YOUR CODE HERE
	//
	// In order to add texture mapping support to the
	// raytracer, you need to implement this function.

	if (data.empty())
	{
		return glm::dvec3(1.0, 1.0, 1.0);
	}

    if(x >= width )
       x = width - 1;
    if( y >= height )
       y = height - 1;

    // Find the position in the big data array...
	const unsigned char *pixel = data.data() + ( x + (y * width) ) * 3;

    return glm::dvec3((double)pixel[0]/255.0, (double)pixel[1]/255.0, (double)pixel[2]/255.0);
}

glm::dvec3 MaterialParameter::value(const isect& is) const
{
	if (0 != _textureMap)
		return _textureMap->getMappedValue(is.getUVCoordinates());
	else
		return _value;
}

double MaterialParameter::intensityValue(const isect& is) const
{
	if (0 != _textureMap) {
		glm::dvec3 value(
		        _textureMap->getMappedValue(is.getUVCoordinates()));
		return (0.299 * value[0]) + (0.587 * value[1]) +
		       (0.114 * value[2]);
	} else
		return (0.299 * _value[0]) + (0.587 * _value[1]) +
		       (0.114 * _value[2]);
}
