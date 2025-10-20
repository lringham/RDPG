#pragma once
#include "Vec3.h"
#include "Color.h"
#include "Log.h"
#include "Mesh.h"

#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include <sstream>


class Ply : 
	public Mesh
{
	struct PlyProperty
	{
		std::string typeStr_;
		std::string name_;
	};

	struct PlyElement
	{
		std::string typeStr_;
		std::size_t count_ = 0;
		std::vector<PlyProperty> properties_;
	};

	std::vector<PlyElement> elements_;
	std::tuple<std::string, int> format_;
	std::string errorMessage_ = "No Error";
	bool valid_ = false;

	bool setValidState(bool validSate, const std::string& message)
	{
		LOG(message);
		errorMessage_ = message;
		valid_ = validSate;
		return valid_;
	}

public:
	Ply(const std::string& filename)
	{
		valid_ = loadPly(filename);
	}

	bool loadPly(const std::string& filename)
	{
		valid_ = false;
		errorMessage_ = "No Error";

		std::ifstream plyFile(filename);
		if (plyFile.is_open())
		{
			// Test for ply str at start of file
			std::string line;
			if (!std::getline(plyFile, line) || line != "ply")
				return setValidState(false, "Ply file is invalid");

			// Read header
			std::string temp;
			while (std::getline(plyFile, line))
			{
				std::stringstream lineStream(line);
				std::vector<std::string> splitLine;
				while (lineStream >> temp)
					splitLine.push_back(temp);

				// Skip empty lines and comments
				if (splitLine.size() == 0 || splitLine[0] == "comment") 
					continue;

				// End of header seen
				std::string heading = splitLine[0];
				if (heading == "end_header")
					break;

				// Parse format line
				else if (heading == "format")
				{
					if (splitLine.size() < 3)
						return setValidState(false, "Invalid format: [" + line + "]");

					std::get<0>(format_) = splitLine[1];
					std::get<1>(format_) = std::atoi(splitLine[2].c_str());
					
					if (std::get<0>(format_).find("ascii") == std::string::npos)
						return setValidState(false, "Non-ascii ply files are not supported: [" + line + "]");
				}

				// Parse element line
				else if (heading == "element")
				{
					if (splitLine.size() < 3)
						return setValidState(false, "Invalid element: [" + line + "]");

					PlyElement element;
					element.typeStr_ = splitLine[1];
					element.count_ = std::atoi(splitLine[2].c_str());
					elements_.push_back(element);
				}

				// Parse property line
				else if (heading == "property")
				{
					if (splitLine.size() < 3)
						return setValidState(false, "Invalid property: [" + line + "]");
					else if (elements_.size() == 0)
						return setValidState(false, "No element specified: [" + line + "]");

					PlyProperty property;
					property.name_ = splitLine[splitLine.size() - 1];
					property.typeStr_ = line.substr(heading.size() + 1, line.size() - (heading.size() + 1 + property.name_.size() + 1));
					elements_[elements_.size()-1].properties_.push_back(property);
				}

			}

			// Read data
			if (std::get<0>(format_).find("ascii") != std::string::npos)
			{
				for (PlyElement& element : elements_)
				{
					for (size_t i = 0; i < element.count_; ++i)
					{
						if (!std::getline(plyFile, line))
							return setValidState(false, "Invalid amount of data in Ply");

						std::stringstream lineStream(line);
						std::vector<std::string> splitLine;
						while (lineStream >> temp)
							splitLine.push_back(temp);

						if (element.typeStr_ == "vertex")
						{
							Vec3 position, normal;
							Color color;
							Vec2 uv;

							int j = 0;
							for (PlyProperty& property : element.properties_)
							{
								std::string str = splitLine[j++].c_str();
								if (property.name_ == "x")
									position.x = std::stof(str);
								else if (property.name_ == "y")
									position.y = std::stof(str);
								else if (property.name_ == "z")
									position.z = std::stof(str);

								else if (property.name_ == "nx")
									normal.x = std::stof(str);
								else if (property.name_ == "ny")
									normal.y = std::stof(str);
								else if (property.name_ == "nz")
									normal.z = std::stof(str);

								else if (property.name_ == "s")
									uv.u_ = std::stof(str);
								else if (property.name_ == "t")
									uv.v_ = std::stof(str);

								else if (property.name_ == "red")
									color.r = static_cast<unsigned char>(std::stoul(str));
								else if (property.name_ == "green")
									color.g = static_cast<unsigned char>(std::stoul(str));
								else if (property.name_ == "blue")
									color.b = static_cast<unsigned char>(std::stoul(str));
								else if (property.name_ == "alpha")
									color.a = static_cast<unsigned char>(std::stoul(str));
							}

							positions_.push_back(position);
							normals_.push_back(normal);
							colors_.push_back(color.vec4());
							textureCoords_.push_back(uv);
						}
						else if (element.typeStr_ == "face")
						{
							size_t count = std::atoi(splitLine[0].c_str());
							size_t j = 0;
							while (++j <= count)
								indices_.push_back(std::atoi(splitLine[j].c_str()));
						}
					}
				}
			}
			plyFile.close();
			valid_ = true;
			destroyVBOs();
			initVBOs();
		}
		else
			setValidState(false, "Failed to open: " + filename);
		
		return valid_;
	}

	bool isLoaded() const 
	{
		return valid_;
	}
};

