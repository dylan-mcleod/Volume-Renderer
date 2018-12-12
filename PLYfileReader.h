#ifndef PLYFILEREADER_H

#define PLYFILEREADER_H

#include <vector>
#include <string>
#include <cstring>
#include <map>

#include "voxel.h"
#include "volume.h"

#include "vollyglm.h"

namespace volly {


	struct Polyhedron {
		std::vector<glm::vec4> verts;
		std::vector<glm::ivec3> inds;
	};

    void normalizePoly(Polyhedron*);
    std::map<glm::ivec4, Voxel, ivec4_cmp>* rasterizeVoxelMapFromPoly(Polyhedron* polyIn, int res);
    
	int stringToInt(std::string);
	Polyhedron* readPLYFile(std::string fileName);
	glm::vec4 stringToVec4(std::string);
	glm::ivec4 stringToivec4(std::string);

}

#endif