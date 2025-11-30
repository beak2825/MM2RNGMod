#pragma once

#include "rngmod/types.h"
#include <span>

namespace rngmod {
    namespace input {
        using input_callback = void (*)();

        void init(std::span<const input_callback> press_callbacks);
        bool is_triggered(uint64 button);
        bool is_multi_triggered(uint64 buttons);
        bool is_pressed(uint64 button);
    }
}