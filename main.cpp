#include <cstdlib>
#include <SDL.h>
#include <cmath>
#include <algorithm>
#include <string>

#include "Engine3D.h"
#include "Object3D.h"
#include "Physic3D.h"

#define TAILLE  (512)
#define WIDTH   (TAILLE+256)
#define HEIGHT  (TAILLE)


#define FPS_MAX 60

#define MODE_3D  3

using std::min;
using std::max;
using std::string;

void drawNumber(SDL_Surface* src, int color, int px, int py, int size, float number);

void putPixel(int x, int y, Uint32 pixel, SDL_Surface *src)
{
    Uint8 *p = (Uint8 *)src->pixels + y * src->pitch + x * src->format->BytesPerPixel;

    if(SDL_BYTEORDER == SDL_BIG_ENDIAN) {
        p[0] = (pixel >> 16) & 0xff;
        p[1] = (pixel >> 8) & 0xff;
        p[2] = pixel & 0xff;
    } else {
        p[0] = pixel & 0xff;
        p[1] = (pixel >> 8) & 0xff;
        p[2] = (pixel >> 16) & 0xff;
    }
}
Uint32 getPixel(int x, int y, SDL_Surface *src)
{
    Uint8 *p = (Uint8 *)src->pixels + y * src->pitch + x * src->format->BytesPerPixel;

    if(SDL_BYTEORDER == SDL_BIG_ENDIAN)
        return p[0] << 16 | p[1] << 8 | p[2];
    else
        return p[2] << 16 | p[1] << 8 | p[0];
}
void draw(SDL_Surface *src, SDL_Surface* dst) {
    SDL_LockSurface(dst);
    for(int x = 0; x < dst->w; ++x)
    for(int y = 0; y < dst->h; ++y)
    {
        putPixel(x,y, getPixel(x/2, y/2, src), dst);
    }
    SDL_UnlockSurface(dst);
}

