#ifndef PLYFILEREADER_H

#define PLYFILEREADER_H

#include <glm/glm.hpp>
#include <vector>
#include <string>

namespace volly {


	struct Polyhedron{
		std::vector<glm::vec3> verts;
		std::vector<glm::ivec3> inds;
	};
    
	int stringToInt(std::string);
	Polyhedron readPLYFile(std::string fileName);
	glm::vec4 stringToVec4(std::string);
	glm::ivec4 stringToivec4(std::string);

}

#endif