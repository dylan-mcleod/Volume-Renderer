#define _USE_MATH_DEFINES
//#define GLM_FORCE_AVX // or GLM_FORCE_SSE2 if your processor doesn't support it
#define GLM_FORCE_ALIGNED
#include <iostream>
#include <fstream>
#include <SDL2/SDL.h>
#include <glm/glm.hpp>
//#include <glm/gtx/simd_vec4.hpp>
//#include <glm/gtx/simd_mat4.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>
#include <map>
// This is temporary, please don't yell at me
#include "volume.cpp"


#define Handle_Error_SDL(x) if(x) std::cout << "SDL Error: " << SDL_GetError() << " (Error code" << x << ")" << std::endl;


VolumeStore<Voxel> *myVol;

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

template<typename Data_T>
struct RaycastReport {
    bool found = false;
    glm::vec3 ptEnd = glm::vec3(0);
    Data_T dataAt = Data_T();
};

RaycastReport<Voxel> raycast_naiive(glm::vec3 start, glm::vec3 direction, float maxLen, VolumeStore<Voxel>* volume) {
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
          )) {
                Voxel vox = volume->sample(roundedPos);
                if(vox.rgba.a > 0) {
                    //std::cout << "a" << std::endl;
                    return RaycastReport<Voxel>{true, glm::vec3(cur), vox};
                }
            }
    }
    return RaycastReport<Voxel>{};
}

RaycastReport<Voxel> raycast_naiive_vec4(glm::vec4 start, glm::vec4 direction, float maxLen, VolumeStore<Voxel>* volume) {
    start.w = 0.5;
    direction.w = 0;
    glm::vec4 lineFitLength;
    glm::vec4 cur = start;
    glm::vec4 vec4VolSz(volume->size,10000);
    glm::vec4 vec4Zeros(0);

	
	glm::vec4 directSign = glm::sign(direction);
    glm::vec4 oneOverDirection = glm::vec4(1)/direction;
	glm::vec4 heaviside  = glm::round((directSign + 1.f) / 2.f);
    glm::vec4 dotWith = glm::vec4(1,volume->size.x,volume->size.y*volume->size.x,0);

    // abritrarily small constant, don't read into it
    float eps = 0.00030;
	glm::vec4 directSignEpsilon = directSign*eps;
	
	
    for(;;) {
		glm::vec4 curPlusEpsilon = cur+directSignEpsilon;    // Ensure that things on a boundary will be rounded to the correct position.
		glm::vec4 distToNextGrid = glm::floor(curPlusEpsilon)+heaviside-cur; // If it's negative, we keep the floor. If not, we change this into a ceil.
        glm::vec4 normalizedDist = distToNextGrid*oneOverDirection;
        // Find horizontal minimum. This could potentially kill our performance on SIMD operations... 
        float minDistN = fmin(fmin(normalizedDist.x,normalizedDist.y),normalizedDist.z);
        cur += (minDistN * direction);

        glm::ivec4 roundPos(cur);

        if(
            (roundPos.x >= volume->size.x) | 
            (roundPos.y >= volume->size.y) |
            (roundPos.z >= volume->size.z) |
            (roundPos.x <  0) |
            (roundPos.y <  0) |
            (roundPos.z <  0)
        ) {
            break;
        }

        //int ind = glm::dot(dotWith,glm::floor(cur));
        Voxel vox = volume->sample(roundPos);

        if(vox.rgba.a > 0) {
            return RaycastReport<Voxel>{true, cur, vox};
        }
    }
    return RaycastReport<Voxel>{};
}

void raycastOntoScreen() {

    screenTex->lock();

    //glm::mat4 model = glm::mat4();


    glm::vec3  camPos(50,50,0);
    glm::vec3  lookAt(1/sqrt(3));
    glm::vec3  up(0,1,0); // This is lazy, but who cares? We're taking the cardinal direction y and calling it up. True up, if you will.

    //glm::mat4 view = glm::lookAt(camPos, lookAt, up);

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

    //std::cout << xdiff.x << " " << xdiff.y << " " << xdiff.z << std::endl;

    //std::cout << ydiff.x << " " << ydiff.y << " " << ydiff.z << std::endl;

    //std::cout << curYLoop.x << " " << curYLoop.y << " " << curYLoop.z << std::endl;


	glm::ivec2 pixel(0,0);
    for(pixel.y = 0; pixel.y < window->size.y; ++pixel.y) {
	
		glm::vec3 curXLoop(curYLoop);
        for(pixel.x = 0; pixel.x < window->size.x; ++pixel.x) {

            RaycastReport<Voxel> b = raycast_naiive_vec4(glm::vec4(camPos,0), glm::vec4(glm::normalize(curXLoop),0), 200.f, myVol);
            
            //glm::vec3 vvvec = glm::normalize(curXLoop);
            //screenTex->data_stream[pixel.x + pixel.y * window->size.x] = Voxel::fromNormalizedFloats(vvvec.x,vvvec.y,vvvec.z).rgba;

			if(b.found) {
                float normB = (glm::length(b.ptEnd-camPos)-20.f) / 30.f * 255.f;
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

int frame = 0;
int ms = 0;
int ms_prev = 0;
int ms_start = 0;
int main(int argc, char* argv[]) {
    window = new Window_SDL();
    window->create(glm::ivec2(400,300));
    screenTex = new Texture_SDL(glm::ivec2(400,300));

    myVol = new VolumeStore<Voxel>({100,100,100}, Voxel(255,255,255,255));

    ms = (ms_start = SDL_GetTicks());

	
	bool running = true;
	while(running) {
		raycastOntoScreen();
        SDL_SetRenderDrawColor(window->renderer, 255, 0, 255, 255);
		window->clear();
		window->drawTexture(screenTex);
		SDL_RenderPresent(window->renderer);
            
            
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

        ms_prev = ms;
        ms = SDL_GetTicks();
        ++frame;

        std::cout << "Frame : " << frame << "\n\tCur Frame Time: " << ms-ms_prev << "\n\tAvg Frame Time: " << (ms - ms_start)/frame << std::endl;
    }
	
    return 0;
}
