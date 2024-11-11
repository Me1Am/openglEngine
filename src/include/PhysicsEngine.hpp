#pragma once

#include <btBulletDynamicsCommon.h>
#include <BulletWorldImporter/btBulletWorldImporter.h>

#include <iostream>
#include <fstream>
#include <cstdio>

#include "Collision.h"
#include "PhysicsDrawer.hpp"

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
				transform.setOrigin(btVector3(0.f, 0.f, 0.f));
				
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

			saveState("./saves/initState.bin");

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
		void castRay(const glm::vec3& origin, const glm::vec3& direction, const float len) {
			btVector3 from = btVector3(origin.x, origin.y, origin.z);
			btVector3 dir = btVector3(direction.x, direction.y, direction.z);
			btVector3 to = from + (dir * len);
			
			btDynamicsWorld::ClosestRayResultCallback callback(from, to);

			dynamicsWorld->rayTest(from, to, callback);

			if(callback.hasHit()){
				const btRigidBody* body = btRigidBody::upcast(callback.m_collisionObject);

				if(body){
					const btVector3 pos = body->getWorldTransform().getOrigin();

					std::cout << 
						"Hit Object at: " << 
							pos.getX() << 
							", " << 
							pos.getY() << 
							", " << 
							pos.getZ() << 
						'\n';
				}
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
		 * @note Attempts to load initState.bin from the saves folder
		*/
		void reset() {
			loadState("saves/initState.bin");
		}
		/**
		 * @brief Saves the current state of the physics engine to a file
		 * @throws runtime_error if it fails to write to the filesystem
		 */
		void saveState(const std::string& filename) {
			btDefaultSerializer* serializer = new btDefaultSerializer();
			
			dynamicsWorld->serialize(serializer);

			std::ofstream file(filename, std::ios::binary);
			if(file.is_open()){
				file.write(reinterpret_cast<const char*>(serializer->getBufferPointer()), serializer->getCurrentBufferSize());
				file.close();
			} else {
				throw std::runtime_error("PhysicsEngine::saveState(): Unable to create file at \"" + filename + '\"');
			}

			delete serializer;
		}
		/**
		 * @brief Loads a saved state of the physics engine
		 * @throws runtime_error if it fails to read or deserialize the file
		 */
		void loadState(const std::string& filename) {
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

			int bufferSize;
			unsigned char* data = getFileData(filename, bufferSize);

			collisionConfig = new btDefaultCollisionConfiguration();
			dispatcher = new btCollisionDispatcher(collisionConfig);
			interface = new btDbvtBroadphase();
			solver = new btSequentialImpulseConstraintSolver();
			dynamicsWorld = new btDiscreteDynamicsWorld(dispatcher, interface, solver, collisionConfig);

			btBulletWorldImporter* importer = new btBulletWorldImporter(dynamicsWorld);

			if(!importer->loadFileFromMemory(reinterpret_cast<char*>(data), bufferSize))
				throw std::runtime_error("PhysicsEngine::loadState(): Unable to deserialize file at \"" + filename + '"');

			dynamicsWorld->setDebugDrawer(debugDrawer);

			for(int i = 0; i < dynamicsWorld->getNumCollisionObjects(); i++) {
				btCollisionObject* obj = dynamicsWorld->getCollisionObjectArray()[i];
				objArray.push_back(obj->getCollisionShape());
			}

			delete[] data;
			delete importer;
		}
	private:
		/**
		 * @brief Loads a file into memory and returns its data
		 * @param filename The file path to load
		 * @param bufferSize A variable to hold the file size
		 * @returns Raw file data and file size through the `bufferSize` parameter
		 */
		unsigned char* getFileData(const std::string& filename, int& bufferSize) {
			std::ifstream file(filename, std::ios::binary | std::ios::ate);	// Open binary file at EOF

			if(file.is_open()){
				bufferSize = file.tellg();	// Get size from current position(end, ie. total size)
				file.seekg(0, std::ios::beg);			// Go to beginning
				
				unsigned char* buffer = new unsigned char[bufferSize];
				file.read(reinterpret_cast<char*>(buffer), bufferSize);
				file.close();

				return buffer;
			} else {
				throw std::runtime_error("PhysicsEngine::getFileData(): Unable to open/load file at \"" + filename + '\"');
			}
		}

		btDefaultCollisionConfiguration* collisionConfig;	// Default memory and collision setup
		btCollisionDispatcher* dispatcher;					// Collision handler
		btBroadphaseInterface* interface;					// AABB collision detection interface
		btSequentialImpulseConstraintSolver* solver;		// Constraint solver
		btDiscreteDynamicsWorld* dynamicsWorld;				// Dynamics world
		btAlignedObjectArray<btCollisionShape*> objArray;	// Collision shape array

		PhysicsDrawer* debugDrawer;
};