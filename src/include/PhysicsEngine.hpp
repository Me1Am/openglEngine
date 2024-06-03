#pragma once

#include <bullet/btBulletDynamicsCommon.h>

#include <iostream>
#include <cstdio>
#include <fstream>

#include "Collision.h"

class PhysicsEngine {
	public:
		PhysicsEngine() {}
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

			objArray.clear();
		}
		bool init() {
			collisionConfig = new btDefaultCollisionConfiguration();
			dispatcher = new btCollisionDispatcher(collisionConfig);
			interface = new btDbvtBroadphase();
			solver = new btSequentialImpulseConstraintSolver();
			dynamicsWorld = new btDiscreteDynamicsWorld(dispatcher, interface, solver, collisionConfig);

			dynamicsWorld->setGravity(btVector3(0.f, -10.f, 0.f));

			///---< Demo Objects >---///
			{	// Static ground, a 100x100x100 cube at (0, -56)
				btCollisionShape* shape = new btBoxShape(btVector3(btScalar(50.f), btScalar(50.f), btScalar(50.f)));
				objArray.push_back(shape);

				btTransform transform;
				transform.setIdentity();
				transform.setOrigin(btVector3(0.f, -56.f, 0.f));
				
				dynamicsWorld->addRigidBody(createRigidBody(shape, transform, 0.f));
			}
			{	// Dynamic sphere, will hit ground object at y = -56
				btCollisionShape* shape = new btSphereShape(btScalar(1.f));
				objArray.push_back(shape);

				/// Create Dynamic Objects
				btTransform transform;
				transform.setIdentity();
				transform.setOrigin(btVector3(2.f, 10.f, 0.f));

				dynamicsWorld->addRigidBody(createRigidBody(shape, transform, 1.f));
			}

			return true;
		}
		void tick(float delta_t) {
			dynamicsWorld->stepSimulation(delta_t, 10);

			// Print all positions
			for(int i = dynamicsWorld->getNumCollisionObjects() - 1; i >= 0; i--) {
				btCollisionObject* obj = dynamicsWorld->getCollisionObjectArray()[i];
				btRigidBody* body = btRigidBody::upcast(obj);
				btTransform transform;

				if(body && body->getMotionState()){
					body->getMotionState()->getWorldTransform(transform);
				} else {
					transform = obj->getWorldTransform();
				}
				std::cout 
					<< "Object " << i << " at "
					<< transform.getOrigin().getX() << ", "
					<< transform.getOrigin().getY() << ", "
					<< transform.getOrigin().getZ()
				<< '\n';
			}
		}
	private:
		btDefaultCollisionConfiguration* collisionConfig;	// Default memory and collision setup
		btCollisionDispatcher* dispatcher;					// Collision handler
		btBroadphaseInterface* interface;					// AABB collision detection interface
		btSequentialImpulseConstraintSolver* solver;		// Constraint solver
		btDiscreteDynamicsWorld* dynamicsWorld;				// Dynamics world
		btAlignedObjectArray<btCollisionShape*> objArray;	// Collision object array

};