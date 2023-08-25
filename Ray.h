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

#include <random>

class Ray
{


public:
    enum Type{primary,secondary};
    Ray(Cartesian3 og,Cartesian3 dir,Type rayType);
    Cartesian3 origin;
    Cartesian3 direction;
    Type ray_type;
    Ray getRandomReflect(const Cartesian3 &hit, const Cartesian3 &normal);
    void createCoordinateSystem(const Cartesian3 &N, Cartesian3 &Nt, Cartesian3 &Nb);
    Cartesian3 uniformSampleHemisphere(const float &r1, const float &r2);

private:
    float bias = 0.0001;
};

#endif // RAY_H
