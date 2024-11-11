#pragma once

#include <btBulletDynamicsCommon.h>
#include <bullet/BulletDynamics/Vehicle/btRaycastVehicle.h>

#include "GameObject.hpp"
#include "Collision.h"

struct WheelProperties {
	btVector3 connectionPoint;		// Wheel connection, relative to the chassis
	btVector3 connectionDirection;	// Wheel direction(usually down), relative to the chassis
	btVector3 axelDirection;		// Wheel axel direction, relative to the chassis
	
	btScalar suspensionRest;		// Maximum distance the suspension can move from its resting position
	
	btScalar radius;
	bool isFront;
};

// TODO Allow more wheel configurations
struct CarProperties {
	uint wheelCount = 4;
	WheelProperties* wheels[4];
	
	btRigidBody* chassis;
	btRaycastVehicle::btVehicleTuning suspension;
};

class CarObject : public GameObject {
	public:
		CarObject(const Pos pos, const Constants constants, const std::string modelPath) : GameObject(pos, constants, modelPath), active(false) {}
		CarObject(const CarProperties properties, const Pos pos, const Constants constants, const std::string modelPath) : GameObject(pos, constants, modelPath), active(false) {
			this->properties = properties;
		}
		void initPhysics(btDynamicsWorld* world) {
			if(world == nullptr)
				throw std::runtime_error("CarObject::initPhysics(): Unable to initialize, `world` is null");

			raycaster = new btDefaultVehicleRaycaster(world);

			btCollisionShape* shape = new btBoxShape(btVector3(btScalar(1.f), btScalar(0.25f), btScalar(2.5f)));

			btTransform transform;
			transform.setIdentity();
			transform.setOrigin(btVector3(0.f, 8.f, 0.f));

			properties.chassis = createRigidBody(shape, transform, 10.f);
			base = new btRaycastVehicle(properties.suspension, properties.chassis, raycaster);

			for(int i = 0; i < properties.wheelCount; i++) {
				WheelProperties* wheel = properties.wheels[i];
				
				base->addWheel(
					wheel->connectionPoint, 
					wheel->connectionDirection, 
					wheel->axelDirection, 
					wheel->suspensionRest, 
					wheel->radius, 
					properties.suspension, 
					wheel->isFront
				);
			}

			world->addAction(base);
		}
		void input(const Uint8* keyboardState) override {
			if(keyboardState[SDL_SCANCODE_W]){
				applyEngineForce(1.f);
			} else if(keyboardState[SDL_SCANCODE_S]){
				applyEngineForce(-1.f);
			}

			if(keyboardState[SDL_SCANCODE_A]){
				setSteerAngle(base->getSteeringValue(0) - (5 / 180 * M_PI));
			} else if(keyboardState[SDL_SCANCODE_D]){
				setSteerAngle(base->getSteeringValue(0) + (5 / 180 * M_PI));
			}
		}
		void tick(const Uint32 deltaTime) override {
			btVector3 pos = properties.chassis->getCenterOfMassPosition();
			this->pos.pos = glm::vec3(pos.getX(), pos.getY(), pos.getZ());

			btVector3 vel = properties.chassis->getLinearVelocity();
			this->pos.linearVelocity = glm::vec3(vel.getX(), vel.getY(), vel.getZ());
		}
		void applyEngineForce(const btScalar force) {
			for(int i = 0; i < properties.wheelCount; i++) {
				base->applyEngineForce(force, i);
			}
		}
		/**
		 * @brief Sets the steer angle for the front wheels
		 */
		void setSteerAngle(const btScalar angle) {
			for(int i = 0; i < base->getNumWheels(); i++) {
				if(base->getWheelInfo(i).m_bIsFrontWheel)
					base->setSteeringValue(angle, i);
			}
		}
		void setWheelSteerAngle(const btScalar angle, const uint wheel) {
			base->setSteeringValue(angle, wheel);
		}
		void setBrake(const btScalar force, const uint wheel) {
			base->setBrake(force, wheel);
		}
		void setActiveState(const bool active) {
			this->active = active;
		}
		bool getActiveState() {
			return active;
		}
	private:
		bool active;	// If the object is controlled

		btScalar steerAngle;	// In radians
		btScalar throttle;		// 0-1, inclusive
		btScalar brake;			// 0-1, inclusive

		btRaycastVehicle* base;
		btVehicleRaycaster* raycaster;

		CarProperties properties;
};
