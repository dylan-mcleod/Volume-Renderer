#include <iostream>
#include <fstream>
#include <glm/glm.hpp>
#include <cmath>

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


// Base class which -- you guessed it -- stores volume data
template<typename Data_T>
class VolumeStore {
public:
    glm::ivec3 size;
    Data_T *data;

    inline Data_T& sample(glm::ivec3 inp) {
        return data[inp.x + size.x * (inp.y + size.y * inp.z)];
    }

    inline Data_T& sample(int x, int y, int z) {
        return data[x + size.x * (y + size.y * z)];
    }

	inline void put(Data_T d, glm::ivec3 inp) {
        data[inp.x + size.x * (inp.y + size.y * inp.z)] = d;
    }

    inline void put(Data_T d, int x, int y, int z) {
    	data[x + size.x * (y + size.y * z)] = d;
    }

	


    VolumeStore(glm::ivec3 size, Data_T fillCircle = Data_T()): size(size) {
        data = new Data_T[size.x*size.y*size.z];


        // make this a sphere

        glm::ivec3 pt(0);
        int ind = 0;
        glm::vec3 mid = glm::vec3(size)/glm::vec3(2.f);
        for(pt.z=0; pt.z < size.z; ++pt.z) {
            for(pt.y=0; pt.y < size.y; ++pt.y) {
                for(pt.x=0; pt.x < size.x; ++pt.x) {
                    glm::vec3 rad = 4.0f*(glm::vec3(pt)-mid)/(glm::vec3(size));
                    float radSquare = glm::dot(rad,rad);
                    bool yes = radSquare < 1;
                    data[ind] = yes?fillCircle:Data_T();
                    ++ind;
                }
            }
        }
    }
};

bool isAboveThreshold(Voxel vox, uint8_t threshold) {
	return ((vox.rgba.r > threshold) | (vox.rgba.b > threshold) | (vox.rgba.g > threshold)) & (vox.rgba.a > threshold);
}

void readVolFile(std::string filename, VolumeStore<Voxel>** myVol) {
	std::string ID;//a tag to identify the file type, might want this later

	char strBuf[100];

	//int headLeangth;//length of the header, should be constant
	int fullH, fullW, fullD;//the height width and depth of the box stored in the file. use these to set up the creation loops to the right sizes
	float voxelH, voxelW, voxelD;//the size of a voxel in the dataset, will be needed for depancaking down the road
	float blackPoint;//we arent really sure if we will need this, but it may help with render balance
	float whitePoint;
	float gamma;//holds the reasonable gamma, another thing like the black point that we probably wont need, but is worth holding on to
	std::string storeType;//a very important 3 character string that indicates how the data is stored. use to turn on and off certain processes in the creation loop before passing to the constructor
	
	bool g08, g16, c24;//bools to flag the format. will change what the read in for the data does
	g08=false;
	g16=false;
	c24=false;

	uint8_t rByte;
	uint8_t gByte;
	uint8_t bByte;//storage for the raw bytes to be transfered to the struct

	uint8_t threshold = 5;//set a threshold for how bright a voxel has to be for it to be filled, to help eliminate random noise. not used yet

	//opening the file
	std::fstream file;
	file.open("assets/" + filename, std::ios_base::binary);
	if(file.is_open()){
		file.read(strBuf, 5);
		ID = std::string(strBuf, 5);
		if(true){
			file.ignore(5);//skips the version identifier and header length, cause we dont need them
			
			file.read((char*)&fullH, 4);
			file.read((char*)&fullW, 4);
			file.read((char*)&fullD, 4);//gets the dimensions of the area needed to propperly render the image

			(*myVol) = new VolumeStore<Voxel>({0,0,0});

			file.read((char*)&voxelH, 4);
			file.read((char*)&voxelW, 4);
			file.read((char*)&voxelD, 4);//these are the sizes of the voxels, needed top calculate ratios to depancake the final model

			file.read((char*)&blackPoint, 4);
			file.read((char*)&whitePoint, 4);
			file.read((char*)&gamma, 4);//dont really know what these are for, but they may be useful

			file.read(strBuf, 3);//very important string, use this to determine what to expect coming in
			storeType = std::string(strBuf, 3);

			if(storeType=="g08"){g08=true;}
			if(storeType=="g16"){g16=true;}
			if(storeType=="c24"){c24=true;}//set the propper flag for the type pf data stored to process propperly

			file.ignore(9951);//skipping past the rest of the header that only holds a description and title, who cares about those here? not me!

			//on to setting up the loops
			for(int z=0;z<fullD;z++){
				for(int y=0;y<fullW;y++){
					for(int x=0;y<fullH;y++){
						Voxel vox;
						if(g08){
							file.read((char*)&rByte, 1);
							//TODO pass it to constructor
							vox=Voxel(rByte);
						}
						if(g16){
							file.read((char*)&rByte, 1);
							file.ignore(1);//we wont need the second byte of the data, its just going to be devided out, so i wont even read it in, save some work
							//TODO pass it to constructor
							vox=Voxel(rByte);
						}
						if(c24){
							file.read((char*)&rByte, 1);
							file.read((char*)&gByte, 1);
							file.read((char*)&bByte, 1);
							//TODO pass all 3 to a constructor
							vox=Voxel(rByte, gByte, bByte);
						}
						if(isAboveThreshold(vox, threshold)) {
							(*myVol)->put(vox, x,y,z);
						}
					}
				}
			}

		}
		else std::cout << "Error: incorrect file type" << std::endl;
	}
	//read first couple of data points and store them
	//set up loop aparatus depending on size of object
	//set up propper outputs to the constructor
	//start the loop and start processing
}