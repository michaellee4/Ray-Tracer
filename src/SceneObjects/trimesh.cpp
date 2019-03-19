#include "trimesh.h"
#include <assert.h>
#include <float.h>
#include <string.h>
#include <algorithm>
#include <cmath>
#include "../ui/TraceUI.h"
#include "glm/ext.hpp"
#include <iostream>
extern TraceUI* traceUI;

using namespace std;

Trimesh::~Trimesh()
{
	for (auto m : materials)
		delete m;
	for (auto f : faces)
		delete f;
}

// must add vertices, normals, and materials IN ORDER
void Trimesh::addVertex(const glm::dvec3& v)
{
	vertices.emplace_back(v);
}

void Trimesh::addMaterial(Material* m)
{
	materials.emplace_back(m);
}

void Trimesh::addNormal(const glm::dvec3& n)
{
	normals.emplace_back(n);
}

// Returns false if the vertices a,b,c don't all exist
bool Trimesh::addFace(int a, int b, int c)
{
	int vcnt = vertices.size();

	if (a >= vcnt || b >= vcnt || c >= vcnt)
		return false;

	TrimeshFace* newFace = new TrimeshFace(
	        scene, new Material(*this->material), this, a, b, c);
	newFace->setTransform(this->transform);
	if (!newFace->degen)
		faces.push_back(newFace);
	else
		delete newFace;

	// Don't add faces to the scene's object list so we can cull by bounding
	// box
	return true;
}

// Check to make sure that if we have per-vertex materials or normals
// they are the right number.
const char* Trimesh::doubleCheck()
{
	if (!materials.empty() && materials.size() != vertices.size())
		return "Bad Trimesh: Wrong number of materials.";
	if (!normals.empty() && normals.size() != vertices.size())
		return "Bad Trimesh: Wrong number of normals.";

	return 0;
}

bool Trimesh::intersectLocal(ray& r, isect& i) const
{

	bool have_one = false;
	if(traceUI->kdSwitch())
	{
		kdtree->intersect(r, i, have_one);
	}
	else {
		for (auto face : faces) {
			isect cur;
			if (face->intersectLocal(r, cur)) {
				if (!have_one || (cur.getT() < i.getT())) {
					i = cur;
					have_one = true;
				}
			}
		}
	}
	if (!have_one)
		i.setT(1000.0);
	return have_one;
}

bool TrimeshFace::intersect(ray& r, isect& i) const
{
	return intersectLocal(r, i);
}

// Intersect ray r with the triangle abc.  If it hits returns true,
// and put the parameter in t and the barycentric coordinates of the
// intersection in u (alpha) and v (beta).
bool TrimeshFace::intersectLocal(ray& r, isect& i) const
{
	// YOUR CODE HERE*
	//
	// FIXME: Add ray-trimesh intersection

	// references to vertices
    const auto& a = this->parent->vertices[ids[0]];
    const auto& b = this->parent->vertices[ids[1]];
    const auto& c = this->parent->vertices[ids[2]];

    // Check if ray is parallel 
    if(abs(glm::dot(normal, r.getDirection())) < RAY_EPSILON)
    	return false;

    double time_of_intersect = glm::dot(normal, (b - r.getPosition())) / glm::dot(normal, r.getDirection());

    // object is behind us
    if(time_of_intersect < RAY_EPSILON)
    	return false;

    // point on plane of triangle
    auto P = r.getPosition() + r.getDirection() * time_of_intersect;
    // get bary coords
    auto m2 = glm::dot(glm::cross((c - a), (P - a)), normal)/(glm::dot(glm::cross((c - a), (b - a)), normal));
    auto m3 = glm::dot(glm::cross((b - a), (P - a)), normal)/(glm::dot(glm::cross((b - a), (c - a)), normal));
    auto m1 = (1.0 - m2 - m3);

    // verify intersect using bary coords
    bool invalid_intersect =   (m1 < RAY_EPSILON || m1 > 1) 
    						|| (m2 < RAY_EPSILON || m2 > 1) 
    						|| (m3 < RAY_EPSILON || m3 > 1) 
    						|| ((m2 + m3) < RAY_EPSILON || (m2 + m3) > 1);
    // if (m1 < RAY_EPSILON || m1 > 1) return false;
    // if (m2 < RAY_EPSILON || m2 > 1) return false;
    // if (m3 < RAY_EPSILON || m3 > 1) return false;
    // if ((m2 + m3) < RAY_EPSILON || (m2 + m3) > 1) return false;
    if(invalid_intersect) return false;

    // set intersect info
	i.setObject(this);
	i.setMaterial(this->getMaterial());
	i.setT(time_of_intersect);
	i.setN(normal);
	i.setBary(m1, m2, m3);

	i.setUVCoordinates(glm::dvec2(m2, m3));
	// interpolate vertex normals
    if (parent->vertNorms)
    {
        i.setN(((m1 * parent->normals[ids[0]]) +
        	    (m2 * parent->normals[ids[1]]) +
        	    (m3 * parent->normals[ids[2]])));
        i.setN(glm::normalize(i.getN())); 
    }
    // interpolate vertex materials
    if (!parent->materials.empty())
    {
        Material m;
        m += (m1 * Material(*parent->materials[ids[0]]));
        m += (m2 * Material(*parent->materials[ids[1]]));
        m += (m3 * Material(*parent->materials[ids[2]]));

        i.setMaterial(m);
    }
    // Check if 
    return true;
}

// Once all the verts and faces are loaded, per vertex normals can be
// generated by averaging the normals of the neighboring faces.
void Trimesh::generateNormals()
{
	int cnt = vertices.size();
	normals.resize(cnt);
	std::vector<int> numFaces(cnt, 0);

	for (auto face : faces) {
		glm::dvec3 faceNormal = face->getNormal();

		for (int i = 0; i < 3; ++i) {
			normals[(*face)[i]] += faceNormal;
			++numFaces[(*face)[i]];
		}
	}

	for (int i = 0; i < cnt; ++i) {
		if (numFaces[i])
			normals[i] /= numFaces[i];
	}

	vertNorms = true;
}

