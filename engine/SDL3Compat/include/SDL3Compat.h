#pragma once

// SDL3 API surface implemented on top of SDL2 — Nintendo Switch homebrew only.
//
// devkitPro ships SDL2 (switch-sdl2) but no SDL3. Rather than fork ~90 engine
// files, the Switch build puts include/SDL3/*.h (thin forwarders to this header)
// on the include path, so engine code keeps compiling against SDL3 names and
// semantics. Only the subset the engine actually uses is provided; anything
// missing fails to compile, which is the intended signal to extend this file.
//
// Design notes:
//  - Engine-facing types (SDL_Event, SDL_DisplayMode, SDL_Rect, ...) use SDL3
//    layouts and are defined here. SDL2 headers are never included into engine
//    TUs except the pure-enum SDL_scancode.h and SDL_stdinc.h.
//  - Functions live in namespace sdl3compat (C++ linkage) so they cannot collide
//    with SDL2's C symbols of the same name; a using-directive exposes them.
//  - SDL3 returns bool (true = success) where SDL2 returned int (0 = success).
//    All conversion happens in SDL3Compat.cpp — callers keep SDL3 semantics.
//  - Display IDs are SDL2 display index + 1 (SDL3 IDs are never 0).

#ifndef __cplusplus
#error "SDL3Compat is C++ only"
#endif

#include <SDL2/SDL_stdinc.h>
#include <SDL2/SDL_scancode.h>

#include <cstdint>

// ── Basic types ─────────────────────────────────────────────────────────────

typedef Uint32 SDL_InitFlags;
typedef Uint32 SDL_DisplayID;
typedef Uint32 SDL_WindowID;
typedef Uint32 SDL_JoystickID;
typedef Uint32 SDL_KeyboardID;
typedef Uint32 SDL_MouseID;
typedef Uint32 SDL_Keycode;
typedef Uint16 SDL_Keymod;
typedef Uint32 SDL_PixelFormat;
typedef Uint64 SDL_WindowFlags;
typedef Uint32 SDL_MessageBoxFlags;
typedef void (*SDL_FunctionPointer)(void);

typedef struct SDL_Window SDL_Window;
typedef struct SDL_Gamepad SDL_Gamepad;
typedef void* SDL_GLContext;

typedef struct SDL_Point { int x, y; } SDL_Point;
typedef struct SDL_FPoint { float x, y; } SDL_FPoint;
typedef struct SDL_Rect { int x, y, w, h; } SDL_Rect;
typedef struct SDL_FRect { float x, y, w, h; } SDL_FRect;

typedef struct SDL_DisplayMode
{
    SDL_DisplayID displayID;
    SDL_PixelFormat format;
    int w;
    int h;
    float pixel_density;
    float refresh_rate;
    int refresh_rate_numerator;
    int refresh_rate_denominator;
    void* internal;
} SDL_DisplayMode;

#define SDL_BITSPERPIXEL(X) (((X) >> 8) & 0xFF)

// ── Init flags (values identical in SDL2 and SDL3) ──────────────────────────

#define SDL_INIT_AUDIO    0x00000010u
#define SDL_INIT_VIDEO    0x00000020u
#define SDL_INIT_JOYSTICK 0x00000200u
#define SDL_INIT_HAPTIC   0x00001000u
#define SDL_INIT_GAMEPAD  0x00002000u
#define SDL_INIT_EVENTS   0x00004000u
#define SDL_INIT_SENSOR   0x00008000u

// ── Window flags (low 32 bits identical in SDL2 and SDL3) ───────────────────

#define SDL_WINDOW_FULLSCREEN         0x0000000000000001ull
#define SDL_WINDOW_OPENGL             0x0000000000000002ull
#define SDL_WINDOW_HIDDEN             0x0000000000000008ull
#define SDL_WINDOW_BORDERLESS         0x0000000000000010ull
#define SDL_WINDOW_RESIZABLE          0x0000000000000020ull
#define SDL_WINDOW_MINIMIZED          0x0000000000000040ull
#define SDL_WINDOW_MAXIMIZED          0x0000000000000080ull
#define SDL_WINDOW_MOUSE_GRABBED      0x0000000000000100ull
#define SDL_WINDOW_INPUT_FOCUS        0x0000000000000200ull
#define SDL_WINDOW_MOUSE_FOCUS        0x0000000000000400ull
#define SDL_WINDOW_HIGH_PIXEL_DENSITY 0x0000000000002000ull

