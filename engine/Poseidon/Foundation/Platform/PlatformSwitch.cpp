// Nintendo Switch (libnx) support: early init and crash reporting, startup/shutdown
// glue for main(), plus POSIX functions newlib declares but libnx does not implement
// (needed by the engine and its vcpkg dependencies).
// Picked up by the engine's source glob on every platform; empty off Switch.

#ifdef __SWITCH__

#include <Poseidon/Foundation/Platform/PlatformSwitch.hpp>

#include <switch.h>

#include <cstdio>
#include <cstring>
#include <errno.h>
#include <malloc.h>
#include <mutex>
#include <stdlib.h>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>

namespace
{
constexpr const char* kAppRoot = "sdmc:/switch/cwr-re";
constexpr const char* kDataDir = "sdmc:/switch/cwr-re/data";
constexpr const char* kLogFile = "sdmc:/switch/cwr-re/log.txt";

int g_nxlinkSocket = -1;
bool g_socketsUp = false;

std::mutex g_preadMutex;

void WriteRaw(const char* text)
{
    write(STDERR_FILENO, text, std::strlen(text));
}
}

alignas(16) u8 __nx_exception_stack[0x8000];
u64 __nx_exception_stack_size = sizeof(__nx_exception_stack);

extern "C" void userAppInit(void);

MemoryInfo ImageText()
{
    MemoryInfo info{};
    u32 pageInfo = 0;
    if (R_FAILED(svcQueryMemory(&info, &pageInfo, reinterpret_cast<u64>(&userAppInit))))
        info = MemoryInfo{};
    return info;
}

u64 ImageBase()
{
    return ImageText().addr;
}

extern "C" void __libnx_exception_handler(ThreadExceptionDump* ctx)
{
    const MemoryInfo text = ImageText();
    const u64 base = text.addr;
    const u64 textEnd = text.addr + text.size;
    char buf[512];
    std::snprintf(buf, sizeof(buf),
                  "\n[switch] CRASH: error_desc=0x%x esr=0x%x\n"
                  "[switch]   pc=0x%lx (image+0x%lx)\n"
                  "[switch]   lr=0x%lx (image+0x%lx)\n"
                  "[switch]   far=0x%lx sp=0x%lx fp=0x%lx\n",
                  ctx->error_desc, ctx->esr, ctx->pc.x, ctx->pc.x - base, ctx->lr.x, ctx->lr.x - base, ctx->far.x,
                  ctx->sp.x, ctx->fp.x);
    WriteRaw(buf);

    u64 fp = ctx->fp.x;
    for (int i = 0; i < 16 && fp != 0 && (fp & 0xF) == 0; ++i)
    {
        const u64* frame = reinterpret_cast<const u64*>(fp);
        const u64 ret = frame[1];
        if (ret < base)
            break;
        std::snprintf(buf, sizeof(buf), "[switch]   #%d image+0x%lx\n", i, ret - base);
        WriteRaw(buf);
        fp = frame[0];
    }

    std::snprintf(buf, sizeof(buf), "[switch]   thread handle=0x%x\n", static_cast<unsigned>(threadGetCurHandle()));
    WriteRaw(buf);
    for (int i = 0; i < 29; i += 4)
    {
        std::snprintf(buf, sizeof(buf), "[switch]   x%-2d=0x%016lx x%-2d=0x%016lx x%-2d=0x%016lx x%-2d=0x%016lx\n", i,
                      ctx->cpu_gprs[i].x, i + 1, i + 1 < 29 ? ctx->cpu_gprs[i + 1].x : 0, i + 2,
                      i + 2 < 29 ? ctx->cpu_gprs[i + 2].x : 0, i + 3, i + 3 < 29 ? ctx->cpu_gprs[i + 3].x : 0);
        WriteRaw(buf);
    }

    MemoryInfo stackInfo{};
    u32 pageInfo = 0;
    const u64 sp = ctx->sp.x;
    if (sp != 0 && R_SUCCEEDED(svcQueryMemory(&stackInfo, &pageInfo, sp)))
    {
        const u64 stackEnd = stackInfo.addr + stackInfo.size;
        int found = 0;
        for (u64 p = sp & ~u64(7); p + 8 <= stackEnd && found < 24; p += 8)
        {
            const u64 v = *reinterpret_cast<const u64*>(p);
            if (v > base && v < textEnd)
            {
                std::snprintf(buf, sizeof(buf), "[switch]   stack[sp+0x%lx] image+0x%lx\n", p - sp, v - base);
                WriteRaw(buf);
                ++found;
            }
        }
    }
    WriteRaw("[switch] map with: aarch64-none-elf-addr2line -Cfpe PoseidonGame.elf <offset>\n");
}

