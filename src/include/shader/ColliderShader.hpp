#pragma once

#include "BaseShader.hpp"

class ColliderShader : public BaseShader {
	public:
		ColliderShader() : BaseShader() {}
		~ColliderShader() {}
		/**
		 * @brief Loads the shader program
		 * @return a bool if loading worked or not 
		*/
		bool loadProgram() {
			GLint programSuccess = GL_TRUE;
			programID = glCreateProgram();

			// Vertex Shader
			GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
			const GLchar* vertSrc[7] = {
				"#version 410 core\n", 
				"layout (location = 0) in vec3 aPos;\n", 
				"layout (location = 1) in vec3 aColor;\n", 
				"out vec3 ourColor;\n", 
				"uniform mat4 view;\n", 
				"uniform mat4 projection;\n", 
				"void main() { gl_Position = projection * view * vec4(aPos, 1.0); ourColor = aColor; }"
			};
			glShaderSource(vertexShader, 7, vertSrc, NULL);
			glCompileShader(vertexShader);
			
			GLint status;
			glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &status);
			if(status != GL_TRUE){
				printShaderLog(vertexShader);
				throw std::logic_error("Unable to initialize ColliderShader");
			}
			glAttachShader(programID, vertexShader);

			// Fragment Shader
			GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
			const GLchar* fragSrc[4] = {
				"#version 410 core\n", 
				"in vec3 ourColor;\n", 
				"out vec4 FragColor;\n", 
				"void main() { FragColor = vec4(ourColor, 1.0); }"
			};
			glShaderSource(fragmentShader, 4, fragSrc, NULL);
			glCompileShader(fragmentShader);
			
			status = GL_TRUE;
			glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &status);
			if(status != GL_TRUE){
				printShaderLog(fragmentShader);
				throw std::logic_error("Unable to initialize ColliderShader");
			}
			glAttachShader(programID, fragmentShader);

			glLinkProgram(programID);	// Link
			
			glGetProgramiv(programID, GL_LINK_STATUS, &programSuccess);
			if(programSuccess != GL_TRUE){
				printProgramLog(programID);
				throw std::logic_error("Unable to initialize ColliderShader");
			}

			glDeleteShader(vertexShader);
			glDeleteShader(fragmentShader);

			// Prefetch uniform locations
			glUseProgram(programID);
			view = glGetUniformLocation(programID, "view");
			projection = glGetUniformLocation(programID, "projection");
			glUseProgram(0);

			return true;
		}
		void perspective(const glm::mat4& cameraView, const float& fov) {
			glm::mat4 model = glm::mat4(1.f);
			glm::mat4 view 	= glm::mat4(1.f);
			glm::mat4 projection = glm::mat4(1.f);

			model = glm::translate(model, pos);                     // Set position
			model = glm::scale(model, scale);						// Set scale
			model = glm::rotate(model, rotationRad, rotationAxis);	// Set rotation
			projection = glm::perspective(glm::radians(fov), 640.f / 480.f, 0.1f, 1000.f);
			view = cameraView;

			glUniformMatrix4fv(this->view, 1, GL_FALSE, glm::value_ptr(view));
			glUniformMatrix4fv(this->projection, 1, GL_FALSE, glm::value_ptr(projection));
			GLenum err = glGetError();
			if(err != GL_FALSE)
				throw std::runtime_error("Unable to set uniforms, err: " + std::to_string(err));
			glUseProgram(0);
		}
	protected:
		GLint view;
		GLint projection;
};