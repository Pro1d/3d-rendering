#include "Noise2D.h"
#include "Interpolator.h"
#include <cmath>
#include <iostream>
#include <cstdio>
#include <ctime>
#include <cstdlib>

Noise2D::Noise2D(int l, int h, int p, int n, int nbOctaveMax, float pers, int seed, interpol_e it, int nbFr) : taille(0), pas2D(p), hauteur(h), longueur(l), persistance(pers),
        nombre_octaves2D(nbOctaveMax) {

    iType = it;
    nbFrame = nbFr;
    frame = 0;

    longueur_max = (int) ceil(longueur * pow(2, nombre_octaves2D  - 1)  / pas2D);
    hauteur_max = (int) ceil(hauteur * pow(2, nombre_octaves2D  - 1)  / pas2D);

    srand(seed);
    valeurs2D[0] = new float[longueur_max * hauteur_max];
    valeurs2D[1] = new float[longueur_max * hauteur_max];
    valeurs2D[2] = new float[longueur_max * hauteur_max];
    valeurs2D[3] = new float[longueur_max * hauteur_max];
    nextValues();
    nextValues();
    nextValues();
    nextValues();

    harmonizedValues = new float[longueur_max * hauteur_max];

    nombre_octaves2D = n;
}
Noise2D::~Noise2D() {
    delete[] valeurs2D[0];
    delete[] valeurs2D[1];
    delete[] valeurs2D[2];
    delete[] valeurs2D[3];
}

void Noise2D::nextValues() {
    float* tmp =  valeurs2D[0];
    valeurs2D[0] = valeurs2D[1];
    valeurs2D[1] = valeurs2D[2];
    valeurs2D[2] = valeurs2D[3];
    valeurs2D[3] = tmp;
    for(int i = longueur_max * hauteur_max; --i >= 0;)
        valeurs2D[3][i] = ((float) rand()) / RAND_MAX;
}
void Noise2D::nextFrame() {
    if(++frame >= nbFrame) {
        frame = 0;
        nextValues();
    }
    float a = (float) frame  / nbFrame;
    float b = 0.5*a*a;
    a = 1-a;
    float c = 0.5*a*a;
    k[3] = b;
    k[2] = 1.0 - c;
    k[1] = 1.0 - b;
    k[0] = c;

    k[0] /= 2;
    k[1] /= 2;
    k[2] /= 2;
    k[3] /= 2;

    harmonize();
}

float Noise2D::sommeValeurs2D(int i) {
    return valeurs2D[0][i] * k[0]
         + valeurs2D[1][i] * k[1]
         + valeurs2D[2][i] * k[2]
         + valeurs2D[3][i] * k[3];
}
void Noise2D::harmonize() {
    for(int i = longueur_max * hauteur_max; --i >= 0;)
    {
        harmonizedValues[i] = (1 - cos(sommeValeurs2D(i) * M_PI)) / 2;
        //printf("%d\n", (int)(harmonizedValues[i]*100));
    }
    //printf("End Value\n");
}

float Noise2D::interpolation_cos1D(float a, float b, float x) {
    float k = (1 - cos(x * M_PI)) / 2;
    return a * (1 - k) + b * k;
}
float Noise2D::interpolation_lin1D(float a, float b, float x) {
    return x * (b - a) + a;
}

float Noise2D::interpolation_cos2D(float a, float b, float c, float d, float x, float y) {
   float y1 = interpolation_cos1D(a, b, x);
   float y2 = interpolation_cos1D(c, d, x);

   return  interpolation_cos1D(y1, y2, y);
}
float Noise2D::interpolation_lin2D(float a, float b, float c, float d, float x, float y) {
   float y1 = interpolation_lin1D(a, b, x);
   float y2 = interpolation_lin1D(c, d, x);

   return  interpolation_lin1D(y1, y2, y);
}

float Noise2D::bruit2D(int i, int j) {
    return harmonizedValues[i * longueur_max + j];
}

float Noise2D::fonction_bruit2D(float x, float y) {
    int i = (int) (x);// pas2D);
    int j = (int) (y);// pas2D);
    switch(iType) {
     case COS:
        return interpolation_cos2D(bruit2D(i, j), bruit2D(i + 1, j), bruit2D(i, j + 1), bruit2D(i + 1, j + 1), fmod(x /* pas2D*/, 1), fmod(y /* pas2D*/, 1));
    case QUAD:
        return interpolation_lin2D(bruit2D(i, j), bruit2D(i + 1, j), bruit2D(i, j + 1), bruit2D(i + 1, j + 1), fmod(x /* pas2D*/, 1), fmod(y /* pas2D*/, 1));
    case LIN: default:
        return interpolation_lin2D(bruit2D(i, j), bruit2D(i + 1, j), bruit2D(i, j + 1), bruit2D(i + 1, j + 1), fmod(x /* pas2D*/, 1), fmod(y /* pas2D*/, 1));
    }
}
float Noise2D::bruit_coherent2D(float x, float y) {
    float somme = 0;
    float p = 1;

    for(int i = 0, f = 1; i < nombre_octaves2D ; ++i, f *= 2) {
        float z = fonction_bruit2D(x/pas2D * f, y/pas2D * f);
        somme += p * z;
        p *= persistance;
    }
    float result = somme * (1 - persistance) / (1 - p);


    return result;
}

void Noise2D::initModel3D(int _nbPt) {
    nbPt = _nbPt;
    model3D.vertex.clear();
    model3D.vertex.resize(nbPt*nbPt);
    model3D.faces.clear();
    model3D.faces.resize((nbPt-1)*(nbPt-1)*2);
}
Object3D& Noise2D::getModel3D(int interpolatorType, float interpolatorNiv, float interpolatorHard) {
    for(int x = 0; x < nbPt; x++)
    for(int y = 0; y < nbPt; y++)
    {
        float z = bruit_coherent2D((float) x / (nbPt-1) * longueur, (float) y / (nbPt-1) * hauteur);
        Vertex &v = model3D.vertex[x+y*nbPt];
        v.n[0] = v.n[1] = v.n[2] = 0.0f;
        v.c.r = v.c.g = v.c.b = 0.5f;
        v.p[0] = (float) x / (nbPt-1) * 2.0f - 1.0f;
        v.p[1] = Interpolator::get(interpolatorType, z, interpolatorNiv, interpolatorHard) * 0.42f;
        v.p[2] = (float) y / (nbPt-1) * 2.0f - 1.0f;
    }
    return model3D;
}
