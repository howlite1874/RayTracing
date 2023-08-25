#include "Ray.h"
#include <cmath>
#include <cstdlib>
#include <limits>

const float MY_PI = static_cast<float>(std::acos(-1));

Ray::Ray(Cartesian3 og, Cartesian3 dir,Type rayType)
{
    origin = og;
    direction = dir;
    ray_type = rayType;
}

void Ray::createCoordinateSystem(const Cartesian3 &N, Cartesian3 &Nt, Cartesian3 &Nb)
{
    if (std::fabs(N.x) > std::fabs(N.y))
        Nt = Cartesian3(N.z, 0, -N.x) / sqrtf(N.x * N.x + N.z * N.z);
    else
        Nt = Cartesian3(0, -N.z, N.y) / sqrtf(N.y * N.y + N.z * N.z);
    Nb = N.cross(Nt);
}

Cartesian3 Ray::uniformSampleHemisphere(const float &r1, const float &r2)
{
    // cos(theta) = u1 = y
    // cos^2(theta) + sin^2(theta) = 1 -> sin(theta) = srtf(1 - cos^2(theta))
    float sinTheta = sqrtf(1 - r1 * r1);
    float phi = 2 * MY_PI * r2;
    float x = sinTheta * cosf(phi);
    float z = sinTheta * sinf(phi);
    return Cartesian3(x, r1, z);
}

std::default_random_engine generator;
std::uniform_real_distribution<float> distribution(0, 1);
Ray Ray::getRandomReflect(const Cartesian3 &hit, const Cartesian3 &normal)
{
    float r1 = distribution(generator);
    float r2 = distribution(generator);
    Cartesian3 Nt, Nb;
    createCoordinateSystem(normal, Nt, Nb);
    Cartesian3 sample = uniformSampleHemisphere(r1, r2);
    Cartesian3 sampleWorld(
                sample.x * Nb.x + sample.y * normal.x + sample.z * Nt.x,
                sample.x * Nb.y + sample.y * normal.y + sample.z * Nt.y,
                sample.x * Nb.z + sample.y * normal.z + sample.z * Nt.z);

    return Ray(hit + sampleWorld * bias, sampleWorld, primary);
}