#define SDL_WINDOWPOS_UNDEFINED_MASK       0x1FFF0000u
#define SDL_WINDOWPOS_UNDEFINED_DISPLAY(X) (SDL_WINDOWPOS_UNDEFINED_MASK | (X))
#define SDL_WINDOWPOS_UNDEFINED            SDL_WINDOWPOS_UNDEFINED_DISPLAY(0)
#define SDL_WINDOWPOS_CENTERED_MASK        0x2FFF0000u
#define SDL_WINDOWPOS_CENTERED_DISPLAY(X)  (SDL_WINDOWPOS_CENTERED_MASK | (X))
#define SDL_WINDOWPOS_CENTERED             SDL_WINDOWPOS_CENTERED_DISPLAY(0)

// ── Message box ─────────────────────────────────────────────────────────────

#define SDL_MESSAGEBOX_ERROR       0x00000010u
#define SDL_MESSAGEBOX_WARNING     0x00000020u
#define SDL_MESSAGEBOX_INFORMATION 0x00000040u

// ── OpenGL ──────────────────────────────────────────────────────────────────

typedef enum SDL_GLAttr
{
    SDL_GL_RED_SIZE,
    SDL_GL_GREEN_SIZE,
    SDL_GL_BLUE_SIZE,
    SDL_GL_ALPHA_SIZE,
    SDL_GL_BUFFER_SIZE,
    SDL_GL_DOUBLEBUFFER,
    SDL_GL_DEPTH_SIZE,
    SDL_GL_STENCIL_SIZE,
    SDL_GL_ACCUM_RED_SIZE,
    SDL_GL_ACCUM_GREEN_SIZE,
    SDL_GL_ACCUM_BLUE_SIZE,
    SDL_GL_ACCUM_ALPHA_SIZE,
    SDL_GL_STEREO,
    SDL_GL_MULTISAMPLEBUFFERS,
    SDL_GL_MULTISAMPLESAMPLES,
    SDL_GL_ACCELERATED_VISUAL,
    SDL_GL_RETAINED_BACKING,
    SDL_GL_CONTEXT_MAJOR_VERSION,
    SDL_GL_CONTEXT_MINOR_VERSION,
    SDL_GL_CONTEXT_FLAGS,
    SDL_GL_CONTEXT_PROFILE_MASK,
    SDL_GL_SHARE_WITH_CURRENT_CONTEXT,
    SDL_GL_FRAMEBUFFER_SRGB_CAPABLE,
    SDL_GL_CONTEXT_RELEASE_BEHAVIOR,
    SDL_GL_CONTEXT_RESET_NOTIFICATION,
    SDL_GL_CONTEXT_NO_ERROR,
    SDL_GL_FLOATBUFFERS,
} SDL_GLAttr;

#define SDL_GL_CONTEXT_PROFILE_CORE           0x0001
#define SDL_GL_CONTEXT_PROFILE_COMPATIBILITY  0x0002
#define SDL_GL_CONTEXT_PROFILE_ES             0x0004
#define SDL_GL_CONTEXT_DEBUG_FLAG              0x0001
#define SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG 0x0002
#define SDL_GL_CONTEXT_ROBUST_ACCESS_FLAG      0x0004
#define SDL_GL_CONTEXT_RESET_ISOLATION_FLAG    0x0008

// ── Keyboard ────────────────────────────────────────────────────────────────
// Scancodes come from SDL2's SDL_scancode.h (same USB-HID values). SDL3 names
// that SDL2 lacks are mapped to their SDL2 equivalent, or to an unused slot.

#define SDL_SCANCODE_COUNT                SDL_NUM_SCANCODES
#define SDL_SCANCODE_MEDIA_NEXT_TRACK     SDL_SCANCODE_AUDIONEXT
#define SDL_SCANCODE_MEDIA_PREVIOUS_TRACK SDL_SCANCODE_AUDIOPREV
#define SDL_SCANCODE_MEDIA_STOP           SDL_SCANCODE_AUDIOSTOP
#define SDL_SCANCODE_MEDIA_PLAY_PAUSE     SDL_SCANCODE_AUDIOPLAY
#define SDL_SCANCODE_MEDIA_SELECT         SDL_SCANCODE_MEDIASELECT
#define SDL_SCANCODE_MEDIA_REWIND         SDL_SCANCODE_AUDIOREWIND
#define SDL_SCANCODE_MEDIA_FAST_FORWARD   SDL_SCANCODE_AUDIOFASTFORWARD
#define SDL_SCANCODE_WAKE                 static_cast<SDL_Scancode>(500) // not in SDL2; never reported

