#include <physics2d.hpp>

#include <engine.hpp>

namespace {
    f32 accumulator = 0;
    bool dirty = true;
    f32 cDeltaTime = 1.0f / 60.0f;

    sol::table lib(sol::this_state state) {
        sol::state_view luaview(state);

        auto module = luaview.create_table();

        module.set_function("Init", &physics2d::init);
        module.set_function("Shutdown", &physics2d::shutdown);
        module.set_function("Update", &physics2d::update);

        return module;
    }

    b2Vec2 gravity(0, -9.8f);
    b2World* world;
}

namespace physics2d {
    void init() {
        TODO("Initialize Physics Simulation");
        world = new b2World(gravity);
    }

    void shutdown() {
        delete world;
    }

    void update() {
        accumulator += engine::DeltaTime();

        while(accumulator >= cDeltaTime) {
            if(dirty) {
                TODO("Optimize Physics2D");
                dirty = false;
            }

            TODO("Run Physics Simulation");
            accumulator -= cDeltaTime;
        }
    }

    void RegisterLuaModule(sol::state& state) {
        state.require("physics2d", sol::c_call<decltype(&lib), &lib>, false);
    }
}
