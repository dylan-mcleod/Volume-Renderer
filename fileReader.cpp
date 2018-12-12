#include <iostream>
#include <fstream>
#include <glm/glm.hpp>
#include <cmath>
using namespace std;


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

void readIn();

int main(){
	cout<<"yeah, it works";
}

void readIn(){
	string ID;//a tag to identify the file type, might want this later
	int headLeangth;//length of the header, should be constant
	int fullH, fullW, fullD;//the height width and depth of the box stored in the file. use these to set up the creation loops to the right sizes
	float voxelH. voxelW, voxelD;//the size of a voxel in the dataset, will be needed for depancaking down the road
	float blackPoint;//we arent really sure if we will need this, but it may help with render balance
	float whitePoint;
	float gamma;//holds the reasonable gamma, another thing like the black point that we probably wont need, but is worth holding on to
	string storeType;//a very important 3 character string that indicates how the data is stored. use to turn on and off certain processes in the creation loop before passing to the constructor
	
	bool g08, g16, c24;//bools to flag the format. will change what the read in for the data does
	g08=false;
	g16=false;
	c24=false;

	uint8_t rByte;
	uint8_t gByte;
	uint8_t bByte;//storage for the raw bytes to be transfered to the struct

	uint8_t threshold=00000101;//set a threshold for how bright a voxel has to be for it to be filled, to help eliminate random noise. not used yet

	//opening the file
	fstream file;
	file.open("assets/Something_rediculous.vol");
	if(file.is_open()){
		file.read(ID, 5);
		if(ID=="mdvol"){
			file.ignore(5);//skips the version identifier and header length, cause we dont need them
			
			file.read(fullH, 4);
			file.read(fullW, 4);
			file.read(fullD, 4);//gets the dimensions of the area needed to propperly render the image

			file.read(voxelH, 4);
			file.read(voxelW, 4);
			file.read(voxelD, 4);//these are the sizes of the voxels, needed top calculate ratios to depancake the final model

			file.read(blackPoint, 4);
			file.read(whitePoint, 4);
			file.read(gamma, 4);//dont really know what these are for, but they may be useful

			file.read(storeType, 3);//very important string, use this to determine what to expect coming in

			if(storeType=="g08"){g08=true;}
			if(storeType=="g16"){g16=true;}
			if(storeType=="c24"){c24=true;}//set the propper flag for the type pf data stored to process propperly

			file.ignore(9951);//skipping past the rest of the header that only holds a description and title, who cares about those here? not me!

			//on to setting up the loops
			for(int z=0;z<fullD;z++){
				for(int y=0;y<fullW;y++){
					for(int x=0;y<fullH;y++){
						if(g08){
							file.read(rByte, 1);
							//TODO pass it to constructor
							Voxel vox=new Voxel(rByte);
							/*TODO holderName*/.put(vox, x,y,z);
						}
						if(g16){
							file.read(rByte, 1);
							file.ignore(1);//we wont need the second byte of the data, its just going to be devided out, so i wont even read it in, save some work
							//TODO pass it to constructor
							Voxel vox=new Voxel(rByte);
							/*TODO holderName*/.put(vox, x,y,z);
						}
						if(c24){
							file.read(rByte, 1);
							file.read(gByte, 1);
							file.read(bByte, 1);
							//TODO pass all 3 to a constructor
							Voxel vox=new Voxel(rByte, gByte, bByte);
							/*TODO holderName*/.put(vox, x,y,z);
						}
					}
				}
			}

		}
		else{cout<<"incorrect file type";}//TODO add an error processing system

	}
	//read first couple of data points and store them
	//set up loop aparatus depending on size of object
	//set up propper outputs to the constructor
	//start the loop and start processing
}