extern "C" void userAppInit(void)
{
    g_socketsUp = R_SUCCEEDED(socketInitializeDefault());
    if (g_socketsUp)
        g_nxlinkSocket = nxlinkStdio(); // -1 unless launched by `nxlink -s`

    if (g_nxlinkSocket < 0)
    {
        // No nxlink host: keep a log on the SD card instead.
        mkdir("sdmc:/switch", 0777);
        mkdir(kAppRoot, 0777);
        std::freopen(kLogFile, "w", stdout);
        std::freopen(kLogFile, "a", stderr);
    }
    std::setvbuf(stdout, nullptr, _IOLBF, 0);
    std::setvbuf(stderr, nullptr, _IONBF, 0);
    std::printf("[switch] userAppInit: nxlink=%s sockets=%s\n", g_nxlinkSocket >= 0 ? "yes" : "no",
                g_socketsUp ? "yes" : "no");
}


extern "C" void userAppExit(void)
{
    std::fflush(stdout);
    if (g_nxlinkSocket >= 0)
    {
        close(g_nxlinkSocket);
        g_nxlinkSocket = -1;
    }
    if (g_socketsUp)
    {
        socketExit();
        g_socketsUp = false;
    }
}


extern "C" {

int posix_memalign(void** memptr, size_t alignment, size_t size)
{
    if (alignment < sizeof(void*) || (alignment & (alignment - 1)) != 0)
        return EINVAL;
    void* p = memalign(alignment, size ? size : 1);
    if (!p)
        return ENOMEM;
    *memptr = p;
    return 0;
}

ssize_t pread(int fd, void* buf, size_t count, off_t offset)
{
    std::lock_guard lock(g_preadMutex);
    const off_t saved = lseek(fd, 0, SEEK_CUR);
    if (saved < 0 || lseek(fd, offset, SEEK_SET) < 0)
        return -1;
    const ssize_t n = read(fd, buf, count);
    const int err = errno;
    lseek(fd, saved, SEEK_SET);
    errno = err;
    return n;
}

long sysconf(int name)
{
    switch (name)
    {
    case _SC_PAGESIZE:
        return 0x1000;
    case _SC_PHYS_PAGES:
    {
        // Memory available to this process (application vs. applet mode differ).
        u64 total = 0;
        if (R_FAILED(svcGetInfo(&total, InfoType_TotalMemorySize, CUR_PROCESS_HANDLE, 0)))
            return -1;
        return static_cast<long>(total / 0x1000);
    }
    case _SC_NPROCESSORS_CONF:
    case _SC_NPROCESSORS_ONLN:
        return 3; // core 3 is reserved by the system
    default:
        errno = EINVAL;
        return -1;
    }
}

char* getlogin(void)
{
    return nullptr; // engine falls back to an empty user name
}


int execvp(const char*, char* const[])
{
    errno = ENOSYS;
    return -1;
}

pid_t waitpid(pid_t, int*, int)
{
    errno = ENOSYS;
    return -1;
}

}


namespace Poseidon::Foundation
{
namespace
{
std::vector<std::string> g_argStorage;
std::vector<char*> g_argv;

bool DirExists(const char* path)
{
    struct stat st{};
    return stat(path, &st) == 0 && S_ISDIR(st.st_mode);
}

bool HasArg(int argc, char** argv, const char* name)
{
    for (int i = 1; i < argc; ++i)
    {
        if (std::strcmp(argv[i], name) == 0)
            return true;
    }
    return false;
}
}

void SwitchStartup(int& argc, char**& argv)
{
    mkdir("sdmc:/switch", 0777);
    mkdir(kAppRoot, 0777);
    std::printf("[switch] main: argc=%d\n", argc);

    const AppletType appletType = appletGetAppletType();
    if (appletType != AppletType_Application && appletType != AppletType_SystemApplication)
    {
        std::printf("[switch] WARNING: running in applet mode (type %d) - memory is limited. "
                    "The game needs a full-memory (application mode) launch.\n",
                    static_cast<int>(appletType));
    }


    setenv("HOME", kAppRoot, 0);


    if (!HasArg(argc, argv, "-C") && !HasArg(argc, argv, "--work-dir"))
    {
        if (DirExists(kDataDir))
        {
            g_argStorage.assign(argv, argv + argc);
            g_argStorage.insert(g_argStorage.begin() + (argc > 0 ? 1 : 0), {"-C", kDataDir});
            for (auto& a : g_argStorage)
                g_argv.push_back(a.data());
            g_argv.push_back(nullptr);
            argc = static_cast<int>(g_argStorage.size());
            argv = g_argv.data();
            std::printf("[switch] using data dir %s\n", kDataDir);
        }
        else
        {
            std::printf("[switch] no -C given and %s not found - copy the game data there\n", kDataDir);
        }
    }

    for (int i = 0; i < argc; ++i)
        std::printf("[switch] argv[%d] = %s\n", i, argv[i]);
}

void SwitchShowError(const char* message, const char* details)
{
    ErrorApplicationConfig cfg;
    if (R_SUCCEEDED(errorApplicationCreate(&cfg, message ? message : "", details)))
        errorApplicationShow(&cfg);
}

void SwitchShutdown()
{
    std::printf("[switch] shutdown\n");
    std::fflush(stdout);
}
}

#endif // __SWITCH__
