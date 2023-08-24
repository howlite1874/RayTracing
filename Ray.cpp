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

Ray Ray::GetRaySampling(Cartesian3 intersection,Cartesian3 normal)
{
    float alpha = (static_cast<float>(std::rand()) / RAND_MAX) * MY_PI;
    float theta = (static_cast<float>(std::rand()) / RAND_MAX) * MY_PI * 2;
    Cartesian3 direction(std::sin(alpha) * std::cos(theta),std::cos(alpha),std::sin(alpha) * std::sin(theta));
    float cosine = direction.dot(normal);
    if (cosine < 0) {
        direction = direction - 2 * cosine * normal;
    }
    return Ray(intersection, direction, primary);
}
