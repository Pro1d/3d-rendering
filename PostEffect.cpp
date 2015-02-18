#include "PostEffect.h"
#include "color.h"
#include <algorithm>
#include <cmath>

using namespace std;

PostEffect::PostEffect(rgb_f *color, float *depth, rgb_f* destination, int width, int height) : color(color), depth(depth), dst(destination), width(width), height(height), enabled(true)
{
    //ctor
}

PostEffect::~PostEffect()
{
    //dtor
}

DistanceFog::DistanceFog(rgb_f *color, float *depth, rgb_f* destination, int width, int height, rgb_f const& fogColor, float density) :
    PostEffect(color, depth, destination, width, height), fogColor(fogColor), density(density)
{

}
void DistanceFog::setFogDensity(float d)
{
    density = d;
}
float DistanceFog::getFogDensity()
{
    return density;
}
void DistanceFog::setFogColor(rgb_f const& color)
{
    fogColor = color;
}
void DistanceFog::apply()
{
    for(int i = width * height; --i >= 0;) {
        float k = pow((1-density), depth[i]);
        dst[i] = color[i] * k + fogColor * (1-k);
    }
}


DepthOfField::DepthOfField(rgb_f *color, float *depth, rgb_f* destination, int width, int height, float depthFocus, float depthMax, float radiusMax) :
        PostEffect(color, depth, destination, width, height), depthFocus(depthFocus), depthMax(depthMax), radiusMax(radiusMax)
{

}
void DepthOfField::setDepthFocus(float focus)
{
    depthFocus = focus;
}
void DepthOfField::setDepthMax(float max)
{
    depthMax = max;
}
void DepthOfField::setRadiusMax(float radius)
{
    radiusMax = radius;
}
void DepthOfField::apply()
{
    int pixelCount = width * height;
    float *CoCarray = new float[pixelCount*3];
    float scale = depthMax / (depthMax - depthFocus);
    for(int i = pixelCount; --i >= 0;) {
        /// Rayon
        CoCarray[i*3] = radiusMax * clampIn01(scale * abs(1.0f - depthFocus / depth[i]));
        /// Carré du rayon
        CoCarray[i*3+1] = CoCarray[i*3]*CoCarray[i*3];
        /// Transparence
        CoCarray[i*3+2] = clampIn01(1.0f/CoCarray[i*3+1]);
    }

    memset(dst, 0, sizeof(rgb_f)*pixelCount);

    int i = 0;
    for(int y = 0; y < height; y++)
    for(int x = 0; x < width; x++, i++) {
        //if(depth[i] < zFar)
        {
            const float CoC = CoCarray[i*3], CoC2 = CoCarray[i*3+1];
            const float alpha = CoCarray[i*3+2];
            const int c = CoC+1;//0.5f;
            for(int a = max(0, x-c), aa = min(width-1, x+c); a <= aa ; a++)
            for(int b = max(0, y-c), bb = min(height-1, y+c); b <= bb ; b++) {
                int d2 = (a-x)*(a-x)+(b-y)*(b-y);
                //if(depthBuf[i] <= depthBuf[a+b*width]+CoC) // zi za
                if(CoC2 >= d2)
                {
                    float overAlpha = (depth[i] > depth[a+b*width]+CoC) ? clampIn01(1.0f-CoCarray[(a+b*width)*3+2]) : 1;
                    dst[a+b*width].add(color[i], alpha * overAlpha);
                }
            }
        }

    }

    delete[] CoCarray;

    for(i = 0; i < height*width; ++i) {
        rgb_f c(dst[i].getRGB(color[i]));
        dst[i].r = c.r;
        dst[i].g = c.g;
        dst[i].b = c.b;
    }
}
