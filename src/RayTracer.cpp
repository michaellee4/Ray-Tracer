// The main ray tracer.

#pragma warning (disable: 4786)

#include "RayTracer.h"
#include "scene/light.h"
#include "scene/material.h"
#include "scene/ray.h"

#include "parser/Tokenizer.h"
#include "parser/Parser.h"

#include "ui/TraceUI.h"
#include <cmath>
#include <algorithm>
#include <glm/glm.hpp>
#include <glm/gtx/io.hpp>
#include <string.h> // for memset

#include <iostream>
#include <thread>
#include <fstream>
#include <future>
#include <atomic>
#include <random>
using namespace std;
extern TraceUI* traceUI;

bool debugMode = false;

// Trace a top-level ray through pixel(i,j), i.e. normalized window coordinates (x,y),
// through the projection plane, and out into the scene.  All we do is
// enter the main ray-tracing method, getting things started by plugging
// in an initial ray weight of (0.0,0.0,0.0) and an initial recursion depth of 0.

glm::dvec3 RayTracer::trace(double x, double y)
{
	// Clear out the ray cache in the scene for debugging purposes,
	if (TraceUI::m_debug)
		scene->intersectCache.clear();

	ray r(glm::dvec3(0,0,0), glm::dvec3(0,0,0), glm::dvec3(1,1,1), ray::VISIBILITY);
	scene->getCamera().rayThrough(x,y,r);
	double dummy;
	glm::dvec3 ret = traceRay(r, glm::dvec3(1.0,1.0,1.0), traceUI->getDepth(), dummy);
	ret = glm::clamp(ret, 0.0, 1.0);
	return ret;
}

glm::dvec3 RayTracer::tracePixel(int i, int j)
{
	glm::dvec3 col(0,0,0);

	if( ! sceneLoaded() ) return col;

	double x = double(i)/double(buffer_width);
	double y = double(j)/double(buffer_height);

	unsigned char *pixel = buffer.data() + ( i + j * buffer_width ) * 3;

	col = trace(x, y);

	return col;
}

glm::dvec3 RayTracer::getReflContribution(ray& r, const glm::dvec3& thresh, int depth, double& t, const isect& i)
{
	auto P = r.at(i.getT());
	auto I = r.getDirection();
	auto N = i.getN();
	const Material& m = i.getMaterial();
	// I - (2.0 * glm::dot(I, N)) * N;
	auto v_refl = glm::normalize( glm::reflect(I, N) );
	
	// Creates a ray in the reflected direction
	ray reflection(P, v_refl, glm::dvec3(1.0, 1.0, 1.0), ray::REFLECTION);
	
	// find reflected color
	auto refColor = traceRay(reflection, thresh, depth-1, t);
	return refColor;
}

glm::dvec3 RayTracer::getRefrContribution(ray& r, const glm::dvec3& thresh, int depth, double& t, const isect& i)
{
	auto P = r.at(i.getT());
	auto I = r.getDirection();
	auto N = i.getN();
	const Material& m = i.getMaterial();
	auto IOR = i.getMaterial().index(i);
	bool going_in = glm::dot(I, N) < 0;

	if(!going_in) { N = -N; }

	// we're exiting and going back into the air
	if(abs(r.source_IOR - IOR) <= RAY_EPSILON && !going_in) IOR = 1.0; 

	// IOR coefficient
	auto eta = r.source_IOR / IOR;
	// attentuate kt
	auto kt = glm::pow(m.kt(i), glm::dvec3(1, 1, 1) * glm::length(P - r.getPosition()));

	auto v_refr = glm::normalize(glm::refract(I, N, eta));
	// glm::refract may return NaN
	bool hasNan = glm::isnan(v_refr[0]) || glm::isnan(v_refr[1]) || glm::isnan(v_refr[2]);
	// total internal reflection
	if( hasNan || glm::length(v_refr) == 0)
	{
		isect invN(i);
		invN.setN(-i.getN());
		return getReflContribution(r, thresh, depth, t, invN) * kt;
	}
	else
	{
		ray refr_ray (P, v_refr, glm::dvec3(1, 1, 1), ray::REFRACTION);
		refr_ray.source_IOR = IOR;
		auto refr_color = traceRay(refr_ray, thresh, depth - 1, t);
		if(!going_in)
			refr_color = refr_color * kt;
		return refr_color;
	}
}
#define VERBOSE 0

