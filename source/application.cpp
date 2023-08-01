//
// Created by Sam on 2023-04-08.
//

#include <iomanip>
#include "application.h"
#include "io/printer.h"
#include "ecs/entities/dense_sphere.h"
#include "ecs/entities/camera.h"

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
    Camera camera{};
    camera.components.is_main_camera = true;
    camera.upload(this->ecs);
    DenseSphere sphere{};
    sphere.upload(this->ecs);

    // Systems
    this->cameraController.create();
    this->sphereController.create();
}

void Application::mainLoop() {
    while (!this->windowManager.shouldClose()) {

        // Input
        this->inputManager.poll();
        if (this->inputManager.getKeyState(IM_CLOSE_WINDOW) == IM_DOWN_EVENT) {
            this->windowManager.close();
        }
        if (this->inputManager.consumeKeyState(IM_FULLSCREEN) == IM_DOWN_EVENT) {
            this->windowManager.toggleFullscreen();
        }

        this->cameraController.update(this->deltaTime, this->ecs, this->inputManager);
        this->sphereController.update(this->deltaTime, this->ecs, this->inputManager);

        // Render
        auto gpuWaitTime = this->renderer.draw(this->deltaTime, this->ecs);

        // Benchmark
        auto time = Timer::now();
        this->deltaTime = Timer::duration(this->lastTimestamp, time);
        this->currentGPUWaitTime = gpuWaitTime;
        this->currentFPS = (uint32_t) Timer::fps(this->deltaTime);

        this->lastTimestamp = time;

        FPS std::fixed << std::setprecision(6) <<
                       "FPS: " <<
                       this->currentFPS <<
                       "\tFrame time: " <<
                       this->deltaTime <<
                       "\tGPU wait time: " <<
                       this->currentGPUWaitTime ENDL;
    }
}

void Application::destroy() {
    INF "Destroying Application" ENDL;

    this->sphereController.destroy();
    this->cameraController.destroy();

    this->renderer.destroy();
    this->windowManager.destroy();
    this->ecs.destroy();
}
