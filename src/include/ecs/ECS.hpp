#pragma once

#include <GL/glew.h>
#include <GL/glu.h>

#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <SDL2/SDL_scancode.h>
#include <bullet/btBulletDynamicsCommon.h>
#include <BulletWorldImporter/btBulletWorldImporter.h>

#include <json/json.h>

#include <unordered_map>
#include <functional>
#include <typeinfo>
#include <iostream>
#include <fstream>
#include <memory>
#include <bitset>
#include <vector>
#include <queue>
#include <array>
#include <set>

#include "../shader/BaseShader.hpp"
#include "../PhysicsDrawer.hpp"
#include "../Model.hpp"

#define MAX_COMPONENTS 32
#define MAX_ENTITIES 4096

using uint32_t = unsigned int;
using uint16_t = unsigned short;
using uint8_t = unsigned char;

using Entity = uint32_t;
using ComponentSet = std::bitset<MAX_COMPONENTS>;
using ComponentID = uint8_t;

/// @brief Handles the creation of entities and specifying their components
class EntityManager {
	public:
		/// @details Fills availibleEntities with every useable Entity
		EntityManager() {
			for(uint16_t i = 0; i < MAX_ENTITIES; i++) {
				availableEntities.push(i);
			}
		}
		/// @brief Returns the next availible Entity or INVALID on failure
		Entity create() {
			if(numLivingEntities > MAX_ENTITIES){
				std::cerr << "Reached maximum number of entities\n";

				return EntityManager::INVALID;
			}

			Entity entity = availableEntities.front();
			availableEntities.pop();

			return entity;
		}
		/// @brief Unassigns the entity's components
		/// @note Assumes the caller properly disposes/handles the now invalid Entity
		void destroy(const Entity& entity) {
			if(entity > MAX_ENTITIES){
				std::cerr << "Invalid entity ID\n";
				return;
			}

			compBitmasks[entity].reset();	// Clear bitmask

			availableEntities.push(entity);
			numLivingEntities--;
		}
		/// @brief Sets the entitiy's component bitmask
		void setComponents(const Entity& entity, const ComponentSet& components) {
			if(entity > MAX_ENTITIES){
				std::cerr << "Invalid entity ID\n";
				return;
			}
			std::cout << "Set entity, " << entity << "'s componentset to " << components << '\n';

			compBitmasks[entity] = components;
		}
		/// @brief Returns the entity's component bitmask
		const ComponentSet getComponents(const Entity& entity) const {
			if(entity > MAX_ENTITIES){
				std::cerr << "Invalid entity ID\n";
				return ComponentSet(0);
			}

			return compBitmasks[entity];
		}

		/// @brief Number representing an invalid Entity
		static const Entity INVALID = std::numeric_limits<Entity>::max();
	private:
		/// @brief The number of living(valid) entities
		Entity numLivingEntities;

		/// @brief Queue of unused Entities
		std::queue<Entity> availableEntities;

		// @brief Array of component bitmasks, corresponding to an entity
		std::array<ComponentSet, MAX_ENTITIES> compBitmasks;
};

/// @brief Interface for ComponentArray
class IComponentArray {
	public:
		virtual ~IComponentArray() = default;
		virtual void remove(const Entity& entity) = 0;
};

/// @brief Component array which holds a packed array of components
/// @details Uses a bitset of size MAX_ENTITIES to determine if a component at that bit position should be active
template<class T> class ComponentArray : public IComponentArray {
	public:
		ComponentArray() : validComponents(0) {}
		void add(const Entity& entity, const T component) {
			if(entityToComponent[entity]){
				std::cerr << "Entity \"" << entity << "\" already has component, doing nothing\n";
				return;
			}

			entityToComponent.set(entity);
			components[entity] = component;
			validComponents++;
		}
		void remove(const Entity& entity) override {
			entityToComponent.reset(entity);
			validComponents--;
		}
		/// @brief Returns entity's component struct, or nullptr if it doesn't exist
		T* get(const Entity& entity) {
			// I want to return a refrence, but there's no way to return invalid, I might just
			// return it always and assume that the caller knows if its "activated"(valid) or not
			return (entityToComponent.test(entity)) ? &components[entity] : nullptr;
		}
	private:
		/// @brief "Packed" component array, every entity has a component already created for it
		/// @details Components are only "activated" when their index is in the bitset
		std::array<T, MAX_ENTITIES> components;

		/// @brief Bitset mapping Entity's to their potential component
		/// @details A component for a given entity is activated when its position(the Entity number) is set
		std::bitset<MAX_ENTITIES> entityToComponent;

		/// @brief Number of valid entries in the array
		/// @attention Most likely useless, will probaly remove
		uint16_t validComponents;
};

