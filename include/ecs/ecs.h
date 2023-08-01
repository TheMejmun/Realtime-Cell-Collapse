//
// Created by Sam on 2023-04-09.
//

#ifndef REALTIME_CELL_COLLAPSE_ECS_H
#define REALTIME_CELL_COLLAPSE_ECS_H

#include <vector>
#include <optional>
#include <tuple>
#include <memory>
#include <functional>
#include "graphics/render_mesh.h"
#include "graphics/projector.h"

struct Components {
    bool isDestroyed = false;
    bool willDestroy = false;
    uint32_t index = 0;
    RenderMesh *renderMesh = nullptr;
    Transformer4 *transform = nullptr;
    bool isMainCamera = false;
    Projector *camera = nullptr;
    bool isRotatingSphere = false;

    /**
     * Destroys all contained components.
     *
     * Warning: Do not call this manually. ECS calls this automatically when required.
     */
    void destroy() {
        delete this->renderMesh;
        this->renderMesh = nullptr;
        delete this->transform;
        this->transform = nullptr;
        delete this->camera;
        this->camera = nullptr;

        this->isDestroyed = true;
        this->willDestroy = false;
    }

    /**
     * Convenience function
     * @return True, if these components are not and will not be destroyed this frame.
     */
    [[nodiscard]] bool isAlive() const {
        return !isDestroyed && !willDestroy;
    }
};

class ECS {
public:
    void create();

    void destroy();

    uint32_t insert(Components &entityComponents);

    // In every frame, always do inserts first, and deletions after. So that the renderer has time to handle allocation
    void remove(const uint32_t &index);

    std::vector<Components *>
    requestComponents(const std::function<bool(const Components &)> &evaluator);

private:
    void destroyReferences(const uint32_t &index);

    std::vector<Components> components{};
};

#endif //REALTIME_CELL_COLLAPSE_ECS_H
