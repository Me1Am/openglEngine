#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>

class Camera {
	public:
		Camera() {
			pitch = 0.f;
			roll = 0.f;
			yaw = -90.f;
			fov = 45.f;

			cameraPos = glm::vec3(0.0f, 0.0f, 3.0f);
			cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
			cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
		}
		~Camera() {}
		/**
		 * @brief Caclulates the view matrix
		 * @return A glm::mat4 representing the camera's view
		*/
		// TODO Implement roll
		glm::mat4 calcCameraView() {
			glm::vec3 cameraTarget = cameraPos + cameraFront;

			glm::vec3 cameraRight = glm::normalize(glm::cross(WORLD_UP, cameraFront));
			cameraUp = glm::cross(cameraFront, cameraRight);

			// LookAt matrix which allows easy camera manipulation
			glm::mat4 view = glm::mat4(1.f);
			//view = glm::lookAt(cameraPos, cameraTarget, cameraUp);
			view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
			view = glm::rotate(view, roll, glm::vec3(0.f, 0.f, 1.f));	// Apply roll

			return view;
		}
		/**
		 * @brief Updates the camera's position
		 * @param forward A float representing if the camera should move forward
		 * @param backward A float representing if the camera should move backwards
		 * @param left A float representing if the camera should strafe left
		 * @param right A float representing if the camera should strafe right
		 * @param deltaTime A float representing the time since last frame in ms
		*/
		void updateCameraPosition(const bool forward, const bool backward, const bool left, const bool right, const bool up, const bool down, const float deltaTime) {
			float frameSpeed = cameraSpeed * deltaTime;

			if(forward)
				cameraPos += cameraFront * frameSpeed;
			if(backward)
				cameraPos -= cameraFront * frameSpeed;
			if(left)
				cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * frameSpeed;
			if(right)
				cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * frameSpeed;
			if(up)
				cameraPos += cameraUp * frameSpeed;
			if(down)
				cameraPos -= cameraUp * frameSpeed;
		}
		void updateCameraDirection() {
			glm::vec3 direction(0.f, 0.f, 0.f);
			
			direction.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
			direction.y = sin(glm::radians(pitch));
			direction.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));

			cameraFront = glm::normalize(direction);
		}
		void incPitch(const float offset) {
			this->pitch += offset;

			// Limit movement
			if(this->pitch > 89.0f)
				this->pitch =  89.0f;
			if(this->pitch < -89.0f)
				this->pitch = -89.0f;
		}
		void incRoll(const float offset) {
			this->roll += offset;
		}
		void incYaw(const float offset) {
			this->yaw += offset;
		}
		void incFOV(const float offset) {
			this->fov += offset;
			
			// Limit
			if(this->fov < MIN_FOV)
				this->fov = MIN_FOV;
			if(this->fov > MAX_FOV)
				this->fov = MAX_FOV;
		}
		void setPitch(const float pitch) {
			this->pitch = pitch;

			// Limit movement
			if(this->pitch > 89.0f)
				this->pitch =  89.0f;
			if(this->pitch < -89.0f)
				this->pitch = -89.0f;
		}
		void setRoll(const float roll) {
			this->roll = roll;
		}
		void setYaw(const float yaw) {
			this->yaw = yaw;
		}
		void setFOV(const float fov) {
			this->fov = fov;
			
			// Limit
			if(this->fov < MIN_FOV)
				this->fov = MIN_FOV;
			if(this->fov > MAX_FOV)
				this->fov = MAX_FOV;
		}
		void setPos(const glm::vec3 pos) {
			this->cameraPos = pos;
		}
		glm::vec3 getPos() {
			return cameraPos;
		}
		glm::vec3 getDir() {
			return cameraFront;
		}
		float getPitch() {
			return pitch;
		}
		float getRoll() {
			return roll;
		}
		float getYaw() {
			return yaw;
		}
		float getFOV() {
			return fov;
		}
	private:
		const float cameraSpeed = 0.012f;

		float pitch;
		float roll;
		float yaw;
		float fov;

		const float MIN_FOV = 1.f;
		const float MAX_FOV = 45.f;

		glm::vec3 cameraPos;
		glm::vec3 cameraFront;
		glm::vec3 cameraUp;
		const glm::vec3 WORLD_UP = glm::vec3(0.0f, 1.0f, 0.0f);
};