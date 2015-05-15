#ifndef NOISE2D_H
#define NOISE2D_H

#include <cstdlib>

class Noise2D
{
    public:
        Noise2D(int width, int height);
        virtual ~Noise2D();

        void setSeed(unsigned int seed) { srand(seed); }
        void changeSeed() { srand(rand()); }

        void setPeriods(float p) { periods = p; }
        void setOctaves(int o) { octaves = o; }
        void setPersistance(float p) { persistance = p; }

        //float get(float x, float y);
        void setSize(int w, int h) { width = w; height = h; }
        float get(int x, int y) { return values[x+y*width]; }
        float get(int index) { return values[index]; }

        void create();
    protected:
    private:
        float randomFloat() { return (float) (rand()%1000) / 1000.0f; }

        float persistance;
        int periods, octaves;
        int height, width;

        float *values;
};

#endif // NOISE2D_H
