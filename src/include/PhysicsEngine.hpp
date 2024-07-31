#pragma once

#include <bullet/btBulletDynamicsCommon.h>
#include <glm/gtc/matrix_transform.hpp>

#include <iostream>
#include <cstdio>
#include <fstream>

#include "Collision.h"
#include "PhysicsDrawer.hpp"

#include <GL/glew.h>
#include <GL/glu.h>

class PhysicsEngine {
	public:
		PhysicsEngine() {
			debugDrawer = new PhysicsDrawer();
		}
		/**
		 * @brief Deletes everything in reverse order from which they were instantiated
		*/
		~PhysicsEngine() {
			// Remove rigidbodies from the dynamics world
			for(int i = dynamicsWorld->getNumCollisionObjects() - 1; i >= 0; i--) {
				btCollisionObject* obj = dynamicsWorld->getCollisionObjectArray()[i];
				btRigidBody* body = btRigidBody::upcast(obj);
				
				if(body && body->getMotionState())
					delete body->getMotionState();
				dynamicsWorld->removeCollisionObject(obj);
				delete obj;
			}

			// Delete collision shapes
			for(int i = 0; i < objArray.size(); i++) {
				btCollisionShape* shape = objArray[i];
				objArray[i] = 0;
				delete shape;
			}

			delete dynamicsWorld;
			delete solver;
			delete interface;
			delete dispatcher;
			delete collisionConfig;

			delete debugDrawer;

			objArray.clear();
		}
		bool init() {
			collisionConfig = new btDefaultCollisionConfiguration();
			dispatcher = new btCollisionDispatcher(collisionConfig);
			interface = new btDbvtBroadphase();
			solver = new btSequentialImpulseConstraintSolver();
			dynamicsWorld = new btDiscreteDynamicsWorld(dispatcher, interface, solver, collisionConfig);

			dynamicsWorld->setGravity(btVector3(0.f, -10.f, 0.f));
			dynamicsWorld->setDebugDrawer(debugDrawer);

			///---< Demo Objects >---///
			// bullet3 dimensions are double that of opengl, ie. 1.0(bullet) -> 0.5(opengl)
			{	// Static ground, a 10x10x10 cube at (0, 0)
				btCollisionShape* shape = new btBoxShape(btVector3(btScalar(5.f), btScalar(5.f), btScalar(5.f)));
				objArray.push_back(shape);

				btTransform transform;
				transform.setIdentity();
				transform.setOrigin(btVector3(50.f, 0.f, 0.f));
				
				dynamicsWorld->addRigidBody(createRigidBody(shape, transform, 0.f));
			}
			{	// Dynamic sphere
				btCollisionShape* shape = new btSphereShape(btScalar(1.f));
				objArray.push_back(shape);

				/// Create Dynamic Objects
				btTransform transform;
				transform.setIdentity();
				transform.setOrigin(btVector3(0.f, 64.f, 0.f));

				dynamicsWorld->addRigidBody(createRigidBody(shape, transform, 1.f));
			}

			return true;
		}
		void addRigidBody(btRigidBody* rigidbody) {
			if(rigidbody){
				if(rigidbody->getCollisionShape())
					objArray.push_back(rigidbody->getCollisionShape());
				else
					throw std::runtime_error("PhysicsEngine::addRigidBody(): Unable to get collision shape");
				dynamicsWorld->addRigidBody(rigidbody);
			} else {
				throw std::runtime_error("PhysicsEngine::addRigidBody(): Arguement \"rigidbody\" is null");
			}
		}
		void tick(float delta_t) {
			dynamicsWorld->stepSimulation(delta_t, 10);
		}
		void debugDraw(const glm::mat4& cameraView, const float& cameraFOV, const int debugMode) {
			debugDrawer->setDebugMode(debugMode);
			debugDrawer->setCamera(cameraView, cameraFOV);
			
			dynamicsWorld->debugDrawWorld();
		}
		/**
		 * @brief Resets the simulation to its starting state
		 * @note Just recreates all objects
		*/
		// TODO Actually implement way to save and load state
		void reset() {
			// Clear
			// Remove rigidbodies from the dynamics world
			for(int i = dynamicsWorld->getNumCollisionObjects() - 1; i >= 0; i--) {
				btCollisionObject* obj = dynamicsWorld->getCollisionObjectArray()[i];
				btRigidBody* body = btRigidBody::upcast(obj);
				
				if(body && body->getMotionState())
					delete body->getMotionState();
				dynamicsWorld->removeCollisionObject(obj);
				delete obj;
			}
			// Delete collision shapes
			for(int i = 0; i < objArray.size(); i++) {
				btCollisionShape* shape = objArray[i];
				objArray[i] = 0;
				delete shape;
			}
			objArray.resize(0);

			{	// Static ground, a 10x10x10 cube at (0, 0)
				btCollisionShape* shape = new btBoxShape(btVector3(btScalar(5.f), btScalar(5.f), btScalar(5.f)));
				objArray.push_back(shape);

				btTransform transform;
				transform.setIdentity();
				transform.setOrigin(btVector3(0.f, 0.f, 0.f));
				
				dynamicsWorld->addRigidBody(createRigidBody(shape, transform, 0.f));
			}
			{	// Dynamic sphere
				btCollisionShape* shape = new btSphereShape(btScalar(1.f));
				objArray.push_back(shape);

				/// Create Dynamic Objects
				btTransform transform;
				transform.setIdentity();
				transform.setOrigin(btVector3(0.f, 50.f, 0.f));

				dynamicsWorld->addRigidBody(createRigidBody(shape, transform, 1.f));
			}
		}
	private:
		btDefaultCollisionConfiguration* collisionConfig;	// Default memory and collision setup
		btCollisionDispatcher* dispatcher;					// Collision handler
		btBroadphaseInterface* interface;					// AABB collision detection interface
		btSequentialImpulseConstraintSolver* solver;		// Constraint solver
		btDiscreteDynamicsWorld* dynamicsWorld;				// Dynamics world
		btAlignedObjectArray<btCollisionShape*> objArray;	// Collision shape array

		PhysicsDrawer* debugDrawer;
};