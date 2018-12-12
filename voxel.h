#ifndef VOXEL_H

#define VOXEL_H

#include "vollyglm.h"





namespace volly {

	struct Voxel {
		glm::u8vec4 rgba;
		glm::u8vec4 norm = glm::u8vec4(0);

		Voxel(const Voxel& v) {
			rgba = v.rgba;
			norm = v.norm;
		} 


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

}


#endif // VOXEL_H