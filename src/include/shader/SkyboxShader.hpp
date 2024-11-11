#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <string>
#include <vector>

#include "CubeShader.hpp"
#include "../FileHandler.hpp"
#include "../Camera.hpp"

/**
 * @brief Shader program which holds a textured quad
 * @extends Shader
*/
class SkyboxShader : public CubeShader {
	public:
		/**
		 * @brief Default constructor
		 * @brief Sets programID to NULL
		*/
		SkyboxShader(const std::vector<std::string> faces) : CubeShader() {
			glGenTextures(1, &texture);
			glBindTexture(GL_TEXTURE_CUBE_MAP, texture);

			// Load cubemap
			int width;
			int height;
			int channels;
			for(GLuint i = 0; i < faces.size(); i++) {
				unsigned char *data = FileHandler::getRawImage(faces[i].c_str(), width, height, channels);
				if(channels != 3){
					std::cerr << "SkyboxShader::loadCubemap(): Failed to load texture at path \"" << faces[i] << "\". Incorrect format\n";
					FileHandler::freeImage(data);
				} else if(data){
					glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
					FileHandler::freeImage(data);
				} else {
					std::cerr << "SkyboxShader::loadCubemap(): Failed to load texture at path \"" << faces[i] << "\"\n";
					FileHandler::freeImage(data);
				}
			}
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
		}
	private:
		GLuint texture;
};