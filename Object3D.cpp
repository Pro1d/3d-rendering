#include "Object3D.h"
#include "Engine3D.h"
#include "color.h"
#include "vertex.h"
#include "face.h"

#include <cstring>
#include <cstdio>
#include <string>
#include <map>
#include <cmath>

using namespace std;

Object3D::Object3D(const char* fileName, bool translucent) : vertexCount(0), facesCount(0), indice(1), reflect(0.3f), isVolumeTranslucent(translucent), hasColor(false), hasTexture(false), texture(NULL)
{
    bool loadFailed = !loadFromFile(fileName);

    const char *s = max((const char*)strrchr(fileName, '/')+1, fileName);
    strcpy(name, s);
    char *e = strrchr(name, '.');
    if(e != NULL) *e = '\0';
    if(loadFailed)
        strcat(name, "#ERROR");
    else if(hasTexture) {
        char textName[512]; strcpy(textName, fileName);

        char *ext = strrchr(textName, '.');
        if(ext != NULL) *ext = '\0';
        strcat(textName, ".bmp");

        loadTexture(textName);
    }

    #ifdef DEBUG
    if(!hasTexture)
        printf("Pas de texture pour \"%s\"\n", name);
    #endif
    //updateFaceNormal();
    initColor();
/*
    printf("%d %d\n", vertexCount, facesCount);
    for(int i = 0; i < vertexCount; i++)
        printf("%f %f %f\t%f %f %f\t%f %f %f\n", vertex[i].p[0], vertex[i].p[1], vertex[i].p[2]
               , vertex[i].n[0], vertex[i].n[1], vertex[i].n[2]
               , vertex[i].c.r, vertex[i].c.g, vertex[i].c.b);
    for(int i = 0; i < facesCount; i++)
        printf("%d %d %d\t%f %f %f\n", faces[i].vertex_indices[0], faces[i].vertex_indices[1], faces[i].vertex_indices[2]
               , faces[i].n[0], faces[i].n[1], faces[i].n[2]);
*/
}
int numObj = 0;
Object3D::Object3D(bool translucent) : vertexCount(0), facesCount(0), indice(1), reflect(0.3f), isVolumeTranslucent(translucent), hasColor(false), hasTexture(false), texture(NULL)
{
    sprintf(name, "newObject#%d", ++numObj);
    initColor();
}

