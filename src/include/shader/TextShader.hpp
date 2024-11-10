#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <string>

#include "BaseShader.hpp"
#include "../FileHandler.hpp"
#include "../Camera.hpp"

/**
 * @brief Shader program which holds a textured quad
 * @extends Shader
*/
class TextShader : public BaseShader {
	public:
		/**
		 * @brief Default constructor
		 * @brief Sets programID to NULL
		*/
		TextShader() : BaseShader() {
			programID = { };
		}
		/**
		 * @brief Calls freeProgram()
		*/
		~TextShader() {
			freeProgram();
		}
		/**
		 * @brief Deletes the shader program
		*/
		void freeProgram() {
			glDeleteProgram(programID);
		}
		/**
		 * @brief Loads the shader program
		 * @return a bool if loading worked or not 
		*/
		bool loadProgram(const std::string vertPath, const std::string fragPath) {
			GLint programSuccess = GL_TRUE;	// Success flag
			programID = glCreateProgram();	// Create program

			GLuint vertexShader;
			try {
				vertexShader = FileHandler::compileShader(vertPath);
			} catch(const std::logic_error& exeception) {
				std::cerr << exeception.what() << std::endl;
				return false;
			}
			glAttachShader(programID, vertexShader);

			GLuint fragmentShader;
			try {
				fragmentShader = FileHandler::compileShader(fragPath);
			} catch(const std::logic_error& exeception) {
				std::cerr << exeception.what() << std::endl;
				return false;
			}
			glAttachShader(programID, fragmentShader);

			glLinkProgram(programID);	// Link
			// Error check
			glGetProgramiv(programID, GL_LINK_STATUS, &programSuccess);
			if(programSuccess != GL_TRUE){
				std::cout << "Error linking program, program ID: " << programID << std::endl;
				printProgramLog(programID);
				return false;
			}

			// Cleanup
			glDeleteShader(vertexShader);
			glDeleteShader(fragmentShader);

			glGenVertexArrays(1, &vao);
			glGenBuffers(1, &vbo);
			glBindVertexArray(vao);

			glBindBuffer(GL_ARRAY_BUFFER, vbo);
			glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
			
			glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
			glEnableVertexAttribArray(0);
			
			// Unbind
			glBindBuffer(GL_ARRAY_BUFFER, 0);
			glBindVertexArray(0);
			
			return true;
		}
		/**
		 * @brief Applies the appropriate transforms to show perspective or not
		 * @param cameraView The 4x4 matrix representing the camera's view
		 * @param fov The camera's fov
		 *//*
		void perspective(const glm::mat4 cameraView, const float fov) {
			glm::mat4 model = glm::mat4(1.f);
			glm::mat4 view 	= glm::mat4(1.f);
			glm::mat4 projection = glm::mat4(1.f);

			model = glm::translate(model, pos);                     // Set position
			model = glm::scale(model, scale);						// Set scale
			model = glm::rotate(model, rotationRad, rotationAxis);	// Set rotation
			projection = glm::perspective(glm::radians(fov), 640.f / 480.f, 0.1f, 100.f);
			view = cameraView;

			glUniformMatrix4fv(glGetUniformLocation(programID, "model"), 1, GL_FALSE, glm::value_ptr(model));
			glUniformMatrix4fv(glGetUniformLocation(programID, "view"), 1, GL_FALSE, glm::value_ptr(view));
			glUniformMatrix4fv(glGetUniformLocation(programID, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
		}*/
		/**
		 * @brief Sets the text's color
		 * @param color A vec3 with floats from 0-1, inclusive, representing the rgb components, respectively
		*/
		void setColor(const glm::vec3 color) {
			glUniform3f(glGetUniformLocation(programID, "textColor"), color.x, color.y, color.z);
		}
		/**
		 * @brief Sets the text's color
		 * @param r A float 0-1, inclusive, representing the red component
		 * @param g A float 0-1, inclusive, representing the green component
		 * @param b A float 0-1, inclusive, representing the blue component
		*/
		void setColor(const float r, const float g, const float b) {
			glUniform3f(glGetUniformLocation(programID, "textColor"), r, g, b);
		}
		/**
		 * @brief Sets the object's position
		 * @param x The x coordinate
		 * @param y The y coordinate
		 * @param z The z coordinate
		 * @note The 'z' component is discarded
		*/
		void setPos(const float x, const float y, const float z) {
			pos = glm::vec3(x, y, z);

			glUniformMatrix4fv(
				glGetUniformLocation(programID, "projection"), 
				1, 
				GL_FALSE, 
				glm::value_ptr(
					glm::ortho(0.0f, pos.x, 0.0f, pos.y)
				)
			);
		}
		/**
		 * @brief Sets the object's position
		 * @param pos The new position vec3
		 * @note The 'z' component is discarded
		*/
		void setPos(const glm::vec3 pos) {
			this->pos = pos;

			glUniformMatrix4fv(
				glGetUniformLocation(programID, "projection"), 
				1, 
				GL_FALSE, 
				glm::value_ptr(
					glm::ortho(0.0f, pos.x, 0.0f, pos.y)
				)
			);
		}
		/**
		 * @brief Gets the position of the object
		 * @return The GLuint representing the position
		*/
		glm::vec3 getPos() {
			return pos;
		}
		/**
		 * @brief Gets the shader's VAO
		 * @return The GLuint representing the VAO
		*/
		GLuint getVAO() {
			return vao;
		}
		/**
		 * @brief Gets the shader's VBO
		 * @return The GLuint representing the VBO
		*/
		GLuint getVBO() {
			return vbo;
		}
		/**
		 * @brief Gets the shader's IBO
		 * @return The GLuint representing the IBO
		*/
		GLuint getEBO() {
			return ebo;
		}
	private:
		GLuint vao = 0;
		GLuint vbo = 0;
		GLuint ebo = 0;
};
