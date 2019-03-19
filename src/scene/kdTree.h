#pragma once
#include <vector>
#include <memory>
#include <unordered_set>
#include "bbox.h"
#include "ray.h"
#include <iostream>
#include "../ui/TraceUI.h"
extern TraceUI* traceUI;
// Note: you can put kd-tree here

template <class T>
class KdTree
{
private:
	BoundingBox _bbox;
	std::unique_ptr<KdTree<T>> _left, _right;
	std::vector<T> _objects;
	void build_tree(std::vector<T>& objects, int depth);
	bool isLeaf() const;
public:
	KdTree();
	KdTree(std::vector<T>& objects, int depth);
	bool intersect(ray& r, isect& i, bool& have_one) const;
	const std::unique_ptr<KdTree<T>>& getLeft() const {return _left; }
	const std::unique_ptr<KdTree<T>>& getRight() const {return _right; }
	std::vector<T> getObjects() const {return _objects; }

	int maxDepth() const;
	int countLeaf() const;
};

// check how balanced the tree is
template <class T>
int KdTree<T>::maxDepth() const
{
	if(!this)
	{
		return 0;
	}
	if (this->isLeaf())
	{
		return 1;
	}
	return 1 + std::max(!_left ? 0 :_left->maxDepth(), !_right ? 0 :_right->maxDepth());
}
// Used to make sure all triangles are properly placed
template <class T>
int KdTree<T>::countLeaf() const
{
	if(!this)
	{
		return 0;
	}
	if (this->isLeaf())
	{
		return this->getObjects().size();
	}
	return (!_left ? 0 : _left->countLeaf()) + (!_right ? 0 : _right->countLeaf());
}

template <class T>
bool KdTree<T>::intersect(ray& r, isect& i, bool& have_one) const
{
	double tmin, tmax;
	if (this->_bbox.intersect(r, tmin, tmax))
	{
		if (this->isLeaf())
		{
			isect check_intersect;
			for (const auto& obj : _objects)
			{
				if(obj->intersect(r, check_intersect))
				{
					// Take the earliest time of intersection
					if (!have_one || check_intersect.getT() < i.getT())
					{
						i = check_intersect;
						have_one = true;
					}
				}
			}
		}
		else
		{
			// do any of the children intersect?
	        this->_left->intersect(r, i, have_one);
	        this->_right->intersect(r, i, have_one);
		}
	}
	return have_one;
}

template <class T>
bool KdTree<T>::isLeaf() const { return !_left && !_right; }

template <class T>
KdTree<T>::KdTree(std::vector<T>& objects, int depth) :
   _bbox(), 
   _left(),
   _right(),
   _objects()
{
	build_tree(objects, depth);
}

template <class T>
KdTree<T>::KdTree() :
   _bbox(), 
   _left(),
   _right(),
   _objects() {}

template <class T>
void  KdTree<T>::build_tree(std::vector<T>& objects, int depth)
{
	// nothing here
	if (!objects.empty())
	{
		this->_bbox = objects[0]->getBoundingBox();
		// Make a bounding box that fits all the objects
		for(int i = 1; i < objects.size(); i++)
		{
			this->_bbox.merge(objects[i]->getBoundingBox());
		}
	}
	// base case
    if (objects.size() < traceUI->getLeafSize() || depth >= traceUI->getMaxDepth())
    {
		this->_objects = objects;
		return;
    }

    std::vector<T> left_objects;
    std::vector<T> right_objects;

    // start with the first longest axis
    int cur_axis = 0;
    // ordered vector of axises
    auto ordered_axis = _bbox.longestAxis();

    // partition each object into the halves based on the longest axis of the current bb
    // if a vector ends up empty then try again with the second longest
    while (cur_axis < 3 && (left_objects.empty() || right_objects.empty()))
    {
    	// get longest axis
		int	m_axis = ordered_axis[cur_axis++];
    	// get pivot point

		auto pivot = _bbox.midPoint()[m_axis];
    	// reset
    	left_objects.clear();
    	right_objects.clear();
		
    	// partition
		for (const auto& obj : objects)
		{
			auto box_axis_value = obj->getBoundingBox().midPoint()[m_axis];
			if (box_axis_value < pivot)
				left_objects.push_back(obj);
			else
				right_objects.push_back(obj);
		}
    }
	this->_left = std::make_unique<KdTree<T>>(left_objects, depth + 1);
	this->_right = std::make_unique<KdTree<T>>(right_objects, depth + 1);
}
