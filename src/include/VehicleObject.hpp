#pragma once

#include <btBulletDynamicsCommon.h>
#include <bullet/BulletDynamics/Vehicle/btRaycastVehicle.h>

#include "GameObject.hpp"

struct CarProperties {
	// TODO Allow more wheel configurations
	uint wheelCount = 4;
	WheelProperties* wheels[4];

	glm::vec3 centerOfGravity;
	
	PhysicsBody chasis;
	btRaycastVehicle::btVehicleTuning suspension;
};

struct WheelProperties {
	btVector3 connectionPoint;		// Wheel connection, relative to the chassis
	btVector3 connectionDirection;	// Wheel direction(usually down), relative to the chassis
	btVector3 axelDirection;		// Wheel axel direction, relative to the chassis
	
	btScalar suspensionRest;		// Maximum distance the suspension can move from its resting position
	btRaycastVehicle::btVehicleTuning suspension;	// Suspension properties
	
	btScalar radius;
	bool isFront;
};

class CarObject : GameObject {
	public:
		CarObject(const Pos pos, const Constants constants, const std::string modelPath) : GameObject(pos, constants, modelPath) {}
		CarObject(btDynamicsWorld* world, const  CarProperties properties, const Pos pos, const Constants constants, const std::string modelPath) : GameObject(pos, constants, modelPath) {
			raycaster = new btDefaultVehicleRaycaster(world);
			
			this->properties = properties;
			base = new btRaycastVehicle(this->properties.suspension, this->properties.chasis.rigidBody, raycaster);

			for(int i = 0; i < this->properties.wheelCount; i++) {
				WheelProperties* wheel = this->properties.wheels[i];
				
				base->addWheel(
					wheel->connectionPoint, 
					wheel->connectionDirection, 
					wheel->axelDirection, 
					wheel->suspensionRest, 
					wheel->radius, 
					wheel->suspension, 
					wheel->isFront
				);
			}
		}
		void applyEngineForce(const btScalar force) {
			for(int i = 0; i < properties.wheelCount; i++) {
				base->applyEngineForce(force, i);
			}
		}
		void setWheelSteerAngle(const btScalar angle, const uint wheel) {
			base->setSteeringValue(angle, wheel);
		}
		void setBrake(const btScalar force, const uint wheel) {
			base->setBrake(force, wheel);
		}
		void tick() {
			btVector3 pos = properties.chasis.rigidBody->getCenterOfMassPosition();
			this->pos.pos = glm::vec3(pos.getX(), pos.getY(), pos.getZ());

			btVector3 vel = properties.chasis.rigidBody->getLinearVelocity();
			this->pos.linearVelocity = glm::vec3(vel.getX(), vel.getY(), vel.getZ());
		}
	private:
		btScalar steerAngle;	// In radians
		btScalar throttle;		// 0-1, inclusive
		btScalar brake;			// 0-1, inclusive

		btRaycastVehicle* base;
		btVehicleRaycaster* raycaster;

		CarProperties properties;
};
