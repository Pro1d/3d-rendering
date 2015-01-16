#ifndef RAYTRACING_H
#define RAYTRACING_H

#include "color.h"
#include "Scene.h"
#include "Object3D.h"
#include "KDTree.h"
#include "MultiThread.h"
#include <list>

class RayData {
    public:
        float dir[3], orig[3];/// donnés à getColorRay
        /// calculés par getColorRay
        float length;
        float intensity;
        rgb_f color;
};

class RayTracing
{
    public:
        RayTracing();
        ~RayTracing();
        void setParams(int w, int h, float z) { width = w; height = h; zNear = z; }
        void setColorBuf(rgb_f *cbuf) { colorBuf = cbuf; }

        void render(Scene &sceneBuffer);
        void renderMutliThread(Scene &sceneBuffer, MultiThread &mt);

        int getFaceCollision(RayData const& ray, Scene &sceneBuffer, float &distOut, bool &reverseSideOut, int lastFaceColl);
        bool isFaceCollision(const float *rayDir, const float *rayOrig, Scene const &sceneBuffer, float distMax, int lastFaceColl);
        void getCoef(Transf_Vertex const& A, Transf_Vertex const& B, Transf_Vertex const& C, RayData const& ray, float *coef);
        float getDist(Transf_Vertex const& A, Transf_Vertex const& B, Transf_Vertex const& C, RayData const& ray);

        float getRefractRay(RayData &rayTransmit, const float *orig, const float *normal, float n1, float n2);
        void getReflectRay(RayData &rayReflect, const float *orig, const float* normal, float n1, float n2);

        rgb_f getColorLight(Scene const& sceneBuffer, rgb_f const& color, Object3D const& obj, float *normal, float *position, int faceColl);
        void getColorRay(RayData &ray, Scene &sceneBuffer, float currentIndice = 1.0f, int lastFaceColl = -1);
        rgb_f getColorSky(float *raydir, Scene &sceneBuffer);

        /// PRIVATE
        int width, height;
        float zNear;
        rgb_f* colorBuf;
        KDTree *currentTree;
    protected:
    private:
};

#endif // RAYTRACING_H
