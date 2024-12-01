#include <SDL2/SDL_events.h>
#include <SDL2/SDL_keyboard.h>
#include <SDL2/SDL_mouse.h>
#include <SDL2/SDL_scancode.h>
#include <SDL2/SDL_stdinc.h>
#include <SDL2/SDL_timer.h>
#include <SDL2/SDL_video.h>
#include <any>
#include <bullet/LinearMath/btIDebugDraw.h>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <string_view>
#include <memory>

#include "include/Heightmap.hpp"
#include "include/UI.hpp"
#include "include/PhysicsEngine.hpp"
#include "include/Window.hpp"
#include "include/Util.hpp"
#include "include/ecs/ECS.hpp"
#include "include/shader/BaseShader.hpp"

std::unique_ptr<PhysicsEngine> physicsEngine;
std::unique_ptr<UI> ui;
std::unique_ptr<Window> mainWindow;

EntityManager entityManager;
ComponentManager compManager;
SystemManager sysManager;

Camera camera;

// Temporary variables for testing
Heightmap* heightfield;
BaseShader baseShader;
TextShader textShader;
BaseShader heightmap;



struct TimeData {
    float minFrameTime = 16.66666667f;  // The minimum time the frame should take (ms)

    Uint32 prevFrameTime = 0;           // Time when last frame completed (ms)
    Uint32 frameTime = 0;               // Total time to complete frame (ms)

    Uint32 prevTickTime = 0;    // Time when last logic tick completed (ms)
    Uint32 deltaT = 0;        // Time since last logic ticks (ms)

    Uint32 debugDrawTime = 0;
};

struct Flags {
    bool paused = true;         // If the engine is paused
    bool debugDraw = false;     // Draw debug information
    bool zbuffer = false;       // Draw the zbuffer
    bool frameLimit = false;    // Limit the framerate with TimeData.minFrameTime
    bool showUI = false;
};

struct EngineState {
    TimeData time;
    Flags flags;

    unsigned char vsync = 0;
} globalState;

const Uint8* KEYBOARD = nullptr;

WindowCreationData parseCmdArgs(int& argc, char** argv) {
    Util::CMDParser cmdArgs(argc, argv);

   	WindowCreationData windowData = { .minFrameTime = 0, };

    // Vsync
  	if(cmdArgs.hasOption("--vsync")){
      	windowData.flags.vsync(1);
  	} else {
        windowData.flags.vsync(0);
    }

    // Width
    std::string_view option = cmdArgs.getOption(std::pair(
        std::string_view("--width"),
        std::string_view("-w")
    ));
    if(option.compare("") != 0){
        windowData.width = std::stoi(option.data());
    }

    // Height
    option = cmdArgs.getOption(std::pair(
        std::string_view("--height"),
        std::string_view("-h")
    ));
    if(option.compare("") != 0){
        windowData.height = std::stoi(option.data());
    }

    return windowData;
}

