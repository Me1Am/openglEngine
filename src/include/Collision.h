#include <bullet/btBulletDynamicsCommon.h>

#include <vector>

#include "Mesh.hpp"

/**
 * @brief Collider struct, holds a rigidbody pointer and tag
*/
struct Collider {
	btCollisionShape* collider;
	Mesh* mesh;	// Mesh for debug drawing

	short tag;	// Can be used to determine the type of objects colliding

	~Collider() {
		delete collider;
		delete mesh;
	}
};

/**
 * @brief Rigidbody struct, holds a rigidbody pointer and Collider struct
*/
struct PhysicsBody {
	btRigidBody* rigidBody;
	Collider collider;

	~PhysicsBody() { delete rigidBody; }
};

/**
 * @brief Create a collision shape with a mesh
 * @param verticies Vector of all of Vertex structs from the collider mesh
 * @param convex If the mesh is concave or convex
 * @note If concave, the collider must be static, but is generally recommended to always be static when using this
 * @return btCollisionShape pointer
*/
btCollisionShape* createCollisionMesh(std::vector<Vertex> verticies, bool convex);

/**
 * @brief Create compound collision shape
 * @param shapes A vector of "simple" collision shapes
 * @param count The initial number of shapes in the compound shape
 * @param dynamicAABBTree If the dynamic AABB tree should be enabled
 * @note `shapes` and `shapeTransforms` should be aligned, will throw an exception if they're different sizes
 * @return btCollisionShape pointer
*/
btCollisionShape* createCollisionShapeCompound(std::vector<btCollisionShape*> shapes, std::vector<btTransform> shapeTransforms, int count, bool dynamicAABBTree);

/**
 * @brief Create the rigid body for the collider
 * @param shape The collision shape, created previously
 * @param transform The initial position and rotation of the rigidbody
 * @param moveAxises A normalized vector determining how the rigidbody will move(ie. 0 disables)
 * @param mass The mass of the rigidbody
*/
btRigidBody* createRigidBody(btCollisionShape* shape, btTransform transform, btScalar mass, btVector3 moveAxises = btVector3(btScalar(1.f), btScalar(1.f), btScalar(1.f)));

/**
 * @brief Draw a Collider struct's mesh as a wireframe
 * @param collider The Collider struct to draw
*/
void drawCollider(const Collider* collider);