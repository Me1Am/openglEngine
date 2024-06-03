#include <iostream>

#include "include/Collision.h"

btCollisionShape* createCollisionMesh(std::vector<Vertex> verticies, bool convex) {
	btCollisionShape* shape;

	if(convex){
		shape = new btConvexHullShape();
		for(int i = 0; i < verticies.size(); i++) {
			Vertex vertex = verticies[i];
			btVector3 pos = btVector3(vertex.pos.x, vertex.pos.y, vertex.pos.z);
			((btConvexHullShape*)shape)->addPoint(pos);
		}
	} else {
		btTriangleMesh* mesh = new btTriangleMesh();
		for(int i = 0; i < verticies.size(); i+=3) {
			Vertex vertex1 = verticies[i];
			Vertex vertex2 = verticies[i+1];
			Vertex vertex3 = verticies[i+2];

			btVector3 pos1 = btVector3(vertex1.pos.x, vertex1.pos.y, vertex1.pos.z);
			btVector3 pos2 = btVector3(vertex2.pos.x, vertex2.pos.y, vertex2.pos.z);
			btVector3 pos3 = btVector3(vertex3.pos.x, vertex3.pos.y, vertex3.pos.z);
			
			mesh->addTriangle(pos1, pos2, pos3);
		}
		shape = new btBvhTriangleMeshShape(mesh, true);
	}

	return shape;
}

btCollisionShape* createCollisionShapeCompound(std::vector<btCollisionShape*> shapes, std::vector<btTransform> shapeTransforms, int count, bool dynamicAABBTree) {
	if(shapes.size() != shapeTransforms.size()) throw std::logic_error("\"shapes\" and \"shapeTransforms\" vectors are of the same not the same size");
	
	btCollisionShape* shape;

	shape = new btCompoundShape(dynamicAABBTree, count);
	for(int i = 0; i < shapes.size(); i++) {
		((btCompoundShape*)shape)->addChildShape(shapeTransforms[i], shapes[i]);
	}

	return shape;
}

btRigidBody* createRigidBody(btCollisionShape* shape, btTransform transform, btScalar mass, btVector3 moveAxises) {
	btVector3 inertia = btVector3(0.f, 0.f, 0.f);
	if(mass != 0.f)
		shape->calculateLocalInertia(mass, inertia);

	btDefaultMotionState* motionState = new btDefaultMotionState(transform);
	btRigidBody::btRigidBodyConstructionInfo rbInfo(mass, motionState, shape, inertia);
				
	btRigidBody* rigidBody = new btRigidBody(rbInfo);
	if(mass != 0.f)
		rigidBody->setLinearFactor(moveAxises);
	
	return rigidBody;
}