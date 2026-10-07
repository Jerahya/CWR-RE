// SDL3 API on SDL2 — see include/SDL3Compat.h for the design.
//
// This TU sees both worlds: the SDL3-shaped engine types (global namespace, from
// SDL3Compat.h) and the real SDL2 API (wrapped in namespace sdl2 below; the C
// linkage of the SDL2 declarations is preserved, so calls still bind to libSDL2).

// System headers SDL2 pulls in must be included before the namespaced include,
// otherwise their declarations would land inside namespace sdl2.
#include <cctype>
#include <cfloat>
#include <cinttypes>
#include <climits>
#include <cmath>
#include <cstdarg>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cwchar>
#include <alloca.h>
#include <iconv.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <wchar.h>
#include <unistd.h>

// Shared with the engine-facing header: base types, SDL_malloc/SDL_free and the
// scancode enum stay global so both sides agree on them.
#include <SDL2/SDL_stdinc.h>
#include <SDL2/SDL_scancode.h>

namespace sdl2
{
#include <SDL2/SDL.h>
} // namespace sdl2

// SDL2 values needed below whose names SDL3Compat.h redefines as macros.
constexpr Uint32 kSdl2Fullscreen = sdl2::SDL_WINDOW_FULLSCREEN;
constexpr Uint32 kSdl2FullscreenDesktop = sdl2::SDL_WINDOW_FULLSCREEN_DESKTOP;
constexpr Uint32 kSdl2Shown = sdl2::SDL_WINDOW_SHOWN;

// SDL2 macros that SDL3Compat.h defines with SDL3 meaning.
#undef SDL_INIT_AUDIO
#undef SDL_INIT_VIDEO
#undef SDL_INIT_JOYSTICK
#undef SDL_INIT_HAPTIC
#undef SDL_INIT_EVENTS
#undef SDL_INIT_SENSOR
#undef SDL_WINDOWPOS_UNDEFINED_MASK
#undef SDL_WINDOWPOS_UNDEFINED_DISPLAY
#undef SDL_WINDOWPOS_UNDEFINED
#undef SDL_WINDOWPOS_CENTERED_MASK
#undef SDL_WINDOWPOS_CENTERED_DISPLAY
#undef SDL_WINDOWPOS_CENTERED
#undef SDL_BITSPERPIXEL
#undef SDLK_SCANCODE_MASK
#undef SDL_SCANCODE_TO_KEYCODE
#undef SDL_BUTTON_LEFT
#undef SDL_BUTTON_MIDDLE
#undef SDL_BUTTON_RIGHT
#undef SDL_BUTTON_X1
#undef SDL_BUTTON_X2

#include "SDL3Compat.h"

#include <deque>
#include <map>
#include <mutex>
#include <optional>

