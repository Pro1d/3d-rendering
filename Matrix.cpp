#include "Matrix.h"

Matrix::Matrix()
{
    setIdentity();
}
Matrix::Matrix(float a, float b, float c, float d, float e, float f, float g, float h, float i, float j, float k, float l, float m, float n, float o, float p) {
    mat[0] = a; mat[1] = b; mat[2] = c; mat[3] = d;
    mat[4] = e; mat[5] = f; mat[6] = g; mat[7] = h;
    mat[8] = i; mat[9] = j; mat[10] = k; mat[11] = l;
    mat[12] = m; mat[13] = n; mat[14] = o; mat[15] = p;
}

Matrix::Matrix(float x, float y, float z) {
    mat[0] =1; mat[1] =0; mat[2] =0; mat[3] = x;
    mat[4] =0; mat[5] =1; mat[6] =0; mat[7] = y;
    mat[8] =0; mat[9] =0; mat[10]=1; mat[11]= z;
    mat[12]=0; mat[13]=0; mat[14]=0; mat[15]= 1;
}

void Matrix::set(float a, float b, float c, float d,
       float e, float f, float g, float h,
       float i, float j, float k, float l,
       float m, float n, float o, float p) {
    mat[0] = a; mat[1] = b; mat[2] = c; mat[3] = d;
    mat[4] = e; mat[5] = f; mat[6] = g; mat[7] = h;
    mat[8] = i; mat[9] = j; mat[10] = k; mat[11] = l;
    mat[12] = m; mat[13] = n; mat[14] = o; mat[15] = p;
}
void Matrix::setIdentity() {
    for(int i = 0; i < 16; i++)
        mat[i] = (i%5 == 0);
}

void Matrix::multiplyBy(Matrix const& m, Matrix &res) const {
    float *r = res.mat;
    const float *a = m.mat;
    const float *b = mat;
    r[0] = a[0]*b[0] + a[1]*b[4] + a[2]*b[8] + a[3]*b[12];
    r[1] = a[0]*b[1] + a[1]*b[5] + a[2]*b[9] + a[3]*b[13];
    r[2] = a[0]*b[2] + a[1]*b[6] + a[2]*b[10] + a[3]*b[14];
    r[3] = a[0]*b[3] + a[1]*b[7] + a[2]*b[11] + a[3]*b[15];

    r[4] = a[4]*b[0] + a[5]*b[4] + a[6]*b[8] + a[7]*b[12];
    r[5] = a[4]*b[1] + a[5]*b[5] + a[6]*b[9] + a[7]*b[13];
    r[6] = a[4]*b[2] + a[5]*b[6] + a[6]*b[10] + a[7]*b[14];
    r[7] = a[4]*b[3] + a[5]*b[7] + a[6]*b[11] + a[7]*b[15];

    r[8] = a[8]*b[0] + a[9]*b[4] + a[10]*b[8] + a[11]*b[12];
    r[9] = a[8]*b[1] + a[9]*b[5] + a[10]*b[9] + a[11]*b[13];
    r[10] = a[8]*b[2] + a[9]*b[6] + a[10]*b[10] + a[11]*b[14];
    r[11] = a[8]*b[3] + a[9]*b[7] + a[10]*b[11] + a[11]*b[15];

    r[12] = a[12]*b[0] + a[13]*b[4] + a[14]*b[8] + a[15]*b[12];
    r[13] = a[12]*b[1] + a[13]*b[5] + a[14]*b[9] + a[15]*b[13];
    r[14] = a[12]*b[2] + a[13]*b[6] + a[14]*b[10] + a[15]*b[14];
    r[15] = a[12]*b[3] + a[13]*b[7] + a[14]*b[11] + a[15]*b[15];
}
void Matrix::applyTo(const float *p, float *r) const {
    const float *m = mat;
    r[0] = p[0]*m[0] + p[1]*m[1] + p[2]*m[2] + m[3];
    r[1] = p[0]*m[4] + p[1]*m[5] + p[2]*m[6] + m[7];
    r[2] = p[0]*m[8] + p[1]*m[9] + p[2]*m[10] + m[11];
    r[3] = p[0]*m[12] + p[1]*m[13] + p[2]*m[14] + m[15];
    r[0] /= r[3];
    r[1] /= r[3];
    r[2] /= r[3];
    r[3] = 1;
}
