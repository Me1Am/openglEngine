#pragma once

#include <GL/glew.h>
#include <GL/glu.h>
#include <SOIL/SOIL.h>

#include <iostream>
#include <fstream>
#include <string>

class FileHandler {
    public:
        /** Load shader file from a given path
		 * @param path The path to the shader file
		 * @return A null-terminated string representing the source
		*/
		static std::string getShaderFromFile(const std::string path) {
			std::ifstream fileStream(path, std::ios::in);	// Create a filestream to the given path
			std::string source;	// Resulting file data

			if(!fileStream.is_open()) {
				std::cerr << "Could not read file: \"" << path << "\"" << std::endl;
				return NULL;
			}

			std::string line = "";	// Current line
			while(!fileStream.eof()) {
				std::getline(fileStream, line);	// Load into 'line'
				source.append(line + "\n");
			}

			source.append("\0");	// Make null-terminated
			fileStream.close();

			return source;	// Return the source string
		}

		/** Compile and return a shader from the given path
		 * @param path The path to the shader file
		 * @return A GLuint representing the compiled shader
		 * @note REQUIRES that the given path uses the following file extensions
		 * @note .vert - a vertex shader
		 * @note .tesc - a tessellation control shader
		 * @note .tese - a tessellation evaluation shader
		 * @note .geom - a geometry shader
		 * @note .frag - a fragment shader
		 * @note .comp - a compute shader
		*/
		static GLuint compileShader(const std::string path) {
			GLuint shader;
			// Determine shader type
			if(path.substr(path.length()-4, 4).compare("vert") == 0){
				shader = glCreateShader(GL_VERTEX_SHADER);
			} else if(path.substr(path.length()-4, 4).compare("tesc") == 0) {
				shader = glCreateShader(GL_TESS_CONTROL_SHADER);
			} else if(path.substr(path.length()-4, 4).compare("tese") == 0) {
				shader = glCreateShader(GL_TESS_EVALUATION_SHADER);
			} else if(path.substr(path.length()-4, 4).compare("geom") == 0) {
				shader = glCreateShader(GL_GEOMETRY_SHADER);
			} else if(path.substr(path.length()-4, 4).compare("frag") == 0) {
				shader = glCreateShader(GL_FRAGMENT_SHADER);
			} else if(path.substr(path.length()-4, 4).compare("comp") == 0) {
				shader = glCreateShader(GL_COMPUTE_SHADER);
			} else {
				throw std::logic_error("FileHandler::compileShader(): Unknown shader file extension");
				return -1;
			}

			// Load and convert the shader file into a const GLchar**
			std::string shaderSourceStr = FileHandler::getShaderFromFile(path);	// Load shader
			const GLchar* shaderSourceArr[] = {shaderSourceStr.c_str()};	// Convert to GLchar array

			// Compile
			glShaderSource(shader, 1, shaderSourceArr, NULL);
			glCompileShader(shader);
			
			// Error check
			GLint status;
			glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
			if(status != GL_TRUE){
				GLint length;	// Length of info buffer
				glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);	// Get length

				std::string log(length, ' ');	// Create string
				glGetShaderInfoLog(shader, length, &length, &log[0]);	// Write into string
				throw std::logic_error(log);	// Throw logic error
				return 0;
			}

			return shader;
		}

		/** Load Image Into OpenGL Texture
		 * @param path The path to the image file
		 * @return A bool representing if the operation was successful
		 * @note Must create and bind a texture first
		 * @note Auto-detects the image format
		*/
		static bool loadImage(const std::string path) {
			int width, height, channels;
			
			unsigned char* image = SOIL_load_image(path.c_str(), &width, &height, &channels, SOIL_LOAD_AUTO);
			if(!image){
				std::cout << "FileHandler::loadImage(): Failed to load image" << std::endl;
				return false;
			}

			// Autodetect format
			GLenum format;
			if(channels == 1)
				format = GL_RED;
			else if(channels == 3)
				format = GL_RGB;
			else if(channels == 4)
				format = GL_RGBA;

			glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, image);
			//glGenerateMipmap(GL_TEXTURE_2D);

			// Texture wrapping parameters
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);	// Map to X(S)
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);	// Map to Y(T)
			// Filtering parameters
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);		// Nearest pixel match for downscale
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);		// Bileanar for upscale

			SOIL_free_image_data(image);
			
			return true;
		}

		/** Load Image Into OpenGL Texture
		 * @param path The path to the image file
		 * @param format The image format
		 * @return A bool representing if the operation was successful
		 * @note Must create and bind a texture first
		*/
		static bool loadImage(const std::string path, const GLuint format) {
			int width, height, channels;
			
			unsigned char* image = SOIL_load_image(path.c_str(), &width, &height, &channels, SOIL_LOAD_AUTO);
			if(!image){
				std::cout << "FileHandler::loadImage(): Failed to load image" << std::endl;
				return false;
			}

			glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, image);
			//glGenerateMipmap(GL_TEXTURE_2D);

			// Texture wrapping parameters
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);	// Map to X(S)
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);	// Map to Y(T)
			// Filtering parameters
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);		// Nearest pixel match for downscale
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);		// Bileanar for upscale

			SOIL_free_image_data(image);
			
			return true;
		}
};