#define SDLK_SCANCODE_MASK         (1u << 30)
#define SDL_SCANCODE_TO_KEYCODE(X) (static_cast<SDL_Keycode>(X) | SDLK_SCANCODE_MASK)

#define SDLK_UNKNOWN     0x00000000u
#define SDLK_RETURN      0x0000000du
#define SDLK_ESCAPE      0x0000001bu
#define SDLK_BACKSPACE   0x00000008u
#define SDLK_TAB         0x00000009u
#define SDLK_SPACE       0x00000020u
#define SDLK_PLUS        0x0000002bu
#define SDLK_MINUS       0x0000002du
#define SDLK_DELETE      0x0000007fu
#define SDLK_F1          SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_F1)
#define SDLK_F2          SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_F2)
#define SDLK_F3          SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_F3)
#define SDLK_F4          SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_F4)
#define SDLK_F5          SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_F5)
#define SDLK_F6          SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_F6)
#define SDLK_INSERT      SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_INSERT)
#define SDLK_HOME        SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_HOME)
#define SDLK_PAGEUP      SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_PAGEUP)
#define SDLK_END         SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_END)
#define SDLK_PAGEDOWN    SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_PAGEDOWN)
#define SDLK_RIGHT       SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_RIGHT)
#define SDLK_LEFT        SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_LEFT)
#define SDLK_DOWN        SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_DOWN)
#define SDLK_UP          SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_UP)
#define SDLK_KP_MULTIPLY SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_MULTIPLY)
#define SDLK_KP_MINUS    SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_MINUS)
#define SDLK_KP_PLUS     SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_PLUS)
#define SDLK_KP_ENTER    SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_ENTER)
#define SDLK_KP_1        SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_1)
#define SDLK_KP_2        SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_2)
#define SDLK_KP_3        SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_3)
#define SDLK_KP_4        SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_4)
#define SDLK_KP_5        SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_5)
#define SDLK_KP_6        SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_6)
#define SDLK_KP_7        SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_7)
#define SDLK_KP_8        SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_8)
#define SDLK_KP_9        SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_9)
#define SDLK_LCTRL       SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_LCTRL)
#define SDLK_LSHIFT      SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_LSHIFT)
#define SDLK_LALT        SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_LALT)
#define SDLK_LGUI        SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_LGUI)
#define SDLK_RCTRL       SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_RCTRL)
#define SDLK_RSHIFT      SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_RSHIFT)
#define SDLK_RALT        SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_RALT)
#define SDLK_RGUI        SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_RGUI)

#define SDL_KMOD_NONE   0x0000u
#define SDL_KMOD_LSHIFT 0x0001u
#define SDL_KMOD_RSHIFT 0x0002u
#define SDL_KMOD_LCTRL  0x0040u
#define SDL_KMOD_RCTRL  0x0080u
#define SDL_KMOD_LALT   0x0100u
#define SDL_KMOD_RALT   0x0200u
#define SDL_KMOD_LGUI   0x0400u
#define SDL_KMOD_RGUI   0x0800u
#define SDL_KMOD_NUM    0x1000u
#define SDL_KMOD_CAPS   0x2000u
#define SDL_KMOD_MODE   0x4000u
#define SDL_KMOD_SCROLL 0x8000u
#define SDL_KMOD_CTRL   (SDL_KMOD_LCTRL | SDL_KMOD_RCTRL)
#define SDL_KMOD_SHIFT  (SDL_KMOD_LSHIFT | SDL_KMOD_RSHIFT)
#define SDL_KMOD_ALT    (SDL_KMOD_LALT | SDL_KMOD_RALT)
#define SDL_KMOD_GUI    (SDL_KMOD_LGUI | SDL_KMOD_RGUI)

// ── Mouse ───────────────────────────────────────────────────────────────────

#define SDL_BUTTON_LEFT   1
#define SDL_BUTTON_MIDDLE 2
#define SDL_BUTTON_RIGHT  3
#define SDL_BUTTON_X1     4
#define SDL_BUTTON_X2     5

// ── Gamepad (SDL3 positional names; numerically equal to SDL2's A/B/X/Y order) ─

