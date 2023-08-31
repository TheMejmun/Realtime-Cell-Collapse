//
// Created by Sam on 2023-04-08.
//

#include "application.h"
#include "io/printer.h"
#include "ecs/entities/dense_sphere.h"
#include "ecs/entities/monkey.h"
#include "ecs/entities/camera.h"
#include "ecs/systems/camera_controller.h"
#include "ecs/systems/sphere_controller.h"
#include "ecs/systems/mesh_simplifier_controller.h"

#include <iomanip>

void Application::run() {
    init();
    mainLoop();
    destroy();
}

void Application::init() {
    INF "Creating Application" ENDL;

    this->ecs.create();
    this->windowManager.create(this->title);
    this->inputManager.create(this->windowManager.window);
    this->renderer.create(this->title, this->windowManager.window);

    // Entities
    InputStateEntity inputStateEntity{};
    inputStateEntity.upload(this->ecs);

    Camera camera{};
    camera.components.isMainCamera = true;
    camera.upload(this->ecs);

//    DenseSphere sphere{};
//    sphere.upload(this->ecs);
    Monkey monkey{};
    monkey.upload(this->ecs);
}

void Application::mainLoop() {
    while (!this->windowManager.shouldClose()) {

        // Input
        this->inputManager.update(this->deltaTime, this->ecs);
        auto &inputState = *ecs.requestEntities(InputController::EvaluatorInputManagerEntity)[0]->inputState;
        if (inputState.closeWindow == IM_DOWN_EVENT)
            this->windowManager.close();
        if (inputState.toggleFullscreen == IM_DOWN_EVENT)
            this->windowManager.toggleFullscreen();

        // UI
        auto uiState = this->renderer.getUiState();
        uiState->fps.update(this->deltaTime);
        uiState->cpuWaitTime = this->currentCpuWaitTime;

        // Systems
        CameraController::update(this->deltaTime, this->ecs);
        SphereController::update(this->deltaTime, this->ecs);
        if (uiState->runMeshSimplifier)
            MeshSimplifierController::update(this->ecs, &uiState->meshSimplifierTimeTaken,
                                             &uiState->meshSimplifierFramesTaken);

        // Render
        if (uiState->returnToOriginalMeshBuffer)
            this->renderer.resetMesh();
        this->currentCpuWaitTime = this->renderer.draw(this->deltaTime, this->ecs);

        // Benchmark
        auto time = Timer::now();
        this->deltaTime = Timer::duration(this->lastTimestamp, time);

        this->lastTimestamp = time;
    }
}

void Application::destroy() {
    INF "Destroying Application" ENDL;

    CameraController::destroy();
    SphereController::destroy();
    MeshSimplifierController::destroy();

    this->renderer.destroy();
    this->windowManager.destroy();
    this->ecs.destroy();
}
