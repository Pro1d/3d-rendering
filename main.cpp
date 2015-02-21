#include <cstdlib>
#include <SDL.h>
#include <cmath>
#include <ctime>
#include <algorithm>
#include <string>

#include "Engine3D.h"
#include "Object3D.h"
#include "Physic3D.h"
#include "PostEffect.h"

#define TAILLE  (512)
#define WIDTH   (TAILLE+256)
#define HEIGHT  (TAILLE)


#define FPS_MAX 60

#define MODE_3D  3

using std::min;
using std::max;
using std::string;

void drawNumber(SDL_Surface* src, int color, int px, int py, int size, float number);

int main ( int argc, char** argv )
{
    /// Initialize SDL video
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
        // ouvre la console pour le mode debug
        freopen("con", "w", stdout);
    #endif

    SDL_Surface* bmp3d = SDL_CreateRGBSurface(SDL_HWSURFACE, WIDTH*(ANTI_ALIASING+1), HEIGHT*(ANTI_ALIASING+1), 24, 0,0,0,0);

    Engine3D engine3d(bmp3d);

    Object3DBuffer objBuffer;
    char sceneFileName[128] = "data/room.scene";
    if(argc > 1)
        strcpy(sceneFileName, argv[1]);
    Scene scene(sceneFileName, objBuffer);

    srand(42);
    SDL_WarpMouse(screen->w/2, screen->h/2);

    bool grabMouse = false;
    SDL_ShowCursor(!grabMouse);
    SDL_WM_GrabInput((SDL_GrabMode)grabMouse);
    int moveX = 0, moveY = 0, moveZ = 0, scale = 0;

    bool rayTracingRenderingDone = false;
    bool rayTracingRenderingRequested = false;

    /// Post effects
    DistanceFog fog(engine3d.getColorBuf(), engine3d.getDepthBuf(), engine3d.getColorBuf2(), engine3d.getWidth(), engine3d.getHeight(), rgb_f(1,1,1), 0.05f);
    fog.setEnabled(false);
    DepthOfField dof(engine3d.getColorBuf(), engine3d.getDepthBuf(), engine3d.getColorBuf2(), engine3d.getWidth(), engine3d.getHeight(), 0.5f, 2.0f);
    dof.setEnabled(false);

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
                if(buttonLeftDown) {
                    engine3d.setPixelFocus(event.motion.x, event.motion.y);
                    float depthFocus = engine3d.getDepthBuf()[event.motion.x + engine3d.getWidth() * event.motion.y];
                    if(SDL_GetModState()&KMOD_ALT)
                        dof.setDepthMax(depthFocus);
                    else
                        dof.setDepthFocus(depthFocus);
                }
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
                        if(rayTracingRenderingDone)
                            rayTracingRenderingDone = false;
                        else
                            rayTracingRenderingRequested = true;
                        break;
                    case SDLK_p: {
                        time_t rawtime = time(NULL);
                        struct tm *date = localtime(&rawtime);
                        char fileName[150], dateStr[150];
                        strftime(dateStr, 150, "%F %Hh%Mm%S", date);
                        sprintf(fileName, "capture - %s.bmp", dateStr);
                        SDL_SaveBMP(bmp3d, fileName);
                    }   break;
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
                        dof.toggleEnabled();
                        engine3d.toggleDepthOfField();
                        break;
                    case SDLK_g:
                        if((SDL_GetModState()&KMOD_SHIFT)) {
                            fog.setFogDensity(fog.getFogDensity()*1.1f);
                        }
                        else if((SDL_GetModState()&KMOD_CTRL)) {
                            fog.setFogDensity(fog.getFogDensity()/1.1f);
                        }
                        else {
                            fog.toggleEnabled();
                            engine3d.toggleFog();
                        }
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
        Uint32 t;
        if(!rayTracingRenderingDone) {

            t = SDL_GetTicks();

            /// Activation du ray tracing
            if(rayTracingRenderingRequested) {
                engine3d.toggleRayTracing();
                rayTracingRenderingRequested = false;
                rayTracingRenderingDone = true;
            }

            /// Moteur graphique
            engine3d.drawScene(scene);
            t = SDL_GetTicks()-t;
        }
        else {
            if(fog.isEnabled()) {
                fog.apply();
            }
            if(dof.isEnabled()) {
                if(fog.isEnabled()) {
                    dof.setColorSrc(engine3d.getColorBuf2());
                    dof.setColorDst(engine3d.getColorBuf3());
                }
                else {
                    dof.setColorSrc(engine3d.getColorBuf());
                    dof.setColorDst(engine3d.getColorBuf3());
                }
                dof.apply();
            }
            if(fog.isEnabled() || dof.isEnabled())
                Engine3D::toBitmap(bmp3d, dof.isEnabled() ? engine3d.getColorBuf3() : engine3d.getColorBuf2(), engine3d.getWidth(), engine3d.getHeight());

            else {
                Engine3D::toBitmap(bmp3d, engine3d.getColorBuf(), engine3d.getWidth(), engine3d.getHeight());
            }
        }
        /// Affichage du temps de rendu
        drawNumber(bmp3d, SDL_MapRGB(bmp3d->format, 0,255,255), 10,10, 12, t);

        /// Mise à jour de la fenêtre
        SDL_BlitSurface(bmp3d, NULL, screen, NULL);
        SDL_Flip(screen);

        Uint32 time = SDL_GetTicks() - frameStartTime;
        if(time < 1000/FPS_MAX)
            SDL_Delay(1000/FPS_MAX - time);
    }

    SDL_FreeSurface(bmp3d);

    return EXIT_SUCCESS;
}


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
void drawNumber(SDL_Surface* src, int color, int px, int py, int size, float number) {
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


