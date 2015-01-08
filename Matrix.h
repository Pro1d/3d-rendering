#ifndef MATRIX_H
#define MATRIX_H

//enum {X, Y, Z, W};
enum {
    ROTATE, ROTATE_X, ROTATE_Y, ROTATE_Z,
    TRANLATE, TRANLATE_X, TRANLATE_Y, TRANLATE_Z,
    SCALE, SCALE_X, SCALE_Y, SCALE_Z,
    PERSPECTIVE,
};

class Matrix
{
    public:
        Matrix(); /// Identité
        Matrix(float a, float b, float c, float d,
               float e, float f, float g, float h,
               float i, float j, float k, float l,
               float m, float n, float o, float p);
        Matrix(float x, float y, float z);
        void set(float a, float b, float c, float d,
               float e, float f, float g, float h,
               float i, float j, float k, float l,
               float m, float n, float o, float p);
        //Matrix(int type, float tx, float ty, float tz); /// translation / échelle / rotation
        //Matrix(int type, float sx, float sy, float sz, float cx, float cy, float cz); /// échelle / rotation
        //Matrix(int type, float sx, float sy, float sz, float cx, float cy, float cz); /// rotation
        //virtual ~Matrix();
        /// res = m * this
        void multiplyBy(Matrix const& m, Matrix &res) const;
        //void multiplyBy(Matrix const& m);
        void applyTo(const float *p, float *r) const;
        void setIdentity();
        float mat[16];
    protected:
    private:
};

#endif // MATRIX_H
