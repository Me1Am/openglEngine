#pragma once

#include <GL/glew.h>
#include <GL/glu.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <string>
#include <vector>

#include "BaseShader.hpp"

/**
 * @brief Properties for drawing a simple shape
*/
struct ShapeProperties {
	float angle = 0.f;	// In radians
	float alpha = 1.f;	// Alpha color value, 0-1 inclusive
	
	glm::vec3 pos = glm::vec3(0.f, 0.f, 0.f);
	glm::vec3 scale = glm::vec3(1.f, 1.f, 1.f);
	glm::vec3 axis = glm::vec3(0.f, 1.f, 0.f);	// Axis of `angle`
	glm::vec3 color = glm::vec3(0.f, 1.f, 0.f);
};

struct Shader {
	GLuint programID;
	GLuint view;
	GLuint projection;
	GLuint model;
	GLuint color;

	~Shader() { glDeleteProgram(programID); }
};

struct ModelShader : public Shader {
	~ModelShader() {
		glDeleteVertexArrays(1, &vao);
		glDeleteBuffers(1, &vbo);
		glDeleteBuffers(1, &ebo);
	}

	GLuint vao;
	GLuint vbo;
	GLuint ebo;

	ShapeProperties properties;
};

enum ShaderType {
	LINE = 0, 
	TRIANGLE = 1, 
	QUAD = 2, 
	CUBE = 3, 
	CUBEMAP = 4, 
	SPHERE = 5
};

/**
 * @brief Simple shader program which draws a cube/rect
*/
class BasicShaders {
	public:
		BasicShaders() {}
		~BasicShaders() {
			for(int i = 0; i < shaders.size(); i++){
				delete shaders[i];
			}
		}
		/**
		 * @brief Applies the appropriate transforms to show perspective or not
		 * @param cameraView The 4x4 matrix representing the camera's view
		 * @param fov The camera's fov
		 */
		void draw(const glm::mat4& cameraView, const float& fov, const ShapeProperties properties) {
			// Vertex shader calculations
			glm::mat4 view 	= glm::mat4(1.f);
			glm::mat4 projection = glm::mat4(1.f);

			projection = glm::perspective(glm::radians(fov), 640.f / 480.f, 0.1f, 100.f);
			view = cameraView;

			glm::mat4 model = glm::mat4(1.f);
			for(int i = 0; i < shaders.size(); i++){
				model = glm::translate(model, properties.pos);					// Set position
				model = glm::scale(model, properties.scale);					// Set scale
				model = glm::rotate(model, properties.angle, properties.axis);	// Set rotation
				
				glUniformMatrix4fv(shaders[i]->model, 1, GL_FALSE, glm::value_ptr(model));
				glUniformMatrix4fv(shaders[i]->view, 1, GL_FALSE, glm::value_ptr(view));
				glUniformMatrix4fv(shaders[i]->projection, 1, GL_FALSE, glm::value_ptr(projection));

				//typecast
				//draw
			}
		}
		void addShader(const ShaderType type) {
			const GLchar* vertSrc[] = {
				"#version 330 core\n", 
				"layout (location = 0) in vec3 aPos;\n", 
				"uniform mat4 model;\n", 
				"uniform mat4 view;\n", 
				"uniform mat4 projection;\n", 
				"void main() { gl_Position = projection * view * model * vec4(aPos, 1.0); }"
			};
			const GLchar* fragSrc[] = {
				"#version 330 core\n", 
				"out vec4 FragColor;\n", 
				"uniform vec4 color;\n", 
				"void main() { FragColor = color; }"
			};
			
			switch(type) {
				case(LINE): {

					break;
				} case(TRIANGLE): {
					
					break;
				} case(QUAD): {
					const GLfloat VERTEX_DATA[12] = {	// VBO data
						 1.f,  1.f,  0.f, 	// Top right
						 1.f, -1.f,  0.f, 	// Bottom right
						-1.f,  1.f,  0.f, 	// Top left
						-1.f, -1.f,  0.f	// Bottom left
					};
					const GLuint INDEX_DATA[6] = { 
						0, 1, 3,
						1, 2, 3,
					};

					ModelShader* quadShader;
					quadShader->programID = compile(vertSrc, fragSrc);
					bindMesh(*quadShader, VERTEX_DATA, INDEX_DATA);

					shaders.push_back(quadShader);

					break;
				} case(CUBE): {
					const GLfloat VERTEX_DATA[24] = {	// VBO data
						1.f,  1.f, -1.f, 	// Right top back
						1.f, -1.f, -1.f, 	// Right bottom back
						1.f,  1.f,  1.f, 	// Right top front
						1.f, -1.f,  1.f, 	// Right bottom front
								
						-1.f,  1.f, -1.f, 	// Left top back
						-1.f, -1.f, -1.f, 	// Left bottom back
						-1.f,  1.f,  1.f, 	// Left top front
						-1.f, -1.f,  1.f	// Left bottom front
					};
					const GLuint INDEX_DATA[36] = { 
						// Right face
						0, 1, 2,
						1, 3, 2,

						// Left face
						4, 6, 5,
						5, 6, 7,

						// Top face
						0, 2, 4,
						2, 6, 4,

						// Bottom face
						1, 5, 3,
						3, 5, 7,

						// Front face
						2, 3, 6,
						3, 7, 6,

						// Back face
						0, 4, 1,
						1, 4, 5
					};

					ModelShader* cubeShader;
					cubeShader->programID = compile(vertSrc, fragSrc);
					bindMesh(*cubeShader, VERTEX_DATA, INDEX_DATA);

					shaders.push_back(cubeShader);

					break;
				} case(CUBEMAP): {
					std::cerr << "Not implemented yet...\n";
					break;
				} case(SPHERE): {
					std::cerr << "Not implemented yet...\n";
					break;
				} default:
					std::cerr << "BasicShaders():addShader(): Unknown shader type \"" << type << "\"\n";
			}
		}
	private:
		void bind(const GLuint& programID) {
			glUseProgram(programID);

			GLenum error = glGetError();
			if(error != GL_NO_ERROR) {
				BaseShader::printProgramLog(programID);
				std::cerr << "BasicShaders::bind(): Unable to bind program \"" << programID << "\"\n";
			}
		}
		GLuint compile(const void* vertSrc, const void* fragSrc) {
			GLint programSuccess = GL_TRUE;	// Success flag
			GLuint programID = glCreateProgram();	// Create program

			// Vertex Shader
			GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
			glShaderSource(vertexShader, 6, (const GLchar**)vertSrc, NULL);
			glCompileShader(vertexShader);
			
			GLint status;
			glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &status);
			if(status != GL_TRUE){
				BaseShader::printShaderLog(vertexShader);
				throw std::runtime_error("Shader(): Unable to load vertex shader");
			}
			glAttachShader(programID, vertexShader);