/// @brief Manages ComponentArrays and Entity interactions with them
class ComponentManager {
	public:
		/// @brief Sets the first element of componentArrays to nullptr and availableID to 0
		ComponentManager() : componentArrays({ nullptr }), availableID(1) {}
		/// @brief Registers a component with the system
		/// @details Gets the component's hash_code with typeid() and adds it to the map
		/// @details Then Creates a new ComponentArray for the given component
		/// @returns The component bitmask
		template<class T> ComponentSet registerComponent() {
			const size_t type = typeid(T).hash_code();

			if(componentIDs.find(type) != componentIDs.end()){
				std::cerr << "Component already registered\n";
				return ComponentSet(0);
			}

			componentIDs[type] = availableID;	// Assign the component an ID
			componentArrays.push_back(new ComponentArray<T>());

			return ComponentSet().set(availableID++);	// Return bitmask
		}
		/// @brief Returns a component's ID and 0 if invalid
		template<class T> ComponentID getComponentID() {
			const size_t type = typeid(T).hash_code();

			return (componentIDs.find(type) != componentIDs.end()) ? componentIDs.at(type) : 0;
		}
		/// @brief Adds a component to an entity of type T
		template<class T> void addComponent(const Entity& entity, T component) {
			IComponentArray* componentArr = getComponentArray<T>();

			if(componentArr == nullptr){
				std::cerr << "Unknown/Unregistered component, doing nothing\n";
				return;
			}

			static_cast<ComponentArray<T>*>(componentArr)->add(entity, component);
			std::cout << "Added component to entity " << entity << '\n';
		}
		/// @brief Removes an entity's component of type T
		template<class T> void removeComponent(const Entity& entity) {
			IComponentArray* componentArr = getComponentArray<T>();

			if(componentArr == nullptr){
				std::cerr << "Unknown/Unregistered component, doing nothing\n";
				return;
			}

			static_cast<ComponentArray<T>*>(componentArr)->remove(entity);
		}
		/// @brief Gets an entity's component of type T and nullptr if invalid
		template<class T> T* getComponent(const Entity& entity) {
			IComponentArray* componentArr = getComponentArray<T>();

			if(componentArr == nullptr){
				std::cerr << "Unknown/Unregistered component, doing nothing\n";
				return nullptr;
			}

			return static_cast<ComponentArray<T>*>(componentArr)->get(entity);
		}
		/// @brief Removes an entity from every ComponentArray
		void removeEntity(const Entity& entity) {
			for(IComponentArray* componentArr : componentArrays) {
				componentArr->remove(entity);
			}
		}
		/// @brief Returns a ComponentArray(implicitly converted to IComponentID), or a nullptr
		/// @note getComponentID<T>() returns 0 on failure and componentArray's first element is nullptr
		template<class T> IComponentArray* getComponentArray() {
			return componentArrays.at(getComponentID<T>());
		}
	private:
		/// @brief Collection of ComponentArrays
		/// @note The first element will always be a nullptr
		std::vector<IComponentArray*> componentArrays;

		/// @brief Matches type hash codes to a component ID
		/// @note Potentially useless, I dont't see a reason for this, so will remove it if I can't find one
		/// @note I could just map the hash code to the ComponentArray
		std::unordered_map<size_t, ComponentID> componentIDs;

