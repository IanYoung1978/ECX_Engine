#include "HDRLoader.h"
#include <stb_image.h>
#include "Logging/ECX_Logging.h"

unsigned int HDRLoader::load(const std::string& filename)
{
    stbi_set_flip_vertically_on_load(true);

    int width, height, channels;
    float* data = stbi_loadf(filename.c_str(), &width, &height, &channels, 0);

    if (!data) {
        LOGGING::ECX_Logger::GetInstance()->LogMessage(
            "Failed to load HDR image: " + filename + " — " + stbi_failure_reason(),
            LOGGING::LogLevel::SEVERE
        );
        return 0;
    }

    unsigned int hdrTexture = 0;
    glGenTextures(1, &hdrTexture);
    glBindTexture(GL_TEXTURE_2D, hdrTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGB, GL_FLOAT, data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    stbi_image_free(data);

    LOGGING::ECX_Logger::GetInstance()->LogMessage(
        "HDR image loaded: " + filename + " (" +
        std::to_string(width) + "x" + std::to_string(height) + ")",
        LOGGING::LogLevel::INFORMATION
    );

    return hdrTexture;
}