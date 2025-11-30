#pragma once

#include "types.h"

namespace nn {
    namespace hid {
        constexpr uint64 BUTTON_STICK_L = 0x10;
        constexpr uint64 BUTTON_STICK_R = 0x20;
        constexpr uint64 BUTTON_MINUS = 0x800;
        constexpr uint64 BUTTON_PLUS = 0x400;

        struct full_key_state {
            int64 sample;
            uint64 buttons;
            int32 sl_x;
            int32 sl_y;
            int32 sr_x;
            int32 sr_y;
            uint32 attributes;
        };
    }
}