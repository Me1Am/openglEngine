#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <string>

#include "../FileHandler.hpp"
#include "../Camera.hpp"

/**
 * @brief Shader program which holds a textured quad
 * @extends Shader
*/
class BaseShader {
	public:
		/**
		 * @brief Default constructor
		 * @brief Sets programID to NULL
		*/
		BaseShader() {
			programID = { };
		}
		/**
		 * @brief Calls freeProgram()
		*/
		virtual ~BaseShader() {
			freeProgram();
		}
		/**
		 * @brief Deletes the shader program
		*/
		void freeProgram() {
			glDeleteProgram(programID);
		}
		/**
		 * @brief Sets this shader as the current program
		 * @return a bool if the binding worked or not
		*/
		bool bind() {
			glUseProgram(programID);

			// Check for errors
			GLenum error = glGetError();
			if(error != GL_NO_ERROR) {
				std::cout << "Error binding shader: " << std::endl;
				printProgramLog( programID );
				return false;
			}

			return true;
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

			return true;
		}
		/**
		 * @brief Applies the appropriate transforms to show perspective or not
		 * @param cameraView The 4x4 matrix representing the camera's view
		 * @param fov The camera's fov
		 */
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
		}
		/**
		 * @brief Sets the rotation of the object by the given radians, on the given axis
		 * @param radians Float representing the amount to rotate by
		 * @param xAxis Float representing the x axis value for applying the rotation
		 * @param yAxis Float representing the y axis value for applying the rotation
		 * @param zAxis Float representing the z axis value for applying the rotation
		 */
		void setRotation(const float radians, const float xAxis, const float yAxis, const float zAxis) {
			rotationRad = radians;
			rotationAxis = glm::vec3(xAxis, yAxis, zAxis);	
		}
		/**
		 * @brief Sets the rotation of the object by the given radians, on the given axis
		 * @param radians Float representing the amount to rotate by
		 * @param axis Vec3 representing the rotation axis
		 */
		void setRotation(const float radians, const glm::vec3 axis) {
			rotationRad = radians;
			rotationAxis = axis;
		}
		/**
		 * @brief Sets the scale ratio for the object
		 * @param xScale Float representing the x scale factor
		 * @param yScale Float representing the y scale factor
		 * @param zScale Float representing the z scale factor
		 */
		void setScale(const float xScale, const float yScale, const float zScale) {
			scale = glm::vec3(xScale, yScale, zScale);
		}
		/**
		 * @brief Sets a Mat4 uniform variable's value
		 * @param field The name of the variable
		 * @param mat4 The matrix data
		 */
		void setMat4(const std::string &field, const glm::mat4 &mat4) {
			glUniformMatrix4fv(glGetUniformLocation(programID, field.c_str()), 1, GL_FALSE, glm::value_ptr(mat4));
		}
		/**
		 * @brief Sets a Vec4 uniform variable's value
		 * @param field The name of the variable
		 * @param value1 The first value of the Vec4
		 * @param value2 The second value of the Vec4
		 * @param value3 The third value of the Vec4
		 * @param value4 The fourth value of the Vec4
		 */
		void setFloat4(const std::string &field, const float value1, const float value2, const float value3, const float value4) {
			glUniform4f(glGetUniformLocation(programID, field.c_str()), value1, value2, value3, value4);
		}
		/**
		 * @brief Sets a Vec3 uniform variable's value
		 * @param field The name of the variable
		 * @param value1 The first value of the Vec3
		 * @param value2 The second value of the Vec3
		 * @param value3 The third value of the Vec3
		 */
		void setFloat3(const std::string &field, const float value1, const float value2, const float value3) {
			glUniform3f(glGetUniformLocation(programID, field.c_str()), value1, value2, value3);
		}
		/**
		 * @brief Sets a Vec3 uniform variable's value
		 * @param field The name of the variable
		 * @param vec3 The 3D vector data
		 */
		void setVec3(const std::string &field, const glm::vec3 &vec3) {
			glUniform3f(glGetUniformLocation(programID, field.c_str()), vec3.x, vec3.y, vec3.z);
		}
		/**
		 * @brief Sets a boolean uniform variable's value
		 * @param field The name of the variable
		 * @param value The value to set
		 */
		void setBool(const std::string &field, const bool value) {
			glUniform1i(glGetUniformLocation(programID, field.c_str()), (int)value); 
		}
		/**
		 * @brief Sets an int uniform variable's value
		 * @param field The name of the variable
		 * @param value The value to set
		 */
		void setInt(const std::string &field, const int value) {
			glUniform1i(glGetUniformLocation(programID, field.c_str()), value); 
		}
		/**
		 * @brief Sets a float uniform variable's value
		 * @param field The name of the variable
		 * @param value The value to set
		 */
		void setFloat(const std::string &field, const float value) {
			glUniform1f(glGetUniformLocation(programID, field.c_str()), value);
		}
		/**
		 * @brief Sets the object's position
		 * @param x The x coordinate
		 * @param y The y coordinate
		 * @param z The z coordinate
		*/
		void setPos(const float x, const float y, const float z) {
			pos = glm::vec3(x, y, z);
		}
		/**
		 * @brief Sets the object's position
		 * @param pos The new position vec3
		*/
		void setPos(const glm::vec3 pos) {
			this->pos = pos;
		}
		/**
		 * @brief Gets the position of the object
		 * @return The GLuint representing the position
		*/
		glm::vec3 getPos() {
			return pos;
		}
		/**
		 * @brief Gets the shader program's ID
		 * @return The GLuint representing the ID
		*/
		GLuint getProgramID() {
			return programID;
		}

		/**
		 *  @brief Prints the log for the given shader
		 */
		static void printShaderLog(const GLuint shader) {
			if(!glIsShader(shader)) return;	// Check if its a shader
			int infoLogLength = 0;
			int maxLength = 0;

			glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &maxLength);	// Get the info string length
			char* infoLog = new char[maxLength];	// Allocate
			
			glGetShaderInfoLog(shader, maxLength, &infoLogLength, infoLog);	// Get info log
			if(infoLogLength > 0) std::cout << infoLog << std::endl;
			
			delete[] infoLog;
		}
		/**
		 *  @brief Prints the log for the given program
		 */
		static void printProgramLog(const GLuint program) {
			if(!glIsProgram(program)) return;	// Check if its a program
			int infoLogLength = 0;
			int maxLength = 0;
			
			glGetProgramiv(program, GL_INFO_LOG_LENGTH, &maxLength);	// Get the info string length
			char* infoLog = new char[maxLength];	// Allocate
			
			glGetProgramInfoLog(program, maxLength, &infoLogLength, infoLog);	// Get info log
			if(infoLogLength > 0) std::cout << infoLog << std::endl;
			
			delete[] infoLog;
		}
    private:
		GLuint programID;
        GLfloat rotationRad = 0.f;
		
        glm::vec3 pos = glm::vec3(0.f, 0.f, 0.f);
		glm::vec3 scale = glm::vec3(1.f, 1.f, 1.f);
		glm::vec3 rotationAxis = glm::vec3(0.f, 0.f, 0.f);
};