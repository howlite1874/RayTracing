//////////////////////////////////////////////////////////////////////
//
//  University of Leeds
//  COMP 5812M Foundations of Modelling & Rendering
//  User Interface for Coursework
////////////////////////////////////////////////////////////////////////


#include <math.h>
#include <random>
#include <QTimer>
#include <algorithm>
// include the header file
#include "RaytraceRenderWidget.h"
#include "Cartesian3.h"
#include <math.h>
#include <algorithm>

#define N_THREADS 16
#define N_LOOPS 100
#define N_BOUNCES 5
#define TERMINATION_FACTOR 0.35f

// constructor
RaytraceRenderWidget::RaytraceRenderWidget
        (
        // the geometric object to show
        std::vector<ThreeDModel>      *newTexturedObject,
        // the render parameters to use
        RenderParameters    *newRenderParameters,
        // parent widget in visual hierarchy
        QWidget             *parent
        )
    // the : indicates variable instantiation rather than arbitrary code
    // it is considered good style to use it where possible
    :
    // start by calling inherited constructor with parent widget's pointer
    QOpenGLWidget(parent),
    // then store the pointers that were passed in
    texturedObjects(newTexturedObject),
    renderParameters(newRenderParameters),
    raytraceScene(texturedObjects,renderParameters)
    { // constructor
        std::srand(static_cast<unsigned int>(std::time(nullptr)));
        restartRaytrace = false;
        QTimer *timer = new QTimer(this);
        connect(timer, &QTimer::timeout, this, &RaytraceRenderWidget::forceRepaint);
        timer->start(30);
    // leaves nothing to put into the constructor body
    } // constructor

void RaytraceRenderWidget::forceRepaint(){
    update();
}
// destructor
RaytraceRenderWidget::~RaytraceRenderWidget()
    { // destructor
    // empty (for now)
    // all of our pointers are to data owned by another class
    // so we have no responsibility for destruction
    // and OpenGL cleanup is taken care of by Qt
    } // destructor

// called when OpenGL context is set up
void RaytraceRenderWidget::initializeGL()
    { // RaytraceRenderWidget::initializeGL()
    // this should remain empty
    } // RaytraceRenderWidget::initializeGL()

// called every time the widget is resized
void RaytraceRenderWidget::resizeGL(int w, int h)
    { // RaytraceRenderWidget::resizeGL()
    // resize the render image
    frameBuffer.Resize(w, h);
    } // RaytraceRenderWidget::resizeGL()

// called every time the widget needs painting
void RaytraceRenderWidget::paintGL()
    { // RaytraceRenderWidget::paintGL()
    // set background colour to white
    glClearColor(1.0, 1.0, 1.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT);

    // and display the image
    glDrawPixels(frameBuffer.width, frameBuffer.height, GL_RGBA, GL_UNSIGNED_BYTE, frameBuffer.block);
    } // RaytraceRenderWidget::paintGL()


    // routine that generates the image
void RaytraceRenderWidget::Raytrace()
{ // RaytraceRenderWidget::Raytrace()

    restartRaytrace = true;
    if(raytracingThread.joinable())
        raytracingThread.join();
    restartRaytrace = false;

    //To make our lifes easier, lets calculate things on VCS.
    //So we need to process our scene to get a triangle soup in VCS.
    //IMPORTANT: You still need to complete the method that gets the modelview matrix in the scene class!
    raytraceScene.updateScene();

    //clear frame buffer before we start
    frameBuffer.clear(RGBAValue(0.0f, 0.0f, 0.0f,1.0f));

    raytracingThread= std::thread(&RaytraceRenderWidget::RaytraceThread,this);
    raytracingThread.detach();

} // RaytraceRenderWidget::Raytrace()

Cartesian3 RaytraceRenderWidget::cwise(Cartesian3 a,Cartesian3 b){
    Cartesian3 c(b.x*a.x,b.y*a.y,b.z*a.z);
    return c;
}

