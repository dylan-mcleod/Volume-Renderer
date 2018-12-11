#define _USE_MATH_DEFINES
//#define GLM_FORCE_AVX // or GLM_FORCE_SSE2 if your processor doesn't support it
#define GLM_FORCE_ALIGNED
#include <iostream>
#include <fstream>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "voxel.h"
#include "volume.h"
#include "main.h"

namespace volly {

    #define Handle_Error_SDL(x) if(x) std::cout << "SDL Error: " << SDL_GetError() << " (Error code" << x << ")" << std::endl;


    template<typename vec4_t>
    void printVec4(vec4_t v, std::string label = "vec4") {
        std::cout << label << ": " << v.x << " " << v.y << " " << v.z << " " << v.w << std::endl;
    }


    Vol_LOD_Set::Vol_LOD_Set(std::string filename, int coloringMode, int sLowR, int lowR, int medR, int highR): sLowR(sLowR), lowR(lowR), medR(medR), highR(highR) {
        med = readOFFFile(filename, medR , coloringMode);
        high = readOFFFile(filename, highR, coloringMode);

        //std::cout << med->size.x <<  "!" << med->size.y << "!" << med->size.z << std::endl;
        //std::cout << high->size.x <<  "!" << high->size.y << "!" << high->size.z << std::endl;

        sLow = new VolumeStore<Voxel>(glm::ivec3(sLowR));
        low = new VolumeStore<Voxel>(glm::ivec3(lowR));

        //std::cout << sLow->size.x <<  " " << sLow->size.y << " " << sLow->size.z << std::endl;
        //std::cout << low->size.x <<  " " << low->size.y << " " << low->size.z << std::endl;

        std::cout << sLowR << std::endl;
        std::cout << lowR << std::endl;

        for(int i = 0; i < highR; ++i) {
            for(int j = 0; j < highR; ++j) {
                for(int k = 0; k < highR; ++k) {

                    if(high->sample(i,j,k).rgba.a > 0) {
                        
                        glm::ivec3 sLowSample(i,j,k);
                        sLowSample *= sLowR;
                        sLowSample /= highR;

                        glm::ivec3 lowSample(i,j,k);
                        lowSample *= lowR;
                        lowSample /= highR;

                        sLow->put(high->sample(i,j,k),sLowSample);
                        low->put(high->sample(i,j,k),lowSample);                        

                    }

                }
            }
        }

        arr[0] = sLow;
        arr[1] = low;
        arr[2] = med;
        arr[3] = high;
    }



    glm::vec3 bgColor(0.0);

    

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
        glm::vec4 ptEnd = glm::vec4(0);
        Data_T dataAt = Data_T();
    };

    template<bool solid = false>
    RaycastReport<Voxel> raycast_naiive_vec4(glm::vec4 start, glm::vec4 direction, VolumeStore<Voxel>* volume) {
        start.w = 0.5;
        direction.w = 0;
        glm::vec4 lineFitLength;
        glm::vec4 vec4VolSz(volume->size,10000);
        glm::vec4 vec4Zeros(0);
        
        glm::vec4 directSign = glm::sign(direction);
        glm::vec4 oneOverDirection = glm::vec4(1)/direction;
        glm::vec4 heaviside  = glm::round((directSign + 1.f) / 2.f);

        // abritrarily small constant, don't read into it
        float eps = 0.00030;
        glm::vec4 directSignEpsilon = directSign*eps;

        // find where the ray intersects the volume, if we're outside it
        glm::vec4 distToVolume = -glm::fma(vec4VolSz, heaviside-glm::vec4(1), start);
        glm::vec4 normDistToVolume = distToVolume * oneOverDirection;
        float maxNormDistToVolume = fmax(fmax(fmax(normDistToVolume.x,normDistToVolume.y),normDistToVolume.z),0);
        glm::vec4 cur = maxNormDistToVolume*direction+start;         

        if(false) {
            printVec4(start, "start");
            printVec4(direction, "direction");
            printVec4(distToVolume, "distToVolume");
            printVec4(normDistToVolume, "normDistToVolume");
            printVec4(cur, "cur");
        }   
        


        glm::vec4 curPlusEpsilon;  
        for(;;) {

            curPlusEpsilon = cur+directSignEpsilon; // Ensure that things on a boundary will be rounded to the correct position.
            glm::ivec4 roundPos(curPlusEpsilon);

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

            Voxel vox = volume->sample(roundPos);

            if(!solid) {
                if(vox.rgba.a > 0) {
                    return RaycastReport<Voxel>{true, cur, vox};
                }
            } else {
                if(vox.rgba.a == 0) {
                    return RaycastReport<Voxel>{true, cur, vox};
                }
            }

            glm::vec4 distToNextGrid = glm::floor(curPlusEpsilon)+heaviside-cur; // If it's negative, we keep the floor. If not, we change this into a ceil.
            glm::vec4 normalizedDist = distToNextGrid*oneOverDirection;
            // Find horizontal minimum. This could potentially kill our performance on SIMD operations... 
            float minDistN = fmin(fmin(normalizedDist.x,normalizedDist.y),normalizedDist.z);
            cur += (minDistN * direction);


           
        }

        Voxel ret(bgColor.r,bgColor.g,bgColor.b);
        return RaycastReport<Voxel>{false, cur, ret};
    }

    RaycastReport<Voxel> raycast_beamOpt(glm::vec4 start, glm::vec4 direction, VolumeStore<Voxel>* lowRes, VolumeStore<Voxel>* highRes, glm::vec4 direction_lowp, glm::vec4 scaleLowptoHighp) {
        RaycastReport<Voxel> lowpRep = raycast_naiive_vec4(start, direction_lowp, lowRes);
        return raycast_naiive_vec4(scaleLowptoHighp*lowpRep.ptEnd, direction, highRes);
    }