// Do recursive ray tracing!  You'll want to insert a lot of code here
// (or places called from here) to handle reflection, refraction, etc etc.
glm::dvec3 RayTracer::traceRay(ray& r, const glm::dvec3& thresh, int depth, double& t )
{
	isect i;
	glm::dvec3 colorC;
#if VERBOSE
	std::cerr << "== current depth: " << depth << std::endl;
#endif

	if(scene->intersect(r, i)) {
		const Material& m = i.getMaterial();
		colorC = m.shade(scene.get(), r, i);
		if (depth == 0)
			return colorC;

		// Check if non-zero reflectiveness
		if(m.Refl()) 
		{
			colorC += getReflContribution(r, thresh, depth, t, i) * m.kr(i);
		}

		// Check if non-zero transparency
		if (m.Trans())
		{
			colorC += getRefrContribution(r, thresh, depth, t, i);
		}

	} else {
		if (traceUI->cubeMap())
		{
			colorC = traceUI->getCubeMap()->getColor(r);
		}
		else colorC = glm::dvec3(0.0, 0.0, 0.0);
	}
#if VERBOSE
	std::cerr << "== depth: " << depth+1 << " done, returning: " << colorC << std::endl;
#endif
	return colorC;
}

RayTracer::RayTracer()
	: scene(nullptr), buffer(0), thresh(0), buffer_width(0), buffer_height(0), m_bBufferReady(false)
{
}

RayTracer::~RayTracer()
{
}

void RayTracer::getBuffer( unsigned char *&buf, int &w, int &h )
{
	buf = buffer.data();
	w = buffer_width;
	h = buffer_height;
}

double RayTracer::aspectRatio()
{
	return sceneLoaded() ? scene->getCamera().getAspectRatio() : 1;
}

bool RayTracer::loadScene(const char* fn)
{
	ifstream ifs(fn);
	if( !ifs ) {
		string msg( "Error: couldn't read scene file " );
		msg.append( fn );
		traceUI->alert( msg );
		return false;
	}

	// Strip off filename, leaving only the path:
	string path( fn );
	if (path.find_last_of( "\\/" ) == string::npos)
		path = ".";
	else
		path = path.substr(0, path.find_last_of( "\\/" ));

	// Call this with 'true' for debug output from the tokenizer
	Tokenizer tokenizer( ifs, false );
	Parser parser( tokenizer, path );
	try {
		scene.reset(parser.parseScene());
	}
	catch( SyntaxErrorException& pe ) {
		traceUI->alert( pe.formattedMessage() );
		return false;
	} catch( ParserException& pe ) {
		string msg( "Parser: fatal exception " );
		msg.append( pe.message() );
		traceUI->alert( msg );
		return false;
	} catch( TextureMapException e ) {
		string msg( "Texture mapping exception: " );
		msg.append( e.message() );
		traceUI->alert( msg );
		return false;
	}

	if (!sceneLoaded())
		return false;
	scene->buildKdTree();

	return true;
}

void RayTracer::traceSetup(int w, int h)
{
	size_t newBufferSize = w * h * 3;
	if (newBufferSize != buffer.size()) {
		bufferSize = newBufferSize;
		buffer.resize(bufferSize);
	}
	buffer_width = w;
	buffer_height = h;
	std::fill(buffer.begin(), buffer.end(), 0);
	m_bBufferReady = true;

	/*
	 * Sync with TraceUI
	 */

	threads = traceUI->getThreads();
	block_size = traceUI->getBlockSize();
	thresh = traceUI->getThreshold();
	samples = traceUI->getSuperSamples();
	aaThresh = traceUI->getAaThreshold();

}

/*
 * RayTracer::traceImage
 *
 *	Trace the image and store the pixel data in RayTracer::buffer.
 *
 *	Arguments:
 *		w:	width of the image buffer
 *		h:	height of the image buffer
 *
 */
