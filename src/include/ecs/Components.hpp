#pragma once

#include <GL/glew.h>
#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <bullet/BulletDynamics/Dynamics/btRigidBody.h>
#include <SDL2/SDL_scancode.h>

#include <iostream>
#include <vector>
#include <unordered_map>
#include <functional>

/// @brief Holds a transform matrix
/// @note If it is part of a child node, the transform matrix is in local space(ie. relative to the parent)
struct PositionComponent {
	glm::mat4x4 transform = glm::mat4x4(1.f);
};

/// @brief Holds a rigidbody and pointer to a node's `PositionComponent`
struct PhysicsComponent {
	btRigidBody* rigidbody = nullptr;

	PositionComponent* positionRef = nullptr;

	~PhysicsComponent() {
		if(rigidbody)
			delete rigidbody;
		if(positionRef)
			delete positionRef;
	}
};

/// @brief Holds a vao, vbo, and ebo
struct MeshComponent {
	GLuint vao = 0;
	GLuint vbo = 0;
	GLuint ebo = 0;

	GLuint numIndices;

	~MeshComponent() {
		glDeleteBuffers(1, &vao);
		glDeleteBuffers(1, &vbo);
		glDeleteBuffers(1, &ebo);
	}
};

/// @brief Holds a vector of mesh pointers
struct ModelComponent {
	std::vector<MeshComponent*> meshes;

	std::string modelPath;

	~ModelComponent() {
		for(MeshComponent* mesh : meshes) {
			delete mesh;
		}
	}
};

/// @brief Holds a pointer to a `ModelComponent` and `PositionComponent`
struct RenderComponent {
	glm::vec3 scale = glm::vec3(1.f);
	bool visible = true;

	ModelComponent* modelRef;
	PositionComponent* positionRef = nullptr;

	~RenderComponent() {
		if(modelRef)
			delete modelRef;
	}
};

/// @brief Holds a map of SDL_Scancode(keys) to functions(values)
/// @note The funcitons are void and take a `PositionComponent` refrence
struct ControlComponent {
	// TODO Allow functions to modify a physics component
	std::unordered_map<SDL_Scancode, std::function<void(PositionComponent&)>> keyMap;	// Accociate keys with an action
};

enum Components {
	CONTROL = 0, 
	POSITION = 1, 
	PHYSICS = 2, 
	RENDER = 3, 
};

class IComponentManager {
	public:
		virtual ~IComponentManager() = default;
		//template<class T> void addComponent(const unsigned int& uid, const T& component);
		virtual void removeComponent(const unsigned int& uid) = 0;
		virtual void clear() = 0;
};

class PositionManager : public IComponentManager {
	public:
		void addComponent(const unsigned int& uid, const PositionComponent& component) {
			components[uid] = component;
		}
		PositionComponent* getComponent(const unsigned int& uid) {
			auto it = components.find(uid);

			if(it != components.end()){
				return &it->second;
			}
			return nullptr;
		}
		void removeComponent(const unsigned int& uid) override {
			auto it = components.find(uid);

			if(it != components.end()){
				components.erase(it);
			} else {
				std::cerr << "Component binded with uid \"" << uid << "\" does not exist\n";
			}
		}
		void clear() override {
			components.clear();
		}
		const std::unordered_map<unsigned int, PositionComponent>& getComponents() const {
			return components;
		}
		unsigned int getNumComponents() const {
			return components.size();
		}
	private:
		std::unordered_map<unsigned int, PositionComponent> components;
};

class PhysicsManager : public IComponentManager {
	public:
		void addComponent(const unsigned int& uid, const PhysicsComponent& component) {
			components[uid] = component;
		}
		PhysicsComponent* getComponent(const unsigned int& uid) {
			auto it = components.find(uid);

			if(it != components.end()){
				return &it->second;
			}
			return nullptr;
		}
		void removeComponent(const unsigned int& uid) override {
			auto it = components.find(uid);

			if(it != components.end()){
				components.erase(it);
			} else {
				std::cerr << "Component binded with uid \"" << uid << "\" does not exist\n";
			}
		}
		void clear() override {
			components.clear();
		}
		const std::unordered_map<unsigned int, PhysicsComponent>& getComponents() const {
			return components;
		}
		unsigned int getNumComponents() const {
			return components.size();
		}
	private:
		std::unordered_map<unsigned int, PhysicsComponent> components;
};

class RenderManager : public IComponentManager {
	public:
		void addComponent(const unsigned int& uid, const RenderComponent& component) {
			components[uid] = component;
		}
		RenderComponent* getComponent(const unsigned int& uid) {
			auto it = components.find(uid);

			if(it != components.end()){
				return &it->second;
			}
			return nullptr;
		}
		void removeComponent(const unsigned int& uid) override {
			auto it = components.find(uid);

			if(it != components.end()){
				components.erase(it);
			} else {
				std::cerr << "Component binded with uid \"" << uid << "\" does not exist\n";
			}
		}
		void clear() override {
			components.clear();
		}
		const std::unordered_map<unsigned int, RenderComponent>& getComponents() const {
			return components;
		}
		unsigned int getNumComponents() const {
			return components.size();
		}
	private:
		std::unordered_map<unsigned int, RenderComponent> components;
};

class ControlManager : public IComponentManager {
	public:
		void addComponent(const unsigned int& uid, const ControlComponent& component) {
			components[uid] = component;
		}
		ControlComponent* getComponent(const unsigned int& uid) {
			auto it = components.find(uid);

			if(it != components.end()){
				return &it->second;
			}
			return nullptr;
		}
		void removeComponent(const unsigned int& uid) override {
			auto it = components.find(uid);

			if(it != components.end()){
				components.erase(it);
			} else {
				std::cerr << "Component binded with uid \"" << uid << "\" does not exist\n";
			}
		}
		void clear() override {
			components.clear();
		}
		const std::unordered_map<unsigned int, ControlComponent>& getComponents() const {
			return components;
		}
		unsigned int getNumComponents() const {
			return components.size();
		}
	private:
		std::unordered_map<unsigned int, ControlComponent> components;
};
