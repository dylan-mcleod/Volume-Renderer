#define _USE_MATH_DEFINES
#include <iostream>
#include <fstream>
#include <SDL2/SDL.h>
#include <glm/glm.hpp>
#include <glm/gtx/simd_vec4.hpp>
#include <glm/gtx/simd_mat4.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>

#define Handle_Error_SDL(x) if(x) std::cout << "SDL Error: " << SDL_GetError() << " (Error code" << x << ")" << std::endl;

// Base class which -- you guessed it -- stores volume data
class VolumeStore {
    glm::ivec3 startB, endB;
    glm::ivec3 size;
    bool *data;

public:
    inline bool& sample_local(glm::ivec3 inp) {
        return data[inp.x + startB.x * (inp.y + startB.y * inp.z)];
    }

    inline bool& sample_local(int x, int y, int z) {
        return data[x + startB.x * (y + startB.y * z)];
    }

    inline bool& sample(glm::ivec3 inp) {
        return data[(inp.x-startB.x) + startB.x * ((inp.y-startB.y) + startB.y * (inp.z-startB.z))];
    }

    inline bool& sample(int x, int y, int z) {
        return data[(x-startB.x) + startB.x * ((y-startB.y) + startB.y * (z-startB.z))];
    }

    VolumeStore(glm::ivec3 startB, glm::ivec3 endB) {
        glm::ivec3 size = endB-startB;
        data = new bool[size.x*size.y*size.z];


        // make this a sphere

        glm::ivec3 pt = startB;
        int ind = 0;
        glm::vec3 mid = glm::vec3(endB-startB)/glm::vec3(2.f) + glm::vec3(endB);
        for(pt.z=startB.z; pt.z < endB.z; ++pt.z) {
            for(pt.y=startB.y; pt.y < endB.y; ++pt.y) {
                for(pt.x=startB.x; pt.x < endB.x; ++pt.x) {
                    glm::vec3 rad = glm::vec3(pt)/glm::vec3(size)-mid;
                    float radSquare = glm::dot(rad,rad);
                    bool yes = radSquare < 1;
                    data[ind] = yes;
                    ++ind;
                }
            }
        }

    }
};



class Window_SDL {
    SDL_Window *window;
    SDL_Renderer *renderer;


public:

    glm::ivec2 size;

    void create(glm::ivec2 size);
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
    glm::ivec2 size;

    // Assigns data_stream and allows it to be written to
    void lock() {
        SDL_LockTexture(_texture, NULL, (void**) &data_stream, &pitch);
    }

    void streamData() {
        SDL_UnlockTexture(_texture);
    }
    
    Texture_SDL(glm::ivec2 size) {

    }

};

void Window_SDL::create(glm::ivec2 size) {
    this->size = size;
    Handle_Error_SDL(SDL_Init(SDL_INIT_VIDEO));

    Handle_Error_SDL(SDL_CreateWindowAndRenderer(size.x, size.y, SDL_WINDOW_RESIZABLE, &window, &renderer));
}

bool raycast_naiive(glm::vec3 start, glm::vec3 direction, float maxLen, VolumeStore* volume) {
    glm::vec3 lineFitLength;
    glm::vec3 cur = start;

    int numIters = maxLen; // todo: compute this

    // abritrarily small constant, don't read into it
    glm::vec3 fiveepsilon = glm::vec3(5);//*glm::epsilon<float>;

    for(int i = 0; i < numIters; i++) {
        glm::vec3 distToNextGrid = glm::ceil(cur+fiveepsilon)-cur;
        glm::vec3 normalizedDist = distToNextGrid/direction;
        // Find horizontal minimum... This could kill our performance on non-batched SIMD operations... 
        float minDistN = fmin(fmin(normalizedDist.x,normalizedDist.y),normalizedDist.z);
        cur += (minDistN * direction);

        if(volume->sample_local(glm::ivec3(cur))) return true;
    }
    return false;
}

struct Voxel {
    glm::u8vec4 rgba;
    glm::u8vec4 norm = glm::u8vec4(0);

    Voxel(uint8_t R, uint8_t G, uint8_t B, uint8_t A = 255): rgba(R,G,B,A) {}
    Voxel(uint8_t l): rgba(l,l,l,255) {}
    static Voxel fromUnsignedShort(unsigned short l) { return Voxel(l>>8,l>>8,l>>8); }
    static Voxel fromNormalizedFloats(float R, float G, float B, float A = 1.f) { 
         return Voxel(
            (uint8_t)floor(R*255.f), 
            (uint8_t)floor(R*255.f), 
            (uint8_t)floor(R*255.f), 
            (uint8_t)floor(R*255.f)
        ); 
    }

    // empty by default
    Voxel(): rgba(0) {}
};

Window_SDL *window;

void raycastOntoScreen() {

    glm::mat4 model = glm::mat4();


    glm::vec3  camPos(0);
    glm::vec3  lookAt(1/sqrt(3));
    glm::vec3  up(0,1,0); // This is lazy, but who cares? We're taking the cardinal direction y and calling it up. True up, if you will.

    glm::mat4 view = glm::lookAt(camPos, lookAt, up);

    float FOV = 45.0 * M_PI / 180.0; // this is in radians, to make everyone's lives easier.
    glm::mat4 projection = glm::perspective(FOV, (float)window->size.x / window->size.y, 0.1f, 10000.f);









/*
    for(pixel.y = 0; pixel.y < window.size.y; ++pixel.y) {

        for(pixel.x = 0; pixel.x < window.size.x; ++pixel.x) {
            

        }
    }*/
    



}


int main(int argc, char* argv[]) {
    window = new Window_SDL();
    window->create(glm::ivec2(800,600));

    VolumeStore* myVol = new VolumeStore({0,0,0}, {50,50,50});
    return 0;
}