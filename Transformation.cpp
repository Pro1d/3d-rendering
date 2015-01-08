#include "Transformation.h"
#include "Matrix.h"
#include "tools.h"
#include <cmath>
#include <stack>

using namespace std;

Transformation::Transformation()
{
    matrixStack.push(Matrix());
}
Transformation::Transformation(float a, float b, float c, float d,
               float e, float f, float g, float h,
               float i, float j, float k, float l,
               float m, float n, float o, float p)
{
    matrixStack.push(Matrix(a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p));
}

void Transformation::clear() {
    getMatrix().setIdentity();
}

void Transformation::rotateX(float a, bool post) {
    float cos_ = cos(a), sin_ = sin(a);
    tmp.set(1,0,   0,    0,
            0,cos_,-sin_,0,
            0,sin_, cos_,0,
            0,0,   0,    1);

    if(post)
        getMatrix().multiplyBy(tmp, buf);
    else
        tmp.multiplyBy(getMatrix(), buf);

    commitBuf();
}

void Transformation::rotateY(float a, bool post) {
    float cos_ = cos(a), sin_ = sin(a);
    tmp.set(cos_, 0,sin_,0,
            0,    1,0,   0,
            -sin_,0,cos_,0,
            0,    0,0,   1);

    if(post)
        getMatrix().multiplyBy(tmp, buf);
    else
        tmp.multiplyBy(getMatrix(), buf);

    commitBuf();
}

void Transformation::rotateZ(float a, bool post) {
    float cos_ = cos(a), sin_ = sin(a);
    tmp.set(cos_,-sin_,0,0,
            sin_, cos_,0,0,
            0,    0,   1,0,
            0,    0,   0,1);

    if(post)
        getMatrix().multiplyBy(tmp, buf);
    else
        tmp.multiplyBy(getMatrix(), buf);

    commitBuf();
}
void Transformation::rotate(float a, float *axis, bool post) {
    float _sin = sin(a), _cos = cos(a);
    tmp.set(_cos+axis[0]*axis[0]*(1-_cos), axis[0]*axis[1]*(1-_cos)-axis[2]*_sin, axis[0]*axis[2]*(1-_cos)+axis[1]*_sin, 0,
            axis[0]*axis[1]*(1-_cos)+axis[2]*_sin, _cos+axis[1]*axis[1]*(1-_cos), axis[1]*axis[2]*(1-_cos)-axis[0]*_sin, 0,
            axis[0]*axis[2]*(1-_cos)-axis[1]*_sin, axis[1]*axis[2]*(1-_cos)+axis[0]*_sin, _cos+axis[2]*axis[2]*(1-_cos), 0,
            0,0,0,1);

    if(post)
        getMatrix().multiplyBy(tmp, buf);
    else
        tmp.multiplyBy(getMatrix(), buf);

    commitBuf();
}

void Transformation::translateX(float t, bool post) {
    Matrix &m = getMatrix();
    if(post)
        m.mat[3] += t;
    else {
        m.mat[3] += t*m.mat[0];
        m.mat[7] += t*m.mat[4];
        m.mat[11] += t*m.mat[8];
    }
}

void Transformation::translateY(float t, bool post) {
    Matrix &m = getMatrix();
    if(post)
        m.mat[7] += t;
    else {
        m.mat[3] += t*m.mat[1];
        m.mat[7] += t*m.mat[5];
        m.mat[11] += t*m.mat[9];
    }
}

void Transformation::translateZ(float t, bool post) {
    Matrix &m = getMatrix();
    if(post)
        m.mat[11] += t;
    else {
        m.mat[3] += t*m.mat[2];
        m.mat[7] += t*m.mat[6];
        m.mat[11] += t*m.mat[10];
    }
}
void Transformation::translate(float x, float y, float z, bool post) {
    Matrix &m = getMatrix();
    if(post) {
        m.mat[3] += x;
        m.mat[7] += y;
        m.mat[11] += z;
    }
    else { // TODO : check formules
        m.mat[3] += (x*m.mat[0] + y*m.mat[1] + z*m.mat[2]);
        m.mat[7] += (x*m.mat[4] + y*m.mat[5] + z*m.mat[6]);
        m.mat[11] += (x*m.mat[8] + y*m.mat[9] + z*m.mat[10]);
    }
}

