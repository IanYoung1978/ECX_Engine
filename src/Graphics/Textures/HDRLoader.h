#pragma once
#include <string>
#include <GL/glew.h>

class HDRLoader
{
public:
    static unsigned int load(const std::string& filename);
};