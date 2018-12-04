#include <iostream>
#include <fstream>
#include "SDL2/SDL.h"

#define Handle_Error_SDL(x) if(x) std::cout << "SDL Error: " << SDL_GetError() << " (Error code" << x << ")" << std::endl;


struct ivec2 {
    int x, y;
};


// Base class which -- you guessed it -- stores volume data
class VolumeStore {

};



class Window_SDL {
    SDL_Window *window;
    SDL_Renderer *renderer;

    ivec2 size;

public:

    void create(ivec2 size);
    void drawTexture();
};

// todo: this
struct Pixel_8888 {

};

// wrapper around SDL_Texture
struct Texture_SDL {
    SDL_Texture *_texture;
    Pixel_8888 *data_stream; // This is write-only data that should only be written to between lock-unlock calls (mutexes?)
    int pitch;
    ivec2 size;

    // Assigns data_stream and allows it to be written to
    void lock() {
        SDL_LockTexture(_texture, NULL, (void**) &data_stream, &pitch);
    }

    void streamData() {
        SDL_UnlockTexture(_texture);
    }
    
    Texture_SDL(ivec2 size) {

    }

};

void Window_SDL::create(ivec2 size) {
    this->size = size;
    Handle_Error_SDL(SDL_Init(SDL_INIT_VIDEO));

    Handle_Error_SDL(SDL_CreateWindowAndRenderer(size.x, size.y, SDL_WINDOW_RESIZABLE, &window, &renderer));
}

// naiive raycast function which makes no use of optimizations such as precomputing a line (though this is a good place to start!). Start and direction should be coordinates within the volume's coordinate space, obviously.
// Any type of volume can be used here, but it would be really dumb to use this with an octree (unless you're using a really smart octree, which we're not)
template<typename vec3_t>
void raycast_naiive(vec3_t start, vec3_t direction, float maxLen, VolumeStore* volume) {
    vec3_t lineFitLength;
    vec3_t cur = start;

    int numIters = maxLen; // todo: compute this

    // abritrarily small constant, don't read into it
    vec3_t fiveepsilon = vec3_t(5)*vec3_t::epsilon();

    for(int i = 0; i < numIters; i++) {
        vec3_t distToNextGrid = vec3_t::ceil(cur+fiveepsilon)-cur;
        vec3_t normalizedDist = distToNextGrid/direction;
        // Find horizontal minimum... This will kill our performance on non-batched SIMD operations... 
        vec3_t minDistN = vec3_t::horizontalmin3(normalizedDist);
        cur += (minDistN * direction);
    }
}


int main(int argc, char* argv[]) {
    Window_SDL window;
    window.create(ivec2{800,600});
    return 0;
}