namespace
{
sdl2::SDL_Window* W(SDL_Window* window)
{
    return reinterpret_cast<sdl2::SDL_Window*>(window);
}

sdl2::SDL_GameController* G(SDL_Gamepad* gamepad)
{
    return reinterpret_cast<sdl2::SDL_GameController*>(gamepad);
}

SDL_bool B(bool v)
{
    return v ? SDL_TRUE : SDL_FALSE;
}

int DisplayIndex(SDL_DisplayID id)
{
    return id == 0 ? -1 : static_cast<int>(id) - 1;
}

SDL_DisplayID DisplayIdFromIndex(int index)
{
    return index < 0 ? 0 : static_cast<SDL_DisplayID>(index + 1);
}

int TranslateWindowPos(int v)
{
    const Uint32 u = static_cast<Uint32>(v);
    const Uint32 mask = u & 0xFFFF0000u;
    if (mask == SDL_WINDOWPOS_CENTERED_MASK || mask == SDL_WINDOWPOS_UNDEFINED_MASK)
    {
        const int index = DisplayIndex(u & 0xFFFFu);
        return static_cast<int>(mask | static_cast<Uint32>(index < 0 ? 0 : index));
    }
    return v;
}

SDL_DisplayMode FromSdl2(const sdl2::SDL_DisplayMode& m, SDL_DisplayID id)
{
    SDL_DisplayMode out{};
    out.displayID = id;
    out.format = m.format;
    out.w = m.w;
    out.h = m.h;
    out.pixel_density = 1.0f;
    out.refresh_rate = static_cast<float>(m.refresh_rate);
    out.refresh_rate_numerator = m.refresh_rate;
    out.refresh_rate_denominator = 1;
    return out;
}

sdl2::SDL_DisplayMode ToSdl2(const SDL_DisplayMode& m)
{
    sdl2::SDL_DisplayMode out{};
    out.format = m.format;
    out.w = m.w;
    out.h = m.h;
    out.refresh_rate = static_cast<int>(std::lround(m.refresh_rate));
    return out;
}

// SDL3 hands out pointers to display modes it owns; keep ours alive here.
std::map<SDL_DisplayID, SDL_DisplayMode> g_desktopModes;
std::map<SDL_DisplayID, SDL_DisplayMode> g_currentModes;
std::map<SDL_Window*, std::optional<SDL_DisplayMode>> g_fullscreenModes;

// Events pushed through SDL_PushEvent or synthesized here (e.g. fullscreen
// enter/leave, which SDL2 never reports). Drained before the SDL2 queue.
std::mutex g_pendingMutex;
std::deque<SDL_Event> g_pending;

// SDL3 text events point at SDL-owned storage valid until the next poll.
char g_textBuffer[64];

void Enqueue(const SDL_Event& ev)
{
    std::lock_guard lock(g_pendingMutex);
    g_pending.push_back(ev);
}

bool PopPending(SDL_Event* out)
{
    std::lock_guard lock(g_pendingMutex);
    if (g_pending.empty())
        return false;
    if (out)
        *out = g_pending.front();
    g_pending.pop_front();
    return true;
}

void QueueWindowEvent(SDL_Window* window, Uint32 type)
{
    SDL_Event ev{};
    ev.window.type = type;
    ev.window.timestamp = sdl2::SDL_GetTicks64() * 1000000ull;
    ev.window.windowID = window ? sdl2::SDL_GetWindowID(W(window)) : 0;
    Enqueue(ev);
}

Uint32 TranslateWindowEventId(Uint8 id)
{
    switch (id)
    {
    case sdl2::SDL_WINDOWEVENT_SHOWN:        return SDL_EVENT_WINDOW_SHOWN;
    case sdl2::SDL_WINDOWEVENT_HIDDEN:       return SDL_EVENT_WINDOW_HIDDEN;
    case sdl2::SDL_WINDOWEVENT_EXPOSED:      return SDL_EVENT_WINDOW_EXPOSED;
    case sdl2::SDL_WINDOWEVENT_MOVED:        return SDL_EVENT_WINDOW_MOVED;
    case sdl2::SDL_WINDOWEVENT_RESIZED:      return SDL_EVENT_WINDOW_RESIZED;
    case sdl2::SDL_WINDOWEVENT_SIZE_CHANGED: return SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED;
    case sdl2::SDL_WINDOWEVENT_MINIMIZED:    return SDL_EVENT_WINDOW_MINIMIZED;
    case sdl2::SDL_WINDOWEVENT_MAXIMIZED:    return SDL_EVENT_WINDOW_MAXIMIZED;
    case sdl2::SDL_WINDOWEVENT_RESTORED:     return SDL_EVENT_WINDOW_RESTORED;
    case sdl2::SDL_WINDOWEVENT_ENTER:        return SDL_EVENT_WINDOW_MOUSE_ENTER;
    case sdl2::SDL_WINDOWEVENT_LEAVE:        return SDL_EVENT_WINDOW_MOUSE_LEAVE;
    case sdl2::SDL_WINDOWEVENT_FOCUS_GAINED: return SDL_EVENT_WINDOW_FOCUS_GAINED;
    case sdl2::SDL_WINDOWEVENT_FOCUS_LOST:   return SDL_EVENT_WINDOW_FOCUS_LOST;
    case sdl2::SDL_WINDOWEVENT_CLOSE:        return SDL_EVENT_WINDOW_CLOSE_REQUESTED;
    case sdl2::SDL_WINDOWEVENT_DISPLAY_CHANGED: return SDL_EVENT_WINDOW_DISPLAY_CHANGED;
    default:                                 return 0;
    }
}

// Returns false for SDL2 events with no SDL3 counterpart the engine consumes.
bool Translate(const sdl2::SDL_Event& in, SDL_Event* out)
{
    SDL_Event ev{};
    switch (in.type)
    {
    case sdl2::SDL_QUIT:
        ev.type = SDL_EVENT_QUIT;
        break;

    case sdl2::SDL_APP_TERMINATING:         ev.type = SDL_EVENT_TERMINATING; break;
    case sdl2::SDL_APP_LOWMEMORY:           ev.type = SDL_EVENT_LOW_MEMORY; break;
    case sdl2::SDL_APP_WILLENTERBACKGROUND: ev.type = SDL_EVENT_WILL_ENTER_BACKGROUND; break;
    case sdl2::SDL_APP_DIDENTERBACKGROUND:  ev.type = SDL_EVENT_DID_ENTER_BACKGROUND; break;
    case sdl2::SDL_APP_WILLENTERFOREGROUND: ev.type = SDL_EVENT_WILL_ENTER_FOREGROUND; break;
    case sdl2::SDL_APP_DIDENTERFOREGROUND:  ev.type = SDL_EVENT_DID_ENTER_FOREGROUND; break;

    case sdl2::SDL_WINDOWEVENT:
        ev.type = TranslateWindowEventId(in.window.event);
        if (ev.type == 0)
            return false;
        ev.window.windowID = in.window.windowID;
        ev.window.data1 = in.window.data1;
        ev.window.data2 = in.window.data2;
        break;

    case sdl2::SDL_KEYDOWN:
    case sdl2::SDL_KEYUP:
        ev.type = in.type == sdl2::SDL_KEYDOWN ? SDL_EVENT_KEY_DOWN : SDL_EVENT_KEY_UP;
        ev.key.windowID = in.key.windowID;
        ev.key.scancode = in.key.keysym.scancode;
        ev.key.key = static_cast<SDL_Keycode>(in.key.keysym.sym);
        ev.key.mod = static_cast<SDL_Keymod>(in.key.keysym.mod);
        ev.key.down = in.key.state == SDL_PRESSED;
        ev.key.repeat = in.key.repeat != 0;
        break;

    case sdl2::SDL_TEXTINPUT:
        ev.type = SDL_EVENT_TEXT_INPUT;
        ev.text.windowID = in.text.windowID;
        std::strncpy(g_textBuffer, in.text.text, sizeof(g_textBuffer) - 1);
        g_textBuffer[sizeof(g_textBuffer) - 1] = '\0';
        ev.text.text = g_textBuffer;
        break;

    case sdl2::SDL_MOUSEMOTION:
        ev.type = SDL_EVENT_MOUSE_MOTION;
        ev.motion.windowID = in.motion.windowID;
        ev.motion.which = in.motion.which;
        ev.motion.state = in.motion.state;
        ev.motion.x = static_cast<float>(in.motion.x);
        ev.motion.y = static_cast<float>(in.motion.y);
        ev.motion.xrel = static_cast<float>(in.motion.xrel);
        ev.motion.yrel = static_cast<float>(in.motion.yrel);
        break;

    case sdl2::SDL_MOUSEBUTTONDOWN:
    case sdl2::SDL_MOUSEBUTTONUP:
        ev.type = in.type == sdl2::SDL_MOUSEBUTTONDOWN ? SDL_EVENT_MOUSE_BUTTON_DOWN : SDL_EVENT_MOUSE_BUTTON_UP;
        ev.button.windowID = in.button.windowID;
        ev.button.which = in.button.which;
        ev.button.button = in.button.button;
        ev.button.down = in.button.state == SDL_PRESSED;
        ev.button.clicks = in.button.clicks;
        ev.button.x = static_cast<float>(in.button.x);
        ev.button.y = static_cast<float>(in.button.y);
        break;

    case sdl2::SDL_MOUSEWHEEL:
        ev.type = SDL_EVENT_MOUSE_WHEEL;
        ev.wheel.windowID = in.wheel.windowID;
        ev.wheel.which = in.wheel.which;
        ev.wheel.x = in.wheel.preciseX;
        ev.wheel.y = in.wheel.preciseY;
        ev.wheel.direction = in.wheel.direction;
        ev.wheel.mouse_x = static_cast<float>(in.wheel.mouseX);
        ev.wheel.mouse_y = static_cast<float>(in.wheel.mouseY);
        break;

    case sdl2::SDL_CONTROLLERDEVICEADDED:
        // SDL2 reports a device index here; SDL3 reports the instance ID.
        ev.type = SDL_EVENT_GAMEPAD_ADDED;
        ev.gdevice.which = static_cast<SDL_JoystickID>(sdl2::SDL_JoystickGetDeviceInstanceID(in.cdevice.which));
        break;

    case sdl2::SDL_CONTROLLERDEVICEREMOVED:
        ev.type = SDL_EVENT_GAMEPAD_REMOVED;
        ev.gdevice.which = static_cast<SDL_JoystickID>(in.cdevice.which);
        break;

    case sdl2::SDL_CONTROLLERDEVICEREMAPPED:
        ev.type = SDL_EVENT_GAMEPAD_REMAPPED;
        ev.gdevice.which = static_cast<SDL_JoystickID>(in.cdevice.which);
        break;

    default:
        return false;
    }

    ev.common.timestamp = static_cast<Uint64>(in.common.timestamp) * 1000000ull;
    if (out)
        *out = ev;
    return true;
}

sdl2::SDL_GLattr TranslateGLAttr(SDL_GLAttr attr, bool* ok)
{
    *ok = true;
    switch (attr)
    {
    case SDL_GL_RED_SIZE:                   return sdl2::SDL_GL_RED_SIZE;
    case SDL_GL_GREEN_SIZE:                 return sdl2::SDL_GL_GREEN_SIZE;
    case SDL_GL_BLUE_SIZE:                  return sdl2::SDL_GL_BLUE_SIZE;
    case SDL_GL_ALPHA_SIZE:                 return sdl2::SDL_GL_ALPHA_SIZE;
    case SDL_GL_BUFFER_SIZE:                return sdl2::SDL_GL_BUFFER_SIZE;
    case SDL_GL_DOUBLEBUFFER:               return sdl2::SDL_GL_DOUBLEBUFFER;
    case SDL_GL_DEPTH_SIZE:                 return sdl2::SDL_GL_DEPTH_SIZE;
    case SDL_GL_STENCIL_SIZE:               return sdl2::SDL_GL_STENCIL_SIZE;
    case SDL_GL_ACCUM_RED_SIZE:             return sdl2::SDL_GL_ACCUM_RED_SIZE;
    case SDL_GL_ACCUM_GREEN_SIZE:           return sdl2::SDL_GL_ACCUM_GREEN_SIZE;
    case SDL_GL_ACCUM_BLUE_SIZE:            return sdl2::SDL_GL_ACCUM_BLUE_SIZE;
    case SDL_GL_ACCUM_ALPHA_SIZE:           return sdl2::SDL_GL_ACCUM_ALPHA_SIZE;
    case SDL_GL_STEREO:                     return sdl2::SDL_GL_STEREO;
    case SDL_GL_MULTISAMPLEBUFFERS:         return sdl2::SDL_GL_MULTISAMPLEBUFFERS;
    case SDL_GL_MULTISAMPLESAMPLES:         return sdl2::SDL_GL_MULTISAMPLESAMPLES;
    case SDL_GL_ACCELERATED_VISUAL:         return sdl2::SDL_GL_ACCELERATED_VISUAL;
    case SDL_GL_RETAINED_BACKING:           return sdl2::SDL_GL_RETAINED_BACKING;
    case SDL_GL_CONTEXT_MAJOR_VERSION:      return sdl2::SDL_GL_CONTEXT_MAJOR_VERSION;
    case SDL_GL_CONTEXT_MINOR_VERSION:      return sdl2::SDL_GL_CONTEXT_MINOR_VERSION;
    case SDL_GL_CONTEXT_FLAGS:              return sdl2::SDL_GL_CONTEXT_FLAGS;
    case SDL_GL_CONTEXT_PROFILE_MASK:       return sdl2::SDL_GL_CONTEXT_PROFILE_MASK;
    case SDL_GL_SHARE_WITH_CURRENT_CONTEXT: return sdl2::SDL_GL_SHARE_WITH_CURRENT_CONTEXT;
    case SDL_GL_FRAMEBUFFER_SRGB_CAPABLE:   return sdl2::SDL_GL_FRAMEBUFFER_SRGB_CAPABLE;
    case SDL_GL_CONTEXT_RELEASE_BEHAVIOR:   return sdl2::SDL_GL_CONTEXT_RELEASE_BEHAVIOR;
    case SDL_GL_CONTEXT_RESET_NOTIFICATION: return sdl2::SDL_GL_CONTEXT_RESET_NOTIFICATION;
    case SDL_GL_CONTEXT_NO_ERROR:           return sdl2::SDL_GL_CONTEXT_NO_ERROR;
    case SDL_GL_FLOATBUFFERS:               return sdl2::SDL_GL_FLOATBUFFERS;
    }
    *ok = false;
    return sdl2::SDL_GL_RED_SIZE;
}

void PrepareInit(SDL_InitFlags flags)
{
    // Face buttons are swapped to label order in SDL_GetGamepadButton; devkitPro's
    // SDL2 ignores SDL_GAMECONTROLLER_USE_BUTTON_LABELS, so no hint is set here.
    (void)flags;
}

bool IsExclusiveFullscreen(SDL_Window* window)
{
    const Uint32 f = sdl2::SDL_GetWindowFlags(W(window));
    return (f & kSdl2FullscreenDesktop) == kSdl2Fullscreen;
}
} // namespace

