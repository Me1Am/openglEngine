#include <iostream>

#include "BaseShader.hpp"
#include "../FileHandler.hpp"

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

/**
 * @brief Simple shader program which draws a cube/rect
*/
class CubeShader {
	public:
		CubeShader() {
			GLint programSuccess = GL_TRUE;	// Success flag
			programID = glCreateProgram();	// Create program

			// Vertex Shader
			GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
			// Load and convert the shader file into a const GLchar**
			const GLchar* vertexSource[] = {
				"#version 330 core\n", 
				"layout (location = 0) in vec3 aPos;\n", 
				"uniform mat4 model;\n", 
				"uniform mat4 view;\n", 
				"uniform mat4 projection;\n", 
				"void main() { gl_Position = projection * view * model * vec4(aPos, 1.0); }"
			};

			// Compile
			glShaderSource(vertexShader, 6, vertexSource, NULL);
			glCompileShader(vertexShader);
			
			// Error check
			GLint status;
			glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &status);
			if(status != GL_TRUE){
				GLint length;	// Length of info buffer
				glGetShaderiv(vertexShader, GL_INFO_LOG_LENGTH, &length);	// Get length

				std::string log(length, ' ');	// Create string
				glGetShaderInfoLog(vertexShader, length, &length, &log[0]);	// Write into string
				throw std::logic_error(log);	// Throw logic error
			}
			glAttachShader(programID, vertexShader);

			// Fragment Shader
			GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
			// Load and convert the shader file into a const GLchar**
			const GLchar* fragmentSource[] = {
				"#version 330 core\n", 
				"out vec4 FragColor;\n", 
				"uniform vec4 color;\n", 
				"void main() { FragColor = color; }"
			};

			// Compile
			glShaderSource(fragmentShader, 4, fragmentSource, NULL);
			glCompileShader(fragmentShader);
			
			// Error check
			status = GL_TRUE;
			glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &status);
			if(status != GL_TRUE){
				GLint length;	// Length of info buffer
				glGetShaderiv(fragmentShader, GL_INFO_LOG_LENGTH, &length);	// Get length

				std::string log(length, ' ');	// Create string
				glGetShaderInfoLog(fragmentShader, length, &length, &log[0]);	// Write into string
				throw std::logic_error(log);	// Throw logic error
			}
			glAttachShader(programID, fragmentShader);

			glLinkProgram(programID);	// Link
			// Error check
			glGetProgramiv(programID, GL_LINK_STATUS, &programSuccess);
			if(programSuccess != GL_TRUE){
				std::string err = "Error linking program, program ID: " + std::to_string(programID) + '\n';
				int infoLogLength = 0;
				int maxLength = 0;
				
				glGetProgramiv(programID, GL_INFO_LOG_LENGTH, &maxLength);
				char* infoLog = new char[maxLength];
				
				glGetProgramInfoLog(programID, maxLength, &infoLogLength, infoLog);
				if(infoLogLength > 0) err += std::string(infoLog);
				
				delete[] infoLog;

				throw std::runtime_error(err);
			}

			// Cleanup
			glDeleteShader(vertexShader);
			glDeleteShader(fragmentShader);

			GLfloat vertexData[] = {	// VBO data
				 1.f,  1.f, -1.f, 	// Right top back
				 1.f, -1.f, -1.f, 	// Right bottom back
				 1.f,  1.f,  1.f, 	// Right top front
				 1.f, -1.f,  1.f, 	// Right bottom front
				
				-1.f,  1.f, -1.f, 	// Left top back
				-1.f, -1.f, -1.f, 	// Left bottom back
				-1.f,  1.f,  1.f, 	// Left top front
				-1.f, -1.f,  1.f	// Left bottom front
			};
			GLuint indexData[] = { 
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

			// Create objects
			glGenVertexArrays(1, &vao);	// Create VAO
			glGenBuffers(1, &vbo);		// Create VBO
			glGenBuffers(1, &ebo);		// Create VBO
			glBindVertexArray(vao);		// Bind VAO to capture calls

			// VBO
			glBindBuffer(GL_ARRAY_BUFFER, vbo);	// Use the vbo
			glBufferData(GL_ARRAY_BUFFER, sizeof(vertexData), vertexData, GL_STATIC_DRAW);	// Set data

			// EBO
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);	// Use the ebo
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indexData), indexData, GL_STATIC_DRAW);	// Set data

			// Set vertex position
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (void*)0);
			glEnableVertexAttribArray(0);	// Enable the attribute at index 0

			glBindBuffer(GL_ARRAY_BUFFER, 0);	// Unbind VBO

			// Prefetch uniform locations
			model = glGetUniformLocation(programID, "model");
			view = glGetUniformLocation(programID, "view");
			projection = glGetUniformLocation(programID, "projection");
			color = glGetUniformLocation(programID, "color");
		}
		~CubeShader() {
			glDeleteVertexArrays(1, &vao);
			glDeleteBuffers(1, &vbo);
			glDeleteBuffers(1, &ebo);
			glDeleteProgram(programID);
		}
		/**
		 * @brief Binds the shader and loads the vao
		 * @throws runtime_error on failure
		*/
		void bind() {
			glUseProgram(programID);
			GLenum error = glGetError();
			if(error != GL_NO_ERROR) {
				std::string err = "Error binding shader: " + std::to_string(error) + '\n';
				int infoLogLength = 0;
				int maxLength = 0;
				
				glGetProgramiv(programID, GL_INFO_LOG_LENGTH, &maxLength);
				char* infoLog = new char[maxLength];
				
				glGetProgramInfoLog(programID, maxLength, &infoLogLength, infoLog);
				if(infoLogLength > 0) err += std::string(infoLog);
				
				delete[] infoLog;

				throw std::runtime_error(err);
			}

			glBindVertexArray(vao);
		}
		/**
		 * @brief Applies the appropriate transforms to show perspective or not
		 * @param cameraView The 4x4 matrix representing the camera's view
		 * @param fov The camera's fov
		 */
		void draw(const glm::mat4& cameraView, const float& fov, const ShapeProperties properties) {
			// Vertex shader calculations
			glm::mat4 model = glm::mat4(1.f);
			glm::mat4 view 	= glm::mat4(1.f);
			glm::mat4 projection = glm::mat4(1.f);

			model = glm::translate(model, properties.pos);					// Set position
			model = glm::scale(model, properties.scale);					// Set scale
			model = glm::rotate(model, properties.angle, properties.axis);	// Set rotation
			projection = glm::perspective(glm::radians(fov), 640.f / 480.f, 0.1f, 100.f);
			view = cameraView;

			glUniformMatrix4fv(this->model, 1, GL_FALSE, glm::value_ptr(model));
			glUniformMatrix4fv(this->view, 1, GL_FALSE, glm::value_ptr(view));
			glUniformMatrix4fv(this->projection, 1, GL_FALSE, glm::value_ptr(projection));

			// Fragment shader
			glUniform4f(this->color, properties.color.x, properties.color.y, properties.color.z, properties.alpha);

			glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
		}
	private:
		GLuint programID;
		GLuint vao = 0;
		GLuint vbo = 0;
		GLuint ebo = 0;

		GLint model;
		GLint view;
		GLint projection;
		GLint color;
};