int main ( int argc, char** argv )
{
    // initialize SDL video
    if(SDL_Init( SDL_INIT_VIDEO ) < 0) {
        printf( "Unable to init SDL: %s\n", SDL_GetError() );
        return 1;
    }
    atexit(SDL_Quit);

    SDL_Surface* screen = SDL_SetVideoMode(WIDTH, HEIGHT, 24, SDL_HWSURFACE|SDL_DOUBLEBUF);
    if ( !screen ) {
        printf("Unable to set 640x480 video: %s\n", SDL_GetError());
        return 1;
    }
    #ifdef DEBUG
        freopen("con", "w", stdout);
    #endif

    SDL_Surface* bmp3d = SDL_CreateRGBSurface(SDL_HWSURFACE, WIDTH*(ANTI_ALIASING+1), HEIGHT*(ANTI_ALIASING+1), 24, 0,0,0,0);

    Engine3D engine3d(bmp3d);

    Object3DBuffer objBuffer;
    Scene scene("data/parc.scene", objBuffer);

    /*Model3D &model_balle_cage(*scene.getModel("balle_cage"));
    Object3D & cage(objBuffer.get("etrange_box"));*/


    Uint32 timeFramePhysic = SDL_GetTicks();
    PointPhysic pointPhysic(1.f);
    pointPhysic.p[0] = pointPhysic.p[1] = 2; pointPhysic.p[2] = 3;
    pointPhysic.v[0] = pointPhysic.v[1] = 0; pointPhysic.v[2] = 0;
    /*PointPhysic pointPhysicCage(1.0f);
    pointPhysicCage.p[0] = pointPhysicCage.p[1] = pointPhysicCage.p[2] = 0;
    pointPhysicCage.v[0] = pointPhysicCage.v[1] = 0; pointPhysicCage.v[2] = 0.05f;*/
    srand(42);
    SDL_WarpMouse(screen->w/2, screen->h/2);

    bool grabMouse = false;
    SDL_ShowCursor(!grabMouse);
    SDL_WM_GrabInput((SDL_GrabMode)grabMouse);
    int moveX = 0, moveY = 0, moveZ = 0, scale = 0;

    float timeAverage = 0;

    bool plop = false;

    bool done = false;
    bool buttonLeftDown = false;
    while (!done)
    {
        SDL_Event event;
        Uint32 frameStartTime = SDL_GetTicks();
        while (SDL_PollEvent(&event))
        {
            switch (event.type)
            {
            case SDL_QUIT:
                done = true;
                break;
            case SDL_MOUSEBUTTONDOWN:
                if(event.button.button == SDL_BUTTON_LEFT)
                    buttonLeftDown = true;
                else if(event.button.button == SDL_BUTTON_RIGHT){
                    grabMouse = true;
                    SDL_ShowCursor(!grabMouse);
                    SDL_WM_GrabInput((SDL_GrabMode)grabMouse);
                }
                break;
            case SDL_MOUSEBUTTONUP:
                if(event.button.button == SDL_BUTTON_LEFT)
                    buttonLeftDown = false;
                else if(event.button.button == SDL_BUTTON_RIGHT){
                    grabMouse = false;
                    SDL_ShowCursor(!grabMouse);
                    SDL_WM_GrabInput((SDL_GrabMode)grabMouse);
                }
                break;
            case SDL_MOUSEMOTION:
                if(grabMouse) {
                    engine3d.rotateCamY((float) event.motion.xrel * M_PI/ 512.0f);
                    engine3d.rotateCamX((float) event.motion.yrel * M_PI/ 512.0f);
                }
                if(buttonLeftDown)
                    engine3d.setPixelFocus(event.motion.x, event.motion.y);
                break;
            case SDL_KEYUP:
                switch(event.key.keysym.sym) {
                    case SDLK_UP:
                    case SDLK_w:
                        moveZ--;
                        break;
                    case SDLK_DOWN:
                    case SDLK_s:
                        moveZ++;
                        break;
                    case SDLK_LEFT:
                    case SDLK_a:
                        moveX--;
                        break;
                    case SDLK_RIGHT:
                    case SDLK_d:
                        moveX++;
                        break;
                    case SDLK_SPACE:
                        moveY++;
                        break;
                    case SDLK_LSHIFT:
                        moveY--;
                        break;
                    case SDLK_q:
                        scale++;
                        break;
                    case SDLK_e:
                        scale--;
                        break;
                    default: break;
                }
                break;
            case SDL_KEYDOWN:
                switch(event.key.keysym.sym) {
                    case SDLK_F10:
                        engine3d.toggleRayTracing();
                        plop = true;
                        break;
                    case SDLK_ESCAPE:
                        done = true;
                        break;
                    case SDLK_RETURN:
                        break;
                    case SDLK_UP:
                    case SDLK_w:
                        moveZ++;
                        break;
                    case SDLK_DOWN:
                    case SDLK_s:
                        moveZ--;
                        break;
                    case SDLK_LEFT:
                    case SDLK_a:
                        moveX++;
                        break;
                    case SDLK_RIGHT:
                    case SDLK_d:
                        moveX--;
                        break;
                    case SDLK_SPACE:
                        moveY--;
                        break;
                    case SDLK_LSHIFT:
                        moveY++;
                        break;
                    case SDLK_q:
                        scale--;
                        break;
                    case SDLK_e:
                        scale++;
                        break;
                    case SDLK_f:
                        engine3d.toggleDepthOfField();
                        break;
                    case SDLK_g:
                        engine3d.toggleFog();
                        break;
                    case SDLK_b:
                        engine3d.toggleGlow();
                        break;
                    case SDLK_m:
                        engine3d.toggleBlurMotion();
                        break;
                    case SDLK_y:
                        engine3d.toggleDrawingFilter();
                        break;
                    case SDLK_z:
                        engine3d.increaseFoV();
                        break;
                    case SDLK_x:
                        engine3d.decreaseFoV();
                        break;
                   /* case SDLK_v:
                        engine3d.toggleVolumeRendering();
                        break;*/
                    default:
                        break;
                }
                break;
            } // end switch
        } // end of message processing
        float speed = ((SDL_GetModState()&KMOD_ALT) ? 0.16f : 0.08f) * magic_sqrt_inv(abs(moveX)+abs(moveY)+abs(moveZ));
        engine3d.translateCamForward(speed * moveZ);
        engine3d.translateCamUp(speed * moveY);
        engine3d.translateCamLeft(speed * moveX);
        if(scale != 0)  engine3d.scaleCam(scale == 1 ? 1.1f : 1.0f/1.1f);

        SDL_FillRect(screen, NULL, SDL_MapRGB(screen->format, 0, 0, 0));
        SDL_FillRect(bmp3d, NULL, SDL_MapRGB(bmp3d->format, 0, 0, 0));

        /*// Moteur physique !!!
        timeFramePhysic = SDL_GetTicks() - timeFramePhysic;
        pointPhysic.v[2] -= 0.0005f * timeFramePhysic;
        timeFramePhysic = SDL_GetTicks();
        pointPhysic.moveInside_PreAlpha(box);
        if(pointPhysic.p[2] < -10) {
            pointPhysic.p[0] = pointPhysic.p[1] = pointPhysic.p[2] = 3;
            pointPhysic.v[0] = (float)rand()/RAND_MAX*0.06f - 0.03f;
            pointPhysic.v[1] = (float)rand()/RAND_MAX*0.06f - 0.03f;
            pointPhysic.v[2] = 0;//(float)rand()/RAND_MAX*0.1f - 0.05f;
        }
        model_balle.getTransformation().getMatrix().mat[3] = pointPhysic.p[0];
        model_balle.getTransformation().getMatrix().mat[7] = pointPhysic.p[1];
        model_balle.getTransformation().getMatrix().mat[11] = pointPhysic.p[2];

        pointPhysicCage.moveInside_PreAlpha(cage);
        model_balle_cage.getTransformation().getMatrix().mat[3] = pointPhysicCage.p[0];
        model_balle_cage.getTransformation().getMatrix().mat[7] = pointPhysicCage.p[1];
        model_balle_cage.getTransformation().getMatrix().mat[11] = pointPhysicCage.p[2];

        /// BALANCOIRE
        Model3D &balancoire = *scene.getModel("balancoire");
        Transformation &tb = balancoire.getTransformation();
        tb.push();
        tb.rotateX(0.05f*sin((float)SDL_GetTicks()/1000), PRE);
        tb.rotateZ(0.03f*sin((float)SDL_GetTicks()/600), PRE);

        /// CORDES
        Model3D &corde1 = *scene.getModel("corde1");
        Transformation &tc1 = corde1.getTransformation();
        Model3D &corde2 = *scene.getModel("corde2");
        Transformation &tc2 = corde2.getTransformation();
        tc1.push();
        tc2.push();
        tc1.rotateX(0.035f*sin((float)SDL_GetTicks()/800 + 350), PRE);
        tc1.rotateY(0.01f*cos((float)SDL_GetTicks()/800 + 350), PRE);
        tc2.rotateX(-0.03f*sin((float)SDL_GetTicks()/800), PRE);
        tc2.rotateY(0.015f*cos((float)SDL_GetTicks()/800), PRE);*/

        /*// ANNEAUX
        Model3D &anneau1 = *scene.getModel("anneau1");
        Transformation &ta1 = anneau1.getTransformation();
        Model3D &anneau2 = *scene.getModel("anneau2");
        Transformation &ta2 = anneau2.getTransformation();
        ta1.push();
        ta2.push();
        ta1.rotateZ(0.2f*sin((float)SDL_GetTicks()/2000 + 250), PRE);
        ta2.rotateZ(0.2f*sin((float)SDL_GetTicks()/2000), PRE);
*/

        Uint32 t = SDL_GetTicks();

        engine3d.drawScene(scene);
        /*tb.pop();
        tc1.pop();
        tc2.pop();
        ta1.pop();
        ta2.pop();*/

        Uint32 tt = SDL_GetTicks()-t;
        printf("%d\n", tt);

        if(timeAverage == 0 || abs(timeAverage - tt) / timeAverage > 0.2f) timeAverage = tt;
        else timeAverage = timeAverage*0.9 + (float)tt*0.1;

        drawNumber(bmp3d, SDL_MapRGB(bmp3d->format, 0,255,255), 10,10, 18, timeAverage);
        drawNumber(bmp3d, SDL_MapRGB(bmp3d->format, 0,255,255), 10,10+40, 18, 1000.0f/timeAverage);

        SDL_BlitSurface(bmp3d, NULL, screen, NULL);
        SDL_Flip(screen);
        if(plop)
        {
            plop = false;
            do {
                SDL_WaitEvent(&event);
                if(event.type == SDL_QUIT)
                    exit(0);
            } while(event.type == SDL_KEYDOWN ? event.key.keysym.sym != SDLK_F10 : true);
        }

        Uint32 time = SDL_GetTicks() - frameStartTime;
        if(time < 1000/FPS_MAX)
            SDL_Delay(1000/FPS_MAX - time);
    }

    SDL_FreeSurface(bmp3d);

    return EXIT_SUCCESS;
}
bool numberMat[10*7] = {
    1,1,1,0,1,1,1,
    0,0,1,0,0,1,0,
    1,0,1,1,1,0,1,
    1,0,1,1,0,1,1,
    0,1,1,1,0,1,0,
    1,1,0,1,0,1,1,
    1,1,0,1,1,1,1,
    1,0,1,0,0,1,0,
    1,1,1,1,1,1,1,
    1,1,1,1,0,1,1,
};
int posMat[7*2*2*2] = {
    0,1, 0,0,  1,1, 0,1,
    0,0, 0,1,  0,1, 1,1,
    1,1, 0,1,  1,2, 1,1,
    0,1, 1,1,  1,1, 1,2,
    0,0, 1,2,  0,1, 2,2,
    1,1, 1,2,  1,2, 2,2,
    0,1, 2,2,  1,1, 2,3,
};
void drawNumber(SDL_Surface* src, int color, int px, int py, int size, float number) {
    int e = size / 3, f = size / (3*3);
    char s[16]; sprintf(s, "%.1f", number);
    for(int i = 0; i < (int)strlen(s); i++)
    {
        if(s[i] == '.')  {
            for(int x = 0; x < f; x++)
            for(int y = 0; y < f; y++)
                putPixel(px+(e+3*f)*i+(e+2*f)/2+x, py+y+2*e+2*f, color, src);
        }
        else
            for(int j = 0; j < 7; j++)
            if(numberMat[j+7*(s[i]-'0')])
            {
                for(int x = e*posMat[j*8+0] + f*posMat[j*8+1]; x < e*posMat[j*8+4] + f*posMat[j*8+5]; x++)
                for(int y = e*posMat[j*8+2] + f*posMat[j*8+3]; y < e*posMat[j*8+6] + f*posMat[j*8+7]; y++)
                    putPixel(px+(e+3*f)*i+x, py+y, color, src);
            }
    }
}