namespace sdl3compat
{
// ── Init / error / misc ─────────────────────────────────────────────────────

bool SDL_Init(SDL_InitFlags flags)
{
    PrepareInit(flags);
    return sdl2::SDL_Init(flags) == 0;
}

bool SDL_InitSubSystem(SDL_InitFlags flags)
{
    PrepareInit(flags);
    return sdl2::SDL_InitSubSystem(flags) == 0;
}

void SDL_QuitSubSystem(SDL_InitFlags flags)
{
    sdl2::SDL_QuitSubSystem(flags);
}

SDL_InitFlags SDL_WasInit(SDL_InitFlags flags)
{
    return sdl2::SDL_WasInit(flags);
}

void SDL_Quit()
{
    sdl2::SDL_Quit();
}

const char* SDL_GetError()
{
    return sdl2::SDL_GetError();
}

Uint64 SDL_GetTicks()
{
    return sdl2::SDL_GetTicks64();
}

int SDL_GetSystemRAM()
{
    const int ram = sdl2::SDL_GetSystemRAM();
    if (ram > 0)
        return ram;
    // devkitPro's SDL2 reports 0 on Switch; use the memory budget of this process
    // (sysconf(_SC_PHYS_PAGES) is implemented over svcGetInfo in PlatformSwitch.cpp).
    const long pages = sysconf(_SC_PHYS_PAGES);
    const long pageSize = sysconf(_SC_PAGESIZE);
    return (pages > 0 && pageSize > 0) ? static_cast<int>((static_cast<long long>(pages) * pageSize) >> 20) : 0;
}

bool SDL_ShowSimpleMessageBox(SDL_MessageBoxFlags flags, const char* title, const char* message, SDL_Window* window)
{
    return sdl2::SDL_ShowSimpleMessageBox(flags, title, message, W(window)) == 0;
}

bool SDL_SetClipboardText(const char* text)
{
    return sdl2::SDL_SetClipboardText(text) == 0;
}

char* SDL_GetClipboardText()
{
    return sdl2::SDL_GetClipboardText();
}

// ── Events ──────────────────────────────────────────────────────────────────

bool SDL_PollEvent(SDL_Event* event)
{
    if (PopPending(event))
        return true;

    sdl2::SDL_Event in;
    while (sdl2::SDL_PollEvent(&in))
    {
        if (Translate(in, event))
            return true;
    }
    return false;
}

bool SDL_PushEvent(SDL_Event* event)
{
    if (!event)
        return false;
    SDL_Event copy = *event;
    if (copy.common.timestamp == 0)
        copy.common.timestamp = sdl2::SDL_GetTicks64() * 1000000ull;
    Enqueue(copy);
    return true;
}

void SDL_PumpEvents()
{
    sdl2::SDL_PumpEvents();
}

// ── Keyboard ────────────────────────────────────────────────────────────────

const bool* SDL_GetKeyboardState(int* numkeys)
{
    // SDL2 returns 0/1 bytes; bool has the same representation on this ABI.
    static_assert(sizeof(bool) == sizeof(Uint8));
    return reinterpret_cast<const bool*>(sdl2::SDL_GetKeyboardState(numkeys));
}

SDL_Keymod SDL_GetModState()
{
    return static_cast<SDL_Keymod>(sdl2::SDL_GetModState());
}

SDL_Keycode SDL_GetKeyFromScancode(SDL_Scancode scancode, SDL_Keymod modstate, bool key_event)
{
    SDL_Keycode key = static_cast<SDL_Keycode>(sdl2::SDL_GetKeyFromScancode(scancode));
    // SDL3 applies Shift/Caps to letters unless the keycode is meant for key
    // events (which stay lowercase). Non-letter shifting is layout-dependent
    // and not reproduced.
    if (!key_event && key >= 'a' && key <= 'z')
    {
        const bool shift = (modstate & SDL_KMOD_SHIFT) != 0;
        const bool caps = (modstate & SDL_KMOD_CAPS) != 0;
        if (shift != caps)
            key = key - 'a' + 'A';
    }
    return key;
}

SDL_Scancode SDL_GetScancodeFromKey(SDL_Keycode key, SDL_Keymod* modstate)
{
    SDL_Keymod mod = SDL_KMOD_NONE;
    if (key >= 'A' && key <= 'Z')
    {
        key = key - 'A' + 'a';
        mod = SDL_KMOD_LSHIFT;
    }
    if (modstate)
        *modstate = mod;
    return sdl2::SDL_GetScancodeFromKey(static_cast<sdl2::SDL_Keycode>(key));
}

bool SDL_StartTextInput(SDL_Window*)
{
    sdl2::SDL_StartTextInput();
    return true;
}

bool SDL_StopTextInput(SDL_Window*)
{
    sdl2::SDL_StopTextInput();
    return true;
}

bool SDL_TextInputActive(SDL_Window*)
{
    return sdl2::SDL_IsTextInputActive() == SDL_TRUE;
}

// ── Mouse ───────────────────────────────────────────────────────────────────

bool SDL_ShowCursor()
{
    return sdl2::SDL_ShowCursor(SDL_ENABLE) >= 0;
}

bool SDL_HideCursor()
{
    return sdl2::SDL_ShowCursor(SDL_DISABLE) >= 0;
}

bool SDL_SetWindowRelativeMouseMode(SDL_Window*, bool enabled)
{
    return sdl2::SDL_SetRelativeMouseMode(B(enabled)) == 0;
}

// ── Windows ─────────────────────────────────────────────────────────────────

SDL_Window* SDL_CreateWindow(const char* title, int w, int h, SDL_WindowFlags flags)
{
    Uint32 f = static_cast<Uint32>(flags & 0xFFFFFFFFull);
    // SDL3 creates fullscreen windows in borderless-desktop mode until a mode is set.
    if (f & kSdl2Fullscreen)
        f |= kSdl2FullscreenDesktop;
    auto* window = sdl2::SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, w, h, f);
    return reinterpret_cast<SDL_Window*>(window);
}