			// Fragment Shader
			GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
			glShaderSource(fragmentShader, 4, (const GLchar**)fragSrc, NULL);
			glCompileShader(fragmentShader);
			
			status = GL_TRUE;
			glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &status);
			if(status != GL_TRUE){
				BaseShader::printShaderLog(vertexShader);
				throw std::runtime_error("Shader(): Unable to load fragment shader");
			}
			glAttachShader(programID, fragmentShader);

			glLinkProgram(programID);	// Link

			glGetProgramiv(programID, GL_LINK_STATUS, &programSuccess);
			if(programSuccess != GL_TRUE){
				BaseShader::printProgramLog(programID);
				throw std::runtime_error("Shader(): Unable to create program");
			}

			// Cleanup
			glDeleteShader(vertexShader);
			glDeleteShader(fragmentShader);

			return programID;
		}
		void bindMesh(ModelShader& shader, const void* vertexData, const void* indexData) {
			// Create objects
			glGenVertexArrays(1, &shader.vao);
			glGenBuffers(1, &shader.vbo);
			glGenBuffers(1, &shader.ebo);
			glBindVertexArray(shader.vao);

			// VBO
			glBindBuffer(GL_ARRAY_BUFFER, shader.vbo);	// Use the vbo
			glBufferData(GL_ARRAY_BUFFER, sizeof(vertexData), vertexData, GL_STATIC_DRAW);	// Set data

			// EBO
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, shader.ebo);	// Use the ebo
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indexData), indexData, GL_STATIC_DRAW);	// Set data

			// Set vertex position
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (void*)0);
			glEnableVertexAttribArray(0);	// Enable the attribute at index 0

			glBindBuffer(GL_ARRAY_BUFFER, 0);	// Unbind VBO

			// Prefetch uniform locations
			shader.model = glGetUniformLocation(shader.programID, "model");
			shader.view = glGetUniformLocation(shader.programID, "view");
			shader.projection = glGetUniformLocation(shader.programID, "projection");
			shader.color = glGetUniformLocation(shader.programID, "color");
		}

		std::vector<Shader*> shaders;
};