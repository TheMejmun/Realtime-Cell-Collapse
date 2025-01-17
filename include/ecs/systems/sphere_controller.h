//
// Created by Saman on 01.08.23.
//

#ifndef REALTIME_CELL_COLLAPSE_SPHERE_CONTROLLER_H
#define REALTIME_CELL_COLLAPSE_SPHERE_CONTROLLER_H

#include "preprocessor.h"
#include "util/timer.h"
#include "ecs/ecs.h"
#include "io/input_manager.h"

namespace SphereController {
    void update(const sec &delta, ECS &ecs);

    void destroy();

    static inline bool EvaluatorRotatingSphere(const Components &components) {
        return components.transform != nullptr && components.isAlive() && components.isRotatingSphere;
    };
};
#endif //REALTIME_CELL_COLLAPSE_SPHERE_CONTROLLER_H
