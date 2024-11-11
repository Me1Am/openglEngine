#pragma once

#include "Components.hpp"

#include "../shader/BaseShader.hpp"

/// @brief Controls input interactions
class InputSystem {
	public:
		InputSystem(const Uint8* keyboardState, PositionManager& positionManager, PhysicsManager& physicsManager, ControlManager& controlManager) 
			: keyboardState(keyboardState), positionManager(positionManager), physicsManager(physicsManager), controlManager(controlManager) {}
		~InputSystem() {}
		void tick(const Uint32& deltaTime) {
			for(const auto& [uid, controlComponent] : controlManager.getComponents()) {
				for(const auto& [scancode, function] : controlComponent.keyMap) {
					if(keyboardState[scancode]){
						PositionComponent* position = positionManager.getComponent(uid);

						if(position){
							function(*position);
						}
					}
				}
			}
		}
	private:
		const Uint8* keyboardState;

		PositionManager& positionManager;
		PhysicsManager& physicsManager;
		ControlManager& controlManager;
};

/// @brief Controls physics interactions
// TODO Move functionality from PhysicsEngine here
class PhysicsSystem {
	public:
		PhysicsSystem(PositionManager& positionManager, PhysicsManager& physicsManager, ControlManager& controlManager) 
			: positionManager(positionManager), physicsManager(physicsManager) {}
		~PhysicsSystem() {}
		void tick(const Uint32& deltaTime) {
			for(const auto& [uid, physicsComponent] : physicsManager.getComponents()) {
				const btMotionState* motionState = physicsComponent.rigidbody->getMotionState();

				PositionComponent* position = positionManager.getComponent(uid);
				if(position == nullptr)
					continue;

				btTransform physicsTransform;
				motionState->getWorldTransform(physicsTransform);

				// Get physics(bullet) components
				btVector3 physicsPos = physicsTransform.getOrigin();
				btQuaternion physicsRot = physicsTransform.getRotation();

				// Calculate world transform
				glm::mat4 worldTransform = glm::mat4(1.0f);
				worldTransform = glm::translate(
					worldTransform, 
					glm::vec3(
						physicsPos.getX(), 
						physicsPos.getY(), 
						physicsPos.getZ()
					)
				);
				worldTransform = (
					glm::mat4_cast(
						glm::quat(
							physicsRot.getW(), 
							physicsRot.getX(), 
							physicsRot.getY(), 
							physicsRot.getZ()
						)
					) * worldTransform
				);

				position->transform = worldTransform;
			}
		}
	private:
		PositionManager& positionManager;
		PhysicsManager& physicsManager;
};

/// @brief Controls graphics
class GraphicsSystem {
	public:
		GraphicsSystem(PositionManager& positionManager, RenderManager& renderManager) 
			: positionManager(positionManager), renderManager(renderManager) {}
		~GraphicsSystem() {}
		void draw(BaseShader& shader, const glm::mat4x4& cameraView, const float& fov) {
			shader.bind();

			for(const auto& [uid, renderComponent] : renderManager.getComponents()) {
				if(!renderComponent.visible)
					continue;

				PositionComponent* position = positionManager.getComponent(uid);
				ModelComponent* model = renderComponent.modelRef;

				if(position == nullptr || model == nullptr)
					continue;

				shader.setScale(
					renderComponent.scale.x, 
					renderComponent.scale.y, 
					renderComponent.scale.z
				);
				shader.perspective(position->transform, cameraView, fov);

				for(const MeshComponent* mesh : model->meshes) {
					glBindVertexArray(mesh->vao);
					glDrawElements(GL_TRIANGLES, mesh->numIndices, GL_UNSIGNED_INT, 0);
				}
				glBindVertexArray(0);
			}
		}
	private:
		PositionManager& positionManager;
		RenderManager& renderManager;
};
