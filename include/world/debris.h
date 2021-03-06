#pragma once
#include "glm/glm.hpp"

#include "draw/map.h"
#include "world/world.h"


namespace ace { namespace world {
    struct DebrisGroup;

    struct Debris {
        Debris(DebrisGroup &group, glm::vec3 position, glm::vec3 velocity, glm::u8vec3 color);

        bool update(double dt);

        DebrisGroup &group;
        glm::vec3 p, v, color;
    };

    struct DebrisGroup : WorldObject {
        DebrisGroup(scene::GameScene &scene, glm::vec3 position, glm::u8vec3 color, float vel_mod, int num);

        bool update(double dt) override;
        void draw() override;

        std::vector<Debris> debris;
        
        constexpr static float MAX_LIFE = 2.5f;
        float life;
    };
}}
