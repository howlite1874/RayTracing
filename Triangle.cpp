#include "Triangle.h"
#include <math.h>

Triangle::Triangle()
{
    valid = false;
    shared_material= nullptr;
}

float Triangle::intersect(Ray r){

    Cartesian3 E1=(verts[1]-verts[0]).Vector();
    Cartesian3 E2=(verts[2]-verts[0]).Vector();
    Cartesian3 S(r.origin.x-verts[0].x,r.origin.y-verts[0].y,r.origin.z-verts[0].z);
    Cartesian3 S1=r.direction.cross(E2);
    Cartesian3 S2=S.cross(E1);
    float t=(S2.dot(E2)/S1.dot(E1));
    float b1=(S1.dot(S)/S1.dot(E1));
    float b2=(S2.dot(r.direction)/S1.dot(E1));
    if(t>0&&b1>0&&b2>0&&1-(b1+b2)>0){
        return t;
    }
    return 0;
}
void Triangle::validate(){
    valid = true;
}

bool Triangle::isValid(){
    return valid;
}

Cartesian3 Triangle::baricentric(Cartesian3 o)
{
   //TODO: Implement this! Input is the intersection between the ray and the triangle.
    float alpha,beta;
   Homogeneous4 E1=verts[2]-verts[1];
   Homogeneous4 E2=verts[0]-verts[2];
   Cartesian3 N=E1.Vector().cross(E2.Vector());
   float len=pow(N.length(),2);
   Cartesian3 na=(E1.Vector()).cross(o-verts[1].Vector());
   Cartesian3 nb=(E2.Vector()).cross(o-verts[2].Vector());
   alpha=(N.dot(na))/len;
   beta=(N.dot(nb))/len;
   Cartesian3 bc(alpha,beta,1-alpha-beta);
   return bc;
}
