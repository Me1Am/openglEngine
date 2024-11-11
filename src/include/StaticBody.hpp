#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

#include <iostream>
#include <string>

#include "Model.hpp"
#include "shader/BaseShader.hpp"

/**
 * @brief Struct holding positional data
*/
struct StaticPos {
	float rotation;			// Rotation in radians

	glm::vec3 pos;			// Position
	glm::vec3 rotationAxis;	// Axis of rotation, 0-1
};

/**
 * @brief A static model in the engine, 
 * @note Does not have an update function and is static
*/
class StaticBody {
	public:
		/** 
		 * @brief Default Constructor
		 * @brief Sets everything in the StaticPos struct to zero
		 * @brief Sets scale to 1
		 * @param uniqueID The unique identifier for the body
		 * @note No error handling, cerr must be watched
		 */
		StaticBody(const short uniqueID) : UNIQUE_ID(uniqueID) {
			pos = {	// Initialize position
				0.f, 						// Rotation
				glm::vec3(0.f, 1.f, 0.f),	// Rotation Axis
				glm::vec3(0.f, 0.f, 0.f),	// Position
			};
			scale = 1;

			// Load model
			if(!model.initialize("")){
				std::cerr << "Unable to model" << std::endl;
				return;
			}
		}
		/**
		 * @brief Parameterized Constructor
		 * @param pos A StaticPos struct with the StaticBody's position and rotation
		 * @param scale A float with the initial scale of the model
		 * @param modelPath The string path to the StaticBody's model
		 * @param uniqueID The unique identifier for the body
		 * @note No error handling, cerr must be watched
		*/
		StaticBody(const StaticPos pos, const float scale, const std::string modelPath, const short uniqueID) : UNIQUE_ID(uniqueID) {
			this->pos = pos;
			this->scale = scale;

			// Load model
			if(!model.initialize(modelPath.c_str())){
				std::cerr << "Unable to model" << std::endl;
				return;
			}
		}
		/**
		 * @brief Prepares the body for rendering and draws itself
		 * @param shader The shader used to draw
		 * @param cameraView The view matrix of the camera
		 * @param cameraFOV The FOV of the camera
		*/
		void draw(BaseShader &shader, const glm::mat4 &cameraView, const float &cameraFOV) {
			shader.bind();
			shader.setRotation(pos.rotation, pos.rotationAxis);
			shader.setScale(scale, scale, scale);
			shader.setPos(pos.pos);
			shader.perspective(cameraView, cameraFOV);

			model.draw(shader);
		}
		/**
		 * @brief Sets the scale
		*/
		void setScale(const float scale) {
			this->scale = scale;
		}
		/**
		 * @brief Loads a new model
		 * @param modelPath The path to the new model
		 * @return Returns if successful
		*/
		bool setModel(const std::string modelPath) {
			if(!model.initialize(modelPath.c_str())){
				std::cerr << "Unable to model" << std::endl;
				return false;
			}

			return true;
		}
		/**
		 * @brief Gets the StaticBody's Pos struct
		 * @return StaticPos struct
		*/
		StaticPos getPos() {
			return pos;
		}
		/**
		 * @brief Gets the StaticBody's unique ID
		 * @return Unique ID
		*/
		short getUniqueID() {
			return UNIQUE_ID;
		}
	protected:
		float scale;	// Scales the bodiy's model across each axis uniformly
		
		const short UNIQUE_ID;
		
		StaticPos pos;
		Model model;
};