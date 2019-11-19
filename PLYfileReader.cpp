//file to be read needs to be called loadable

#include <iostream>
#include <fstream>
#include <stack>
#include <cmath>
#include <cstdlib>
#include <set>
#include "triangleCube.c"
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

	void swapVec4(glm::vec4& a, glm::vec4& b) {
		glm::vec4 c = a;
		a = b;
		b = c;
	}

	glm::vec4 stepRayForward(glm::vec4 cur, glm::vec4 direction, glm::vec4 volZs) {
		cur.w = 0.5;
        direction.w = 0;
        glm::vec4 lineFitLength;
        glm::vec4 vec4Zeros(0);
        
        glm::vec4 directSign = glm::sign(direction);
        glm::vec4 oneOverDirection = glm::vec4(1)/direction;
        glm::vec4 heaviside  = glm::round((directSign + 1.f) / 2.f);

        // abritrarily small constant, don't read into it
        float eps = 0.00030;
        glm::vec4 directSignEpsilon = directSign*eps;
		       
		glm::vec4 curPlusEpsilon = cur+directSignEpsilon; // Ensure that things on a boundary will be rounded to the correct position.

		glm::vec4 distToNextGrid = glm::floor(curPlusEpsilon)+heaviside-cur; // If it's negative, we keep the floor. If not, we change this into a ceil.
		glm::vec4 normalizedDist = distToNextGrid*oneOverDirection;
		// Find horizontal minimum. This could potentially kill our performance on SIMD operations... 
		glm::vec4 minDistN(fmin(fmin(normalizedDist.x,normalizedDist.y),normalizedDist.z));
		cur = glm::fma(minDistN, direction, cur);

		curPlusEpsilon = cur+directSignEpsilon; // Ensure that things on a boundary will be rounded to the correct position.

		glm::ivec4 roundPos(curPlusEpsilon);
		return glm::vec4(roundPos);
	}

	template<typename T>
	void q_swap(T& A, T& B) {
		T t = A;
		A = B;
		B = t;
	}

	bool triangle_contains_voxel(glm::vec3 a, glm::vec3 b, glm::vec3 c, glm::vec3 point) {

	}

    std::map<glm::ivec4, Voxel, ivec4_cmp>* rasterizeVoxelMapFromPoly(Polyhedron* polyIn, int res) {
		normalizePoly(polyIn);
		std::map<glm::ivec4, Voxel, ivec4_cmp>* ret = new std::map<glm::ivec4, Voxel, ivec4_cmp>();

		float resF = res-1;
		glm::vec4 resV(resF);

		for(int i = 0; i < polyIn->inds.size(); i++) {
			glm::ivec3 g = polyIn->inds[i];

			glm::vec4 a = polyIn->verts[g.x] * resV;
			glm::vec4 b = polyIn->verts[g.y] * resV;
			glm::vec4 c = polyIn->verts[g.z] * resV;
			

			glm::vec3 norm = glm::cross(glm::vec3(b-a),glm::vec3(b-c));
			norm = glm::normalize(norm);
			norm = glm::clamp(norm,glm::vec3(0),glm::vec3(1.f));

			glm::u8vec4 normU8 = glm::u8vec4(norm.x * 255.f, norm.y * 255.f, norm.z * 255.f, 0);

			std::set<glm::ivec4, ivec4_cmp> already_checked;
			std::stack<glm::ivec4> to_check;

			for(auto k:{a,b,c}) {
				glm::ivec4 start = glm::ivec4(k);
				start.w = 0;

				to_check.push(start);
				already_checked.insert(start);
			}

			while(!to_check.empty()) {
				
				glm::ivec4 check_this = to_check.top();
				to_check.pop();

				glm::vec4 tri_offs(check_this);
				tri_offs += glm::vec4(0.5,0.5,0.5,0);
				glm::vec4 aO = (a - tri_offs);
				glm::vec4 bO = (b - tri_offs);
				glm::vec4 cO = (c - tri_offs);
				bool is_contained = t_c_intersection(
					Triangle3 {
						Point3{aO.x,aO.y,aO.z},
						Point3{bO.x,bO.y,bO.z},
						Point3{cO.x,cO.y,cO.z}
					}
				) == 0;
					
				
				if (is_contained) {


					glm::ivec3 toPlace(check_this.x,check_this.y,check_this.z);
					int LoD = 0;
					glm::vec3 col(toPlace);
					col /= glm::vec3(resV);
					col = glm::vec3(1);
					Voxel v = Voxel::fromNormalizedFloats(col.x,col.y,col.z,1);
					while(toPlace.x > 1) {
						if((toPlace.x < 0) | (toPlace.y < 0) | (toPlace.z < 0) | (toPlace.x >= res/(1<<LoD)) | (toPlace.y >= res/(1<<LoD)) | (toPlace.z >= res/(1<<LoD))) { 
							++LoD; toPlace /= 2; continue; 
						}
						v.norm = normU8;
						(*ret)[glm::ivec4(toPlace,LoD)] = v;
						toPlace /= 2; 
						++LoD;
					}


					glm::ivec4 neighbors[26] = {

						glm::ivec4(-1,-1,-1,0), glm::ivec4(-1,-1,0,0), glm::ivec4(-1,-1,1,0),
						glm::ivec4(-1,0,-1,0),  glm::ivec4(-1,0,0,0),  glm::ivec4(-1,0,1,0),
						glm::ivec4(-1,1,-1,0),  glm::ivec4(-1,1,0,0),  glm::ivec4(-1,1,1,0),

						glm::ivec4(0,-1,-1,0), glm::ivec4(0,-1,0,0), glm::ivec4(0,-1,1,0),
						glm::ivec4(0,0,-1,0),                        glm::ivec4(0,0,1,0),
						glm::ivec4(0,1,-1,0),  glm::ivec4(0,1,0,0),  glm::ivec4(0,1,1,0),

						glm::ivec4(1,-1,-1,0), glm::ivec4(1,-1,0,0), glm::ivec4(1,-1,1,0),
						glm::ivec4(1,0,-1,0),  glm::ivec4(1,0,0,0),  glm::ivec4(1,0,1,0),
						glm::ivec4(1,1,-1,0),  glm::ivec4(1,1,0,0),  glm::ivec4(1,1,1,0),
						
					};

					for (glm::ivec4 candidate:neighbors) {
						candidate = check_this + candidate;
						if (already_checked.count(candidate) == 0) {
							to_check.push(candidate);
							already_checked.insert(candidate);
						}

					}

				}

			}

		}

		return ret;
	}
	

}