#include <bullet/btBulletDynamicsCommon.h>

#include <iostream>

#include "Mesh.hpp"

enum class ColliderType {
	RigidBody, 
	SoftBody, 
	Collider
};
enum class ColliderState {
	Static, 	// Doesn't move, mass = 0
	Kinematic,  // Can move by setting values, mass = 0
	Dynamic,	// Can move using physics, mass > 0
};


class Collider {
	public:
		Collider() {}
		~Collider() {}
		void createFromVerticies(std::vector<Vertex> vertices, bool convex) {
			if(convex){
				shape = new btConvexHullShape();
				for(int i = 0; i < vertices.size(); i++) {
					Vertex vertex = vertices[i];
					btVector3 pos = btVector3(vertex.pos.x, vertex.pos.y, vertex.pos.z);
					((btConvexHullShape*)shape)->addPoint(pos);
				}
			} else {
				btTriangleMesh* mesh = new btTriangleMesh();
				for(int i = 0; i < vertices.size(); i+=3) {
					Vertex vertex1 = vertices[i];
					Vertex vertex2 = vertices[i+1];
					Vertex vertex3 = vertices[i+2];

					btVector3 pos1 = btVector3(vertex1.pos.x, vertex1.pos.y, vertex1.pos.z);
					btVector3 pos2 = btVector3(vertex2.pos.x, vertex2.pos.y, vertex2.pos.z);
					btVector3 pos3 = btVector3(vertex3.pos.x, vertex3.pos.y, vertex3.pos.z);

					mesh->addTriangle(pos1, pos2, pos3);
				}
				shape = new btBvhTriangleMeshShape(mesh, true);
			}
		}
		void createBodyWithMass(float mass, btVector3 rot, btVector3 pos) {
			btQuaternion rotation;
			rotation.setEulerZYX(rot.getX(), rot.getY(), rot.getZ());
			
			btVector3 position = btVector3(pos.getX(), pos.getY(), pos.getZ());
			btDefaultMotionState* motionState = new btDefaultMotionState(btTransform(rotation, position));
			
			btScalar bodyMass = mass;
			btVector3 bodyInertia;
			shape->calculateLocalInertia(bodyMass, bodyInertia);
			
			btRigidBody::btRigidBodyConstructionInfo bodyCI = btRigidBody::btRigidBodyConstructionInfo(bodyMass, motionState, shape, bodyInertia);
			bodyCI.m_restitution = 1.0f;
			bodyCI.m_friction = 0.5f;
			
			rigidBody = new btRigidBody(bodyCI);
			//rigidBody->setUserPointer();
			rigidBody->setLinearFactor(btVector3(1.f, 1.f, 0.f));
		}
	private:
		btRigidBody* rigidBody;
		btCollisionShape* shape;

		btScalar mass;
		bool convex;
		short uid;
		int tag;
};