void RayTracer::traceImage(int w, int h)
{
	// Always call traceSetup before rendering anything.
	traceSetup(w,h);
	std::size_t max = buffer_width * buffer_height;
	std::size_t cores = threads;
	volatile std::atomic<std::size_t> count(0);
	std::vector<std::future<void>> future_vector;
	while (cores--)
	{
	    future_vector.emplace_back(
	        std::async([&]()
	        {
	            while (true)
	            {
	                std::size_t index = count++;
	                if (index >= max)
	                    break;
	                std::size_t x = index % buffer_width;
	                std::size_t y = index / buffer_width;
					glm::dvec3 color = this->tracePixel(x, y);
					this->setPixel(x, y, color);
	            }
	        }));
	}
}

void RayTracer::SIRD()
{
	const auto& v_dir = scene->getCamera().getLook();
	const auto& updir = scene->getCamera().getUpdir();
	const double angle_of_rotation = 0.00872665;

	auto left_view = v_dir*glm::cos(angle_of_rotation) + (glm::cross(updir, v_dir)*glm::sin(angle_of_rotation) + updir*(glm::dot(updir, v_dir))*(1 - glm::cos(angle_of_rotation)));
	auto right_view = v_dir*glm::cos(-angle_of_rotation) + (glm::cross(updir, v_dir)*glm::sin(-angle_of_rotation) + updir*(glm::dot(updir, v_dir))*(1 - glm::cos(-angle_of_rotation)));   
	scene->getCamera().setLook(left_view, updir);
	for (int x = 0; x < buffer_width; ++x)
	{
		for (int y = 0; y < buffer_height; ++y)
		{
			glm::dvec3 color = this->tracePixel(x, y);
			color[1] = 0; color[2] = 0;
			this->setPixel(x, y, color);
		}
	}

	scene->getCamera().setLook(right_view, updir);
	for (int x = 0; x < buffer_width; ++x)
	{
		for (int y = 0; y < buffer_height; ++y)
		{
			glm::dvec3 pixel_col = this->getPixel(x, y);
			glm::dvec3 color = this->tracePixel(x, y);
			color[0] = 0;
			this->setPixel(x, y, pixel_col + color);
		}
	}
	scene->getCamera().setLook(v_dir, updir);
}


glm::dvec3 RayTracer::adaptiveSS(double x_bl, double y_bl, double x_tr, double y_tr, int depth)
{
	if (x_tr > buffer_width || y_tr > buffer_height)
		return trace(x_bl, y_bl);
	auto bl_col = trace(x_bl / (double) buffer_width, y_bl / (double) buffer_height);
	auto tr_col = trace(x_tr / (double) buffer_width, y_tr / (double) buffer_height);
	auto tl_col = trace(x_bl / (double) buffer_width, y_tr / (double) buffer_height);
	auto br_col = trace(x_tr / (double) buffer_width, y_bl / (double) buffer_height);
	double center_x = x_bl + (x_tr - x_bl) / 2.0;
	double center_y = y_bl + (y_tr - y_bl) / 2.0;
	auto center_col = trace( center_x / (double) buffer_width, center_y / (double) buffer_height);

	const double rgb_mag = std::sqrt(3);
	const double col_threshold = 0.1;

	glm::dvec3 bl_res = (center_col + bl_col) / 2.0;
	glm::dvec3 br_res = (center_col + br_col) / 2.0;
	glm::dvec3 tl_res = (center_col + tl_col) / 2.0;
	glm::dvec3 tr_res = (center_col + tr_col) / 2.0;

	if(depth < samples)
	{
		if(glm::length(center_col - bl_col) / rgb_mag > col_threshold)
		{
			bl_res = adaptiveSS(x_bl, y_bl, center_x, center_y, depth + 1);
		}
		if(glm::length(center_col - br_col) / rgb_mag > col_threshold)
		{
			br_res = adaptiveSS( center_x, y_bl, x_tr, center_y, depth + 1);
		}
		if(glm::length(center_col - tl_col) / rgb_mag > col_threshold)
		{
			tl_res = adaptiveSS( x_bl, center_y, center_x, y_tr, depth + 1);
		}
		if(glm::length(center_col - tr_col) / rgb_mag > col_threshold)
		{
			tr_res = adaptiveSS(center_x, center_y, x_tr, y_tr, depth + 1);
		}
	}
	return ( bl_res+ br_res + tl_res + tr_res) / 4.0;
}
glm::dvec3 RayTracer::doAdaptive(double x, double y)
{
	return this->adaptiveSS(x, y, x + 1, y + 1, 0);
}

