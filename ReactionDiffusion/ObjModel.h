#pragma once
#include "Drawable.h"
#include "Mesh.h"
#include "Vec3.h"
#include "Vec2.h"

#include <fstream>
#include <stdlib.h>
#include <vector>
#include <iostream>
#include <string>


#ifdef DEBUG
	#include <errno.h>
	#define checkErrno if (errno != 0) std::cout << "Error parsing obj (Ln " << __LINE__ << ")" << std::endl
	#define checkEndString(endPtr) if(*endPtr == '\0') std::cout << "At end of string! (Ln " << __LINE__ << ")" << std::endl
    // TODO: Add parse check...
#else
	#define checkErrno
	#define checkEndString
#endif

class ObjModel : 
	public Mesh
{
private:
	enum Type { V = 0, VT = 3, VTN = 6, VN, F, NONE};
	struct Face
	{
		unsigned v0, t0, n0, 
				 v1, t1, n1, 
				 v2, t2, n2;
	};

	std::vector<Face> faces;
	Type compType = NONE;
	void parseLine(char* line, std::vector<Vec3>& parsedPositions, std::vector<Vec3>& parsedNormals, std::vector<Vec2>& parsedTextureCoords)
	{		
		if(line == nullptr)
			return;
		
		//Determine the line type
		Type lineType = NONE;
		int offset = 0;
		switch(line[0])
		{			
			case 'f': //face
				offset = 2;
				lineType = F;
				break;
			case 'v': //vertex	
				switch(line[1])
				{	
				case ' ': // position
					offset = 2;
					lineType = V;
					break;
				case 'n': // normal
					offset = 3;
					lineType = VN;
					break;
				case 't': // texture coordinate
					offset = 3;
					lineType = VT;
					break;
				}
				break;
		}

		//Convert Line Values
		switch (lineType)
		{
			case F: //Parse a face
			{
				//replace all slashes with spaces to allow for conversion				
				char c = ' ';
				for (int i = offset; c != '\0'; ++i)
				{
					c = line[i];
					if (c == '/')
						line[i] = ' ';
				}

				// parse face indices_
				Face f;
				char* extra;
				switch (compType)
				{
				case V:
					f.v0 = strtol(line + offset, &extra, 0) - 1; checkErrno;
					f.v1 = strtol(extra, &extra, 0) - 1; checkErrno;
					f.v2 = strtol(extra, nullptr, 0) - 1; checkErrno; 
					break;
				case VT:
					f.v0 = strtol(line + offset, &extra, 0) - 1; checkErrno;
					f.t0 = strtol(extra, &extra, 0) - 1; checkErrno;

					f.v1 = strtol(extra, &extra, 0) - 1; checkErrno;
					f.t1 = strtol(extra, &extra, 0) - 1; checkErrno;

					f.v2 = strtol(extra, &extra, 0) - 1; checkErrno;
					f.t2 = strtol(extra, nullptr, 0) - 1; checkErrno;
					break;
				case VN:
					f.v0 = strtol(line + offset, &extra, 0) - 1; checkErrno;
					f.n0 = strtol(extra, &extra, 0) - 1; checkErrno;

					f.v1 = strtol(extra, &extra, 0) - 1; checkErrno;
					f.n1 = strtol(extra, &extra, 0) - 1; checkErrno;

					f.v2 = strtol(extra, &extra, 0) - 1; checkErrno;
					f.n2 = strtol(extra, nullptr, 0) - 1; checkErrno;
					break;
				case VTN:
					f.v0 = strtol(line + offset, &extra, 0) - 1; checkErrno;
					f.t0 = strtol(extra, &extra, 0) - 1; checkErrno;
					f.n0 = strtol(extra, &extra, 0) - 1; checkErrno;

					f.v1 = strtol(extra, &extra, 0) - 1; checkErrno;
					f.t1 = strtol(extra, &extra, 0) - 1; checkErrno;
					f.n1 = strtol(extra, &extra, 0) - 1; checkErrno;

					f.v2 = strtol(extra, &extra, 0) - 1; checkErrno;
					f.t2 = strtol(extra, &extra, 0) - 1; checkErrno;
					f.n2 = strtol(extra, nullptr, 0) - 1; checkErrno;
					break;
				case F: break;
				case NONE: break;
				}
				faces.push_back(f);
				break;
			}
			case V: //Parse a position
			{
				char* extra;
				float x = strtof(line + offset, &extra); checkErrno;
				float y = strtof(extra, &extra); checkErrno;
				float z = strtof(extra, nullptr); checkErrno;
				parsedPositions.emplace_back(x, y, z);
				break;
			}
			case VN: //Parse a normal
			{
				char* extra;
				float x = strtof(line + offset, &extra); checkErrno;
				float y = strtof(extra, &extra); checkErrno;
				float z = strtof(extra, nullptr); checkErrno;
				parsedNormals.emplace_back(x, y, z);
				break;
			}
			case VT:  //Parse a texture coordinate
			{
				char* extra;
				float x = strtof(line + offset, &extra); checkErrno;
				float y = strtof(extra, nullptr); checkErrno;
				parsedTextureCoords.emplace_back(x, y);
				break;
			}
			case VTN: break;
			case NONE: break;
		}
	}

	void parseFaces(std::vector<Vec3>& parsedPositions, std::vector<Vec3>& parsedNormals, std::vector<Vec2>& parsedTextureCoords)
	{
		//set all the indices_ of the triangles
		positions_.resize(parsedPositions.size());
		switch (compType)
		{
		case VTN:			
			normals_.resize(parsedPositions.size());
			textureCoords_.resize(parsedPositions.size());
			for (Face& face : faces)
			{
				indices_.push_back(face.v0);
				indices_.push_back(face.v1);
				indices_.push_back(face.v2);

				positions_[face.v0] = parsedPositions[face.v0];
				positions_[face.v1] = parsedPositions[face.v1];
				positions_[face.v2] = parsedPositions[face.v2];

				normals_[face.v0] = parsedNormals[face.n0];
				normals_[face.v1] = parsedNormals[face.n1];
				normals_[face.v2] = parsedNormals[face.n2];

				textureCoords_[face.v0] = parsedTextureCoords[face.t0];
				textureCoords_[face.v1] = parsedTextureCoords[face.t1];
				textureCoords_[face.v2] = parsedTextureCoords[face.t2];
			}
			break;
		case VN:
			normals_.resize(parsedPositions.size());
			for (Face& face : faces)
			{
				indices_.push_back(face.v0);
				indices_.push_back(face.v1);
				indices_.push_back(face.v2);

				positions_[face.v0] = parsedPositions[face.v0];
				positions_[face.v1] = parsedPositions[face.v1];
				positions_[face.v2] = parsedPositions[face.v2];

				normals_[face.v0] = parsedNormals[face.n0];
				normals_[face.v1] = parsedNormals[face.n1];
				normals_[face.v2] = parsedNormals[face.n2];
			}
			break;
		case VT:
			textureCoords_.resize(parsedPositions.size());
			for (Face& face : faces)
			{
				indices_.push_back(face.v0);
				indices_.push_back(face.v1);
				indices_.push_back(face.v2);

				positions_[face.v0] = parsedPositions[face.v0];
				positions_[face.v1] = parsedPositions[face.v1];
				positions_[face.v2] = parsedPositions[face.v2];

				textureCoords_[face.v0] = parsedTextureCoords[face.t0];
				textureCoords_[face.v1] = parsedTextureCoords[face.t1];
				textureCoords_[face.v2] = parsedTextureCoords[face.t2];
			}
			break;
		case V:
			for (Face& face : faces)
			{
				indices_.push_back(face.v0);
				indices_.push_back(face.v1);
				indices_.push_back(face.v2);

				positions_[face.v0] = parsedPositions[face.v0];
				positions_[face.v1] = parsedPositions[face.v1];
				positions_[face.v2] = parsedPositions[face.v2];
			}
			break;
		case F: break;
		case NONE: break;
		}	
		/*file.close();*/
	}

	bool loaded = false;

public:
	ObjModel() = default;
	
	ObjModel(const std::string& filename)
	{
		loadModel(filename);
		initVBOs();
	}

	~ObjModel() = default;

	bool isLoaded()
	{
		return loaded;
	}

	void printModelInfo(const std::string& filename)
	{
		size_t pos = filename.find_last_of("/\\");
		if (pos != filename.size())
			std::cout << filename.substr(pos + 1);
		else
			std::cout << filename;

		std::cout << " Info" << std::endl;
		std::cout << "--------------------------" << std::endl;
		std::cout << " Vertices " << positions_.size() << std::endl;
		std::cout << " Faces " << faces.size() << std::endl;
	}

	bool loadModel(std::string filename)
	{
		std::ifstream reader(filename);
		if(reader.is_open())
		{
			std::vector<Vec3> newPositions;
			std::vector<Vec3> newNormals;
			std::vector<Vec2> newTextureCoords;

			char line[256];
			while (reader.getline(line, 256))
			{
				if (compType != NONE)
					parseLine(line, newPositions, newNormals, newTextureCoords);
				else //determine component type
				{
					// -----------------------------------------
					// If componentType is not found, 
					// and the line defines a face,
					// use the '/' count and placement 
					// to determine how to parse face lines.
					// componentType designates if normals_ and or 
					// texture coordinates are avaliable
					// -----------------------------------------
					if (line[0] == 'f')
					{
						int slashCount = 0;
						char c = ' ';
						for (int i = 2; c != '\0' && compType == NONE; ++i)
						{
							c = line[i];
							if (c == '/')
							{
								if (line[i + 1] == '/') //Check for double slash denoting v//n
									compType = VN;
								slashCount++;
							}
						}
						if(compType != VN)
							compType = (Type)slashCount;
					}
					parseLine(line, newPositions, newNormals, newTextureCoords);
				}
			}
			reader.close();
			parseFaces(newPositions, newNormals, newTextureCoords);
			if (textureCoords_.size() == 0)
				std::cout << "Warning: No texture coordinates associated with model.\n";
			loaded = true;
			return true;
		}
		std::cout << "Can't load model: " << filename << std::endl;
		return false;
	}
};

void saveObj(char* filename, const Drawable& mesh);