bool Object3D::loadFromFile(const char* fileName)
{
    FILE* f = fopen(fileName, "r");

    if(f == NULL) {
        printf("Impossible d'ouvrir \"%s\"\n", fileName);
        return false;
    }
    else
        printf("Chargement de \"%s\"\n", fileName);

    /// Header
    char word[64] = "";
    while(strcmp(word, "end_header") != 0) {
        fscanf(f, "%s", word);
        //printf("%s", word);
        if(strcmp(word, "element") == 0) {
            fscanf(f, "%s", word);
            if(strcmp(word, "vertex") == 0) {
                fscanf(f, "%s", word);
                vertex.clear(); vertex.resize(vertexCount = atoi(word));
            }
            else if(strcmp(word, "face") == 0) {
                fscanf(f, "%s", word);
                faces.clear(); faces.resize(facesCount = atoi(word));
            }
        }
        else if(strcmp(word, "property") == 0) {
            fscanf(f, "%s", word);
            if(strcmp(word, "uchar") == 0) {
                fscanf(f, "%s", word);
                if(strcmp(word, "red") == 0 || strcmp(word, "green") == 0|| strcmp(word, "blue") == 0)
                    hasColor = true;
            }
            else if(strcmp(word, "float") == 0) {
                fscanf(f, "%s", word);
                // x y z nx ny nz s t
                if(strcmp(word, "s") == 0 || strcmp(word, "t") == 0)
                    hasTexture = true;
            }
            else if(strcmp(word, "list") == 0) {
                fscanf(f, "%s", word); // uchar
                fscanf(f, "%s", word); // uint
                fscanf(f, "%s", word); // vertex_indeices
            }
        }
    }
    /// Vertex
    for(int i = 0; i < vertexCount; i++) {
        /// Coord
        float p[3];
        fscanf(f, "%f%f%f", &p[0], &p[1], &p[2]);
        vertex[i].setPos(p);
        /// Mise à jour de la AABB
        if(i == 0) {
            AABB[0] = AABB[1] = p[0];
            AABB[2] = AABB[3] = p[1];
            AABB[4] = AABB[5] = p[2];
        } else {
            if(p[0] < AABB[0])
                AABB[0] = p[0];
            else if(p[0] > AABB[1])
                AABB[1] = p[0];
            if(p[1] < AABB[2])
                AABB[2] = p[1];
            else if(p[1] > AABB[3])
                AABB[3] = p[1];
            if(p[2] < AABB[4])
                AABB[4] = p[2];
            else if(p[2] > AABB[5])
                AABB[5] = p[2];
        }

        /// Normal
        float n[3];
        fscanf(f, "%f%f%f", &n[0], &n[1], &n[2]);
        vertex[i].setNormal(n);

        /// texture if existing
        if(hasTexture) {
            fscanf(f, "%f%f", &vertex[i].s, &vertex[i].t);
        }

        int c[3] = {1,1,1};
        /// Color if existing
        if(hasColor) {
            fscanf(f, "%d%d%d", &c[0], &c[1], &c[2]);
        }
        vertex[i].setColor(c);
    }
    centerBS[0] = (AABB[0] + AABB[1]) / 2;
    centerBS[1] = (AABB[2] + AABB[3]) / 2;
    centerBS[2] = (AABB[4] + AABB[5]) / 2;
    centerBS[3] = 1;
    radiusBS = sqrt((AABB[0] - AABB[1])*(AABB[0] - AABB[1])+(AABB[2] - AABB[3])*(AABB[2] - AABB[3])+(AABB[4] - AABB[5])*(AABB[4] - AABB[5]));

    /// Faces
    for(int i = 0; i < facesCount;) {
        int count; /// nb de points par face
        fscanf(f, "%d", &count);
        /// découpage de la face en plusieurs triangles
        count -= 2; /// nombre de triangles résultants
        facesCount += count-1;

        int first;
        fscanf(f, "%d", &first);
        int c[3];

        /// si plus de 1 triangle créé -> augmenter taille du tableau
        fscanf(f, "%d", &c[2]);
        for(int j = 0; j < count; j++) {
            fscanf(f, "%d", &c[j%3]);
            c[(j+1)%3] = first;

            if(j != 0) faces.push_back(Face());
            faces[i].setVertexIndices(c);
            i++;
        }
    }

    fclose(f);

    return true;
}
/*void Object3D::updateFaceNormal() {
    for(int i = faces.size(); --i >= 0;) {
        //    printf("%d / %d\n", faces[i].vertex_indices[0], vertexCount);
        faces[i].setNormal(vertex[faces[i].vertex_indices[0]].n);
    }
}*/
void Object3D::loadTexture(const char *fileName) {
    #ifdef DEBUG
        printf("Chargement de la texture \"%s\"\n", fileName);
    #endif
    SDL_Surface *bmp = SDL_LoadBMP(fileName);
    if(bmp == NULL) {
        hasTexture = false;
        return;
    }
    textWidth = bmp->w;
    textHeight = bmp->h;
    texture = new rgb_f[bmp->w*bmp->h];
    int bpp = bmp->format->BytesPerPixel;
    for(int y = 0; y < bmp->h; y++)
    for(int x = 0; x < bmp->w; x++)
    {
        Uint8 r,g,b;
        Uint8 *p = (Uint8 *)bmp->pixels + y * bmp->pitch + x * bpp;
        switch(bpp) {
        case 1:
            SDL_GetRGB(*p, bmp->format, &r, &g, &b);
            break;
        case 2:
            SDL_GetRGB(*(Uint16 *)p, bmp->format, &r, &g, &b);
            break;
        case 3:
            if(SDL_BYTEORDER == SDL_BIG_ENDIAN)
                SDL_GetRGB(p[0] << 16 | p[1] << 8 | p[2], bmp->format, &r, &g, &b);
            else
                SDL_GetRGB(p[0] | p[1] << 8 | p[2] << 16, bmp->format, &r, &g, &b);
            break;
        case 4:
            SDL_GetRGB(*(Uint32 *)p, bmp->format, &r, &g, &b);
            break;
        }
        texture[x+(textHeight-1-y)*textWidth].r = (float) r / 255;
        texture[x+(textHeight-1-y)*textWidth].g = (float) g / 255;
        texture[x+(textHeight-1-y)*textWidth].b = (float) b / 255;
    }
}
rgb_f const& Object3D::getTexturePoint(float s, float t) const {
    return texture[(int)(s*textWidth)+(int)(t*textHeight)*textWidth]; /// Warning /!\ : should be x=s*(textWidth+1), y=t*(textHeight+1), x==w?x--, y==h?y--
}
void Object3D::initColor()
{
    ambient.r  = ambient.g  = ambient.b  = 1;
    diffuse.r  = diffuse.g  = diffuse.b  = 1;
    specular.r = specular.g = specular.b = 1;
    emissive.r = emissive.g = emissive.b = 0;
    shininess = 128;
}


Object3DBuffer::Object3DBuffer()
{
}
void Object3DBuffer::loadFromFile(const char* fileName)
{
    #if defined(DEBUG) | defined(_DEBUG)
        printf("Chargement du buffer \"%s\"\n", fileName);;
    #endif
    FILE* f = fopen(fileName, "r");

    char key[512], file[FILENAME_MAX];
    while(fscanf(f, "%s %[^\n]\n", key, file) != EOF)
        loadObject(key, file);

    fclose(f);
}
Object3DBuffer::~Object3DBuffer()
{
    for(map<string, Object3D*>::iterator it = hashmap.begin(); it != hashmap.end(); ++it)
    {
        delete it->second;
    }
}
Object3D& Object3DBuffer::get(std::string name)
{
    return *hashmap[name];
}
void Object3DBuffer::loadObject(std::string name, const char *fileName)
{
    if(hashmap.count(name) == 0)
        hashmap[name] = new Object3D(fileName);
}
