//file to be read needs to be called loadable

#include <iostream>
#include <fstream>
#include <stack>
#include <cmath>
#include <cstdlib>
#include "PLYfileReader.h"



namespace volly {
	Polyhedron* readPLYFile(std::string filename){
		int numVert;
		int numFaces;

		bool header=true;
		std::string inbound;
		Polyhedron* product = new Polyhedron{};


		std::fstream file;
		file.open("assets/" + filename + ".ply");
		if(file.is_open()){
			while(header){
				getline(file,inbound);
				if(inbound[0]=='e'){//everything i need to extract from the headed starts with e, how convenient
					if(inbound=="end_header"){//testing for end_header
						header=false;
					}
					else{
						if(inbound.substr(0,7)=="element"){
							if(inbound.substr(7, 8)==" vertex "){numVert=stringToInt(inbound);}
							else if(inbound.substr(7, 6)==" face "){numFaces=stringToInt(inbound);}
						}
					}
				}
			}

			for(int i=0;i<numVert;i++){//loops through and creates a vector of vertecies
				glm::vec4 holdMe;
				std::getline(file, inbound);
				holdMe=stringToVec4(inbound);
				product->verts.push_back(holdMe);
			}

			for(int i=0;i<numFaces;i++){//loops through and creats a vector of faces
				glm::ivec3 aLine;
				std::getline(file, inbound);
				aLine=stringToivec4(inbound);
				product->inds.push_back(aLine);
			}
			file.close();
		}
		else {
			std::cout<<"Failed to find file to load.\nPlease make sure the file is in the \"assets\" folder" << std::endl;
		}
		return product;
	}

	int stringToInt(std::string inbound){
		int length=inbound.length();
		std::stack <int> s;
		int answer=0;
		int multiplier=1;
		int num=0;

		//this loop scans through the inbound string and scans for any digits, then crops them out as a character to cheat them into being an int, then pushes it into a stack a digit at a time
		for(int i=0; i<length; i++){
			num=inbound[i];
			if((num<58)&&(num>47)){
				num-=48;
				s.push(num);
			}
		}

		//turns the stack into a number. multiplier is to move a 10s place
		while(!s.empty()){
			answer+=(s.top()*multiplier);
			s.pop();
			multiplier *= 10;
		}
		return answer;
	}

	glm::vec4 stringToVec4(std::string inbound){
		int spacePos;
		std::string holder1, holder2, holder3;
		float X, Y, Z;
		glm::vec4 Loc;

		spacePos=inbound.find(' ');
		holder1=inbound.substr(0, spacePos);
		holder2=inbound.substr(spacePos+1);

		spacePos=holder2.find(' ');
		holder3=holder2.substr(spacePos+1);
		holder2=holder2.substr(0, spacePos);

		X=stof(holder1);
		Y=stof(holder2);
		Z=stof(holder3);

		Loc.x=X;
		Loc.y=Y;
		Loc.z=Z;

		return Loc;
	}

	glm::ivec4 stringToivec4(std::string inbound){
		int spacePos;
		std::string holder1, holder2, holder3;
		int X, Y, Z;
		glm::ivec4 Loc;

		spacePos=inbound.find(' ');
		inbound = inbound.erase(0, spacePos+1);

		spacePos=inbound.find(' ');
		holder1=inbound.substr(0, spacePos);
		holder2=inbound.substr(spacePos+1);

		spacePos=holder2.find(' ');
		holder3=holder2.substr(spacePos+1);
		holder2=holder2.substr(0, spacePos);

		X=stringToInt(holder1);
		Y=stringToInt(holder2);
		Z=stringToInt(holder3);

		Loc.x=X;
		Loc.y=Y;
		Loc.z=Z;

		return Loc;
	}




	void normalizePoly(Polyhedron* polyIn) {
		
		glm::vec4 min( 100000000000.f);
		glm::vec4 max(-100000000000.f);

		for(int i = 0; i < polyIn->verts.size(); i++) {
			glm::vec4 g = polyIn->verts[i];
			min.x = fmin(min.x, g.x);
			min.y = fmin(min.y, g.y);
			min.z = fmin(min.z, g.z);
			max.x = fmax(max.x, g.x);
			max.y = fmax(max.y, g.y);
			max.z = fmax(max.z, g.z);
		}

		glm::vec4 mul = glm::vec4(1)/(max-min);
		glm::vec4 add = -min*mul;

		mul.w = 0;
		add.w = 0;

		for(int i = 0; i < polyIn->verts.size(); i++) {
			glm::vec4 g = polyIn->verts[i];
			g = glm::fma(g,mul,add);
			g = glm::clamp(g,glm::vec4(0),glm::vec4(1));
			polyIn->verts[i] = g;
		}
	}

	
    std::map<glm::ivec3, Voxel, ivec3_cmp>* rasterizeVoxelMapFromPoly(Polyhedron* polyIn, int res) {
		normalizePoly(polyIn);
		std::map<glm::ivec3, Voxel, ivec3_cmp>* ret = new std::map<glm::ivec3, Voxel, ivec3_cmp>();

		float resF = res-1;
		glm::vec4 resV(resF-1);

		ret->emplace(glm::ivec3(0,0,0), Voxel::fromNormalizedFloats(1,1,1));

		for(int i = 0; i < polyIn->inds.size(); i++) {
			glm::ivec3 g = polyIn->inds[i];

			glm::vec4 a = polyIn->verts[g.x];
			glm::vec4 b = polyIn->verts[g.y];
			glm::vec4 c = polyIn->verts[g.z];

			glm::vec4 aS = a*resV;
			glm::vec4 bS = b*resV;
			glm::vec4 cS = c*resV;

			glm::vec3 norm = glm::cross(glm::vec3(b-a),glm::vec3(b-c));
			norm = glm::normalize(norm);
			norm = glm::clamp(norm,glm::vec3(0),glm::vec3(1.f));

			glm::u8vec4 normU8 = glm::u8vec4(norm.x * 255.f, norm.y * 255.f, norm.z * 255.f, 0);

			Voxel v = Voxel::fromNormalizedFloats(a.r,a.g,a.b,1);
			v.norm = normU8;
			(*ret)[glm::ivec3(aS)] = v;
		}
		return ret;
	}
}