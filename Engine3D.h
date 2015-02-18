#ifndef ENGINE3D_H
#define ENGINE3D_H

#include <SDL.h>
#include <algorithm>
#include <cmath>
#include <queue>
#include <vector>
#include "Object3D.h"
#include "Model3D.h"
#include "color.h"
#include "Transformation.h"
#include "Camera.h"
#include "Light.h"
#include "Scene.h"
#include "MultiThread.h"
#include "RayTracing.h"

#define ANTI_ALIASING   0
#define THREAD_COUNT  8
#define MTH_ENABLED 1
#define DRAW_NORMAL_MAP 0
#define RAYTRACING_ENABLED  1



class PixelVolume
{
    public:
        PixelVolume(int d, bool f) : depth(d), front(f) {}
        // int volumeId;
        float depth;
        bool front;

};
class PixelVolumeComparison
{
    public:
        bool operator() (const PixelVolume& lhs, const PixelVolume&rhs) const {
            return (lhs.depth > rhs.depth);
        }
};
typedef std::priority_queue<PixelVolume, std::vector<PixelVolume>, PixelVolumeComparison> HeapVolumeDepth;
class Engine3D
{
    public:
        Engine3D(SDL_Surface* s);
        virtual ~Engine3D();

        void drawTriangle();

        void rotateCamX(float a) { cam.rotateX(a); }
        void rotateCamY(float a) { cam.rotateY(a); }
        void translateCamForward(float z) { cam.translate(0,0,z); }
        void translateCamBackward(float z) { cam.translate(0,0,-z); }
        void translateCamLeft(float x) { cam.translate(x,0,0); }
        void translateCamRight(float x) { cam.translate(-x,0,0); }
        void translateCamUp(float y) { cam.translate(0,y,0); }
        void translateCamDown(float y) { cam.translate(0,-y,0); }
        void scaleCam(float factor) { cam.scale(factor); }

        float getZFar() { return zFar; }
        void decreaseFoV() { fov = std::max(fov-5.0f, 5.0f); updateZNearFar(); }
        void increaseFoV() { fov = std::min(fov+5.0f, 175.0f); updateZNearFar(); }
        void updateZNearFar() {
            zFar -= zNear;
            zNear = /*1.0/*/screenWidth/**//2 / tan(fov / 2.0f * M_PI/180.0f);
            zFar += zNear;
            mRT.setParams(width, height, zNear);
            #if defined(DEBUG) | defined(_DEBUG)
                printf("zNearn=%f\n", zNear);
                printf("fov=%f->zNear=%f\n", fov, zNear);
            #endif
        }
        int getWidth() { return width; }
        int getHeight() { return height; }
        rgb_f* getColorBuf() { return colorBuf; }
        rgb_f* getColorBuf2() { return colorBuf_f; }
        rgb_f* getColorBuf3() { return colorBuf_g; }
        float* getDepthBuf() { return depthBuf; }

        void toggleDrawingFilter() { enableDrawingFilter = !enableDrawingFilter; }
        void toggleRayTracing() { enableRayTracing = !enableRayTracing; }
        void toggleVolumeRendering() { enableVolumeRendering = !enableVolumeRendering; if(enableVolumeRendering) for(int i = 0; i < src->w*src->h; i++)while(!volumeDepthBuf[i].empty()) volumeDepthBuf[i].pop(); }
        void toggleDepthOfField() { enableDepthOfField = !enableDepthOfField; }
        void toggleFog() { enableFog = !enableFog; }
        void toggleGlow() { enableGlow = !enableGlow; }
        void toggleBlurMotion() { enableBlurMotion = !enableBlurMotion; }
        void setPixelFocus(int x, int y) { pixelFocusIndex = x + y*src->w; }

        int getPixelCount() { return pixelCount; }
        void putPixel(int x, int y, rgb_f const& c);
        void putPixel(int x, int y, Uint8 r, Uint8 g, Uint8 b);
        void putPointBuf(rgb_f *dst, float* depth, int x, int y, float z, rgb_f const& c);
        void clearBuf();
        void clearZBuffer();
        void glowMultiThread();
        void glow();
        void volumeRenderingMultiThread();
        void volumeRendering();
        void distanceFogMultiThread();
        void distanceFog();