int processEvents() {
    SDL_Event event;

    // Loop over all events for this frame
    while(SDL_PollEvent(&event)) {
        switch(event.type) {
            // Window-specific events, pass onto respective window
            case SDL_WINDOWEVENT: {
                if(event.window.windowID == SDL_GetWindowID(mainWindow->getSDLWindow()))
                    mainWindow->processEvent(event);
                break;
            } case SDL_KEYDOWN: {
                if(event.key.keysym.scancode == SDL_SCANCODE_ESCAPE) {
                    globalState.flags.showUI = globalState.flags.paused;
                    globalState.flags.paused = !globalState.flags.paused;

                    if(globalState.flags.paused) {
                        SDL_SetRelativeMouseMode(SDL_FALSE);
                        SDL_ShowCursor(SDL_TRUE);
                    } else {
                        SDL_SetRelativeMouseMode(SDL_TRUE);
                        SDL_ShowCursor(SDL_FALSE);
                    }
                } else if(event.key.keysym.scancode == SDL_SCANCODE_LALT && globalState.flags.paused) {
                    globalState.flags.zbuffer = !globalState.flags.zbuffer;

                    if(globalState.flags.zbuffer) {
                        if(!baseShader.loadProgram("../shaders/texture.vert", "../shaders/zBuffer.frag", "", "")){
                            std::cerr << "Unable to load z-buffer shader" << std::endl;
                            baseShader.loadProgram("../shaders/texture.vert", "../shaders/pureTexture.frag", "", "");

                            globalState.flags.zbuffer = false;
                        }
                    } else {
                        baseShader.loadProgram("../shaders/texture.vert", "../shaders/pureTexture.frag", "", "");
                    }
                } else if(event.key.keysym.scancode == SDL_SCANCODE_R) {
                    physicsEngine->reset();
                } else if(event.key.keysym.scancode == SDL_SCANCODE_F1) {
                    globalState.flags.debugDraw = !globalState.flags.debugDraw;
                } else if(event.key.keysym.scancode == SDL_SCANCODE_F5) {
                    physicsEngine->loadState("./saves/savedState.bin");

                    #ifdef DEBUG
                        std::cout << "Loaded physics state from file: ./saves/savedState.bin\n";
                    #endif
                }
                break;
            } case SDL_KEYUP: {
                break;
            } case SDL_MOUSEBUTTONDOWN: {
                int32_t mouseX;
                int32_t mouseY;
                Uint32 mbState = SDL_GetMouseState(&mouseX, &mouseY);

                switch(mbState) {
                    case SDL_BUTTON(1): {   // LMB
                        if(!globalState.flags.paused)
                            break;

                        glm::vec2 windowDimensions = mainWindow->getDimensions();

                        // Normalize mouse position to [-1, 1]
                        glm::vec3 rayNDS = glm::vec3(
                            (2.f * mouseX) / windowDimensions.x - 1.f,
                            1.f - (2.f * mouseY) / windowDimensions.y,
                            1.f
                        );

                        glm::vec4 rayClip = glm::vec4(rayNDS.x, rayNDS.y, -1.f, 1.f);
                        glm::vec4 rayEye = glm::inverse(glm::perspective(glm::radians(camera.getFOV()), (float)(windowDimensions.x / windowDimensions.y), 0.1f, 1000.f)) * rayClip;
                        rayEye = glm::vec4(rayEye.x, rayEye.y, -1.f, 0.f);

                        glm::vec3 rayWorld = glm::vec3(glm::inverse(camera.calcCameraView()) * rayEye);
                        rayWorld = glm::normalize(rayWorld);

                        glm::vec3 rayOrigin = glm::vec3(glm::inverse(camera.calcCameraView()) * glm::vec4(0, 0, 0, 1));

                        physicsEngine->castRay(rayOrigin, rayWorld, 100.f);
                    } case SDL_BUTTON(2): { // MMB
                        break;
                    } case SDL_BUTTON(3): { // RMB
                        break;
                    }
                }
                break;
            } case SDL_MOUSEBUTTONUP: {
                break;
            } case SDL_MOUSEWHEEL: {
                if(globalState.flags.paused)
                    break;
                camera.incFOV(-event.wheel.y * 2.5);

                break;
            } case SDL_MOUSEMOTION: {
                if(globalState.flags.paused)
                    break;

                camera.incYaw(event.motion.xrel * 0.1f);
                camera.incPitch(-event.motion.yrel * 0.1f);
                break;
            } case SDL_QUIT:
                SDL_Quit();
                return 1;
            default:
                break;
        }
    }
    return 0;
}