//mirror reflect ray
Ray RaytraceRenderWidget::reflect(Ray inRay,Cartesian3 normal,Cartesian3 intersect){
    normal=normal.unit();
    Cartesian3 reflectDir=inRay.direction+2*normal*(normal.dot(-1*inRay.direction));
    return Ray(intersect,reflectDir,Ray::secondary);
}
Ray RaytraceRenderWidget::refract(Ray inRay,Cartesian3 normal,Cartesian3 intersect,float IOR){
    float cosi=inRay.direction.dot(normal.unit());
    if(cosi>1.0f){cosi=1.0f;}
    else if(cosi<-1.0f){cosi=-1.0f;}
    float etai = 1, etat = IOR;
    if(cosi<=0){
        cosi=-1*cosi;
    }
    else {
        std::swap(etai,etat);
        normal=-1*normal;
    }

    float eta=etai/etat;
    float k = 1 - pow(eta,2)*(1 - cosi*cosi);
    if(k<0){
        return Ray(inRay.origin,inRay.direction,Ray::secondary);
    }
    else{
        Cartesian3 tmp=(eta*inRay.direction+(eta*cosi-sqrtf(k))*normal).unit();
        return Ray(intersect,tmp,Ray::secondary);
        }
}

void RaytraceRenderWidget::fresnel(Ray inRay, Cartesian3 normal, float curIOR,float nextIOR,float &kr){
    float cosi=inRay.direction.unit().dot(normal.unit());
    float etai = 1, etat = curIOR/nextIOR;
    if(cosi>1.0f)   cosi=1.0f;
    if(cosi<-1.0f)  cosi=-1.0f;
    if(cosi>0)  std::swap(curIOR,nextIOR);
    //use Snell's law
    float sint=etai / etat*sqrtf(fmax(0.f,1-pow(cosi,2)));
    if(sint>=1){
        kr=1.0f;
    }
    else{
        cosi=abs(cosi);
        float cost=sqrt(fmax(0.f,1-pow(sint,2)));
        float Rs = ((etat * cosi) - (etai * cost)) / ((etat * cosi) + (etai * cost));
        float Rp = ((etai * cosi) - (etat * cost)) / ((etai * cosi) + (etat * cost));
        kr = 0.5f * (pow(Rs, 2) + pow(Rp, 2));
    }
}