        void postEffects();
        void filters2D();
        void toBitmap();
        static void toBitmap(SDL_Surface *bmp, rgb_f* color, int width, int height);

        void drawScene(Scene& scene);
        void drawSkyBox(Object3D const& sky);

        void setVertexColorWithLights(Transf_Vertex & vertex, Scene const& sceneBuffer);

        /// Triangle couleur volume
        void triangleVolume(const float* A, const float* B, const float* C, rgb_f const& c, bool front);
        /// Triangle couleurs interpolées
        void triangle(rgb_f *dst, float* depth, const float* A, const float* B, const float* C, rgb_f const& colorA, rgb_f const& colorB, rgb_f const& colorC);

    protected:
        bool isOrientZ(const float* A, const float* B, const float* C);
        rgb_f getColor(rgb_f const& mat, float *normal);

        /// Triangle couleur volume
        void triangleSupVolume(const float *A, const float *B, const float *C, rgb_f const& color, bool front);
        void triangleInfVolume(const float *A, const float *B, const float *C, rgb_f const& color, bool front);
        void triangleSortedPointsVolume(const float* A, const float* B, const float* C, rgb_f const& c, bool front);
        /// Triangle couleurs interpolées
        void triangleSup(rgb_f *dst, float* depth, const float *A, const float *B, const float *C, rgb_f const& colorA, rgb_f const& colorB, rgb_f const& colorC);
        void triangleInf(rgb_f *dst, float* depth, const float *A, const float *B, const float *C, rgb_f const& colorA, rgb_f const& colorB, rgb_f const& colorC);
        void triangleSortedPoints(rgb_f *dst, float* depth, const float* A, const float* B, const float* C, rgb_f const& colorA, rgb_f const& colorB, rgb_f const& colorC);

        void putPointBufSecure(int x, int y, float z, rgb_f const&  c);
        void putPointBufVolume(int x, int y, float z, rgb_f const&  c, bool front);
        void putPointBufSecureVolume(int x, int y, float z, rgb_f const&  c, bool front);

        void drawObject(Object3D const& obj, bool enablePerspective = false);
        void draw(Model3D const& model);

        void objectToBuffer(Object3D const& object, Scene& sceneBuffer);
        void modelToBuffer(Model3D const& model, Scene& sceneBuffer);
        void drawMultiThread(Scene& sceneBuffer);
        void draw(Scene& sceneBuffer);


    private:
        SDL_Surface *src;
        int width, height;
        int pixelCount;

        /// COLOR BUFFER
        rgb_f *colorBuf;
        rgb_f *colorBuf_f;
        rgb_f *colorBuf_g;

        /// DEPTH BUFFER
        float *depthBuf;

        /// VOLUME RENDERING
        HeapVolumeDepth *volumeDepthBuf;
        bool enableVolumeRendering;
        rgb_f volumeColor;

        /// MODEL
        Transformation transformation;

        /// FOG
        bool enableFog;
        float fogNear, fogFar;
        rgb_f fogColor;

        /// CAMERA
        float zNear, zFar;
        float xMin, xMax, yMin, yMax;
        float screenWidth, screenHeight, screenRatio;
        float fov;
        NaturalCamera cam;

        /// DEPTH OF FIELD
        bool enableDepthOfField;
        float zDoFFocus, speedChangeFocusDoF;
        float maxRadiusDoF, gradientRadiusDoF;
        int pixelFocusIndex;

        /// GLOW (Bloom)
        bool enableGlow;
        float glowRadius;
        float minLuminosityGlow;

        /// BLUR MOTION
        bool enableBlurMotion;
        float speedBlurMotion;

        /// DRAWING FILTER (2D)
        bool enableDrawingFilter;

        /// MULTI-THREADING
        MultiThread mTh;

        /// RAY TRACING
        RayTracing mRT;
        bool enableRayTracing;
};



#endif // ENGINE3D_H
