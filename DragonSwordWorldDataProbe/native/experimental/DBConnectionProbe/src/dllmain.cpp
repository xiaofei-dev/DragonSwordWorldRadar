#include "db_probe.hpp"

#include <memory>

namespace {
std::unique_ptr<dsw::dbprobe::Probe> g_probe;
}

BOOL APIENTRY DllMain(HMODULE module, DWORD reason, LPVOID) {
    switch (reason) {
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(module);
            g_probe = std::make_unique<dsw::dbprobe::Probe>(module);
            g_probe->start();
            break;
        case DLL_PROCESS_DETACH:
            if (g_probe) {
                g_probe->stop();
                g_probe.reset();
            }
            break;
        default:
            break;
    }
    return TRUE;
}
