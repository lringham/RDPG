#include "ObjModel.h"


void saveObj(char * filename, const Drawable& mesh)
{
	std::ofstream file;
	file.open(filename);
	if (file.is_open())
	{
		for (unsigned i = 0; i < mesh.positions_.size(); ++i)
		{
			file << "v ";
			file << mesh.positions_.at(i).x << " ";
			file << mesh.positions_.at(i).y << " ";
			file << mesh.positions_.at(i).z << "\n";
		}

		for (unsigned i = 0; i < mesh.normals_.size(); ++i)
		{
			file << "vn ";
			file << mesh.normals_.at(i).x << " ";
			file << mesh.normals_.at(i).y << " ";
			file << mesh.normals_.at(i).z << "\n";
		}

		for (unsigned i = 0; i < mesh.textureCoords_.size(); ++i)
		{
			file << "vt ";
			file << mesh.textureCoords_.at(i).x_ << " ";
			file << mesh.textureCoords_.at(i).y_ << "\n";
		}

		for (unsigned i = 0; i < mesh.indices_.size() - 3; i += 3)
		{
			file << "f ";

			unsigned j = mesh.indices_.at(i) + 1;
			file << j << "/"; file << j << "/"; file << j << " ";

			j = mesh.indices_.at(i + 1) + 1;
			file << j << "/"; file << j << "/"; file << j << " ";

			j = mesh.indices_.at(i + 2) + 1;
			file << j << "/"; file << j << "/"; file << j << "\n";
		}
	}
	file.close();
}
