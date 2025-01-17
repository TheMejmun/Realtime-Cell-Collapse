//
// Created by Saman on 26.08.23.
//

#ifndef REALTIME_CELL_COLLAPSE_UI_STATE_H
#define REALTIME_CELL_COLLAPSE_UI_STATE_H

#include "preprocessor.h"
#include "util/timer.h"

#include <glfw/glfw3.h>
#include <string>

struct UiState {
    std::string title{};
    GLFWwindow *window = nullptr;

    float cameraZ = 0;

    FPSCounter fps{};
    sec cpuWaitTime = 0;
    bool loggingStarted = false;
    chrono_sec_point loggingStartTime{};

    uint32_t currentMeshVertices = 0;
    uint32_t currentMeshTriangles = 0;
    bool isMonkeyMesh = false;
    bool switchMesh = false;

    sec meshSimplifierTimeTaken = 0.0f;
    uint32_t meshSimplifierFramesTaken = 0;
    bool runMeshSimplifier = false;
    bool returnToOriginalMeshBuffer = false;

    sec meshUploadTimeTaken = 0.0f;
};

#endif //REALTIME_CELL_COLLAPSE_UI_STATE_H
