#ifndef MAIN_H

#define MAIN_H

#include <SDL2/SDL.h>
#include <cmath>
#include <map>
#include <vector>
#include "voxel.h"
#include "volume.h"

#include "vollyglm.h"

namespace volly {
    struct Texture_SDL;

    class Window_SDL {
    public:
        SDL_Window *window;
        SDL_Renderer *renderer;

        glm::ivec2 size;

        void create(glm::ivec2 size);
        void drawTexture(Texture_SDL * tex, glm::ivec2 _pos = glm::ivec2(0), glm::ivec2 _size = glm::ivec2(-1));
        void clear();
    };


    // wrapper around SDL_Texture
    struct Texture_SDL {
        SDL_Texture *_texture;
        glm::u8vec4 *data_stream; // This is write-only data that should only be written to between lock-unlock calls (mutexes?)
        int pitch;
        glm::ivec2 size;

        // Assigns data_stream and allows it to be written to
        void lock() {
            SDL_LockTexture(_texture, NULL, (void**) &data_stream, &pitch);
        }

        void unlock() {
            SDL_UnlockTexture(_texture);
            data_stream = nullptr;
        }
        
        Texture_SDL(glm::ivec2 size, Window_SDL* win) {
            this->size = size;

            _texture = SDL_CreateTexture(win->renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, size.x, size.y);

            data_stream = nullptr;
        }

    };

    
    struct KeyHandler {
        
        std::map<SDL_Scancode, bool> keysDown;
        
        bool getIfKeyDown(SDL_Scancode scan) {
            return keysDown[scan];
        }
        
        /** state is either down (true) or up (false) */
        void setKeyState(SDL_Scancode scan, bool state=true) {
            keysDown[scan] = state;
        }
        
        
    };

    struct Vol_LOD_Set {
        int sLowR, lowR, medR, highR;
        VolumeStore<Voxel> *sLow, *low, *med, *high;
        VolumeStore<Voxel> *arr[4]; // different way to reference the same thing

        Vol_LOD_Set(std::string filename, int coloringMode = 0, int sLowR=16, int lowR=32, int medR=128, int highR=768);

        ~Vol_LOD_Set() {
            delete sLow;
            delete low;
            delete med;
            delete high;
        }
    };

    struct State {
        Window_SDL  *window;
        Texture_SDL *screenTex;
        std::vector<Vol_LOD_Set*> volumes;
        int curVolume = 0, curLoD = 0;
        int frame = 0;

        glm::vec3 camPos = glm::vec3(0);
        glm::vec3 camDir = glm::vec3(0);
        glm::vec3 camUp = glm::vec3(0,0,-1);

        glm::mat4 view;

        KeyHandler keys;

        int mouseX, mouseY, mouseDX, mouseDY;
        

        int ms = 0;
        int ms_prev = 0;
        int ms_start = 0;

        int winSizeX = 1152, winSizeY = 864;
        int renderTargetSizeX = 400, renderTargetSizeY = 300;

        void raycastOntoScreen();
        void initAllVolumes();
        void resolveUserInput();
        void vollyMainLoop();
        VolumeStore<Voxel>* getCurVol() { return volumes[curVolume]->arr[curLoD]; }
        VolumeStore<Voxel>* getCurVol_LowLOD() { return volumes[curVolume]->arr[1]; }
        VolumeStore<Voxel>* getCurVol_SLowLOD() { return volumes[curVolume]->arr[0]; }
    };
}



#endif // MAIN_H