// set this to 1 to enable nxlink logging
// upload your nro using "nxlink -s *.nro" to have printf be sent to your terminal
#define NXLINK_LOG 0

#include <switch.h>
#if NXLINK_LOG
#include <unistd.h>
#endif
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <cstdarg>
#include <experimental/scope>

namespace {

#define R_SUCCEED() return 0
#define R_THROW(_rc) return _rc
#define R_UNLESS(_rc, _msg) { \
    if (!(_rc)) { \
        std::printf("failed: %s %s:%d %s\n", #_rc, __FILE__, __LINE__, _msg); \
        R_THROW(0x1); \
    } \
}
#define R_TRY(r) { \
    if (const auto _rc = (r); R_FAILED(_rc)) { \
        std::printf("failed: %s %s:%d 0x%X mod: %u desc: %u val: %u\n", #r, __FILE__, __LINE__, _rc, R_MODULE(_rc), R_DESCRIPTION(_rc), R_VALUE(_rc)); \
        R_THROW(_rc); \
    } \
}
#define R_TRY_IGNORE(r, rignore) { \
    if (const auto _rc = (r); R_FAILED(_rc) && _rc != rignore) { \
        std::printf("failed: %s %s:%d 0x%X mod: %u desc: %u val: %u\n", #r, __FILE__, __LINE__, _rc, R_MODULE(_rc), R_DESCRIPTION(_rc), R_VALUE(_rc)); \
        R_THROW(_rc); \
    } \
}

#define CONCATENATE_IMPL(s1, s2) s1##s2
#define CONCATENATE(s1, s2) CONCATENATE_IMPL(s1, s2)
#define ANONYMOUS_VARIABLE(pref) CONCATENATE(pref, __COUNTER__)

// #define ON_SCOPE_FAIL(_f) std::experimental::scope_exit ANONYMOUS_VARIABLE(SCOPE_EXIT_STATE_){[&] { _f; }};
#define ON_SCOPE_EXIT(_f) std::experimental::scope_exit ANONYMOUS_VARIABLE(SCOPE_EXIT_STATE_){[&] { _f; }};

// prints to the screen and displays it immediately
__attribute__((format (printf, 1, 2)))
void consolePrint(const char* f, ...) {
    std::va_list argv;
    va_start(argv, f);
    std::vprintf(f, argv);
    va_end(argv);
    consoleUpdate(nullptr);
}

} // namespace


// called before main
extern "C" void userAppInit(void) {
    Result rc;

    if (R_FAILED(rc = appletLockExit())) // block exit until everything is cleaned up
        diagAbortWithResult(rc);
    #if NXLINK_LOG
    if (R_FAILED(rc = socketInitializeDefault()))
        diagAbortWithResult(rc);
    #endif
}

// called after main has exit
extern "C" void userAppExit(void) {
    #if NXLINK_LOG
    socketExit();
    #endif
    appletUnlockExit(); // unblocks exit to cleanly exit
}

int main(int argc, char** argv) {
    #if NXLINK_LOG
    int fd = nxlinkStdio();
    ON_SCOPE_EXIT(close(fd));
    #endif

    consoleInit(nullptr); // consol to display to the screen
    ON_SCOPE_EXIT(consoleExit(nullptr));

    // init controller
    PadState pad;
    padConfigureInput(1, HidNpadStyleSet_NpadStandard);
    padInitializeDefault(&pad);

    consolePrint("Press (+) to exit\n\n");

    // loop until + button is pressed
    while (appletMainLoop()) {
        padUpdate(&pad);

        const u64 kDown = padGetButtonsDown(&pad);
        if (kDown & HidNpadButton_Plus)
            break; // break in order to return to hbmenu

        svcSleepThread(1000000);
    }
}