typedef enum SDL_GamepadButton
{
    SDL_GAMEPAD_BUTTON_INVALID = -1,
    SDL_GAMEPAD_BUTTON_SOUTH,
    SDL_GAMEPAD_BUTTON_EAST,
    SDL_GAMEPAD_BUTTON_WEST,
    SDL_GAMEPAD_BUTTON_NORTH,
    SDL_GAMEPAD_BUTTON_BACK,
    SDL_GAMEPAD_BUTTON_GUIDE,
    SDL_GAMEPAD_BUTTON_START,
    SDL_GAMEPAD_BUTTON_LEFT_STICK,
    SDL_GAMEPAD_BUTTON_RIGHT_STICK,
    SDL_GAMEPAD_BUTTON_LEFT_SHOULDER,
    SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER,
    SDL_GAMEPAD_BUTTON_DPAD_UP,
    SDL_GAMEPAD_BUTTON_DPAD_DOWN,
    SDL_GAMEPAD_BUTTON_DPAD_LEFT,
    SDL_GAMEPAD_BUTTON_DPAD_RIGHT,
    SDL_GAMEPAD_BUTTON_MISC1,
    SDL_GAMEPAD_BUTTON_COUNT = 21
} SDL_GamepadButton;

typedef enum SDL_GamepadAxis
{
    SDL_GAMEPAD_AXIS_INVALID = -1,
    SDL_GAMEPAD_AXIS_LEFTX,
    SDL_GAMEPAD_AXIS_LEFTY,
    SDL_GAMEPAD_AXIS_RIGHTX,
    SDL_GAMEPAD_AXIS_RIGHTY,
    SDL_GAMEPAD_AXIS_LEFT_TRIGGER,
    SDL_GAMEPAD_AXIS_RIGHT_TRIGGER,
    SDL_GAMEPAD_AXIS_COUNT
} SDL_GamepadAxis;

// ── Events (SDL3 layout) ────────────────────────────────────────────────────

typedef enum SDL_EventType
{
    SDL_EVENT_FIRST = 0,
    SDL_EVENT_QUIT = 0x100,
    SDL_EVENT_TERMINATING,
    SDL_EVENT_LOW_MEMORY,
    SDL_EVENT_WILL_ENTER_BACKGROUND,
    SDL_EVENT_DID_ENTER_BACKGROUND,
    SDL_EVENT_WILL_ENTER_FOREGROUND,
    SDL_EVENT_DID_ENTER_FOREGROUND,

    SDL_EVENT_WINDOW_SHOWN = 0x202,
    SDL_EVENT_WINDOW_HIDDEN,
    SDL_EVENT_WINDOW_EXPOSED,
    SDL_EVENT_WINDOW_MOVED,
    SDL_EVENT_WINDOW_RESIZED,
    SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED,
    SDL_EVENT_WINDOW_METAL_VIEW_RESIZED,
    SDL_EVENT_WINDOW_MINIMIZED,
    SDL_EVENT_WINDOW_MAXIMIZED,
    SDL_EVENT_WINDOW_RESTORED,
    SDL_EVENT_WINDOW_MOUSE_ENTER,
    SDL_EVENT_WINDOW_MOUSE_LEAVE,
    SDL_EVENT_WINDOW_FOCUS_GAINED,
    SDL_EVENT_WINDOW_FOCUS_LOST,
    SDL_EVENT_WINDOW_CLOSE_REQUESTED,
    SDL_EVENT_WINDOW_HIT_TEST,
    SDL_EVENT_WINDOW_ICCPROF_CHANGED,
    SDL_EVENT_WINDOW_DISPLAY_CHANGED,
    SDL_EVENT_WINDOW_DISPLAY_SCALE_CHANGED,
    SDL_EVENT_WINDOW_SAFE_AREA_CHANGED,
    SDL_EVENT_WINDOW_OCCLUDED,
    SDL_EVENT_WINDOW_ENTER_FULLSCREEN,
    SDL_EVENT_WINDOW_LEAVE_FULLSCREEN,
    SDL_EVENT_WINDOW_DESTROYED,

    SDL_EVENT_KEY_DOWN = 0x300,
    SDL_EVENT_KEY_UP,
    SDL_EVENT_TEXT_EDITING,
    SDL_EVENT_TEXT_INPUT,

    SDL_EVENT_MOUSE_MOTION = 0x400,
    SDL_EVENT_MOUSE_BUTTON_DOWN,
    SDL_EVENT_MOUSE_BUTTON_UP,
    SDL_EVENT_MOUSE_WHEEL,

    SDL_EVENT_GAMEPAD_AXIS_MOTION = 0x650,
    SDL_EVENT_GAMEPAD_BUTTON_DOWN,
    SDL_EVENT_GAMEPAD_BUTTON_UP,
    SDL_EVENT_GAMEPAD_ADDED,
    SDL_EVENT_GAMEPAD_REMOVED,
    SDL_EVENT_GAMEPAD_REMAPPED,

    SDL_EVENT_USER = 0x8000,
    SDL_EVENT_LAST = 0xFFFF
} SDL_EventType;

