//file to be read needs to be called loadable

#include <iostream>
#include <fstream>
#include <glm/glm.hpp>
#include <vector>
#include <stack>
#include <string>
using namespace std;

int stringToInt(string);
void readIn();
glm::vec4 stringToVec4(string);
glm::Ivec4 stringToIVec4(string);

struct PolygonObject{
	std::vector<glm::vec3> verts;
	std::vector<glm::ivec3> inds;
};

int main(){
	cout<<"yeah, it works";
	return 0;
}

PolygonObject readIn(){
	int numVert;
	int numFaces;

	bool header=true;
	string inbound;
	PolygonObject product;


	fstream file;
	file.open("assets/loadable")
	if(file.is_open()){
		while(header){
			getline(file,inbound);
			if(inbound[0]=='e'){//everything i need to extract from the headed starts with e, how convenient
				if(inbound=="end_header"){//testing for end_header
					header=false;
				}
				else{
					if(inbound.subsrt(0,7)=="element"){
						if(inbound.substr(7, 8)==" vertex "){numVert=stringToInt(inbound)}
						else(inbound.substr(7, 6)==" face "){numFaces=stringToInt(inbound)}
					}
				}
			}
		}

		for(int i=0;i<numVert;i++){//loops through and creates a vector of vertecies
			vec4 holdMe;
			getline(file, inbound);
			holdMe=stringToVec4(inbound);
			product.verts.push(holdMe);
		}

		for(int i=0;i<numFaces;i++){//loops through and creats a vector of faces
			Ivec4 aLine;
			getLine(file, inbound);
			aLine=stringToIvec4(inbound);
			product.inds.push(aLine);
		}
		file.close();
	}
	else{cout<<"Failed to find file to load.\nPlease make sure the file is named \"loadable\" and is in the \"assets\" folder";}
	return product;
}

int stringToInt(string inbound){
	int length=inbound.length();
	stack <int> s;
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

glm::vec4 stringToVec4(string inbound){
	int spacePos;
	string holder1, holder2, holder3;
	float X, Y, Z;
	vec4 Loc;

	spacePos=inbound.find(' ');
	holder1=inbound.substr(0, spacePos);
	holder2=inbound.substr(spacePos+1);

	spacePos=holder2.find(' ');
	holder3=holder2.substr(spacePos+1);
	holder2=holed2.substr(0, spacePos);

	X=stof(holder1);
	Y=stof(holder2);
	Z=stof(holder3);

	Loc.x=X;
	Loc.y=Y;
	Loc.z=Z;

	return Loc;
}

glm::Ivec4 stringToIVec4(string inbound){
	int spacePos;
	string holder1, holder2, holder3;
	int X, Y, Z;
	vec4 Loc;

	spacePos=inbound.find(' ');
	inbound.erase(0, spacePos);

	spacePos=inbound.find(' ');
	holder1=inbound.substr(0, spacePos);
	holder2=inbound.substr(spacePos+1);

	spacePos=holder2.find(' ');
	holder3=holder2.substr(spacePos+1);
	holder2=holed2.substr(0, spacePos);

	X=stringToInt(holder1);
	Y=stringToInt(holder2);
	Z=stringToInt(holder3);

	Loc.x=X;
	Loc.y=Y;
	Loc.z=Z;

	return Loc;
}