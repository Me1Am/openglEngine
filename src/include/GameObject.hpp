#pragma once

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

#include <iostream>
#include <string>

#include "Model.hpp"
#include "shader/BaseShader.hpp"

/**
 * @brief Struct holding positional and motion data
*/
struct Pos {
	float rotation = 0.f;		// Rotation in radians
	float angAccel = 0.f;		// Angular acceleration, radians per milisecond^2
	float angVelocity = 0.f;	// Angular velocity, radians per milisecond

	glm::vec3 pos = { 0.f, 0.f, 0.f };				// Position
	glm::vec3 rotationAxis = { 0.f, 1.f, 0.f };		// Axis of rotation, 0-1

	glm::vec3 linearAccel = { 0.f, 0.f, 0.f };		// Linear acceleration in each direction, units per milisecond squared
	glm::vec3 linearVelocity = { 0.f, 0.f, 0.f };	// Velocity of each direction, units per milisecond
	
	glm::vec3 angAccelAxis = { 0.f, 1.f, 0.f };		// Axis of angular acceleration, 0-1
	glm::vec3 angVelocityAxis = { 0.f, 1.f, 0.f };	// Axis of angular velocity, 0-1
};

/**
 * @brief Struct holding constants
 * @note UNIQUE_ID is first, and REQUIRED
*/
struct Constants {
	const short UNIQUE_ID;

	const glm::vec3 MIN_VELOCITY = { -100.f, -100.f, -100.f };	// Minimum velocity for all directions, units per milisecond
	const glm::vec3 MAX_VELOCITY = { 100.f, 100.f, 100.f };		// Maximum velocity for all directions, units per milisecond
	
	Constants();
	Constants(const short uid) : UNIQUE_ID(uid) {}
	Constants(const short uid, const glm::vec3 minVel, const glm::vec3 maxVel) 
		: UNIQUE_ID(uid), MIN_VELOCITY(minVel), MAX_VELOCITY(maxVel) {}
};

/**
 * @brief An object in the engine which has an update function
 * @details 
*/
class GameObject {
	public:
		/** 
		 * @brief Default Constructor
		 * @brief Sets scale to 1.f
		 * @param uniqueID The unique identifier for the object
		 */
		GameObject(const short uniqueID) {
			scale = 1.f;

			// Load empty model, doesn't actually do anything
			model.initialize("");
		}
		/**
		 * @brief Parameterized Constructor
		 * @param pos A Pos struct with the GameObjects initial position and rotation
		 * @param modelPath The string path to the GameObject's model
		 * @param uniqueID The unique identifier for the object
		 * @param scale A float with the initial scale of the model, optional
		 * @note Default value is 1.f for scale
		*/
		GameObject(const Pos pos, const Constants constants, const std::string modelPath, const float scale = 1.f) : constants(constants) {
			this->pos = pos;
			this->scale = scale;

			// Load model
			if(!model.initialize(modelPath.c_str())) 
				throw std::runtime_error("Unable to load model at '" + modelPath + "'");
		}
		/**
		 * @brief Prepares object for rendering and draws itself
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
		 * @brief The update function for the object
		 * @param deltaTime Miliseconds since last update
		 * @note Updates the position with its velocity
		*/
		void tick(const float deltaTime) {
			pos.pos += pos.linearAccel * deltaTime;
			pos.pos += pos.linearVelocity * deltaTime;
		}
		/**
		 * @brief Changes the velocity of the object
		 * @param velocity The amount to change by, default 1 unit per milisecond
		 * @note Adds the ammount
		*/
		void changeLinearVelocity(const glm::vec3 velocity = glm::vec3(1.f)) {
			pos.linearVelocity.x = std::clamp((pos.linearVelocity.x + velocity.x), constants.MIN_VELOCITY.x, constants.MAX_VELOCITY.x);
			pos.linearVelocity.y = std::clamp((pos.linearVelocity.y + velocity.y), constants.MIN_VELOCITY.y, constants.MAX_VELOCITY.y);
			pos.linearVelocity.z = std::clamp((pos.linearVelocity.z + velocity.z), constants.MIN_VELOCITY.z, constants.MAX_VELOCITY.z);
		}
		/**
		 * @brief Sets the velocity of the object
		 * @param velocity The velocity for each direction
		*/
		void setLinearVelocity(const glm::vec3 velocity) {
			pos.linearVelocity.x = std::clamp((velocity.x), constants.MIN_VELOCITY.x, constants.MAX_VELOCITY.x);
			pos.linearVelocity.y = std::clamp((velocity.y), constants.MIN_VELOCITY.y, constants.MAX_VELOCITY.y);
			pos.linearVelocity.z = std::clamp((velocity.z), constants.MIN_VELOCITY.z, constants.MAX_VELOCITY.z);
		}
		/**
		 * @brief Sets the Pos struct
		*/
		void setPos(const Pos pos) {
			this->pos = pos;
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
		 * @brief Gets the GameObject's Pos struct
		 * @return Pos struct
		*/
		Pos getPos() {
			return pos;
		}
		/**
		 * @brief Gets the GameObject's Constants struct
		 * @return Constants struct
		*/
		Constants getConstants() {
			return constants;
		}	
		/**
		 * @brief Gets the GameObject's unique ID
		 * @return Unique ID
		*/
		short getUniqueID() {
			return constants.UNIQUE_ID;
		}
	protected:		
		float scale;	// Scales the object's model across each axis uniformly

		Pos pos;
		Model model;
		Constants constants;
};