void SDL_DestroyWindow(SDL_Window* window)
{
    g_fullscreenModes.erase(window);
    sdl2::SDL_DestroyWindow(W(window));
}

bool SDL_SetWindowTitle(SDL_Window* window, const char* title)
{
    sdl2::SDL_SetWindowTitle(W(window), title);
    return true;
}

bool SDL_SetWindowSize(SDL_Window* window, int w, int h)
{
    sdl2::SDL_SetWindowSize(W(window), w, h);
    return true;
}

bool SDL_GetWindowSize(SDL_Window* window, int* w, int* h)
{
    sdl2::SDL_GetWindowSize(W(window), w, h);
    return true;
}

bool SDL_GetWindowSizeInPixels(SDL_Window* window, int* w, int* h)
{
    // The engine's only window is an OpenGL window.
    sdl2::SDL_GL_GetDrawableSize(W(window), w, h);
    return true;
}

bool SDL_SetWindowPosition(SDL_Window* window, int x, int y)
{
    sdl2::SDL_SetWindowPosition(W(window), TranslateWindowPos(x), TranslateWindowPos(y));
    return true;
}

bool SDL_SetWindowBordered(SDL_Window* window, bool bordered)
{
    sdl2::SDL_SetWindowBordered(W(window), B(bordered));
    return true;
}

