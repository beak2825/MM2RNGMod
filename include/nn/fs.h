#pragma once

#include "types.h"

namespace nn {
    namespace fs {
        struct FileHandle {
            void* handle;
        };

        constexpr int MODE_READ = 1;
        constexpr int MODE_WRITE = 2;

        uint32 MountSdCardForDebug(const char* mount);

        uint32 OpenFile(FileHandle* handle, const char* path, int mode);
        uint32 ReadFile(sz* bytes_read, FileHandle handle, int64 off, void* data, sz bytes_to_read);
        void CloseFile(FileHandle handle);
    }
}