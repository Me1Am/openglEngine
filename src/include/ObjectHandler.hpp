#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <vector>
#include <string>
#include <memory>
#include <cmath>

#include "GameObject.hpp"

/**
 * @brief Utility class which handles GameObjects and their processes
*/
class ObjectHandler {
	public:
		/**
		 * @brief Removes a GameObject
		 * @param uniqueID The 
		 * @note Runs the GameObject's deconstructor
		*/
		static void deleteObject(short uniqueID) {
			std::vector<std::unique_ptr<GameObject>>::iterator objectToDelete;
			bool clean = false; // Used for error statement

			// Clean the objectList of null_ptrs
			if(uniqueID == -1){
				auto iter = objectList.begin();
				while((iter = std::find_if(iter, objectList.end(), [](const std::unique_ptr<GameObject>& obj) { return obj == nullptr; })) != objectList.end()) {
					objectToDelete->reset();	// Runs deconstructor for GameObject
					objectList.erase(objectToDelete);

					iter++;
				}
			} else {
				// Return iterator for unique_ptr with object that matches uniqueID
				objectToDelete = std::find_if(objectList.begin(), objectList.end(),
					[uniqueID](const std::unique_ptr<GameObject>& obj) { return obj != nullptr && obj->getUniqueID() == uniqueID; });

				// If the iterator is on an object
				if(objectToDelete != objectList.end()) {
					objectToDelete->reset();
					objectList.erase(objectToDelete);

					std::cout << "Object Deleted" << std::endl;
				} else {
					std::cerr << "Failed to Delete Object" << std::endl;
				}
			}
        }
		/**
		 * @brief Creates a new GameObject in objectList
		 * @param modelPath The path to the GameObject's model
		 * @param initPos The Pos struct representing the initial position
		 * @param constants The Constants struct representing the constants
		 * @param args Optional parameter(s), use this for passing extra data to the constructor
		 * @return The unique ID of the new object
		 * @note The unique ID from the constants struct is overwritten
		 * @note 'args' must be first and in the same order as they are in the GameObject's constructor
		 */
		template<class Object, typename... Args>
		static short newGameObject(std::string modelPath, Pos initPos, Constants constants = Constants(0), Args... args) {
			short uniqueID;
			do {	// Calculate uniqueID
				uniqueID = rand();
			} while(std::find_if(uniqueIDs.begin(), uniqueIDs.end(), [uniqueID](short& uID) { return uID == uniqueID; }) != uniqueIDs.end());

			objectList.push_back(std::unique_ptr<Object>(new Object(args..., initPos, { uniqueID, constants.MIN_VELOCITY, constants.MAX_VELOCITY }, modelPath)));
			
			return uniqueID;
		}
		/**
		 * @brief Runs the update functions for each GameObject
		 * @param deltaTime The time since last frame, in miliseconds
		*/
		static void tick(float deltaTime) {
			for(long long unsigned int i = 0; i < objectList.size(); i++) {
				auto pointer = objectList[i].get();
				pointer->tick(deltaTime);
			}
		}
		/**
		 * @brief Gets a pointer to the GameObject
		 * @param uniqueID The unique ID of the desired GameObject
		 * @return A pointer to the desired GameObject
		*/
		static GameObject* getGameObject(const short uniqueID) {
			auto iter = std::find_if(objectList.begin(), objectList.end(), 
				[uniqueID](const std::unique_ptr<GameObject>& obj) { return obj != nullptr && obj->getUniqueID() == uniqueID; });
			return objectList[std::distance(objectList.begin(), iter)].get();
		}
		
		static std::vector<std::unique_ptr<GameObject>> objectList;	// Vector with unique_ptrs holding a GameObject
	private:
		static std::vector<short> uniqueIDs;
};

std::vector<short> ObjectHandler::uniqueIDs;
std::vector<std::unique_ptr<GameObject>> ObjectHandler::objectList;