bool SDL_SetWindowResizable(SDL_Window* window, bool resizable)
{
    sdl2::SDL_SetWindowResizable(W(window), B(resizable));
    return true;
}

SDL_WindowFlags SDL_GetWindowFlags(SDL_Window* window)
{
    Uint32 f = sdl2::SDL_GetWindowFlags(W(window));
    // SDL2-only bits that alias different SDL3 flags: SHOWN (SDL3 OCCLUDED)
    // and the FULLSCREEN_DESKTOP marker bit (SDL3 UTILITY).
    f &= ~static_cast<Uint32>(kSdl2Shown);
    f &= ~static_cast<Uint32>(kSdl2FullscreenDesktop & ~kSdl2Fullscreen);
    return f;
}

bool SDL_SetWindowFullscreen(SDL_Window* window, bool fullscreen)
{
    Uint32 mode = 0;
    if (fullscreen)
    {
        const auto it = g_fullscreenModes.find(window);
        const bool exclusive = it != g_fullscreenModes.end() && it->second.has_value();
        if (exclusive)
        {
            const sdl2::SDL_DisplayMode m = ToSdl2(*it->second);
            sdl2::SDL_SetWindowDisplayMode(W(window), &m);
        }
        mode = exclusive ? kSdl2Fullscreen : kSdl2FullscreenDesktop;
    }
    if (sdl2::SDL_SetWindowFullscreen(W(window), mode) != 0)
        return false;
    QueueWindowEvent(window, fullscreen ? SDL_EVENT_WINDOW_ENTER_FULLSCREEN : SDL_EVENT_WINDOW_LEAVE_FULLSCREEN);
    return true;
}

