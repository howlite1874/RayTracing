//////////////////////////////////////////////////////////////////////
//
//  University of Leeds
//  COMP 5812M Foundations of Modelling & Rendering
//  User Interface for Coursework
//
//  September, 2022
//
//  ------------------------
//  Scene.h
//  ------------------------
//
//  Contains a definition of a scene, with triangles and transformations.
//
///////////////////////////////////////////////////

#ifndef SCENE_H
#define SCENE_H

#include "Homogeneous4.h"
#include "ThreeDModel.h"
#include <vector>
#include "Ray.h"
#include "Triangle.h"
#include "Material.h"
#include "ArcBall.h"

class Scene
{
public:

    struct CollisionInfo{
        Triangle tri;
        float t;
    };

    std::vector<ThreeDModel>* objects;
    RenderParameters* rp;
    Material *default_mat;

    std::vector<Triangle> triangles;

    Scene(std::vector<ThreeDModel> *texobjs,RenderParameters *renderp);
    void updateScene();
    CollisionInfo closestTriangle(Ray r);
    Matrix4 getModelview();
};

#endif // SCENE_H