glm::dvec3 RayTracer::jitteredSS(int i, int j)
{
	/*
		input: pixel coords
		performs supersampling on input pixel
	*/
	double subPixelXsize = (1.0 / (buffer_width * samples));
	double subPixelYsize = (1.0 / (buffer_height * samples));

	double x = double(i)/double(buffer_width);
	double y = double(j)/double(buffer_height);

	std::random_device rd;
	std::mt19937 gen(rd()); // Create and seed the generator
	std::normal_distribution<> d(0, 1); // Create distribution

	glm::dvec3 color(0,0,0);
	for (int sampX = 0; sampX < samples; ++sampX)
	{
		for (int sampY = 0; sampY < samples; ++sampY)
		{
			auto x_rand_normal = glm::clamp(d(gen), -3.0, 3.0 );
			auto y_rand_normal = glm::clamp(d(gen), -3.0, 3.0 );
			color+=trace((double) (x) + (sampX + 0.5 + (x_rand_normal / 6)) * subPixelXsize, 
						 (double)(y) + (sampY + 0.5 + (y_rand_normal / 6)) * subPixelYsize);
		}
	}
	// average the color
	color/=(samples * samples);
	return color;
}

glm::dvec3 RayTracer::superSamplePixel(int i, int j)
{
	/*
		input: pixel coords
		performs supersampling on input pixel
	*/
	double subPixelXsize = (1.0 / (buffer_width * samples));
	double subPixelYsize = (1.0 / (buffer_height * samples));

	double x = double(i)/double(buffer_width);
	double y = double(j)/double(buffer_height);

	glm::dvec3 color(0,0,0);
	for (int sampX = 0; sampX < samples; ++sampX)
	{
		for (int sampY = 0; sampY < samples; ++sampY)
		{
			color+=trace((double) (x) + sampX * subPixelXsize, (double)(y) + sampY * subPixelYsize);
		}
	}
	// average the color
	color/=(samples * samples);
	return color;
}

int RayTracer::aaImage()
{

	std::size_t max = buffer_width * buffer_height;
	std::size_t cores = threads;
	volatile std::atomic<std::size_t> count(0);
	std::vector<std::future<void>> future_vector;
	// auto SS = traceUI->adaptiveSSSwitch() ? doAdaptive : traceUI->jitterSwitch() ? 
	while (cores--)
	{
	    future_vector.emplace_back(
	        std::async([&]()
	        {
	            while (true)
	            {
	                std::size_t index = count++;
	                if (index >= max)
	                    break;
	                std::size_t x = index % buffer_width;
	                std::size_t y = index / buffer_width;

	                glm::dvec3 color;
	                if (traceUI->adaptiveSSSwitch())
						color = this->doAdaptive(x, y);
					else if (traceUI->jitterSwitch())
					{
						color = this->jitteredSS(x, y);
					}
					else
						color = this->superSamplePixel(x, y);
					this->setPixel(x, y, color);
	            }
	        }));
	}
	return 0;
}


glm::dvec3 RayTracer::getPixel(int i, int j)
{
	unsigned char *pixel = buffer.data() + ( i + j * buffer_width ) * 3;
	return glm::dvec3((double)pixel[0]/255.0, (double)pixel[1]/255.0, (double)pixel[2]/255.0);
}

void RayTracer::setPixel(int i, int j, glm::dvec3 color)
{
	unsigned char *pixel = buffer.data() + ( i + j * buffer_width ) * 3;

	pixel[0] = (int)( 255.0 * color[0]);
	pixel[1] = (int)( 255.0 * color[1]);
	pixel[2] = (int)( 255.0 * color[2]);
}