bool SDL_SetWindowFullscreenMode(SDL_Window* window, const SDL_DisplayMode* mode)
{
    auto& slot = g_fullscreenModes[window];
    const Uint32 flags = sdl2::SDL_GetWindowFlags(W(window));
    const bool isFullscreen = (flags & kSdl2Fullscreen) != 0;

    if (mode)
    {
        slot = *mode;
        const sdl2::SDL_DisplayMode m = ToSdl2(*mode);
        if (sdl2::SDL_SetWindowDisplayMode(W(window), &m) != 0)
            return false;
        if (isFullscreen && !IsExclusiveFullscreen(window))
            return sdl2::SDL_SetWindowFullscreen(W(window), kSdl2Fullscreen) == 0;
        return true;
    }

    slot.reset();
    if (isFullscreen && IsExclusiveFullscreen(window))
        return sdl2::SDL_SetWindowFullscreen(W(window), kSdl2FullscreenDesktop) == 0;
    return true;
}

const SDL_DisplayMode* SDL_GetWindowFullscreenMode(SDL_Window* window)
{
    const auto it = g_fullscreenModes.find(window);
    if (it == g_fullscreenModes.end() || !it->second)
        return nullptr;
    return &*it->second;
}

float SDL_GetWindowPixelDensity(SDL_Window* window)
{
    int w = 0, h = 0, pw = 0, ph = 0;
    sdl2::SDL_GetWindowSize(W(window), &w, &h);
    sdl2::SDL_GL_GetDrawableSize(W(window), &pw, &ph);
    return w > 0 ? static_cast<float>(pw) / static_cast<float>(w) : 1.0f;
}

