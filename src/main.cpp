#include "rngmod/input.h"
#include "rngmod/rng.h"
#include "nn/fs.h"
#include <array>

static const std::array<rngmod::input::input_callback, 1> cb{rngmod::rng::input_press_callback};
//static const std::array<rngmod::input::input_callback, 0> cb{};
extern "C" void hkMain() {
    nn::fs::MountSdCardForDebug("sd");

    rngmod::rng::init();
    rngmod::input::init(cb);
}