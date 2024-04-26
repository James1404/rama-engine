#pragma once

#include <types.hpp>
#include <scripting.hpp>

#include <box2d/box2d.h>

class Body {
private:
    bool is_static;
public:
};

namespace physics2d {
    void init();
    void shutdown();
    void update();

    void RegisterLuaModule(sol::state& state);
};
