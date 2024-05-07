#pragma once

#include <glm/vec3.hpp>

#include <iostream>

#include <cmath>
#include <vector>
#include <tuple>

#include "Car.hpp"

#define SIGN(x) (((x) >= 0) ? 1 : -1)
#define MAGNITUDE3(v) (std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z))
#define IS_NAN_VEC3(v) (std::isnan(v.x) || std::isnan(v.y) || std::isnan(v.y))
#define IS_INF_VEC3(v) (std::isinf(v.x) || std::isinf(v.y) || std::isinf(v.y))
#define IS_INVALID(x) (std::isnan(x) || std::isinf(x))
#define IS_INVALID_VEC3(v) (IS_INVALID(v.x) || IS_INVALID(v.y) || IS_INVALID(v.z))
#define UNSIGN_ZERO(x) ((x != 0) ? x : abs(x))	// Prevents negative zeros, returns input if its not zero
#define UNSIGN_ZERO_VEC3(v) { \
	v.x = UNSIGN_ZERO(v.x); \
	v.y = UNSIGN_ZERO(v.y); \
	v.z = UNSIGN_ZERO(v.z); }

class PhysicsEngine {
	public:
		PhysicsEngine();
		~PhysicsEngine();
		/**
		 * @brief Checks and removes nan or inf values
		 * @param vec Reference to the vec3 to be checked
		 * @param defVals The vec3 holding the values to set the invalid members to
		*/
		static void fixVec3(glm::vec3 &vec, const glm::vec3 &defVals) {
			if(IS_INVALID(vec.x))
				vec.x = defVals.x;
			if(IS_INVALID(vec.y))
				vec.y = defVals.y;
			if(IS_INVALID(vec.z))
				vec.z = defVals.z;
		}
		/**
		 * @brief Converts and axis-angle pair to a quaternion
		 * @param axis The unit vector
		 * @param angle The angle in radians
		 * @return A quaternion equal to the axis-angle
		 * @note The axis must be normalized before calling
		*/
		static glm::quat axisAngleToQuaternion(const glm::vec3 axis, const float angle) {
			return glm::quat( 
				cos(angle / 2), 
				axis.x * sin(angle / 2), 
				axis.y * sin(angle / 2), 
				axis.z * sin(angle / 2)
			);
		}
		/**
		 * @brief Converts and axis-angle pair to a quaternion
		 * @param axis The unit vector
		 * @param angle The angle in radians
		 * @return A tuple with the axis and angle, respectively
		 * @note The axis must be normalized before calling
		*/
		static std::tuple<glm::vec3, float> quaternionToAxisAngle(const glm::quat quaternion) {
			return {
				glm::vec3(
					(quaternion.w == 1) ? 0 : quaternion.x / sqrt(1 - quaternion.w*quaternion.w), 
					(quaternion.w == 1) ? 1 : quaternion.y / sqrt(1 - quaternion.w*quaternion.w), 
					(quaternion.w == 1) ? 0 : quaternion.z / sqrt(1 - quaternion.w*quaternion.w)
				), 
				2 * acos(quaternion.w)
			};
		}
		static glm::quat deltaRotation(const glm::vec3 angVelocity, const float deltaTime) {
			glm::vec3 halfAngle = angVelocity * (deltaTime * 0.5f);
			float magnitude = MAGNITUDE3(halfAngle);
			if(magnitude > 0)
				halfAngle *= sin(magnitude) / magnitude;
			return glm::quat(cos(magnitude), halfAngle.x, halfAngle.y, halfAngle.z);
		}
};