float SDL_GetWindowDisplayScale(SDL_Window* window)
{
    // SDL2 has no content scale; pixel density is the closest match.
    return SDL_GetWindowPixelDensity(window);
}

// ── Displays ────────────────────────────────────────────────────────────────

SDL_DisplayID* SDL_GetDisplays(int* count)
{
    const int n = sdl2::SDL_GetNumVideoDisplays();
    if (n < 0)
    {
        if (count)
            *count = 0;
        return nullptr;
    }
    auto* ids = static_cast<SDL_DisplayID*>(SDL_malloc(sizeof(SDL_DisplayID) * static_cast<size_t>(n + 1)));
    if (!ids)
        return nullptr;
    for (int i = 0; i < n; ++i)
        ids[i] = DisplayIdFromIndex(i);
    ids[n] = 0;
    if (count)
        *count = n;
    return ids;
}

SDL_DisplayID SDL_GetPrimaryDisplay()
{
    return sdl2::SDL_GetNumVideoDisplays() > 0 ? 1 : 0;
}

SDL_DisplayID SDL_GetDisplayForWindow(SDL_Window* window)
{
    return DisplayIdFromIndex(sdl2::SDL_GetWindowDisplayIndex(W(window)));
}

const char* SDL_GetDisplayName(SDL_DisplayID displayID)
{
    return sdl2::SDL_GetDisplayName(DisplayIndex(displayID));
}

bool SDL_GetDisplayBounds(SDL_DisplayID displayID, SDL_Rect* rect)
{
    sdl2::SDL_Rect r{};
    if (sdl2::SDL_GetDisplayBounds(DisplayIndex(displayID), &r) != 0)
        return false;
    *rect = {r.x, r.y, r.w, r.h};
    return true;
}

bool SDL_GetDisplayUsableBounds(SDL_DisplayID displayID, SDL_Rect* rect)
{
    sdl2::SDL_Rect r{};
    if (sdl2::SDL_GetDisplayUsableBounds(DisplayIndex(displayID), &r) != 0)
        return false;
    *rect = {r.x, r.y, r.w, r.h};
    return true;
}

const SDL_DisplayMode* SDL_GetDesktopDisplayMode(SDL_DisplayID displayID)
{
    sdl2::SDL_DisplayMode m{};
    if (sdl2::SDL_GetDesktopDisplayMode(DisplayIndex(displayID), &m) != 0)
        return nullptr;
    return &(g_desktopModes[displayID] = FromSdl2(m, displayID));
}

const SDL_DisplayMode* SDL_GetCurrentDisplayMode(SDL_DisplayID displayID)
{
    sdl2::SDL_DisplayMode m{};
    if (sdl2::SDL_GetCurrentDisplayMode(DisplayIndex(displayID), &m) != 0)
        return nullptr;
    return &(g_currentModes[displayID] = FromSdl2(m, displayID));
}

SDL_DisplayMode** SDL_GetFullscreenDisplayModes(SDL_DisplayID displayID, int* count)
{
    const int index = DisplayIndex(displayID);
    const int n = sdl2::SDL_GetNumDisplayModes(index);
    if (n < 0)
    {
        if (count)
            *count = 0;
        return nullptr;
    }

    // SDL3 contract: one allocation, NULL-terminated pointer array, freed with SDL_free.
    const size_t ptrBytes = sizeof(SDL_DisplayMode*) * static_cast<size_t>(n + 1);
    const size_t modeBytes = sizeof(SDL_DisplayMode) * static_cast<size_t>(n);
    auto* block = static_cast<Uint8*>(SDL_malloc(ptrBytes + modeBytes));
    if (!block)
        return nullptr;
    auto** ptrs = reinterpret_cast<SDL_DisplayMode**>(block);
    auto* modes = reinterpret_cast<SDL_DisplayMode*>(block + ptrBytes);

    int out = 0;
    for (int i = 0; i < n; ++i)
    {
        sdl2::SDL_DisplayMode m{};
        if (sdl2::SDL_GetDisplayMode(index, i, &m) != 0)
            continue;
        modes[out] = FromSdl2(m, displayID);
        ptrs[out] = &modes[out];
        ++out;
    }
    ptrs[out] = nullptr;
    if (count)
        *count = out;
    return ptrs;
}

// ── OpenGL ──────────────────────────────────────────────────────────────────

bool SDL_GL_SetAttribute(SDL_GLAttr attr, int value)
{
    bool ok = false;
    const sdl2::SDL_GLattr a = TranslateGLAttr(attr, &ok);
    if (!ok)
        return sdl2::SDL_SetError("SDL3Compat: unsupported GL attribute %d", static_cast<int>(attr)), false;
    return sdl2::SDL_GL_SetAttribute(a, value) == 0;
}

SDL_GLContext SDL_GL_CreateContext(SDL_Window* window)
{
    return sdl2::SDL_GL_CreateContext(W(window));
}

