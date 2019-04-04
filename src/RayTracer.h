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

	/**
		@brief Gets a pointer to the window buffer by reference.
		@param buf the returned pointer
		@param w the width of the buffer
		@param h the height of the buffer
		@return None
	*/	
	void getBuffer(unsigned char*& buf, int& w, int& h);
	double aspectRatio();

	/**
		@brief Traces all pixels in the image
		@param w the width of the buffer
		@param h the height of the buffer
		@return None
	*/	
	void traceImage(int w, int h);

	/**
		@brief Helper function for adaptive super sampling
		@param x_bl bottom left range of x
		@param y_bl bottom left range of y
		@param x_tr top right range of x
		@param y_tr top right range of y
		@return None
	*/	
	glm::dvec3 adaptiveSS(double x_bl, double y_bl, double x_tr, double y_tr, int depth);

	/**
		@brief main function for adaptive super sampling for anti aliasing
		@param x x coord of the pixel to super sample
		@param y y coord of the pixel to super sample
		@return None
	*/	
	glm::dvec3 doAdaptive(double x, double y);

	/**
		@brief Performs jittered super sampling for anti aliasing
		@param i the x coord of the pixel to super sample
		@param j the y coord of the pixel to super sample
		@return None
	*/	
	glm::dvec3 jitteredSS(int i, int j);

	/**
		@brief Performs normal super sampling for anti aliasing
		@param i the x coord of the pixel to super sample
		@param j the y coord of the pixel to super sample
		@return None
	*/	
	glm::dvec3 superSamplePixel(int i, int j);

	/**
		@brief Generates a red/cyan 3d image of the scene
		@return None
	*/	
	void SIRD();
	
	/**
		@brief Performs anti aliasing on the image which reduces jaggedness
		@return None
	*/	
	int aaImage();

	/**
		@brief Checks to see if the tracer is done tracing
		@return None
	*/	
	bool checkRender();

	/**
		@brief Blocks until the tracer is done
		@return None
	*/	
	void waitRender();

	/**
		@brief Initializes necessary variables and sets up the window buffer
		@param w the width of the window buffer
		@param h the height of the window buffer
		@return None
	*/	
	void traceSetup(int w, int h);

	/**
		@brief Loads a .ray file to be traced
		@param fn The file name/path to be traced
		@return true if successfully loaded, false otherwise 
	*/	
	bool loadScene(const char* fn);

	/**
		@brief Checks if the scene has been successfully loaded
		@return true if successfully loaded, false otherwise 
	*/	
	bool sceneLoaded() { return scene != 0; }

	/**
		@brief Marks the buffer as available to draw to
		@param ready true if we can draw, false otherwise
		@return None
	*/	
	void setReady(bool ready) { m_bBufferReady = ready; }

	/**
		@brief Checks if the buffer is ready to be drawn to 
		@return True if ready, false otherwise
	*/	
	bool isReady() const { return m_bBufferReady; }

	/**
		@brief Gets a pointer to our scene 
		@return the pointer to our scene object
	*/	
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