		/// @brief Next available ID for a component
		/// @note The first available ID is 1
		ComponentID availableID;
};

/// @brief Holds a transform matrix
/// @note If it is part of a child node, the transform matrix is in local space(ie. relative to the parent)
struct PositionComponent {
	glm::mat4x4 transform = glm::mat4x4(1.f);
};

/// @brief Holds a rigidbody
struct PhysicsComponent {
	btRigidBody* rigidbody = nullptr;

	~PhysicsComponent() {
		if(rigidbody)
			delete rigidbody;
	}
};

#include "../Mesh.hpp"
/// @brief Holds a vao, vbo, and ebo
struct MeshComponent {
	GLuint vao = 0;
	GLuint vbo = 0;
	GLuint ebo = 0;

	GLuint numIndices = 0;

	std::vector<Texture> textures = {};

	~MeshComponent() {
		glDeleteBuffers(1, &vao);
		glDeleteBuffers(1, &vbo);
		glDeleteBuffers(1, &ebo);
	}
};

/// @brief Holds a pointer to a `ModelComponent` and `PositionComponent`
struct RenderComponent {
	Model model;
	glm::vec3 scale = glm::vec3(1.f);
	bool visible = true;
};

/// @brief Holds a map of SDL_Scancode(keys) to functions(values)
/// @note The functions take either a PostitionComponent& or a PhysicsComponent& and a const Uint32 deltaTime(ms)
struct ControlComponent {
	std::unordered_map<SDL_Scancode, std::function<void(PositionComponent&, const Uint32&)>> positionKeyMap;
	std::unordered_map<SDL_Scancode, std::function<void(PhysicsComponent&, const Uint32&)>> physicsKeyMap;
};

class System {
	public:
		System() = default;
		virtual ~System() = default;

		/// @brief A unique set of entities
		/// @note Cheaper to have a set of actual values as pointers are 8 bytes on x64 systems
		std::set<Entity> entities;
};

/// @brief Controls input interactions
class InputSystem : public System {
	public:
		InputSystem(const Uint8* keyboardState, ComponentArray<PositionComponent>* positionCompArr, ComponentArray<PhysicsComponent>* physicsCompArr, ComponentArray<ControlComponent>* controlCompArr)
			: keyboardState(keyboardState), positionCompArr(positionCompArr), physicsCompArr(physicsCompArr), controlCompArr(controlCompArr) {}
		~InputSystem() {}
		/// @brief Initialize the system
		void initialize(const Uint8* keyboardState, ComponentArray<PositionComponent>* positionCompArr, ComponentArray<PhysicsComponent>* physicsCompArr, ComponentArray<ControlComponent>* controlCompArr) {
			this->keyboardState = keyboardState;
			this->positionCompArr = positionCompArr;
			this->physicsCompArr = physicsCompArr;
			this->controlCompArr = controlCompArr;
		}
		void tick(const Uint32& deltaTime) {
			for(const Entity& entity : entities) {
				ControlComponent* controlComponent = controlCompArr->get(entity);

				for(const auto& [scancode, function] : controlComponent->positionKeyMap) {
					if(keyboardState[scancode]){
						PositionComponent* positionComponent = positionCompArr->get(entity);

						function(*positionComponent, deltaTime);
					}
				}

				for(const auto& [scancode, function] : controlComponent->physicsKeyMap) {
					if(keyboardState[scancode]){
						PhysicsComponent* physicsComponent = physicsCompArr->get(entity);

						function(*physicsComponent, deltaTime);
					}
				}
			}
		}
	private:
		const Uint8* keyboardState;

		ComponentArray<PositionComponent>* positionCompArr;
		ComponentArray<PhysicsComponent>* physicsCompArr;
		ComponentArray<ControlComponent>* controlCompArr;
};