int iterationNumber = 3;
glm::vec4 oneOver127 = glm::vec4(1.f/127.f);
glm::vec4 oneOver255 = glm::vec4(1.f/255.f);
glm::vec4 minusOne = glm::vec4(1);
    glm::vec4 raytrace_naiive(int curIteration, glm::vec4 start, glm::vec4 direction, VolumeStore<Voxel>* volume) {
        RaycastReport<Voxel> rep = raycast_naiive_vec4(start, direction, volume);

        if(curIteration <= 1 || !rep.found) return glm::vec4(rep.dataAt.rgba)*oneOver255;

        glm::vec4 norm(rep.dataAt.norm);
        norm = glm::fma(norm, oneOver127, minusOne);

        glm::vec4 reflectDir = glm::reflect(direction, norm);
        glm::vec4 refractDir = glm::refract(direction, norm, 2.f);

        RaycastReport<Voxel> repRefract = raycast_naiive_vec4<true>(rep.ptEnd, refractDir, volume);

        glm::vec4 reflectCol = raytrace_naiive(curIteration-1, rep.ptEnd, reflectDir, volume);
        glm::vec4 refractCol = raytrace_naiive(curIteration-1, repRefract.ptEnd, direction, volume);

        float amtReflect = 0.35;
        float amtRefract = 0.00;
        float amtDiffuse = 0.65;
        return amtReflect * reflectCol + amtRefract * refractCol + amtDiffuse * glm::vec4(rep.dataAt.rgba)*oneOver255;
    }


    void State::raycastOntoScreen() {

        screenTex->lock();

        VolumeStore<Voxel>* volToRender = getCurVol();


        glm::mat4 model = glm::scale(glm::mat4(1),glm::vec3(1.f)/glm::vec3(volToRender->size));
        glm::mat4 MV = view * model;

        float FOV = 90.0 * M_PI / 180.0; // this is in radians, to make everyone's lives easier.
        float d = 1/tan(FOV/2);
        
        glm::mat4 iMV = glm::inverse(MV);

        //std::cout << iMV[0][0] << " " << iMV[1][0] << " " << iMV[2][0] << " " << iMV[3][0] << "\n" <<
        //            iMV[0][1] << " " << iMV[1][1] << " " << iMV[2][1] << " " << iMV[3][1] << "\n" <<
        //            iMV[0][2] << " " << iMV[1][2] << " " << iMV[2][2] << " " << iMV[3][2] << "\n" <<
        //            iMV[0][3] << " " << iMV[1][3] << " " << iMV[2][3] << " " << iMV[3][3] << std::endl;



        glm::vec4 curYLoop = iMV*glm::vec4(-1 * ((float)renderTargetSizeX)/renderTargetSizeY,-1,d,0);
        glm::vec4 ydiff    = iMV*glm::vec4(0,  2,0,0)/(float)renderTargetSizeY;
        glm::vec4 xdiff    = iMV*glm::vec4(2 * ((float)renderTargetSizeX)/renderTargetSizeY, 0,0,0)/(float)renderTargetSizeX;  

        //printVec4(glm::vec4(camPos,0), "camPos");
        //printVec4(glm::vec4(camDir,0), "camDir");
        //printVec4(glm::vec4(camUp,0),   "camUp");


        glm::vec4 camPos_Local = glm::inverse(model) * glm::vec4(camPos,1);
        camPos_Local /= camPos_Local.w;
        camPos_Local.w = 0;

        curYLoop.w = ydiff.w = xdiff.w = 0;


        glm::mat4 lowpmodel = glm::scale(glm::mat4(1),glm::vec3(1.f)/glm::vec3(getCurVol_LowLOD()->size));
        glm::mat4 lowpMV = view * lowpmodel;
        glm::mat4 lowpiMV = glm::inverse(lowpMV);

        glm::vec4 scaleLowptoHighp = glm::vec4(volToRender->size,0) / glm::vec4(getCurVol_LowLOD()->size,1);

        glm::vec4 curYLooplowp = lowpiMV*glm::vec4(-1 * ((float)renderTargetSizeX)/renderTargetSizeY,-1,d,0);
        glm::vec4 ydifflowp    = lowpiMV*glm::vec4(0,  2,0,0)/(float)renderTargetSizeY;
        glm::vec4 xdifflowp    = lowpiMV*glm::vec4(2 * ((float)renderTargetSizeX)/renderTargetSizeY, 0,0,0)/(float)renderTargetSizeX;  
        curYLooplowp.w = ydifflowp.w = xdifflowp.w = 0;


        glm::vec4 camPos_Local_lowP = glm::inverse(lowpmodel) * glm::vec4(camPos,1);
        camPos_Local /= camPos_Local.w;
        camPos_Local.w = 0;

        glm::ivec2 pixel(0,0);
        for(pixel.y = 0; pixel.y < renderTargetSizeY; ++pixel.y) {
        
            glm::vec4 curXLoop(curYLoop);
            glm::vec4 curXLooplowp(curYLooplowp);
            for(pixel.x = 0; pixel.x < renderTargetSizeX; ++pixel.x) {

                
                RaycastReport<Voxel> b = raycast_beamOpt(camPos_Local_lowP, glm::normalize(curXLoop), getCurVol_LowLOD(), volToRender, glm::normalize(curXLooplowp), scaleLowptoHighp);
                //RaycastReport<Voxel> b = raycast_naiive_vec4(camPos_Local, glm::normalize(curXLoop), volToRender);
                if(b.found) {
                    screenTex->data_stream[pixel.x + pixel.y*renderTargetSizeX] = glm::u8vec4(255,b.dataAt.rgba.r,b.dataAt.rgba.g,b.dataAt.rgba.b);
                } else {
                    screenTex->data_stream[pixel.x + pixel.y*renderTargetSizeX] = glm::u8vec4(255,0,0,0);
                }
                
                //glm::vec4 rayTraceColor = glm::clamp(raytrace_naiive(iterationNumber, camPos_Local, glm::normalize(curXLoop), volToRender),glm::vec4(0),glm::vec4(1));

                //glm::u8vec4 colorVec(255, glm::vec3(rayTraceColor)*255.f);

                //screenTex->data_stream[pixel.x + pixel.y*renderTargetSizeX] = colorVec;


                curXLoop += xdiff;
                curXLooplowp += xdifflowp;
            }
            curYLoop += ydiff;
            curYLooplowp += ydifflowp;
        }
        

        screenTex->unlock();
    }

    void State::vollyMainLoop() {
        window = new Window_SDL();
        window->create(glm::ivec2(winSizeX,winSizeY));
        screenTex = new Texture_SDL(glm::ivec2(renderTargetSizeX,renderTargetSizeY), window);

        initAllVolumes();

        ms = (ms_start = SDL_GetTicks());

        
        bool running = true;
        
        SDL_SetRelativeMouseMode(SDL_TRUE);
        mouseX = 0;
        mouseY = 0;
        mouseDX = 0;
        mouseDY = 0;
        while(running) {
            resolveUserInput(); 
            

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

                case SDL_MOUSEMOTION:
                    mouseX = event.motion.x;
                    mouseY = (winSizeY - event.motion.y);
                    mouseDX += event.motion.xrel;
                    mouseDY += -event.motion.yrel;
                    break;
                }
            }
            
            running &= !keys.getIfKeyDown(SDL_SCANCODE_ESCAPE); // can exit program using escape key

            ms_prev = ms;
            ms = SDL_GetTicks();

            ++frame;

            std::cout << "Frame : " << frame << "\n\tCur Frame Time: " << ms-ms_prev << "\n\tAvg Frame Time: " << (ms - ms_start)/frame << std::endl;

            std::cout << "\tCasting " << renderTargetSizeX << "*" << renderTargetSizeY << " rays per frame, this is " 
                      << renderTargetSizeX * renderTargetSizeY * (1000.f/(ms-ms_prev)) << " raycasts per second" << std::endl;

            std::cout << "\ton a " << getCurVol()->size.x << "*" << getCurVol()->size.y << "*" << getCurVol()->size.z << " dataset." << std::endl;
        }
    }


