#include <iostream>
#include <fstream>
#include <cmath>
#include <ctime>
#include <cstdlib>



// We'll use this to load volume files, for the time being
#include <DGtal/helpers/StdDefs.h>
#include <DGtal/io/readers/GenericReader.h>
#include <DGtal/base/BasicFunctors.h>
#include <DGtal/images/ImageContainerBySTLMap.h>

#include <iostream>
#include <fstream>
#include <chrono>
#include <DGtal/helpers/StdDefs.h>
#include <DGtal/shapes/MeshVoxelizer.h>
#include <DGtal/kernel/sets/CDigitalSet.h>
#include <DGtal/kernel/domains/HyperRectDomain.h>
#include <DGtal/io/readers/MeshReader.h>
#include <DGtal/io/Display3D.h>
#include <DGtal/io/writers/GenericWriter.h>
#include <DGtal/images/ImageContainerBySTLVector.h>
#include <boost/program_options/options_description.hpp>
#include <boost/program_options/parsers.hpp>
#include <boost/program_options/variables_map.hpp>


#include "voxel.h"
#include "volume.h"
#include "PLYfileReader.h"
#include "vollyglm.h"


// I suppose this is necessary when you're interfacing with another volume library.
namespace volly {

	bool isAboveThreshold(Voxel vox, uint8_t threshold) {
		return ((vox.rgba.r > threshold) | (vox.rgba.b > threshold) | (vox.rgba.g > threshold)) & (vox.rgba.a > threshold);
	}



	// shamelessly adapted from https://github.com/DGtal-team/DGtalTools/blob/master/converters/mesh2vol.cpp
	VolumeStore<Voxel>* readOFFFile(std::string filename, int resolution, int coloringMode) {

		int margin = 0;
		using namespace std;
		using namespace DGtal;

		using Domain   = Z3i::Domain;
		using PointR3  = Z3i::RealPoint;
		using PointZ3  = Z3i::Point;

		std::cout << "Preparing the mesh" << std::endl;
		Mesh<PointR3> inputMesh;
		MeshReader<PointR3>::importOFFFile(("assets/"+filename+".off").c_str(), inputMesh);
		std::cout <<  " [done]" << std::endl;
		const std::pair<PointR3, PointR3> bbox = inputMesh.getBoundingBox();
		std::cout << "Mesh bounding box: "<<bbox.first <<" "<<bbox.second<<std::endl;

		const double smax = (bbox.second - bbox.first).max();
		const double factor = resolution / smax;
		const PointR3 translate = -bbox.first;
		std::cout <<  "Scale = "<<factor<<" translate = "<<translate<<std::endl;
		for(auto it = inputMesh.vertexBegin(), itend = inputMesh.vertexEnd();
			it != itend; ++it)
			{
			//scale + translation
			*it += translate;
			*it *= factor;
		}
		std::cout << std::endl;

		std::cout << "Voxelization" << std::endl;
		std::cout << "Voxelization " << 26 << "-separated ; " << resolution << "^3 ";
		Domain aDomain(PointZ3().diagonal(-margin), PointZ3().diagonal(resolution+margin));

		//Digitization step
		Z3i::DigitalSet mySet(aDomain);
		MeshVoxelizer<Z3i::DigitalSet, 26> aVoxelizer;
		aVoxelizer.voxelize(mySet, inputMesh, 1.0);
		std::cout <<  " [done] " << std::endl;
		std::cout << std::endl;

		std::cout << "Writing to VolumeStore" << std::endl;
		// Export the digital set to a vol file
		std::cout << aDomain<<std::endl;

		PointZ3 r = aDomain.upperBound();
		VolumeStore<Voxel>* myVol = new VolumeStore<Voxel>({resolution,resolution,resolution});
		

		for(auto p: mySet)
			if(myVol->inBounds({p[0],p[1],p[2]})) {
				Voxel v;
				if(coloringMode == 0) {
					v = Voxel::fromNormalizedFloats(p[0]/(float)r[0],p[1]/(float)r[1],p[2]/(float)r[2]);
				} else if(coloringMode == 1) {
					v = Voxel(rand()%256,rand()%256,rand()%256);
				} else if(coloringMode == 2) {
					v = Voxel::fromNormalizedFloats(p[0]/(float)r[0],p[1]/(float)r[1],p[2]/(float)r[2]);
					v.rgba.r ^= rand()%4;
					v.rgba.g ^= rand()%4;
					v.rgba.b ^= rand()%4;
				} else if(coloringMode == 3) {
					v = Voxel::fromNormalizedFloats(p[0]/(float)r[0],p[1]/(float)r[1],p[2]/(float)r[2]);
					v.rgba.r ^= rand()%24;
					v.rgba.g ^= rand()%24;
					v.rgba.b ^= rand()%24;
				}


				glm::vec3 norm({p[0],p[1],p[2]});
				norm = glm::vec3(myVol->size)/2.f - norm;
				norm = glm::normalize(norm) + 1.f;
				norm = norm*127.f;
				v.norm = glm::u8vec4(norm,0);


				myVol->put(v,p[0],p[1],p[2]);
			}
		std::cout << myVol->size.x << " " << myVol->size.y << " " << myVol->size.z << std::endl;
		return myVol;
	}


	


	//typedef DGtal::ImageContainerBySTLMap<DGtal::Z3i::Domain, uint8_t> Image3D;

/*
	// You heard it here, folks -- this is another, entirely seperate volume file format which ends its filenames in .vol
	// I sure hope somebody got fired for that blunder 
	void readSimpleVolFile(std::string filename, VolumeStore<Voxel>** myVol) {
		Image3D an3Dimage= DGtal::GenericReader<Image3D>::import(("assets/"+filename).c_str());

		Image3D::Domain d = an3Dimage.domain();

		Image3D::Point r = d.upperBound();
		Image3D::Point l = d.lowerBound();


		std::cout << l[0] << l[1] << l[2] << std::endl;
		std::cout << r[0] << r[1] << r[2] << std::endl;

		(*myVol) = new VolumeStore<Voxel>({r[0],r[1],r[2]});
		unsigned long long i=0, j=0;
		for(auto pt:d) {

			++i;


			uint8_t v = an3Dimage(pt);
			if(v>0 && (*myVol)->inBounds({pt[0],pt[1],pt[2]})) {
				(*myVol)->put(Voxel(255),pt[0],pt[1],pt[2]);
				++j;
			}
		}

		std::cout << i << " " << j << std::endl;
		
		/*for(int i = 0; i < viola.sizeX(); ++i) {
			for(int j = 0; j < viola.sizeY(); ++j) {
				for(int k = 0; k < viola.sizeZ(); ++k) {
					voxel g = viola.get(i,j,k);
					if(g>0) {
						(*myVol)->put(Voxel(255,255,255,g),i,j,k);
					}
				}
			}
		}*/

//	}




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


}