/// @brief Controls physics interactions
/// @details holds everything required to host a physics world
class PhysicsSystem : public System {
	public:
		PhysicsSystem(ComponentArray<PositionComponent>* positionCompArr, ComponentArray<PhysicsComponent>* physicsCompArr, const std::string& initialStatePath = "")
			: positionCompArr(positionCompArr), physicsCompArr(physicsCompArr), debugDrawer(new PhysicsDrawer()) {
				// Initialize bullet subsystems
				collisionConfig = new btDefaultCollisionConfiguration();
				dispatcher = new btCollisionDispatcher(collisionConfig);
				interface = new btDbvtBroadphase();
				solver = new btSequentialImpulseConstraintSolver();
				dynamicsWorld = new btDiscreteDynamicsWorld(dispatcher, interface, solver, collisionConfig);

				dynamicsWorld->setGravity(btVector3(0.f, -10.f, 0.f));
				dynamicsWorld->setDebugDrawer(debugDrawer);

				if(initialStatePath.compare("") != 0)
					loadState(initialStatePath);

				saveState("./saves/initState.bin");
			}
		~PhysicsSystem() {}
		void tick(const Uint32& deltaTime) {
			dynamicsWorld->stepSimulation(deltaTime / 1000.f, 10);

			for(const Entity& entity : entities) {
				PhysicsComponent* physicsComp = physicsCompArr->get(entity);
				PositionComponent* positionComp = positionCompArr->get(entity);

				const btMotionState* motionState = physicsComp->rigidbody->getMotionState();

				btTransform physicsTransform;
				motionState->getWorldTransform(physicsTransform);

				// Get physics(bullet) components
				btVector3 physicsPos = physicsTransform.getOrigin();
				btQuaternion physicsRot = physicsTransform.getRotation();

				// Calculate world transform
				glm::mat4 worldTransform = glm::mat4(1.0f);
				worldTransform = glm::translate(	// Apply position
					worldTransform,
					glm::vec3(
						physicsPos.getX(),
						physicsPos.getY(),
						physicsPos.getZ()
					)
				);
				worldTransform = (
					glm::mat4_cast(	// Apply rotation
						glm::quat(
							physicsRot.getW(),
							physicsRot.getX(),
							physicsRot.getY(),
							physicsRot.getZ()
						)
					) * worldTransform
				);

				positionComp->transform = worldTransform;
			}
		}
		/// @brief Casts a ray from `origin` with a heading of `direction` and length of `len`
		/// @returns A pointer to the hit rigidbody or nullptr if there's no collision
		const btRigidBody* castRay(const glm::vec3& origin, const glm::vec3& direction, const float len) {
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

					return body;
				}
			}
			return nullptr;
		}
		/// @brief Use the physics debugger to draw with the given debug level
		void debugDraw(const glm::mat4& cameraView, const float& cameraFOV, const int debugMode) {
			debugDrawer->setDebugMode(debugMode);
			debugDrawer->setCamera(cameraView, cameraFOV);

			dynamicsWorld->debugDrawWorld();
		}
		/// @brief Resets the simulation to its starting state
		/// @note Attempts to load initState.bin from the saves folder
		void reset() {
			loadState("saves/initState.bin");
		}
		/// @brief Saves the current state of the physics engine to a file
		void saveState(const std::string& filename) {
			btDefaultSerializer* serializer = new btDefaultSerializer();

			dynamicsWorld->serialize(serializer);

			std::ofstream file(filename, std::ios::binary);
			if(file.is_open()){
				file.write(reinterpret_cast<const char*>(serializer->getBufferPointer()), serializer->getCurrentBufferSize());
				file.close();
			} else {
				std::cerr << "PhysicsSystem::saveState(): Unable to create file at \"" + filename + "\"\n";
			}

			delete serializer;
		}
		/// @brief Loads a saved state of the physics engine
		/// @throws runtime_error if it fails to deserialize the file
		void loadState(const std::string& filename) {
			// Attemp to load the file before deleting everything
			int bufferSize;
			unsigned char* data = getFileData(filename, bufferSize);
			if(data == nullptr){
				return;
			}

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

			// Recreate
			collisionConfig = new btDefaultCollisionConfiguration();
			dispatcher = new btCollisionDispatcher(collisionConfig);
			interface = new btDbvtBroadphase();
			solver = new btSequentialImpulseConstraintSolver();
			dynamicsWorld = new btDiscreteDynamicsWorld(dispatcher, interface, solver, collisionConfig);

			btBulletWorldImporter* importer = new btBulletWorldImporter(dynamicsWorld);

			if(!importer->loadFileFromMemory(reinterpret_cast<char*>(data), bufferSize))
				throw std::runtime_error("PhysicsSystem::loadState(): Unable to deserialize file at \"" + filename + '"');

			dynamicsWorld->setDebugDrawer(debugDrawer);

			for(int i = 0; i < dynamicsWorld->getNumCollisionObjects(); i++) {
				btCollisionObject* obj = dynamicsWorld->getCollisionObjectArray()[i];
				objArray.push_back(obj->getCollisionShape());
			}

			delete[] data;
			delete importer;
		}
	private:
		/// @brief Loads a file into memory and returns its data
		/// @param filename The file path to load
		/// @param bufferSize A variable to hold the file size
		/// @returns Raw file data and file size through the `bufferSize` parameter
		unsigned char* getFileData(const std::string& filename, int& bufferSize) {
			std::ifstream file(filename, std::ios::binary | std::ios::ate);	// Open binary file at EOF

			if(file.is_open()){
				bufferSize = file.tellg();
				file.seekg(0, std::ios::beg);

				unsigned char* buffer = new unsigned char[bufferSize];
				file.read(reinterpret_cast<char*>(buffer), bufferSize);
				file.close();

				return buffer;
			} else {
				std::cerr << "PhysicsSystem::getFileData(): Unable to open/load file at \"" + filename + "\"\n";
				return nullptr;
			}
		}

		// Component dependencies
		ComponentArray<PositionComponent>* positionCompArr;
		ComponentArray<PhysicsComponent>* physicsCompArr;

		// Bullet Physics Members
		btDefaultCollisionConfiguration* collisionConfig;	// Default memory and collision setup
		btCollisionDispatcher* dispatcher;					// Collision handler
		btBroadphaseInterface* interface;					// AABB collision detection interface
		btSequentialImpulseConstraintSolver* solver;		// Constraint solver
		btDiscreteDynamicsWorld* dynamicsWorld;				// Dynamics world
		btAlignedObjectArray<btCollisionShape*> objArray;	// Collision shape array

		PhysicsDrawer* debugDrawer;
};

