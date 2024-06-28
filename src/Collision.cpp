#include <iostream>

#include "include/Collision.h"

Collider* createMeshCollider(const std::vector<Vertex>& verticies, const bool& convex, const short& tag) {
	Collider* collider = new Collider();
	collider->collider = createCollisionMesh(verticies, convex);
	collider->mesh = new Mesh(verticies, {}, {}, "collider");
	collider->tag = tag;

	return collider;
}

Collider* createShapeCollider(std::vector<btCollisionShape*> shapes, std::vector<btTransform> shapeTransforms, int count, bool dynamicAABBTree, const short& tag) {
	Collider* collider = new Collider();
	collider->collider = createCollisionShapeCompound(shapes, shapeTransforms, count, dynamicAABBTree);
	
	btCompoundShapeChild* child = ((btCompoundShape*)(collider->collider))->getChildList();
	for(int i = 0; i < ((btCompoundShape*)(collider->collider))->getNumChildShapes(); i++) {
		int shape = child->m_childShapeType;
		switch(shape) {
			case BOX_SHAPE_PROXYTYPE:
				std::cout << "Shape is a Box" << std::endl;
				break;
			case SPHERE_SHAPE_PROXYTYPE:
				std::cout << "Shape is a Sphere" << std::endl;
				break;
			case CAPSULE_SHAPE_PROXYTYPE:
				std::cout << "Shape is a Capsule" << std::endl;
				break;
			case CONE_SHAPE_PROXYTYPE:
				std::cout << "Shape is a Cone" << std::endl;
				break;
			case CYLINDER_SHAPE_PROXYTYPE:
				std::cout << "Shape is a Cylinder" << std::endl;
				break;
			case STATIC_PLANE_PROXYTYPE:
				std::cout << "Shape is a Static Plane" << std::endl;
				break;
			case COMPOUND_SHAPE_PROXYTYPE:
				std::cout << "Shape is a Compound Shape" << std::endl;
				break;
		}
	}
	return collider;
}

btCollisionShape* createCollisionMesh(const std::vector<Vertex>& verticies, const bool& convex) {
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
	if(shapes.size() != shapeTransforms.size())
		throw std::logic_error("\"shapes\" and \"shapeTransforms\" vectors are of the same not the same size");
	
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

void drawCollider(const Collider* collider) {
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);	// Wireframe
	glBindVertexArray(collider->mesh->getVAO());
	
	glDrawElements(GL_TRIANGLES, static_cast<GLuint>(collider->mesh->getIndices().size()), GL_UNSIGNED_INT, 0);
	
	glBindVertexArray(0);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}