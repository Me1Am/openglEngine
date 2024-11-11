#pragma once

#include <type_traits>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <bitset>
#include <map>

#include "Node.hpp"
#include "Components.hpp"
#include "Systems.hpp"

class GlobalManager {
	#define MAX_COMPONENTS 32

	public:
		GlobalManager(const Uint8* keyboard) {
			graphics = new GraphicsSystem(
				*getComponentManager<PositionManager>(),
				*getComponentManager<RenderManager>()
			);
			physics = new PhysicsSystem(
				*getComponentManager<PositionManager>(),
				*getComponentManager<PhysicsManager>(),
				*getComponentManager<ControlManager>()
			);
			input = new InputSystem(
				keyboard,
				*getComponentManager<PositionManager>(),
				*getComponentManager<PhysicsManager>(),
				*getComponentManager<ControlManager>()
			);

			components[CONTROL] = new ControlManager();
			components[POSITION] = new PositionManager();
			components[PHYSICS] = new PhysicsManager();
			components[RENDER] = new RenderManager();
		}
		~GlobalManager() {
			delete graphics;
			delete physics;
			delete input;
		}
		void registerNode(Node* node) {
			nodes.push_back(node);
		}
		/// @brief Removes the node with the `uid` from the internal vector
		/// @note Does not removes the node's components
		Node* unregisterNode(const unsigned int& uid) {
			auto it = std::find_if(
				nodes.begin(), 
				nodes.end(), 
				[uid](const Node* node) { return node->UID == uid; }
			);

			if(it == nodes.end()){
				std::cerr << "Object does not exists with uid \"" << uid << "\"n";
				return nullptr;
			}
			nodes.erase(it);

			return *it;
		}
		/// @brief Returns a pointer to the node with the given uid
		Node* getNode(const unsigned int& uid) {
			auto it = std::find_if(
				nodes.begin(), 
				nodes.end(), 
				[uid](const Node* node) { return node->UID == uid; }
			);

			return (it == nodes.end()) ? nullptr: *it;
		}
		/// @brief Registers a component with a given uid
		template<class T> void registerComponent(const unsigned int& uid, const Components type, const T& component = T()) {
			// Ugly and absolutley terrible code
			// Uses constexpr to change what it casts to
			// TODO Find a way to use a virtual function in IComponentManager to prevent this
			if constexpr(std::is_same_v<T, ControlComponent>){
				((ControlManager*)components[CONTROL])->addComponent(uid, component);
			} else if constexpr(std::is_same_v<T, PositionComponent>){
				std::cout << "AYA\n";
				((PositionManager*)components[POSITION])->addComponent(uid, component);
			} else if constexpr(std::is_same_v<T, PhysicsComponent>){
				((PhysicsManager*)components[PHYSICS])->addComponent(uid, component);
			} else if constexpr(std::is_same_v<T, RenderComponent>){
				((RenderManager*)components[RENDER])->addComponent(uid, component);
			} else {
				static_assert(std::is_same_v<T, void>, "Unknown component");
			}
		}
		/// @brief Returns the component bitmask for the given uid
		const std::bitset<MAX_COMPONENTS> getComponentMask(const unsigned int& uid) {
			try {
				return componentBitmasks.at(uid);
			} catch(std::out_of_range) {
				std::cerr << "GlobalManager::getComponentMask(): No known component bitmask for uid \"" << uid << "\"\n";

				return std::bitset<MAX_COMPONENTS>(0);
			}
		}
		void tick(BaseShader& shader, const Uint32& deltaTime, const glm::mat4x4& cameraView, const float& fov) {
			input->tick(deltaTime);
			physics->tick(deltaTime);

			graphics->draw(shader, cameraView, fov);
		}
	private:
		/// @brief Returns the component manager for the given type
		/// @attention Not type safe, make sure to match the type and 
		template<class T> T* getComponentManager() {
			// Absolutely ugly and terrible, see 'registerComponent()'
			if constexpr(std::is_same_v<T, ControlManager>){
				return ((ControlManager*)components[CONTROL]);
			} else if constexpr(std::is_same_v<T, PositionManager>){
				return ((PositionManager*)components[POSITION]);
			} else if constexpr(std::is_same_v<T, PhysicsManager>){
				return ((PhysicsManager*)components[PHYSICS]);
			} else if constexpr(std::is_same_v<T, RenderManager>){
				return ((RenderManager*)components[RENDER]);
			} else {
				static_assert(std::is_same_v<T, void>, "Unknown component");
			}
		}

