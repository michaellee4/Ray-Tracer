#ifndef __RAYTRACER_H__
#define __RAYTRACER_H__

#define MAX_THREADS 32

// The main ray tracer.

#include <time.h>
#include <glm/vec3.hpp>
#include <queue>
#include <thread>
#include "scene/cubeMap.h"
#include "scene/ray.h"

class Scene;

/** 
* Stores pixel data related to the screen buffer.
*/
class Pixel {
public:
	Pixel(int i, int j, unsigned char* ptr) : ix(i), jy(j), value(ptr) {}

	int ix;
	int jy;
	// RGB val
	unsigned char* value;
};

/** 
* Heavy lifter of the ray tracer.
*/
class RayTracer {
public:
	RayTracer();
	~RayTracer();

	/**
		@brief colors in a pixel on the screen buffer.
		@param i, j the coordinates of the pixel
		@return The color of the pixel
	*/
	glm::dvec3 tracePixel(int i, int j);


	/**
		@brief Gets the reflective contribution of a secondary ray.
		@param r secondary ray
		@param depth recursive depth
		@param t time of intersection
		@param i intersection info
		@return The color of the reflective contribution
	*/
	glm::dvec3 getReflContribution(ray& r, const glm::dvec3& thresh, int depth, double& t, const isect& i);

	/**
		@brief Gets the refractive contribution of a secondary ray.
		@param r secondary ray
		@param depth recursive depth
		@param t time of intersection
		@param i intersection info
		@return The color of the refractive contribution
	*/
	glm::dvec3 getRefrContribution(ray& r, const glm::dvec3& thresh, int depth, double& t, const isect& i);

	/**
		@brief Gets the overall contribution of a primary ray
		@param r the primary ray
		@param depth recursive depth
		@param lenght the distance the ray has travedl
		@return The color of the overall contribution
	*/
	glm::dvec3 traceRay(ray& r, const glm::dvec3& thresh, int depth,
	                    double& length);

	/**
		@brief Gets the color of a pixel.
		@param i, j the coordinates of the pixel
		@return The color of the pixel
	*/
	glm::dvec3 getPixel(int i, int j);

	/**
		@brief Sets the color of a pixel.
		@param i, j the coordinates of the pixel
		@return None
	*/
	void setPixel(int i, int j, glm::dvec3 color);
	void getBuffer(unsigned char*& buf, int& w, int& h);
	double aspectRatio();

	void traceImage(int w, int h);
	glm::dvec3 adaptiveSS(double x_bl, double y_bl, double x_tr, double y_tr, int depth);
	glm::dvec3 doAdaptive(double x, double y);
	glm::dvec3 jitteredSS(int i, int j);
	glm::dvec3 superSamplePixel(int i, int j);
	void SIRD();
	int aaImage();
	bool checkRender();
	void waitRender();

	void traceSetup(int w, int h);

	bool loadScene(const char* fn);
	bool sceneLoaded() { return scene != 0; }

	void setReady(bool ready) { m_bBufferReady = ready; }
	bool isReady() const { return m_bBufferReady; }

	const Scene& getScene() { return *scene; }

	bool stopTrace;

private:
	glm::dvec3 trace(double x, double y);

	std::vector<unsigned char> buffer;
	int buffer_width, buffer_height;
	int bufferSize;
	unsigned int threads;
	int block_size;
	double thresh;
	double aaThresh;
	int samples;
	std::unique_ptr<Scene> scene;

	bool m_bBufferReady;

};

#endif // __RAYTRACER_H__
