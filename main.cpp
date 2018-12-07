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
#include <map>

#define Handle_Error_SDL(x) if(x) std::cout << "SDL Error: " << SDL_GetError() << " (Error code" << x << ")" << std::endl;

// Base class which -- you guessed it -- stores volume data
class VolumeStore {
public:
    glm::ivec3 startB, endB;
    glm::ivec3 size;
    bool *data;

    inline bool& sample_local(glm::ivec3 inp) {
        return data[inp.x + size.x * (inp.y + size.y * inp.z)];
    }

    inline bool& sample_local(int x, int y, int z) {
        return data[x + size.x * (y + size.y * z)];
    }

    inline bool& sample(glm::ivec3 inp) {
        return data[(inp.x-startB.x) + startB.x * ((inp.y-startB.y) + startB.y * (inp.z-startB.z))];
    }

    inline bool& sample(int x, int y, int z) {
        return data[(x-startB.x) + startB.x * ((y-startB.y) + startB.y * (z-startB.z))];
    }

    VolumeStore(glm::ivec3 startB, glm::ivec3 endB) {
        size = endB-startB;
        data = new bool[size.x*size.y*size.z];


        // make this a sphere

        glm::ivec3 pt = startB;
        int ind = 0;
        glm::vec3 mid = glm::vec3(endB)/glm::vec3(2.f);
        for(pt.z=startB.z; pt.z < endB.z; ++pt.z) {
            for(pt.y=startB.y; pt.y < endB.y; ++pt.y) {
                for(pt.x=startB.x; pt.x < endB.x; ++pt.x) {
                    glm::vec3 rad = 4.3f*(glm::vec3(pt)-mid)/(glm::vec3(size));
                    float radSquare = glm::dot(rad,rad);
                    bool yes = radSquare < 1;
                    data[ind] = yes;
                    ++ind;
                }
            }
        }

    }
};
VolumeStore *myVol;

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

Window_SDL  *window;

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
    
    Texture_SDL(glm::ivec2 size) {
        this->size = size;

        _texture = SDL_CreateTexture(window->renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, size.x, size.y);

        data_stream = nullptr;
    }

};

Texture_SDL *screenTex;

void Window_SDL::create(glm::ivec2 size) {
    this->size = size;
    Handle_Error_SDL(SDL_Init(SDL_INIT_VIDEO));

    Handle_Error_SDL(SDL_CreateWindowAndRenderer(size.x, size.y, SDL_WINDOW_RESIZABLE, &window, &renderer));
}

void Window_SDL::drawTexture(Texture_SDL * tex, glm::ivec2 _pos, glm::ivec2 _size) {
	if(_size.x < 0) _size.x = this->size.x;
	if(_size.y < 0) _size.y = this->size.y;
	
	SDL_Rect texture_rect;
	texture_rect.x = _pos.x;
	texture_rect.y = _pos.y;
	texture_rect.w = _size.x;
	texture_rect.h = _size.y;
	
	SDL_RenderCopy(renderer, tex->_texture, NULL, &texture_rect);
}

void Window_SDL::clear() {
	SDL_RenderClear(renderer);
}

float raycast_naiive(glm::vec3 start, glm::vec3 direction, float maxLen, VolumeStore* volume) {
    glm::vec3 lineFitLength;
    glm::vec3 cur = start;

    int numIters = maxLen; // todo: compute this
	
	glm::vec3 directSign = glm::sign(direction);
	glm::vec3 heaviside  = glm::round((directSign + 1.f) / 2.f);

    // abritrarily small constant, don't read into it
    float eps = 5.f * 0.00006;
	glm::vec3 directSignEpsilon = directSign*eps;
	
	
    for(int i = 0; i < numIters; i++) {
		glm::vec3 curPlusEpsilon = cur+directSignEpsilon;    // Ensure that things on a boundary will be rounded to the correct position.
		glm::vec3 distToNextGrid = glm::floor(curPlusEpsilon)+heaviside-cur; // If it's negative, we keep the floor. If not, we change this into a ceil.
        glm::vec3 normalizedDist = distToNextGrid/direction;
        // Find horizontal minimum. This could potentially kill our performance on SIMD operations... 
        float minDistN = fmin(fmin(normalizedDist.x,normalizedDist.y),normalizedDist.z);
        cur += (minDistN * direction);

        glm::ivec3 roundedPos = glm::ivec3(glm::floor(cur));
        if((
            (roundedPos.x < volume->size.x) & (roundedPos.y < volume->size.y) & (roundedPos.z < volume->size.z) 
          & (roundedPos.x >=0) & (roundedPos.y >=0) & (roundedPos.z >=0)
          ) 
            && volume->sample_local(roundedPos)) {
                //std::cout << "a" << std::endl;
                return glm::length(cur-start);
            }
    }
    return -1;
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
            (uint8_t)floor(G*255.f), 
            (uint8_t)floor(B*255.f), 
            (uint8_t)floor(A*255.f)
        ); 
    }

    // empty by default
    Voxel(): rgba(0) {}
};

