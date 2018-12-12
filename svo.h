#ifndef SVO_H
#define SVO_H




#include <iostream>
#include <fstream>
#include <cmath>
#include <ctime>
#include <cstdlib>
#include <vector>
#include <map>

#include "voxel.h"
#include "vollyglm.h"
#include "volume.h"

namespace volly {
    // SVO data structure from this lovely paper:
    // https://www.nvidia.com/docs/IO/88889/laine2010i3d_paper.pdf
    struct SVO_Voxel {
        uint16_t childPtr; // &1 is far bit -- this must be masked out to dereference
        // ^ unused for now
        uint8_t  valid_mask;
        uint8_t  leaf_mask;
        uint32_t countour; // we're using this as the child pointer for now...

        Voxel vox; // wow SO temporary, this should be interleaved smartly
    };

    // Spooky business here. A real black magic data structure. 
    class SparseVoxelOctree {
        // There's a better way to store this, but this is what we're going with for now.
        std::vector<SVO_Voxel> data;
    public:

        SparseVoxelOctree(std::map<glm::ivec4, Voxel, ivec4_cmp>* coords, int res);
    };
}


#endif