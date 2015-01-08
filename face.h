
#ifndef FACE_H
#define FACE_H

#include "vertex.h"

class Face
{
    public:
        Face() {}
        Face(int v1, int v2, int v3) {
            vertex_indices[0] = v1;
            vertex_indices[1] = v2;
            vertex_indices[2] = v3;
        }
        //void setNormal(float *_n) { memcpy(n, _n, sizeof(float)*3); }
        void setVertexIndices(int *_i) { memcpy(vertex_indices, _i, sizeof(int)*3); }
        int vertex_indices[3];

        void setTBNMatrix(Transf_Vertex const& v1, Transf_Vertex const& v2, Transf_Vertex const& v3) {
            //matrixTBN[];
        }
        //float n[3]; /// normal
        float matrixTBN[3*3];
};

class Drawable_Face : public Face
{
    public:
        bool isTranslucent;
        bool isOrientZ;
        float n[3];/// normal
};

#endif

/**
Tangent and Binormal
The relevant input data to your problem are the texture coordinates.
Tangent and Binormal are vectors locally parallel to the object's
surface. And in the case of normal mapping they're describing the
local orientation of the normal texture.

So what you have to do is calculate the direction (in the model's
space) in which the texturing vectors point. Say you have a triangle
ABC, with texture coordinates HKL. This gives us vectors:

D = B-A
E = C-A

F = K-H
G = L-H
Now we want to express D and E in terms of tangent space T, U, i.e.

D = F.s * T + F.t * U
E = G.s * T + G.t * U
This is a system of linear equations with 6 unknowns and 6 equations,
it can be written as

| D.x D.y D.z |   | F.s F.t | | T.x T.y T.z |
|             | = |         | |             |
| E.x E.y E.z |   | G.s G.t | | U.x U.y U.z |
Inverting the FG matrix yields

| T.x T.y T.z |           1         |  G.t  -F.t | | D.x D.y D.z |
|             | = ----------------- |            | |             |
| U.x U.y U.z |   F.s G.t - F.t G.s | -G.s   F.s | | E.x E.y E.z |
Together with the vertex normal T and U form a local space basis,
called the tangent space, described by the matrix

| T.x U.x N.x |
| T.y U.y N.y |
| T.z U.z N.z |
transforming from tangent space into object space. To do lighting
calculations one needs the inverse of this. With a little bit of
exercise one finds:

T' = T - (N·T) N
U' = U - (N·U) N - (T'·U) T'
normalizing the vectors T' and U', calling them tangent and binormal
we obtain the matrix transforming from object into tangent space,
where we do the lighting:

| T'.x T'.y T'.z |
| U'.x U'.y U'.z |
| N.x  N.y  N.z  |
we store T' and U' them together with the vertex normal as a part
of the model's geometry (as vertex attributes), so that we can use
them in the shader for lighting calculations. I repeat: You don't
determine tangent and binormal in the shader, you precompute them
and store them as part of the model's geometry (just like normals).

*/