void raycastOntoScreen() {

    screenTex->lock();

    //glm::mat4 model = glm::mat4();


    glm::vec3  camPos(50,50,0);
    glm::vec3  lookAt(1/sqrt(3));
    glm::vec3  up(0,1,0); // This is lazy, but who cares? We're taking the cardinal direction y and calling it up. True up, if you will.

    glm::mat4 view = glm::lookAt(camPos, lookAt, up);

    float FOV = 60.0 * M_PI / 180.0; // this is in radians, to make everyone's lives easier.
	
	
	// This should hopefully set the ray end to be at the screen, and we set the start to be at the camera. However, this probably doesn't matter.
	float d = 1/tan(FOV/2);
	
    glm::mat4 projection = glm::perspective(FOV, (float)window->size.x / window->size.y, 0.001f, d);
	
	
	glm::mat4 VP = projection;
	glm::mat4 iVP = glm::inverse(VP);



	glm::vec4 topleft    (-1,-1,1,1);
	glm::vec4 topright   (-1, 1,1,1);
	glm::vec4 bottomleft ( 1,-1,1,1);
	glm::vec4 bottomright( 1, 1,1,1);
	

	topleft = iVP * topleft;
	topright = iVP * bottomright;
	bottomleft = iVP * topleft;
	bottomright = iVP * bottomright;
	
	topleft     /= topleft.w;
	topright    /= topright.w;
	bottomleft  /= bottomleft.w;
	bottomright /= bottomright.w;

	glm::vec3 xdiff = topright-topleft;
	glm::vec3 ydiff = bottomleft-topleft;
	xdiff /= window->size.x;
	ydiff /= window->size.y;

	glm::vec3 curYLoop = topleft;
    curYLoop -= camPos;



    curYLoop = glm::vec3(-1 * ((float)window->size.x)/window->size.y,-1,d);
    ydiff    = glm::vec3(0,  2,0)/(float)window->size.y;
    xdiff    = glm::vec3(2 * ((float)window->size.x)/window->size.y, 0,0)/(float)window->size.x;  

    std::cout << xdiff.x << " " << xdiff.y << " " << xdiff.z << std::endl;

    std::cout << ydiff.x << " " << ydiff.y << " " << ydiff.z << std::endl;

    std::cout << curYLoop.x << " " << curYLoop.y << " " << curYLoop.z << std::endl;


	glm::ivec2 pixel(0,0);
    for(pixel.y = 0; pixel.y < window->size.y; ++pixel.y) {
	
		glm::vec3 curXLoop(curYLoop);
        for(pixel.x = 0; pixel.x < window->size.x; ++pixel.x) {

            float b = raycast_naiive(camPos, glm::normalize(curXLoop), 200.f, myVol);
            
            //glm::vec3 vvvec = glm::normalize(curXLoop);
            //screenTex->data_stream[pixel.x + pixel.y * window->size.x] = Voxel::fromNormalizedFloats(vvvec.x,vvvec.y,vvvec.z).rgba;

			if(b >= 0) {
                float normB = (b-20.f) / 30.f * 255.f;
                uint8_t l = normB;
				screenTex->data_stream[pixel.x + pixel.y*window->size.x] = glm::u8vec4(255,l,l,l);
                //std::cout << "Wow!" << std::endl;
			} else {
				screenTex->data_stream[pixel.x + pixel.y*window->size.x] = glm::u8vec4(255,0,0,0);
			}
			curXLoop += xdiff;
        }
		curYLoop += ydiff;
    }
    

    screenTex->unlock();


}

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

KeyHandler keys;


int main(int argc, char* argv[]) {
    window = new Window_SDL();
    window->create(glm::ivec2(400,300));
    screenTex = new Texture_SDL(glm::ivec2(400,300));

    myVol = new VolumeStore({0,0,0}, {100,100,100});

	
	bool running = true;
	while(running) {
		raycastOntoScreen();
        SDL_SetRenderDrawColor(window->renderer, 255, 0, 255, 255);
		window->clear();
		window->drawTexture(screenTex);
		SDL_RenderPresent(window->renderer);

        for(int i = 0; i < 50; i++) {
            SDL_Delay(10);
            
            
            
            SDL_Event event;
            while(SDL_PollEvent(&event)) {
                switch (event.type) {
                    
                case SDL_QUIT:
                    running = false;
                    break;
                    
                case SDL_KEYDOWN:
                    keys.setKeyState(event.key.keysym.scancode, true);
                    break;
                    
                case SDL_KEYUP:
                    keys.setKeyState(event.key.keysym.scancode, false);
                    break;
                
                }
            }
            
            running &= !keys.getIfKeyDown(SDL_SCANCODE_ESCAPE); // can exit program using escape key
        }
    }
	
    return 0;
}