void Transformation::scale(float s, bool post) {
    Matrix &m = getMatrix();
    if(post)
        for(int i = 0; i < 12; i++)
            m.mat[i] *= s;
    else {
        for(int i = 0; i < 11; i++)
            if((i+1)%4 != 0)
                m.mat[i] *= s;
    }
}

void Transformation::transform(Matrix const& mat, bool post) {
    if(post)
        getMatrix().multiplyBy(mat, buf);
    else
        mat.multiplyBy(getMatrix(), buf);
    commitBuf();
}

void Transformation::transform(Transformation const& trans, bool post) {
    if(post)
        getMatrix().multiplyBy(trans.getMatrix(), buf);
    else
        trans.getMatrix().multiplyBy(getMatrix(), buf);
    commitBuf();
}

void Transformation::fitWithScreen(float screenWidth, float screenHeight) {
    Matrix &m = getMatrix();
    m.mat[0] *= screenWidth * .5f;
    m.mat[1] *= screenWidth * .5f;
    m.mat[2] *= screenWidth * .5f;
    m.mat[3] += screenWidth * .5f;

    m.mat[4] *= -screenHeight * .5f;
    m.mat[5] *= -screenHeight * .5f;
    m.mat[6] *= -screenHeight * .5f;
    m.mat[7] += -screenHeight * .5f + screenHeight;
}


void Transformation::applyTo(const float *p, float *r) const {
    getMatrix().applyTo(p, r);
}
void Transformation::applyToVector(const float *v, float *r) const {
    getMatrix().applyTo(v, r);

    //Matrix &m = getMatrix();
    r[0] -= getMatrix().mat[3];
    r[1] -= getMatrix().mat[7];
    r[2] -= getMatrix().mat[11];
}
void Transformation::applyToUnitaryVector(const float *v, float *r) const {
    getMatrix().applyTo(v, r);

    //Matrix &m = getMatrix();
    r[0] -= getMatrix().mat[3];
    r[1] -= getMatrix().mat[7];
    r[2] -= getMatrix().mat[11];

    float norm = magic_sqrt_inv(r[0]*r[0] +r[1]*r[1] + r[2]*r[2]);
    r[0] *= norm;
    r[1] *= norm;
    r[2] *= norm;
}
void Transformation::applyPerspectiveTo(float *p, float screenWidth, float screenHeight, float zNear, float *dst)
{
    dst[0] = screenWidth/2 + (p[0]-screenWidth/2) * (zNear / abs(-p[2]));
    dst[1] = screenHeight - (screenHeight/2 + (p[1]-screenHeight/2) * (zNear / abs(-p[2])));
    dst[2] = p[2];
}

float Transformation::getGlobalScale_2() {
    Matrix &m = getMatrix();
    return m.mat[0]*m.mat[0] + m.mat[4]*m.mat[4] + m.mat[8]*m.mat[8] // scale_x²
        + m.mat[1]*m.mat[1] + m.mat[5]*m.mat[5] + m.mat[9]*m.mat[9] // scale_y²
        + m.mat[2]*m.mat[2] + m.mat[6]*m.mat[6] + m.mat[10]*m.mat[10]; // scale_z²
}
float Transformation::getMaxGlobalScale() {
Matrix &m = getMatrix();
    return  sqrt(max(
        m.mat[0]*m.mat[0] + m.mat[4]*m.mat[4] + m.mat[8]*m.mat[8], // scale_x²
        max(m.mat[1]*m.mat[1] + m.mat[5]*m.mat[5] + m.mat[9]*m.mat[9], // scale_y²
            m.mat[2]*m.mat[2] + m.mat[6]*m.mat[6] + m.mat[10]*m.mat[10] // scale_z²
    )));
}
