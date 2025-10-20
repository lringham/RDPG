#include "Utils.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>


std::string loadTextFile(const std::string& filename)
{
    std::string line;
    std::string sourceCode;
    std::ifstream file(filename);

    if (file.is_open())
    {
        //load source code
        while (getline(file, line))
            sourceCode += line + '\n';
        file.close();
    }
    else
        std::cout << "Unable to load text file: " << filename << "\n";

    return sourceCode;
}

std::vector<char> loadBinaryFile(const std::string& filename, bool* loaded)
{
    std::vector<char> bytes;
    std::ifstream file(filename, std::ios::binary);
    if (file.is_open())
    {
        file.seekg(0, file.end);
        int length = int(file.tellg());
        file.seekg(0, file.beg);
        bytes.resize(length);
        file.read(&bytes[0], length);
        file.close();
        if (loaded != nullptr)
            *loaded = true;
    }
    else
        std::cout << "Unable to load binary file: " << filename << std::endl;
    return bytes;
}

void Utils::saveImage(const std::string& filename, int width, int height, unsigned char* pixels, int numColorComponents)
{
    if (!stbi_write_png(filename.c_str(), width, height, numColorComponents, pixels, 0))
        std::cout << "Unable to save image: " << filename << std::endl;
}

void Utils::loadImage(const std::string& filename, int* width, int* height, unsigned char** pixels, int* numColorComponents)
{
    stbi_set_flip_vertically_on_load(true);
    (*pixels) = stbi_load(filename.c_str(), width, height, numColorComponents, 0);
}

Image Utils::loadImage(const std::string& filename)
{
    stbi_set_flip_vertically_on_load(true);
    
    // Load texture
    int width = 0;
    int height = 0;
    unsigned char* pixels = 0;
    int numComponents = 0;
    loadImage(filename, &width, &height, &pixels, &numComponents);
    
    // Copy pixels
    auto num_colors = width * height * numComponents;
    std::vector<unsigned char> data(num_colors, 0);
    for (int i = 0; i < num_colors; ++i)
        data[i] = pixels[i];

    delete[] pixels;

    return Image {
        std::string(filename),
        width, height, numComponents,
        data
    };
}