//////////////////////////////////////////////////////////////////////
//
//  University of Leeds
//  COMP 5812M Foundations of Modelling & Rendering
//  User Interface for Coursework
//
//  September, 2022
//
//  ------------------------
//  Ray.h
//  ------------------------
//
//  A simple class to describe a ray.
//
///////////////////////////////////////////////////

#ifndef RAY_H
#define RAY_H

#include "Cartesian3.h"

class Ray
{


public:
    enum Type{primary,secondary};
    Ray(Cartesian3 og,Cartesian3 dir,Type rayType);
    Cartesian3 origin;
    Cartesian3 direction;
    Type ray_type;
    Ray GetRaySampling(Cartesian3 intersection,Cartesian3 normal);

};

#endif // RAY_H