typedef struct SDL_CommonEvent
{
    Uint32 type;
    Uint32 reserved;
    Uint64 timestamp;
} SDL_CommonEvent;

typedef struct SDL_WindowEvent
{
    Uint32 type;
    Uint32 reserved;
    Uint64 timestamp;
    SDL_WindowID windowID;
    Sint32 data1;
    Sint32 data2;
} SDL_WindowEvent;

typedef struct SDL_KeyboardEvent
{
    Uint32 type;
    Uint32 reserved;
    Uint64 timestamp;
    SDL_WindowID windowID;
    SDL_KeyboardID which;
    SDL_Scancode scancode;
    SDL_Keycode key;
    SDL_Keymod mod;
    Uint16 raw;
    bool down;
    bool repeat;
} SDL_KeyboardEvent;

typedef struct SDL_TextInputEvent
{
    Uint32 type;
    Uint32 reserved;
    Uint64 timestamp;
    SDL_WindowID windowID;
    const char* text;
} SDL_TextInputEvent;

typedef struct SDL_MouseMotionEvent
{
    Uint32 type;
    Uint32 reserved;
    Uint64 timestamp;
    SDL_WindowID windowID;
    SDL_MouseID which;
    Uint32 state;
    float x;
    float y;
    float xrel;
    float yrel;
} SDL_MouseMotionEvent;

typedef struct SDL_MouseButtonEvent
{
    Uint32 type;
    Uint32 reserved;
    Uint64 timestamp;
    SDL_WindowID windowID;
    SDL_MouseID which;
    Uint8 button;
    bool down;
    Uint8 clicks;
    Uint8 padding;
    float x;
    float y;
} SDL_MouseButtonEvent;

typedef struct SDL_MouseWheelEvent
{
    Uint32 type;
    Uint32 reserved;
    Uint64 timestamp;
    SDL_WindowID windowID;
    SDL_MouseID which;
    float x;
    float y;
    Uint32 direction;
    float mouse_x;
    float mouse_y;
} SDL_MouseWheelEvent;

typedef struct SDL_GamepadDeviceEvent
{
    Uint32 type;
    Uint32 reserved;
    Uint64 timestamp;
    SDL_JoystickID which;
} SDL_GamepadDeviceEvent;

typedef struct SDL_QuitEvent
{
    Uint32 type;
    Uint32 reserved;
    Uint64 timestamp;
} SDL_QuitEvent;

typedef union SDL_Event
{
    Uint32 type;
    SDL_CommonEvent common;
    SDL_WindowEvent window;
    SDL_KeyboardEvent key;
    SDL_TextInputEvent text;
    SDL_MouseMotionEvent motion;
    SDL_MouseButtonEvent button;
    SDL_MouseWheelEvent wheel;
    SDL_GamepadDeviceEvent gdevice;
    SDL_QuitEvent quit;
    Uint8 padding[128];
} SDL_Event;

// ── Functions ───────────────────────────────────────────────────────────────

