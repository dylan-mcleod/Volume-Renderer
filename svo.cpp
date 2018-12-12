#include <iostream>
#include <fstream>
#include <cmath>
#include <ctime>
#include <cstdlib>
#include <vector>
#include <map>
#include <set>

#include "voxel.h"
#include "vollyglm.h"

namespace volly {
    // SVO data structure from this lovely paper:
    // https://www.nvidia.com/docs/IO/88889/laine2010i3d_paper.pdf
    struct SVO_Voxel {
        uint16_t childPtr; // &1 is far bit -- this must be masked out to dereference
        uint8_t  valid_mask;
        uint8_t  leaf_mask;
        uint32_t countour; // we're using this as the child pointer for now...
    };

    // Spooky business here. A real black magic data structure. 
    class SparseVoxelOctree {
        // There's most likely a better way to store this, but this is what we're going with for now.
        std::vector<uint64_t> data;

        void build(std::set<glm::ivec3> coords);
    };
}