#pragma once

#include "rngmod/types.h"
#include <string_view>

namespace rngmod {
    namespace svutil {
        std::string_view split(std::string_view s, char c, sz& first);
        std::string_view strip(std::string_view s);
    }
}