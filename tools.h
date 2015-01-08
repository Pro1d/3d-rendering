#ifndef TOOLS_H
#define TOOLS_H


float magic_sqrt_inv(float number);
float clamp(float min, float max, float v);
float clampIn01(float v);
void normalizeVector(float *v);
float triangleArea(float *a, float *b, float *c);
float smoothStep(float min, float max, float x);

void getFaceNormal(float* A, float* C, float* B, float *n);
/*// donne une approximation de exp(-x)
float magic_exp_inv(float x);*/



#endif // TOOLS_H
