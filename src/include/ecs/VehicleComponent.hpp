#pragma once

#include <bullet/BulletDynamics/Dynamics/btDynamicsWorld.h>
#include <bullet/BulletDynamics/Vehicle/btRaycastVehicle.h>
#include <bullet/BulletDynamics/Vehicle/btVehicleRaycaster.h>
#include <bullet/LinearMath/btVector3.h>
#include <bullet/LinearMath/btScalar.h>

#include <bitset>

#include "ECS.hpp"

/// @brief Holds data relating to a wheel
struct WheelPhysicsData {
    btVector3 connectionPoint;  // Wheel connection, relative to the chassis
    btVector3 wheelDirection;   // Suspension direction(usually down), relative to the chassis
    btVector3 axis;             // Wheel rotation axis

    float radius;               // Radius of the wheel
    float rollInfluence;        // Reduces body roll at high speeds
    float suspensionRestLength; // Length of the suspension at rest
    btRaycastVehicle::btVehicleTuning suspension; // Various suspension settings

    // Constructor with default values
    WheelPhysicsData(const btVector3& connection, const btVector3& direction = btVector3(0, -1, 0),
                     const btVector3& axle = btVector3(-1, 0, 0), const btScalar radius = 0.5f,
                     const btScalar suspensionRestLength = 0.6f,
                     const btRaycastVehicle::btVehicleTuning& suspensionSettings = btRaycastVehicle::btVehicleTuning())
        : connectionPoint(connection), wheelDirection(direction), axis(axle), radius(radius),
          rollInfluence(0.1f), suspensionRestLength(suspensionRestLength), suspension(suspensionSettings) {}
};

/// @brief Holds data relating to a raycast vehicle
struct VehicleComponent : PhysicsComponent {
    btVehicleRaycaster* vehicleRaycaster = nullptr; // Raycaster for simulating wheel contact
    btRaycastVehicle* vehicle = nullptr;            // Bullet vehicle object for car physics
    btRaycastVehicle::btVehicleTuning tuning = btRaycastVehicle::btVehicleTuning();       // Vehicle-unique physics settings

    btScalar engineForce = 0.f;     // Current engine force
    btScalar brakeForce = 0.f;      // Current brake force
    btScalar steeringAngle = 0.f;   // Current steering angle, in radians

    // Suspension properties specific to car dynamics
    float suspensionStiffness = 5.88f;      // Suspension stiffness for stability
    float suspensionDamping = 0.88f;        // Damping factor for suspension
    float suspensionCompression = 0.83f;    // Compression factor for suspension

    VehicleComponent(btDynamicsWorld* dynamicsWorld, const std::vector<std::pair<WheelPhysicsData, bool>>& wheels = {})
        : vehicleRaycaster(new btDefaultVehicleRaycaster(dynamicsWorld)),
          vehicle(new btRaycastVehicle(tuning, rigidbody, vehicleRaycaster)) {
        // Add wheels to the vehicle
        for(const auto [wheel, canSteer] : wheels) {
            addWheel(wheel, canSteer);
        }
    }
    // Destructor to clean up Bullet objects
    ~VehicleComponent() {
        delete vehicle;
        delete vehicleRaycaster;
    }
    void addWheel(const WheelPhysicsData& wheel, const bool canSteer = true) {
        btWheelInfo& wheelInfo = vehicle->addWheel(
            wheel.connectionPoint,
            wheel.wheelDirection,
            wheel.axis,
            wheel.suspensionRestLength,
            wheel.radius,
            wheel.suspension,
            canSteer
        );

        wheelInfo.m_rollInfluence = wheel.rollInfluence;
    }
    void applyEngineForce(btScalar force) {
        engineForce = force;
        for(int i = 0; i < vehicle->getNumWheels(); i++) {
            vehicle->applyEngineForce(engineForce, i);
        }
    }
    void setBrakeForce(btScalar force) {
        brakeForce = force;
        for(int i = 0; i < vehicle->getNumWheels(); i++) {
            vehicle->setBrake(brakeForce, i);
        }
    }
    void setSteeringAngle(btScalar angle) {
        steeringAngle = angle;

        for(int i = 0; i < vehicle->getNumWheels(); i++) {
            if(vehicle->getWheelInfo(i).m_bIsFrontWheel) {
                vehicle->setSteeringValue(angle, i);
            }
        }
    }
};

/// @brief Controls vehicle interactions
/// @details Requires PositionComponent, PhysicsComponent, and VehicleComponent
class VehicleSystem : public System {
    public:
        VehicleSystem(ComponentArray<PositionComponent>* positionCompArr, ComponentArray<PhysicsComponent>* physicsCompArr,
                      ComponentArray<VehicleComponent>* vehicleCompArr, btDynamicsWorld* dynamicsWorld)
            : positionCompArr(positionCompArr), physicsCompArr(physicsCompArr), vehicleCompArr(vehicleCompArr),
              dynamicsWorld(dynamicsWorld) {}
        ~VehicleSystem() {}
        void tick(const Uint32& deltaTime) { }
        /// @brief Attempts to free a vehicle that is flipped or stuck
        /// @detail Resets the rotation and moves the vehicle up 10 units
        void reset() { }
    private:
        // Component dependencies
        ComponentArray<PositionComponent>* positionCompArr;
        ComponentArray<PhysicsComponent>* physicsCompArr;
        ComponentArray<VehicleComponent>* vehicleCompArr;

        btDynamicsWorld* dynamicsWorld;   // Dynamics world from PhysicsSystem
};
