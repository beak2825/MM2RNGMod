#include "rngmod/rng.h"
#include "hk/hook/Trampoline.h"
#include "hk/svc/api.h"
#include "hk/util/Context.h"
#include "nn/fs.h"
#include "nn/hid.h"
#include "nn/os.h"
#include "rngmod/input.h"
#include "sead/rand.h"
#include "types.h"
#include "rngmod/svutil.h"
#include "rngmod/bufmap.h"
#include <array>
#include <charconv>
#include <climits>
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <system_error>
#include <cstdio>
#include <cinttypes>

namespace rngmod {
    namespace rng {
        struct rng_entry {
            sead::rng_context* ctx;
            int count;
            int index;
        };
        static std::array<rng_entry, 2> entries = {};
        static nn::os::MutexType mutex;

        static rng_entry* entry_add(sead::rng_context* ctx) {
            int i = 0;
            for (auto&& entry : entries) {
                if (entry.ctx == ctx) {
                    return &entry;
                } else if (entry.ctx == nullptr) {
                    entry.ctx = ctx;
                    entry.index = i;
                    return &entry;
                }

                i++;
            }

            return nullptr;
        }

        static rng_entry* entry_find(sead::rng_context* ctx) {
            for (auto&& entry : entries) {
                if (entry.ctx == ctx) {
                    return &entry;
                }
            }

            return nullptr;
        }

        // RNG init
        static nn::fs::FileHandle rng_config_file;
        static std::array<char, 128> rng_config_buf;
        static sead::rng_context init_rng_context{};
        static int init_x = 1;

        // RNG name lookup
        static nn::fs::FileHandle rng_name_config_file;
        static bufmap<std::uintptr_t, std::string_view, 1024> name_lookup;
        static std::array<char, 32768> name_buf;
        static int name_count = 0;

        static void print_entry(rng_entry& entry, std::uintptr_t ra = 0) {
            std::array<char, 128> buf;
            int written = 0;
            if (ra != 0) {
                auto name = name_lookup.get(ra);
                if (name.has_value() && name->size() < INT_MAX) {
                    int slen = static_cast<int>(name->size());
                    written = std::snprintf(buf.begin(), buf.size(), "[%d] %10d (%08" PRIx32 ") %.*s", entry.index, entry.count, entry.ctx->W, slen, name->data());
                } else {
                    written = std::snprintf(buf.begin(), buf.size(), "[%d] %10d (%08" PRIx32 ") %08" PRIxPTR, entry.index, entry.count, entry.ctx->W, ra);
                }
            } else {
                written = std::snprintf(buf.begin(), buf.size(), "[%d] %10d (%08" PRIx32 ")", entry.index, entry.count, entry.ctx->W);
            }
            if (written > 0 && written < buf.size()) {
                hk::svc::OutputDebugString(buf.begin(), written);
            }
        }

        static HkTrampoline<void, sead::rng_context*, uint32> rnginit_func = 
        hk::hook::trampoline([](sead::rng_context* ctx, uint32 seed) -> void {
            if (seed != 0) {
                rnginit_func.orig(ctx, seed);
                return;
            }

            nn::os::LockMutex(&mutex);
            auto entry = entry_add(ctx);
            if (entry != nullptr) {
                entry->count = init_x;
                entry->ctx->X = init_rng_context.X;
                entry->ctx->Y = init_rng_context.Y;
                entry->ctx->Z = init_rng_context.Z;
                entry->ctx->W = init_rng_context.W;

                print_entry(*entry);
            } else {
                rnginit_func.orig(ctx, seed);
            }
            nn::os::UnlockMutex(&mutex);
        });

        static HkTrampoline<uint32, sead::rng_context*> rng_func = 
        hk::hook::trampoline([](sead::rng_context* ctx) -> uint32 {
            auto ret = rng_func.orig(ctx);

            nn::os::LockMutex(&mutex);
            auto entry = entry_find(ctx);
            if (entry == nullptr) {
                nn::os::UnlockMutex(&mutex);
                return ret;
            }
            auto ra = hk::util::getReturnAddress(0);
            entry->count++;

            print_entry(*entry, ra);
            nn::os::UnlockMutex(&mutex);

            return ret;
        });