namespace sdl3compat
{
// Init / error / misc
bool SDL_Init(SDL_InitFlags flags);
bool SDL_InitSubSystem(SDL_InitFlags flags);
void SDL_QuitSubSystem(SDL_InitFlags flags);
SDL_InitFlags SDL_WasInit(SDL_InitFlags flags);
void SDL_Quit();
const char* SDL_GetError();
Uint64 SDL_GetTicks();
int SDL_GetSystemRAM();
bool SDL_ShowSimpleMessageBox(SDL_MessageBoxFlags flags, const char* title, const char* message, SDL_Window* window);
bool SDL_SetClipboardText(const char* text);
char* SDL_GetClipboardText();

// Events
bool SDL_PollEvent(SDL_Event* event);
bool SDL_PushEvent(SDL_Event* event);
void SDL_PumpEvents();

// Keyboard
const bool* SDL_GetKeyboardState(int* numkeys);
SDL_Keymod SDL_GetModState();
SDL_Keycode SDL_GetKeyFromScancode(SDL_Scancode scancode, SDL_Keymod modstate, bool key_event);
SDL_Scancode SDL_GetScancodeFromKey(SDL_Keycode key, SDL_Keymod* modstate);
bool SDL_StartTextInput(SDL_Window* window);
bool SDL_StopTextInput(SDL_Window* window);
bool SDL_TextInputActive(SDL_Window* window);

// Mouse
bool SDL_ShowCursor();
bool SDL_HideCursor();
bool SDL_SetWindowRelativeMouseMode(SDL_Window* window, bool enabled);

// Windows
SDL_Window* SDL_CreateWindow(const char* title, int w, int h, SDL_WindowFlags flags);
void SDL_DestroyWindow(SDL_Window* window);
bool SDL_SetWindowTitle(SDL_Window* window, const char* title);
bool SDL_SetWindowSize(SDL_Window* window, int w, int h);
bool SDL_GetWindowSize(SDL_Window* window, int* w, int* h);
bool SDL_GetWindowSizeInPixels(SDL_Window* window, int* w, int* h);
bool SDL_SetWindowPosition(SDL_Window* window, int x, int y);
bool SDL_SetWindowBordered(SDL_Window* window, bool bordered);
bool SDL_SetWindowResizable(SDL_Window* window, bool resizable);
SDL_WindowFlags SDL_GetWindowFlags(SDL_Window* window);
bool SDL_SetWindowFullscreen(SDL_Window* window, bool fullscreen);
bool SDL_SetWindowFullscreenMode(SDL_Window* window, const SDL_DisplayMode* mode);
const SDL_DisplayMode* SDL_GetWindowFullscreenMode(SDL_Window* window);
float SDL_GetWindowPixelDensity(SDL_Window* window);
float SDL_GetWindowDisplayScale(SDL_Window* window);

// Displays
SDL_DisplayID* SDL_GetDisplays(int* count);
SDL_DisplayID SDL_GetPrimaryDisplay();
SDL_DisplayID SDL_GetDisplayForWindow(SDL_Window* window);
const char* SDL_GetDisplayName(SDL_DisplayID displayID);
bool SDL_GetDisplayBounds(SDL_DisplayID displayID, SDL_Rect* rect);
bool SDL_GetDisplayUsableBounds(SDL_DisplayID displayID, SDL_Rect* rect);
const SDL_DisplayMode* SDL_GetDesktopDisplayMode(SDL_DisplayID displayID);
const SDL_DisplayMode* SDL_GetCurrentDisplayMode(SDL_DisplayID displayID);
SDL_DisplayMode** SDL_GetFullscreenDisplayModes(SDL_DisplayID displayID, int* count);

// OpenGL
bool SDL_GL_SetAttribute(SDL_GLAttr attr, int value);
SDL_GLContext SDL_GL_CreateContext(SDL_Window* window);
bool SDL_GL_DestroyContext(SDL_GLContext context);
bool SDL_GL_SwapWindow(SDL_Window* window);
bool SDL_GL_SetSwapInterval(int interval);
bool SDL_GL_GetSwapInterval(int* interval);
SDL_FunctionPointer SDL_GL_GetProcAddress(const char* proc);

// Gamepads
SDL_JoystickID* SDL_GetGamepads(int* count);
bool SDL_HasGamepad();
SDL_Gamepad* SDL_OpenGamepad(SDL_JoystickID instance_id);
void SDL_CloseGamepad(SDL_Gamepad* gamepad);
SDL_JoystickID SDL_GetGamepadID(SDL_Gamepad* gamepad);
bool SDL_GetGamepadButton(SDL_Gamepad* gamepad, SDL_GamepadButton button);
Sint16 SDL_GetGamepadAxis(SDL_Gamepad* gamepad, SDL_GamepadAxis axis);
bool SDL_RumbleGamepad(SDL_Gamepad* gamepad, Uint16 low_frequency_rumble, Uint16 high_frequency_rumble, Uint32 duration_ms);
} // namespace sdl3compat

using namespace sdl3compat;