/// @brief Controls graphics
class GraphicsSystem : public System {
	public:
		GraphicsSystem(ComponentArray<PositionComponent>* positionCompArr, ComponentArray<RenderComponent>* renderCompArr)
			: positionCompArr(positionCompArr), renderCompArr(renderCompArr){}
		~GraphicsSystem() {}
		void tick(BaseShader& shader, const glm::mat4x4& cameraView, const float& fov) {
			shader.bind();

			for(const Entity& entity : entities) {
				RenderComponent* renderComp = renderCompArr->get(entity);
				if(!renderComp->visible)	// Assume "entities" has a valid set
					continue;

				// Also assume "entity" fulfils this system's dependencies
				PositionComponent* positionComp = positionCompArr->get(entity);

				shader.setScale(
					renderComp->scale.x,
					renderComp->scale.y,
					renderComp->scale.z
				);
				shader.perspective(positionComp->transform, cameraView, fov);
				renderComp->model.draw(shader);
			}

			GLenum err = glGetError();
			if(err != GL_NO_ERROR) {
				std::cerr << "GraphicsSystem ERROR: Unhandled OpenGL Error: " << err << std::endl;
			}
		}
	private:
		ComponentArray<PositionComponent>* positionCompArr;
		ComponentArray<RenderComponent>* renderCompArr;
};