		GraphicsSystem* graphics = NULL;
		PhysicsSystem* physics = NULL;
		InputSystem* input = NULL;

		/// Active nodes that are being processed
		std::vector<Node*> nodes;

		/// Maps bitmask positions to components (managers)
		std::map<unsigned int, IComponentManager*> components;

		/// Maps UIDs to components with a bitmask, holds a maximum of 32 components
		std::unordered_map<unsigned int, std::bitset<MAX_COMPONENTS>> componentBitmasks;
};

/** JSON GRAVEYARD
		///
		/// Node Saving
		///

		/// @brief Saves a node to a json file
		void saveNode(const Node* node, const std::string& path) {
			Json::Value root = nodeToJson(node);

			Json::StreamWriterBuilder writer;
			std::ofstream file_id(path);

			file_id << Json::writeString(writer, root);
			file_id.close();
		}
		/// @brief Appends component data to the given json object
		void saveComponent(Json::Value& root) {
			root["components"] = Json::Value(Json::arrayValue);
			std::bitset<MAX_COMPONENTS> bitmask = getComponentMask((unsigned int)root["uid"].asInt());

			for(const auto& [pos, manager] : components) {
				if(bitmask.test(pos)){
					root["components"].append(manager->componentToJson((unsigned int)root["uid"].asInt()));

					root["components"][root["components"].size()-1]["pos"] = pos;
				}
			}

			for(Json::Value& child : root["childs"]) {
				saveComponent(child);
			}
		}

		///
		/// Node Loading
		///

		/// @brief Creates a `Node` pointer from a json file
		/// @param path The path to the json file
		Node* loadNode(const std::string& path) {
			std::ifstream docBuffer(path, std::ifstream::binary);
			Json::Value root;
			docBuffer >> root;

			Json::Reader reader;
			Json::IStringStream jsonBuffer;
			if(!reader.parse(jsonBuffer, root, false)){
				std::cerr << "NodeLoader::loadNode(): Unable to load json at \"" << path << "\"\n";
				std::cerr << reader.getFormattedErrorMessages() << '\n';

				return nullptr;
			}

			return jsonToNode(root);
		}

		/// @brief Creates json object from the given node
		/// @note Recursive function
		Json::Value nodeToJson(const Node* node) {
			Json::Value root;

			root["uid"] = node->UID;
			root["parent"] = (node->parent != nullptr) ? node->parent->UID : 0;

			Json::Value childsJson(Json::arrayValue);
			if(node->childs.size() != 0) {
				for(Node* child : node->childs){
					childsJson.append(nodeToJson(child));
				}
			}
			root["childs"] = childsJson;

			return root;
		}
		/// @brief Creates a Node* from the given json object
		/// @note Recursive function
		Node* jsonToNode(const Json::Value& root) {
			Node* node = new Node();

			node->UID = (unsigned int)root["uid"].asInt();
			node->parent = (root["parent"].asInt() != 0) ? getNode(root["parent"].asInt()) : nullptr;

			for(const Json::Value& component : root["components"]) {
				loadComponent(node->UID, component);
			}

			for(const Json::Value& child : root["childs"]) {
				node->childs.push_back(jsonToNode(child));
			}

			return node;
		}
		/// @brief Registers a component to the given node from the given json object
		void loadComponent(const unsigned int& uid, const Json::Value& root) {
			IComponentManager* manager = components.at(root["pos"].asInt());

			switch(root["pos"].asInt()) {
				case(1):
					break;
			}



			//manager.addComponent(uid, root);
		}
 * */