//Recursive to get the colour
Homogeneous4 RaytraceRenderWidget::getColor(Ray r,int depth,float curIOR){
    Homogeneous4 color;
    if(depth<=0){
        return Homogeneous4(0,0,0,0);
    }
    //this matrix is used to calculate light position in world coordinate system
    Matrix4 m_l = raytraceScene.getModelview();
    Scene::CollisionInfo ci= raytraceScene.closestTriangle(r);
    if (ci.tri.isValid()) {
        //find the closest triangle to render
        Triangle tri = ci.tri;

        //get the intersection to use baricentric interpolation
        Cartesian3 intersect = r.origin + r.direction.unit() * ci.t;
        Cartesian3 bari = tri.baricentric(intersect);


        if (renderParameters->interpolationRendering == 1) {
        //the color of triangle is stored in noramals,and it needs to be a postive value
            //use baricentric interpolation to get the normal vector
            Homogeneous4 n1(abs(tri.normals[0].x), abs(tri.normals[0].y), abs(tri.normals[0].z), abs(tri.normals[0].w));
            Homogeneous4 n2(abs(tri.normals[1].x), abs(tri.normals[1].y), abs(tri.normals[1].z), abs(tri.normals[1].w));
            Homogeneous4 n3(abs(tri.normals[2].x), abs(tri.normals[2].y), abs(tri.normals[2].z), abs(tri.normals[2].w));
            color = n1 * bari.x + n2 * bari.y + n3 * bari.z;
        }
        //blinn phong shading
        else if (renderParameters->phongEnabled == 1) {
            float EPS = std::numeric_limits<float>::epsilon();
            //normal
            Homogeneous4 n1(tri.normals[0].x, tri.normals[0].y, tri.normals[0].z, tri.normals[0].w);
            Homogeneous4 n2(tri.normals[1].x, tri.normals[1].y, tri.normals[1].z, tri.normals[1].w);
            Homogeneous4 n3(tri.normals[2].x, tri.normals[2].y, tri.normals[2].z, tri.normals[2].w);
            Cartesian3 N=n1.Vector()*bari.x+n2.Vector() * bari.y + n3.Vector() * bari.z;
            float cosine = r.direction.dot(N);
            Ray reflectRay=reflect(r,N,intersect);
            float reflectivity=1-tri.shared_material->transparency;

            //ior
            float ior_from = curIOR, ior_to = tri.shared_material->indexOfRefraction;
            if(cosine > 0)
            {
                ior_from = ior_to;
                ior_to = 1.f;
            }

            auto ior=ior_from/ior_to;

            Ray refractRay=refract(r,N,intersect,ior);
            if (tri.shared_material->isLight())
            {
                return tri.shared_material->emissive;
            }
            if(renderParameters->fresnelRendering && (tri.shared_material->transparency > EPS || !renderParameters->monteCarloEnabled)){
                fresnel(r, N,ior_from,ior_to,reflectivity);
                if(renderParameters->reflectionEnabled && tri.shared_material->reflectivity > EPS && cosine < 0){
                   auto reflectedColor=tri.shared_material->reflectivity*getColor(reflectRay,--depth,curIOR);
                   color=(1 - tri.shared_material->reflectivity) * color+reflectivity * getColor(reflectRay,--depth,curIOR);
                }
                if(renderParameters->refractionEnabled&&tri.shared_material->transparency!=0){
                   auto refractedColor = tri.shared_material->transparency * getColor(refractRay,--depth,ior_to);
                   color=(1 - tri.shared_material->transparency) * color+(1-reflectivity) * getColor(refractRay,--depth,ior_to);
                }
            }
            else{
                if(renderParameters->reflectionEnabled && tri.shared_material->reflectivity > EPS && cosine < 0){
                   auto reflectedColor=tri.shared_material->reflectivity*getColor(reflectRay,--depth,curIOR);
                   color = (1 - tri.shared_material->reflectivity) * color +reflectedColor;

                }
                if(renderParameters->refractionEnabled==1&&tri.shared_material->transparency!=0){
                   auto refractedColor = tri.shared_material->transparency * getColor(refractRay,--depth,ior_to);
                   color = (1 - tri.shared_material->transparency) * color + refractedColor;
                }
            }

            //loop through each light
            int len = renderParameters->lights.size();
            for (int i = 0; i < len; i++) {
                //light position
                Cartesian3 lightPos = renderParameters->lights[i]->GetPositionCenter().Point() ;
                Cartesian3 li = m_l * lightPos - intersect;
                //light color
                Cartesian3 lightColor=renderParameters->lights[i]->GetColor().Point();

                Cartesian3 N_norm = N.unit();
                //light direction
                Cartesian3 L_norm = li.unit();
                //ray direction
                Cartesian3 R_norm = (r.origin-intersect).unit();
                //half vector
                Cartesian3 h_norm = (L_norm + R_norm).unit();
                //cos_theta
                float m1 = fmax(N_norm.dot(h_norm), 0.f);
                float m2 = fmax(N_norm.dot(L_norm), 0.f);
                float dis=(m_l * lightPos-intersect).dot(m_l * lightPos-intersect);

                Cartesian3 specularLight(1.0,1.0,1.0);
                Cartesian3 diffuseLight(1.0,1.0,1.0);

                //emissive
                Cartesian3 emissive=tri.shared_material->emissive;
                //ambient
                Cartesian3 ambient=tri.shared_material->ambient;

                //specular
                Cartesian3 specular=cwise(specularLight,tri.shared_material->specular/dis)* pow(m1,tri.shared_material->shininess);

                //diffuse
                Cartesian3 diffuse=cwise(diffuseLight,tri.shared_material->diffuse/dis)*m2;

                if(renderParameters->fresnelRendering){
                    float FresnelValue;
                    fresnel(r, N,curIOR,ior_to,FresnelValue);
                    Cartesian3 specular = lerp(diffuse, specular, FresnelValue);
                }

                //if do shadow
                if(renderParameters->shadowsEnabled == 1){
                    Ray shadow_ray( m_l*lightPos,(intersect-m_l*lightPos).unit(),Ray::secondary);
                    Scene::CollisionInfo ci2=raytraceScene.closestTriangle(shadow_ray);
                    float distanceLight=li.length();
                    float bias=1e-2;
                    //the distance should be smaller than bias,or there will be shadow
                    if (!(abs(ci2.t-distanceLight)<bias) && ci2.tri.shared_material)
                    {
                        //handle opaque object
                        if(!(ci2.tri.shared_material->transparency > 0.f && renderParameters->refractionEnabled) && !ci2.tri.shared_material->isLight())
                            color=color+diffuse;
                        else if(ci2.tri.shared_material->transparency > 0.f && renderParameters->refractionEnabled )
                        {
                               float shadowStrength =ci2.tri.shared_material->transparency * 1.5;
                               color = color +shadowStrength * diffuse;
                        }
                    }

                    else {
                        color=color+diffuse+specular;
                    }
                    color=color+ambient+emissive;
                 }
                else{
                    color=color+ambient+emissive+diffuse;
                }


            }
        }

          //if no buttion is chosen,return white
          else {
              color.x = 1;
              color.y = 1;
              color.z = 1;
              color.w = 1;
           }
    }

    else{
        color.x = 0.3;
        color.y = 0.3;
        color.z = 0.3;
        color.w = 0.3;
    }

    return color;
}



