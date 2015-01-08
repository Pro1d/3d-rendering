#ifndef NOISE2D_H
#define NOISE2D_H

#include "Object3D.h"

typedef enum {COS, LIN, QUAD} interpol_e;

class Noise2D
{
    public:
        Noise2D(int l, int h, int p, int n, int nbOctaveMax, float pers, int seed, interpol_e it, int nbFrame);
        virtual ~Noise2D();

        float bruit_coherent2D(float x, float y);
        void nextValues();
        void nextFrame();

        void setNbOctave(int n) {nombre_octaves2D = n;}
        void setPersistance(float p) {persistance = p;}

        void initModel3D(int nbPt);
        Object3D& getModel3D(int interpolatorType, float interpolatorNiv, float interpolatorHard);

    protected:
    private:

        float sommeValeurs2D(int i);
        void harmonize();

        float interpolation_cos1D(float a, float b, float x);
        float interpolation_cos2D(float a, float b, float c, float d, float x, float y);
        float interpolation_lin1D(float a, float b, float x);
        float interpolation_lin2D(float a, float b, float c, float d, float x, float y);
        float bruit2D(int i, int j);
        float fonction_bruit2D(float x, float y);

        const int taille;
        const int pas2D;
        const int hauteur;
        const int longueur;
        float persistance;
        int nombre_octaves2D;
        int longueur_max;
        int hauteur_max;
        float* valeurs2D[4];
        float* harmonizedValues;
        float* interpol1D;
        float k[4];
        int nbFrame, frame;

        interpol_e iType;

        Object3D model3D;
        int nbPt;
};

#endif // NOISE2D_H
