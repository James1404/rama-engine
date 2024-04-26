#pragma once

#include <rama/types.hpp>

#define SOL_ALL_SAFETIES_ON 1
#include <sol/sol.hpp>

#include <SDL3/SDL.h>

namespace scripting {
    sol::state& get_state();

    void setup();

    i32 load(string path);
    sol::safe_function getfunc(string func_name);
}