bool SDL_GL_DestroyContext(SDL_GLContext context)
{
    sdl2::SDL_GL_DeleteContext(context);
    return true;
}

bool SDL_GL_SwapWindow(SDL_Window* window)
{
    sdl2::SDL_GL_SwapWindow(W(window));
    return true;
}

bool SDL_GL_SetSwapInterval(int interval)
{
    return sdl2::SDL_GL_SetSwapInterval(interval) == 0;
}

bool SDL_GL_GetSwapInterval(int* interval)
{
    if (interval)
        *interval = sdl2::SDL_GL_GetSwapInterval();
    return true;
}

SDL_FunctionPointer SDL_GL_GetProcAddress(const char* proc)
{
    return reinterpret_cast<SDL_FunctionPointer>(sdl2::SDL_GL_GetProcAddress(proc));
}

// ── Gamepads ────────────────────────────────────────────────────────────────

SDL_JoystickID* SDL_GetGamepads(int* count)
{
    const int n = sdl2::SDL_NumJoysticks();
    auto* ids = static_cast<SDL_JoystickID*>(SDL_malloc(sizeof(SDL_JoystickID) * static_cast<size_t>((n > 0 ? n : 0) + 1)));
    if (!ids)
        return nullptr;
    int out = 0;
    for (int i = 0; i < n; ++i)
    {
        if (sdl2::SDL_IsGameController(i))
            ids[out++] = static_cast<SDL_JoystickID>(sdl2::SDL_JoystickGetDeviceInstanceID(i));
    }
    ids[out] = 0;
    if (count)
        *count = out;
    return ids;
}

bool SDL_HasGamepad()
{
    const int n = sdl2::SDL_NumJoysticks();
    for (int i = 0; i < n; ++i)
    {
        if (sdl2::SDL_IsGameController(i))
            return true;
    }
    return false;
}

SDL_Gamepad* SDL_OpenGamepad(SDL_JoystickID instance_id)
{
    const int n = sdl2::SDL_NumJoysticks();
    for (int i = 0; i < n; ++i)
    {
        if (static_cast<SDL_JoystickID>(sdl2::SDL_JoystickGetDeviceInstanceID(i)) == instance_id && sdl2::SDL_IsGameController(i))
            return reinterpret_cast<SDL_Gamepad*>(sdl2::SDL_GameControllerOpen(i));
    }
    sdl2::SDL_SetError("SDL3Compat: no gamepad with instance id %u", instance_id);
    return nullptr;
}

void SDL_CloseGamepad(SDL_Gamepad* gamepad)
{
    sdl2::SDL_GameControllerClose(G(gamepad));
}

SDL_JoystickID SDL_GetGamepadID(SDL_Gamepad* gamepad)
{
    return static_cast<SDL_JoystickID>(sdl2::SDL_JoystickInstanceID(sdl2::SDL_GameControllerGetJoystick(G(gamepad))));
}

bool SDL_GetGamepadButton(SDL_Gamepad* gamepad, SDL_GamepadButton button)
{
    // devkitPro's SDL2 maps Joy-Con/Pro Controller face buttons by position (Xbox
    // style: bottom = A) and ignores SDL_GAMECONTROLLER_USE_BUTTON_LABELS. The engine
    // treats SOUTH as confirm and EAST as cancel, so swap to label order: the button
    // marked A confirms, B goes back, and on-screen A/B/X/Y prompts match the labels.
    switch (button)
    {
    case SDL_GAMEPAD_BUTTON_SOUTH: button = SDL_GAMEPAD_BUTTON_EAST; break;
    case SDL_GAMEPAD_BUTTON_EAST:  button = SDL_GAMEPAD_BUTTON_SOUTH; break;
    case SDL_GAMEPAD_BUTTON_WEST:  button = SDL_GAMEPAD_BUTTON_NORTH; break;
    case SDL_GAMEPAD_BUTTON_NORTH: button = SDL_GAMEPAD_BUTTON_WEST; break;
    default: break;
    }
    return sdl2::SDL_GameControllerGetButton(G(gamepad), static_cast<sdl2::SDL_GameControllerButton>(button)) != 0;
}

Sint16 SDL_GetGamepadAxis(SDL_Gamepad* gamepad, SDL_GamepadAxis axis)
{
    return sdl2::SDL_GameControllerGetAxis(G(gamepad), static_cast<sdl2::SDL_GameControllerAxis>(axis));
}

bool SDL_RumbleGamepad(SDL_Gamepad* gamepad, Uint16 low_frequency_rumble, Uint16 high_frequency_rumble, Uint32 duration_ms)
{
    return sdl2::SDL_GameControllerRumble(G(gamepad), low_frequency_rumble, high_frequency_rumble, duration_ms) == 0;
}
} // namespace sdl3compat