/// @brief Manages Systems
class SystemManager {
	public:
		/// @brief Sets the first element of componentArrays to nullptr and availableID to 0
		SystemManager() : systems() {}
		/// @brief Registers a component with the system
		/// @details Gets the component's hash_code with typeid() and creates a system of type T
		/// @details Then adds them to the map
		/// @details Uses `dependencies` to mark the systems dependencies in the "systemDependencies" map
		/// @param args Variable list of arguments to past to the system upon creation
		template<class T, typename... Args> T* registerSystem(const ComponentSet& dependencies, const Args... args) {
			const size_t type = typeid(T).hash_code();

			if(systems.find(type) != systems.end()){
				std::cerr << "System already registered\n";
			} else {
				systems[type] = std::make_unique<T>(args...);
				systemDependencies[type] = dependencies;
			}

			return (T*)systems.at(type).get();
		}
		/// @brief Returns a pointer to the given system
		template<class T> T* getSystem() {
			try {
				return (T*)systems.at(typeid(T).hash_code()).get();
			} catch(std::out_of_range) {
				std::cerr << "SystemManager getSystem() ERROR: Class \"" << typeid(T).name() << "\"(hashcode " << typeid(T).hash_code() << ") is not registered\n";
				return nullptr;
			}
		}
		/// @brief Changes every system's entity list to match its new component set
		void entityChanged(const Entity& entity, const ComponentSet& componentSet) {
			std::cout << "Entity " << entity << " Changed, with componentset of " << componentSet << '\n';
			for(auto& [id, system] : systems) {
				const ComponentSet& components = systemDependencies.at(id);

				if((componentSet & components) == components){
					system.get()->entities.insert(entity);
					std::cout << "Added entity to system with componentset of " << components << '\n';
				} else {
					std::cout << "Removed entity from system with componentset of " << components << '\n';
					system.get()->entities.erase(entity);
				}
			}
		}
		/// @brief Removes an entity from every System
		void removeEntity(const Entity& entity) {
			for(auto& [hash, system] : systems) {
				system.get()->entities.erase(entity);
			}
		}
	private:
		/// @brief Matches system hash codes to a ComponentSet, declaring it's component dependencies
		std::unordered_map<size_t, ComponentSet> systemDependencies;

		/// @brief Matches type hash codes to a system
		std::unordered_map<size_t, std::unique_ptr<System>> systems;
};

///
/// Utilities
///
std::unordered_map<std::string_view, std::pair<ComponentID, std::function<void(const Entity&, ComponentManager&)>>> loadMap;

/// @brief Registers a component's add function with its name
void registerComponentName(const std::string_view& name, const ComponentID& id, const std::function<void(const Entity&, ComponentManager&)>& addFunction) {
	loadMap[name] = std::pair<ComponentID, std::function<void(const Entity&, ComponentManager&)>>(id, addFunction);
}

/// @brief Constructs and registers an entity and its components from a json prefab
Entity loadEntityFromPrefab(const std::string& path, EntityManager& entityManager, ComponentManager& compManager, SystemManager& sysManager) {
	std::ifstream docBuffer(path, std::ifstream::binary);
	Json::Value root;
	docBuffer >> root;

	Json::Reader reader;
	Json::IStringStream jsonBuffer;
	if(!reader.parse(jsonBuffer, root, false)){
		std::cerr << "Unable to load json at \"" << path << "\"\n";
		std::cerr << reader.getFormattedErrorMessages() << '\n';

		return EntityManager::INVALID;
	}

	Entity entity = entityManager.create();
	if(entity == EntityManager::INVALID)
		return EntityManager::INVALID;

	const std::string name = root["name"].asString();	// Unused right now
	ComponentSet components(0);
	for(const Json::Value& comp : root["components"]) {
		if(loadMap.find(comp.asString()) != loadMap.end()){
			const std::pair<ComponentID, std::function<void(const Entity&, ComponentManager&)>>& component = loadMap.find(comp.asString())->second;

			component.second(entity, compManager);
			components.set(component.first);
		} else {
			std::cerr << "Unable to load component \"" << comp.asString() << "\". Component not registered in map, skipping\n";
		}
	}

	return entity;
}