void RaytraceRenderWidget::RaytraceThread()
{
    int loops = renderParameters->monteCarloEnabled? N_LOOPS:1;
    std::cout << "I Will do " << loops << " loops" << std::endl;

    //Each pixel in parallel using openMP.
    for(int loop = 0; loop < loops; loop++){
        #pragma omp parallel for schedule(dynamic)
        for(int j = 0; j < frameBuffer.height; j++){
            for (int i = 0; i < frameBuffer.width; i++) {
                //TODO: YOUR CODE GOES HERE
                // calculate your raytraced color here.
                float x = (2.0f * i) / float(frameBuffer.width) - 1.0;
                float y = (2.0f * j) / float(frameBuffer.height) - 1.0;
                //produce eye-pixel ray
                Cartesian3 s(0, 0, 0);
                Cartesian3 l(x, y, -1);
                l = l.unit();
                Ray r(s, l, Ray::primary);
                int depth=100;
                Homogeneous4 color=getColor(r,depth,1.0f);
                //Gamma correction....
                float gamma = 2.2f;
                //We already calculate everything in float, so we just do gamma correction before putting it integer format.
                color.x = pow(color.x,1/gamma)/float(loop+1);
                color.y = pow(color.y,1/gamma)/float(loop+1);
                color.z = pow(color.z,1/gamma)/float(loop+1);
                frameBuffer[j][i] = ((loop)/float(loop+1))*frameBuffer[j][i]+  RGBAValue(color.x*255.0f,color.y*255.0f,color.z*255.0f,255.0f);
                frameBuffer[j][i].alpha = 255;

           }
        }
                std::cout << " Done a loop!" << std::endl;
    }
                       if(restartRaytrace){
                           return;
                       }
                   std::cout << "Done!" << std::endl;
}
// mouse-handling
void RaytraceRenderWidget::mousePressEvent(QMouseEvent *event)
    { // RaytraceRenderWidget::mousePressEvent()
    // store the button for future reference
    int whichButton = int(event->button());
    // scale the event to the nominal unit sphere in the widget:
    // find the minimum of height & width
    float size = (width() > height()) ? height() : width();
    // scale both coordinates from that
    float x = (2.0f * event->x() - size) / size;
    float y = (size - 2.0f * event->y() ) / size;


    // and we want to force mouse buttons to allow shift-click to be the same as right-click
    unsigned int modifiers = event->modifiers();

    // shift-click (any) counts as right click
    if (modifiers & Qt::ShiftModifier)
        whichButton = Qt::RightButton;

    // send signal to the controller for detailed processing
    emit BeginScaledDrag(whichButton, x,y);
    } // RaytraceRenderWidget::mousePressEvent()

void RaytraceRenderWidget::mouseMoveEvent(QMouseEvent *event)
    { // RaytraceRenderWidget::mouseMoveEvent()
    // scale the event to the nominal unit sphere in the widget:
    // find the minimum of height & width
    float size = (width() > height()) ? height() : width();
    // scale both coordinates from that
    float x = (2.0f * event->x() - size) / size;
    float y = (size - 2.0f * event->y() ) / size;

    // send signal to the controller for detailed processing
    emit ContinueScaledDrag(x,y);
    } // RaytraceRenderWidget::mouseMoveEvent()

void RaytraceRenderWidget::mouseReleaseEvent(QMouseEvent *event)
    { // RaytraceRenderWidget::mouseReleaseEvent()
    // scale the event to the nominal unit sphere in the widget:
    // find the minimum of height & width
    float size = (width() > height()) ? height() : width();
    // scale both coordinates from that
    float x = (2.0f * event->x() - size) / size;
    float y = (size - 2.0f * event->y() ) / size;

    // send signal to the controller for detailed processing
    emit EndScaledDrag(x,y);
    } // RaytraceRenderWidget::mouseReleaseEvent()

template<typename T>
T RaytraceRenderWidget::lerp(const T& a, const T& b, float t) {
    return a + t * (b - a);
}

template<typename T>
T RaytraceRenderWidget::clamp(const T& value, const T& low, const T& high) {
    return std::max(low, std::min(value, high));
}

float RaytraceRenderWidget::smoothStep(float edge0, float edge1, float x) {
    float t = clamp<float>((x - edge0) / (edge1 - edge0), 0.f, 1.f);
    return t * t * (3.0 - 2.0 * t);
}