// Only load one of the models, because that takes time!
#define FASTLOAD

    
    void State::initAllVolumes() {
        // what a glorious collection!
        
#ifndef FASTLOAD
        volumes.push_back(new Vol_LOD_Set("icosahedron.off")); // 0
        volumes.push_back(new Vol_LOD_Set("teapot.off"));      // 1
#endif
        volumes.push_back(new Vol_LOD_Set("galleon.off"));     // 2
#ifndef FASTLOAD
        volumes.push_back(new Vol_LOD_Set("dragon.off"));      // 3
        volumes.push_back(new Vol_LOD_Set("footbones.off"));   // 4
        volumes.push_back(new Vol_LOD_Set("sandal.off"));      // 5
        volumes.push_back(new Vol_LOD_Set("stratocaster.off"));// 6
        volumes.push_back(new Vol_LOD_Set("walkman.off"));     // 7
        volumes.push_back(new Vol_LOD_Set("dolphins.off"));    // 8
#endif

    }

    void State::resolveUserInput() {
        // mouse (set camera direction)

        camUp  = glm::vec3(0,0,-1);
        camDir = glm::vec3(cos(2.5f*(-mouseX)*M_PI / winSizeX), sin(2.5f*(-mouseX)*M_PI / winSizeX), -4.f*mouseY/(float)winSizeY+2.f);
        camDir += 0.000003f;
        camDir = glm::normalize(camDir);

        view = glm::lookAt(glm::vec3(0), camDir, camUp);

        // WASD (move the camera)
        float dy = 0, dx = 0, dz = 0;
        float speed = 0.01;
        if(keys.getIfKeyDown(SDL_SCANCODE_W)) dy += 1;
        if(keys.getIfKeyDown(SDL_SCANCODE_A)) dx -= 1;
        if(keys.getIfKeyDown(SDL_SCANCODE_S)) dy -= 1;
        if(keys.getIfKeyDown(SDL_SCANCODE_D)) dx += 1;
        if(keys.getIfKeyDown(SDL_SCANCODE_LCTRL)) dz -= 1;
        if(keys.getIfKeyDown(SDL_SCANCODE_LSHIFT)) dz += 1;

        glm::vec3 Dy = -camDir*dy;
        camPos += speed*Dy;
        
        glm::vec3 right = glm::cross(camUp, camDir);
        
        glm::vec3 Dx = -right*dx;
        camPos += speed*Dx;

        glm::vec3 Dz = -camUp*dz;
        camPos += speed*Dz;

#ifndef FASTLOAD
        // interface
        if(keys.getIfKeyDown(SDL_SCANCODE_0)) curVolume = 0;
        if(keys.getIfKeyDown(SDL_SCANCODE_1)) curVolume = 1; 
        if(keys.getIfKeyDown(SDL_SCANCODE_2)) curVolume = 2;
        if(keys.getIfKeyDown(SDL_SCANCODE_3)) curVolume = 3;
        if(keys.getIfKeyDown(SDL_SCANCODE_4)) curVolume = 4;
        if(keys.getIfKeyDown(SDL_SCANCODE_5)) curVolume = 5;
        if(keys.getIfKeyDown(SDL_SCANCODE_6)) curVolume = 6;
        if(keys.getIfKeyDown(SDL_SCANCODE_7)) curVolume = 7;
        if(keys.getIfKeyDown(SDL_SCANCODE_8)) curVolume = 8;
#endif

        if(keys.getIfKeyDown(SDL_SCANCODE_M))      curLoD = 0;
        if(keys.getIfKeyDown(SDL_SCANCODE_COMMA))  curLoD = 1;
        if(keys.getIfKeyDown(SDL_SCANCODE_PERIOD)) curLoD = 2;
        if(keys.getIfKeyDown(SDL_SCANCODE_SLASH))  curLoD = 3;
    }
}

int main(int argc, char* argv[]) {
    volly::State state;
    state.vollyMainLoop();
    return 0;
}
