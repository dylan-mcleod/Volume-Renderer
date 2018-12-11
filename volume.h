#ifndef VOLUME_H

#define VOLUME_H

#include "voxel.h"
#include <string>

namespace volly {

/*
    // Base class which -- you guessed it -- stores volume data
    template<typename Data_T>
    class VolumeStore_Base {
    public:
        glm::ivec3 size;
        //glm::ivec3 mid = glm::ivec3(0);
        //glm::vec4 size_v4  = glm::vec4(0);
        //glm::vec4 mid_v4  = glm::vec4(0);
        VolumeStore_Base(glm::ivec3 size): size(size) {}

        virtual ~VolumeStore_Base() {}

        bool inBounds(glm::ivec3 p) {
            return 
                (p.x < size.x) & 
                (p.y < size.y) &
                (p.z < size.z) &
                (p.x >= 0) &
                (p.y >= 0) &
                (p.z >= 0);
        }

        virtual Data_T& sample(int x, int y, int z) = 0;

        virtual void put(Data_T d, int x, int y, int z) = 0;

        virtual Data_T& sample(glm::ivec3 inp) = 0;

        virtual void put(Data_T d, glm::ivec3 inp) = 0;

    };*/

	// Default Volume Store which stores data in an array. Pretty great, right?
	template<typename Data_T>
	class VolumeStore{
	public:
        glm::ivec3 size;
		Data_T *data;

        bool inBounds(glm::ivec3 p) {
            return 
                (p.x < size.x) & 
                (p.y < size.y) &
                (p.z < size.z) &
                (p.x >= 0) &
                (p.y >= 0) &
                (p.z >= 0);
        }

		Data_T& sample(int x, int y, int z) {
			return data[x + this->size.x * (y + this->size.y * z)];
		}

		void put(Data_T d, int x, int y, int z) {
			data[x + this->size.x * (y + this->size.y * z)] = d;
		}

		Data_T& sample(glm::ivec3 inp) {
			return data[inp.x + this->size.x * (inp.y + this->size.y * inp.z)];
		}

		void put(Data_T d, glm::ivec3 inp) {
			data[inp.x + this->size.x * (inp.y + this->size.y * inp.z)] = d;
		}

		VolumeStore(glm::ivec3 size): size(size) {
			data = new Data_T[this->size.x*this->size.y*this->size.z];
		}

		~VolumeStore() {
			delete[] data;
		}

		// load this up with a hardcoded sphere, so we can test without access to files
		void fillSphere(Data_T fillWith) {
			glm::ivec3 pt(0);
			int ind = 0;
			glm::vec3 mid = glm::vec3(this->size)/glm::vec3(2.f);
			for(pt.z=0; pt.z < this->size.z; ++pt.z) {
				for(pt.y=0; pt.y < this->size.y; ++pt.y) {
					for(pt.x=0; pt.x < this->size.x; ++pt.x) {
						glm::vec3 rad = 4.0f*(glm::vec3(pt)-mid)/(glm::vec3(this->size));
						float radSquare = glm::dot(rad,rad);
						bool yes = radSquare < 1;
						data[ind] = yes?fillWith:Data_T();
						++ind;
					}
				}
			}
		}
	};

	VolumeStore<Voxel>* readOFFFile(std::string filename, int resolution, int coloringMode);
    
    void readSimpleVolFile(std::string filename, VolumeStore<Voxel>** myVol);

    void readVolFile(std::string filename, VolumeStore<Voxel>** myVol);

}



#endif // VOLUME_H