int main(int argc, char** argv) {
   	std::cout << "start" << std::endl;

    WindowCreationData windowData = parseCmdArgs(argc, argv);
    globalState.vsync = windowData.flags.vsync();

    // Initialize SDL2
    {
        // Init Video and Events Subsystems
        if(SDL_Init(windowData.sdlSubsystems) < 0){
            CREATE_ERROR("Unable to initialize subsystems, SDL_Error: ", SDL_GetError());
        }

        // Use OpenGL 4.1 core
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

        // Other OpenGL settings
        SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
        SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);

        // Set mouse options
        SDL_SetRelativeMouseMode(SDL_FALSE);
        SDL_ShowCursor(SDL_TRUE);

        KEYBOARD = SDL_GetKeyboardState(NULL);
    }

    // Create main window
    mainWindow = std::make_unique<Window>(Util::getWorkingDir());
   	if(!mainWindow->init(windowData))
        return 1;

    // Initialize PhysiceEngine
    physicsEngine = std::make_unique<PhysicsEngine>();
    if(!physicsEngine->init()) {
        std::cerr << "Unable to initialize physics engine\n";
        return 1;
    }

    // Initialize ECS
    {
        ComponentSet posID = compManager.registerComponent<PositionComponent>();
        ComponentSet phsID = compManager.registerComponent<PhysicsComponent>();
        ComponentSet renID = compManager.registerComponent<RenderComponent>();

        std::cout << "Position ID:    " << posID << '\n';
        std::cout << "Physics ID:     " << phsID << '\n';
        std::cout << "Render ID:      " << renID << '\n';

        sysManager.registerSystem<PhysicsSystem>(
            ComponentSet(posID | phsID),
            static_cast<ComponentArray<PositionComponent>*>(compManager.getComponentArray<PositionComponent>()),
            static_cast<ComponentArray<PhysicsComponent>*>(compManager.getComponentArray<PhysicsComponent>())
        );
        sysManager.registerSystem<GraphicsSystem>(
            ComponentSet(posID | renID),
            static_cast<ComponentArray<PositionComponent>*>(compManager.getComponentArray<PositionComponent>()),
            static_cast<ComponentArray<RenderComponent>*>(compManager.getComponentArray<RenderComponent>())
        );

        // Test model
        Entity testModel = entityManager.create();
        std::cout << "Entity: " << testModel << " created\n";

        entityManager.setComponents(testModel, ComponentSet(posID | renID));
        compManager.addComponent(testModel, (PositionComponent){ glm::translate(glm::mat4x4(1.f), glm::vec3(0, 0, 0)) });
        compManager.addComponent(testModel, RenderComponent());
        sysManager.entityChanged(testModel, ComponentSet(posID | renID));

        compManager.getComponent<RenderComponent>(testModel)->model.initialize("../assets/character/character.obj");
        std::cout << "ECS System created and initialized\n";
    }

    // Test Objects
    {
        heightfield = new Heightmap("../assets/heightmap.png");
        physicsEngine->addRigidBody(heightfield->getRigidBody());

        baseShader.loadProgram("../shaders/texture.vert", "../shaders/pureTexture.frag", "", "");
        heightmap.loadProgram("../shaders/heightmap.vert", "../shaders/heightmap.frag", "../shaders/heightmap.tesc", "../shaders/heightmap.tese");
        textShader.loadProgram("../shaders/text.vert", "../shaders/text.frag");
    }

    // Main loop
    while(processEvents() == 0) {
        // Update deltaT
        globalState.time.deltaT = SDL_GetTicks() - globalState.time.prevTickTime;
        globalState.time.prevTickTime = SDL_GetTicks();

        // Runtime logic
        if(globalState.flags.paused) {

        } else {
            // Camera
            if(KEYBOARD[SDL_SCANCODE_E]) {
                camera.incRoll(1.5f * globalState.time.deltaT / 1000);	// Roll right(increase)
            } else if(KEYBOARD[SDL_SCANCODE_Q]) {
                camera.incRoll(-1.5f * globalState.time.deltaT / 1000);	// Roll left(decrease)
            }
            if(KEYBOARD[SDL_SCANCODE_EQUALS] && camera.getSpeed() < 1.f) {
                camera.setSpeed(camera.getSpeed() + (0.00005f * globalState.time.deltaT));
            } else if(KEYBOARD[SDL_SCANCODE_MINUS]) {
                camera.setSpeed(camera.getSpeed() - (0.00005f * globalState.time.deltaT));
            }

            // Camera position for view calculations
            camera.updateCameraPosition(
                KEYBOARD[SDL_SCANCODE_W],
                KEYBOARD[SDL_SCANCODE_S],
                KEYBOARD[SDL_SCANCODE_A],
                KEYBOARD[SDL_SCANCODE_D],
                KEYBOARD[SDL_SCANCODE_SPACE],
                KEYBOARD[SDL_SCANCODE_LCTRL],
                globalState.time.deltaT
            );
            camera.updateCameraDirection();

            physicsEngine->tick(globalState.time.deltaT / 1000.f);
        }

        // Render
        {
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            heightfield->draw(heightmap, camera.calcCameraView(), camera.getFOV(), false);
            sysManager.getSystem<GraphicsSystem>()->tick(baseShader, camera.calcCameraView(), camera.getFOV());

            if(globalState.flags.debugDraw) {
                Uint32 currentTime = SDL_GetTicks();
                physicsEngine->debugDraw(camera.calcCameraView(), camera.getFOV(), btIDebugDraw::DBG_DrawWireframe);
                globalState.time.debugDrawTime = SDL_GetTicks() - currentTime;
            }

            if(globalState.flags.showUI) {
                //ui->drawTextElements(textShader);
            }

            glUseProgram(0);
            SDL_GL_SwapWindow(mainWindow->getSDLWindow());
        }

        // Update frame times
        Uint32 currentTime = SDL_GetTicks();
        if(globalState.vsync != 1 && globalState.flags.frameLimit) {
            Uint32 frameTime = currentTime - globalState.time.prevFrameTime;
            if(frameTime < globalState.time.minFrameTime) {
                SDL_Delay(globalState.time.minFrameTime - frameTime);
            }

            Uint32 newTime = SDL_GetTicks();
            globalState.time.frameTime = newTime - globalState.time.prevFrameTime;
            globalState.time.prevFrameTime = newTime;
        } else {
            globalState.time.frameTime = currentTime - globalState.time.prevFrameTime;
            globalState.time.prevFrameTime = currentTime;
        }
    }

    // Cleanup
    delete heightfield;

   	std::cout << "end" <<std::endl;
   	return 0;
}
