#ifndef TRANSFORMATION_H
#define TRANSFORMATION_H

#include "Matrix.h"
#include <stack>

enum {PRE=false, POST=true};

class Transformation
{
    public:
        Transformation();
        Transformation(float a, float b, float c, float d,
               float e, float f, float g, float h,
               float i, float j, float k, float l,
               float m, float n, float o, float p);
        /// matrice actuelle <- identité
        void clear();
        /// push/pop :)
        void push()  { matrixStack.push(matrixStack.top()); }
        void pop() { if(matrixStack.size() > 1) matrixStack.pop(); }

        void rotateX(float a, bool post=POST);
        void rotateY(float a, bool post=POST);
        void rotateZ(float a, bool post=POST);
        void translate(float x, float y, float z, bool post=POST);
        void rotate(float a, float *axis, bool post=POST);
        void translateX(float t, bool post=POST);
        void translateY(float t, bool post=POST);
        void translateZ(float t, bool post=POST);
        void transform(Matrix const& mat, bool post=POST);
        void transform(Transformation const& mat, bool post=POST);
        void scale(float s, bool post=POST);
        void fitWithScreen(float screenWidth, float screenHeight);

        void applyTo(const float *p, float *r) const;
        void applyToVector(const float *p, float *r) const;
        void applyToUnitaryVector(const float *p, float *r) const;
        static void applyPerspectiveTo(float *p, float screenWidth, float screenHeight, float zNear, float *dst);

        /// Retourne le carré de l'échelle globale
        float getGlobalScale_2();
        float getMaxGlobalScale();

        Matrix const& getMatrix() const { return matrixStack.top(); }//m[current_matrix]; }
        Matrix& getMatrix() { return matrixStack.top(); }//m[current_matrix]; }
    protected:
        //inline Matrix& getMatrixBuf() { return m[!current_matrix]; }
        void commitBuf() { matrixStack.top() = buf; }// current_matrix = !current_matrix; }
    private:
       // Matrix m[2];
        std::stack<Matrix> matrixStack;
        Matrix tmp;
        Matrix buf;
        //int current_matrix;
};

#endif // TRANSFORMATION_H