        extern "C" void rand_init(sead::rng_context* ctx, uint32 seed);
        extern "C" uint32 rand_get32(sead::rng_context* ctx);

        static void rng_config_read() {
            bool success = nn::fs::OpenFile(&rng_config_file, "sd:/rng.txt", nn::fs::MODE_READ) == 0;
            if (!success) {
                return;
            }

            sz read;
            success = nn::fs::ReadFile(&read, rng_config_file, 0, rng_config_buf.data(), rng_config_buf.size()) == 0;
            nn::fs::CloseFile(rng_config_file);
            if (!success) {
                return;
            }
            std::string_view config(rng_config_buf.data(), read);

            auto config_clean = rngmod::svutil::strip(config);
            if (config_clean.empty()) {
                return;
            }

            int x_val;
            auto parse_res = std::from_chars(config_clean.begin(), config_clean.end(), x_val);
            if (parse_res.ec != std::errc()) {
                x_val = 1;
            }

            rnginit_func.orig(&init_rng_context, 0);
            for (int i = 1; i < x_val; i++) {
                rng_func.orig(&init_rng_context);
            }
            init_x = x_val;

            std::array<char, 64> buf;
            int written = std::snprintf(buf.begin(), buf.size(), "RNG config loaded X=%d", x_val);
            if (written > 0 && written < buf.size()) {
                hk::svc::OutputDebugString(buf.begin(), written);
            }
        }

        static void name_parse_line(std::string_view line) {
            sz pos = 0;
            auto column1 = rngmod::svutil::split(line, '=', pos);
            if (pos >= line.size()) {
                return;
            }
            auto column2 = line.substr(pos+1);

            column1 = rngmod::svutil::strip(column1);
            column2 = rngmod::svutil::strip(column2);
            column2 = column2.substr(0, std::min<sz>(column2.size(), 20));
            if (column1.empty() || column2.empty()) {
                return;
            }

            std::uintptr_t ra;
            auto parse_res = std::from_chars(column1.begin(), column1.end(), ra, 16);
            if (parse_res.ec != std::errc()) {
                return;
            }
            
            name_lookup.set(ra, column2);
            name_count++;
        }

        static void rng_names_config_read() {
            bool success = nn::fs::OpenFile(&rng_name_config_file, "sd:/rngnames.txt", nn::fs::MODE_READ) == 0;;
            if (!success) {
                return;
            }

            sz read;
            success = nn::fs::ReadFile(&read, rng_name_config_file, 0, name_buf.data(), name_buf.size()) == 0;
            nn::fs::CloseFile(rng_name_config_file);
            if (!success) {
                return;
            }
            std::string_view config(name_buf.data(), read);

            name_lookup.clear();
            name_count = 0;

            sz pos = 0;
            while (pos < read) {
                auto line = rngmod::svutil::split(config, '\n', pos);
                name_parse_line(line);

                pos++;
            }

            std::array<char, 64> buf;
            int written = std::snprintf(buf.begin(), buf.size(), "RNG name config loaded %d entries", name_count);
            if (written > 0 && written < buf.size()) {
                hk::svc::OutputDebugString(buf.begin(), written);
            }
        }

        void init() {
            nn::os::InitializeMutex(&mutex, false, 0);
            rand_init(&init_rng_context, 0);
            
            rnginit_func.installAtSym<"rand_init">();
            rng_func.installAtSym<"rand_get32">();

            nn::os::LockMutex(&mutex);
            rng_config_read();
            rng_names_config_read();
            nn::os::UnlockMutex(&mutex);
        }

        void input_press_callback() {
            if (rngmod::input::is_multi_triggered(nn::hid::BUTTON_STICK_L | nn::hid::BUTTON_STICK_R)) {
                nn::os::LockMutex(&mutex);
                rng_config_read();
                nn::os::UnlockMutex(&mutex);
            }
            if (rngmod::input::is_multi_triggered(nn::hid::BUTTON_PLUS | nn::hid::BUTTON_MINUS)) {
                nn::os::LockMutex(&mutex);
                rng_names_config_read();
                nn::os::UnlockMutex(&mutex);
            }
        }
    }
}