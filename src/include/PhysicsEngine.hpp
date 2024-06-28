#pragma once

#include <bullet/btBulletDynamicsCommon.h>
#include <glm/gtc/matrix_transform.hpp>

#include <iostream>
#include <cstdio>
#include <fstream>

#include "Collision.h"

#include <GL/glew.h>
#include <GL/glu.h>

class PhysicsEngine {
	public:
		PhysicsEngine() {
			cubeShader = new CubeShader();
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
			// bullet3 dimensions are double that of opengl, ie. 1.0(bullet) -> 0.5(opengl)
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

			return true;
		}
		void tick(float delta_t) {
			dynamicsWorld->stepSimulation(delta_t, 10);
		}
		void drawCollider(const glm::mat4& cameraView, const float& cameraFOV, const bool wireframe) {
			ShapeProperties properties;

			{
				btCollisionObject* obj = dynamicsWorld->getCollisionObjectArray()[0];
				btRigidBody* body = btRigidBody::upcast(obj);
				btTransform transform;

				if(body && body->getMotionState()){
					body->getMotionState()->getWorldTransform(transform);
				} else {
					transform = obj->getWorldTransform();
				}

				// Set shader properties
				properties.pos = glm::vec3(
					transform.getOrigin().getX(), 
					transform.getOrigin().getY(), 
					transform.getOrigin().getZ()
				);
				properties.angle = transform.getRotation().getAngle();
				transform.getRotation().getEulerZYX(properties.axis.y, properties.axis.z, properties.axis.x);
				if(properties.axis == glm::vec3(0.f)) properties.axis = glm::vec3(0.f, 1.f, 0.f);

				btVector3 dimensions = ((btBoxShape*)(body->getCollisionShape()))->getHalfExtentsWithMargin();
				properties.scale = glm::vec3(dimensions.getX(), dimensions.getY(), dimensions.getZ());

				// Draw
				if(wireframe){
					glDisable(GL_CULL_FACE);	// Culling is flipped with wireframe
					glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
				}
				
				cubeShader->bind();
				cubeShader->draw(cameraView, cameraFOV, properties);

				if(wireframe){
					glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
					glEnable(GL_CULL_FACE);
				}
			}
			
			{
				btCollisionObject* obj = dynamicsWorld->getCollisionObjectArray()[1];
				btRigidBody* body = btRigidBody::upcast(obj);
				btTransform transform;

				if(body && body->getMotionState()){
					body->getMotionState()->getWorldTransform(transform);
				} else {
					transform = obj->getWorldTransform();
				}

				// Set shader properties
				properties = ShapeProperties();
				properties.pos = glm::vec3(
					transform.getOrigin().getX(), 
					transform.getOrigin().getY(), 
					transform.getOrigin().getZ()
				);
				properties.angle = transform.getRotation().getAngle();
				transform.getRotation().getEulerZYX(properties.axis.y, properties.axis.z, properties.axis.x);
				if(properties.axis == glm::vec3(0.f)) properties.axis = glm::vec3(0.f, 1.f, 0.f);

				btScalar radius = ((btSphereShape*)(body->getCollisionShape()))->getRadius();
				properties.scale = glm::vec3(radius, radius, radius);
				properties.color = glm::vec3(1.f, 0.f, 0.f);

				// Draw
				glDisable(GL_CULL_FACE);	// Culling is flipped with wireframe
				glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
				
				cubeShader->bind();
				cubeShader->draw(cameraView, cameraFOV, properties);

				glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
				glEnable(GL_CULL_FACE);
			}
		}
	private:
		btDefaultCollisionConfiguration* collisionConfig;	// Default memory and collision setup
		btCollisionDispatcher* dispatcher;					// Collision handler
		btBroadphaseInterface* interface;					// AABB collision detection interface
		btSequentialImpulseConstraintSolver* solver;		// Constraint solver
		btDiscreteDynamicsWorld* dynamicsWorld;				// Dynamics world
		btAlignedObjectArray<btCollisionShape*> objArray;	// Collision shape array

		CubeShader* cubeShader;	// Shader to draw cubic colliders
};