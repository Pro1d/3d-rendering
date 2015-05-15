#include "Noise2D.h"
#include <cstring>
#include <cmath>

Noise2D::Noise2D(int width, int height) : width(width), height(height), periods(2), persistance(0.65f), octaves(6)
{
    values = new float[width * height];
    create();
}

Noise2D::~Noise2D()
{
    //dtor
}

void Noise2D::create()
{
    int size[octaves];
    float* map[octaves];
    for(int i = 0; i < octaves; i++) {
        size[i] = (periods << i) + 1;
        map[i] = new float[size[i]*size[i]];
        for(int j = 0; j < size[i]*size[i]; j++)
            map[i][j] = randomFloat();
    }

    memset(values, 0, sizeof(float)*width*height);

    float persistanceSum = (persistance == 1 ? octaves : (1-pow(persistance, octaves)) / (1-persistance));

    for(int i = 0; i < octaves; i++)
    {
        float octavePersistance = pow(persistance, i+1) / persistanceSum;
        int w = width  / (size[i]-1);
        int h = height / (size[i]-1);
        for(int y = 0; y < height; y++)
        {
            float dy = (float) (y % h) / h;
            int py = y/h;
            for(int x = 0; x < width; x++)
            {
                float dx = (float) (x % w) / w;
                int px = x/w;
                float zy1 = (1-dy) * map[i][px+py*size[i]] + dy * map[i][px+(1+py)*size[i]];
                float zy2 = (1-dy) * map[i][(1+px)+py*size[i]] + dy * map[i][(1+px)+(1+py)*size[i]];
                float z = (1-dx) * zy1 + dx * zy2;
                values[x + y*width] += z * octavePersistance;
            }
        }
    }

    for(int i = 0; i < octaves; i++)
        delete[] map[i];
}
