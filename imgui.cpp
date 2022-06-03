#if defined(_MSC_VER) && !defined(_CRT_SECURE_NO_WARNINGS)
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "imgui.h"
#ifndef IMGUI_DISABLE

#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif
#include "imgui_internal.h"

// System includes
#include <ctype.h>      // toupper
#include <stdio.h>      // vsnprintf, sscanf, printf
#if defined(_MSC_VER) && _MSC_VER <= 1500 // MSVC 2008 or earlier
#include <stddef.h>     // intptr_t
#else
#include <stdint.h>     // intptr_t
#endif

// [Windows] On non-Visual Studio compilers, we default to IMGUI_DISABLE_WIN32_DEFAULT_IME_FUNCTIONS unless explicitly enabled
#if defined(_WIN32) && !defined(_MSC_VER) && !defined(IMGUI_ENABLE_WIN32_DEFAULT_IME_FUNCTIONS) && !defined(IMGUI_DISABLE_WIN32_DEFAULT_IME_FUNCTIONS)
#define IMGUI_DISABLE_WIN32_DEFAULT_IME_FUNCTIONS
#endif

// [Windows] OS specific includes (optional)
#if defined(_WIN32) && defined(IMGUI_DISABLE_DEFAULT_FILE_FUNCTIONS) && defined(IMGUI_DISABLE_WIN32_DEFAULT_CLIPBOARD_FUNCTIONS) && defined(IMGUI_DISABLE_WIN32_DEFAULT_IME_FUNCTIONS) && !defined(IMGUI_DISABLE_WIN32_FUNCTIONS)
#define IMGUI_DISABLE_WIN32_FUNCTIONS
#endif
#if defined(_WIN32) && !defined(IMGUI_DISABLE_WIN32_FUNCTIONS)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef __MINGW32__
#include <Windows.h>        // _wfopen, OpenClipboard
#else
#include <windows.h>
#endif
#if defined(WINAPI_FAMILY) && (WINAPI_FAMILY == WINAPI_FAMILY_APP) // UWP doesn't have all Win32 functions
#define IMGUI_DISABLE_WIN32_DEFAULT_CLIPBOARD_FUNCTIONS
#define IMGUI_DISABLE_WIN32_DEFAULT_IME_FUNCTIONS
#endif
#endif

// [Apple] OS specific includes
#if defined(__APPLE__)
#include <TargetConditionals.h>
#endif

// Visual Studio warnings
#ifdef _MSC_VER
#pragma warning (disable: 4127)             // condition expression is constant
#pragma warning (disable: 4996)             // 'This function or variable may be unsafe': strcpy, strdup, sprintf, vsnprintf, sscanf, fopen
#if defined(_MSC_VER) && _MSC_VER >= 1922   // MSVC 2019 16.2 or later
#pragma warning (disable: 5054)             // operator '|': deprecated between enumerations of different types
#endif
#pragma warning (disable: 26451)            // [Static Analyzer] Arithmetic overflow : Using operator 'xxx' on a 4 byte value and then casting the result to a 8 byte value. Cast the value to the wider type before calling operator 'xxx' to avoid overflow(io.2).
#pragma warning (disable: 26495)            // [Static Analyzer] Variable 'XXX' is uninitialized. Always initialize a member variable (type.6).
#pragma warning (disable: 26812)            // [Static Analyzer] The enum type 'xxx' is unscoped. Prefer 'enum class' over 'enum' (Enum.3).
#endif

// Clang/GCC warnings with -Weverything
#if defined(__clang__)
#if __has_warning("-Wunknown-warning-option")
#pragma clang diagnostic ignored "-Wunknown-warning-option"         // warning: unknown warning group 'xxx'                      // not all warnings are known by all Clang versions and they tend to be rename-happy.. so ignoring warnings triggers new warnings on some configuration. Great!
#endif
#pragma clang diagnostic ignored "-Wunknown-pragmas"                // warning: unknown warning group 'xxx'
#pragma clang diagnostic ignored "-Wold-style-cast"                 // warning: use of old-style cast                            // yes, they are more terse.
#pragma clang diagnostic ignored "-Wfloat-equal"                    // warning: comparing floating point with == or != is unsafe // storing and comparing against same constants (typically 0.0f) is ok.
#pragma clang diagnostic ignored "-Wformat-nonliteral"              // warning: format string is not a string literal            // passing non-literal to vsnformat(). yes, user passing incorrect format strings can crash the code.
#pragma clang diagnostic ignored "-Wexit-time-destructors"          // warning: declaration requires an exit-time destructor     // exit-time destruction order is undefined. if MemFree() leads to users code that has been disabled before exit it might cause problems. ImGui coding style welcomes static/globals.
#pragma clang diagnostic ignored "-Wglobal-constructors"            // warning: declaration requires a global destructor         // similar to above, not sure what the exact difference is.
#pragma clang diagnostic ignored "-Wsign-conversion"                // warning: implicit conversion changes signedness
#pragma clang diagnostic ignored "-Wformat-pedantic"                // warning: format specifies type 'void *' but the argument has type 'xxxx *' // unreasonable, would lead to casting every %p arg to void*. probably enabled by -pedantic.
#pragma clang diagnostic ignored "-Wint-to-void-pointer-cast"       // warning: cast to 'void *' from smaller integer type 'int'
#pragma clang diagnostic ignored "-Wzero-as-null-pointer-constant"  // warning: zero as null pointer constant                    // some standard header variations use #define NULL 0
#pragma clang diagnostic ignored "-Wdouble-promotion"               // warning: implicit conversion from 'float' to 'double' when passing argument to function  // using printf() is a misery with this as C++ va_arg ellipsis changes float to double.
#pragma clang diagnostic ignored "-Wimplicit-int-float-conversion"  // warning: implicit conversion from 'xxx' to 'float' may lose precision
#elif defined(__GNUC__)
// We disable -Wpragmas because GCC doesn't provide an has_warning equivalent and some forks/patches may not following the warning/version association.
#pragma GCC diagnostic ignored "-Wpragmas"                  // warning: unknown option after '#pragma GCC diagnostic' kind
#pragma GCC diagnostic ignored "-Wunused-function"          // warning: 'xxxx' defined but not used
#pragma GCC diagnostic ignored "-Wint-to-pointer-cast"      // warning: cast to pointer from integer of different size
#pragma GCC diagnostic ignored "-Wformat"                   // warning: format '%p' expects argument of type 'void*', but argument 6 has type 'ImGuiWindow*'
#pragma GCC diagnostic ignored "-Wdouble-promotion"         // warning: implicit conversion from 'float' to 'double' when passing argument to function
#pragma GCC diagnostic ignored "-Wconversion"               // warning: conversion to 'xxxx' from 'xxxx' may alter its value
#pragma GCC diagnostic ignored "-Wformat-nonliteral"        // warning: format not a string literal, format string not checked
#pragma GCC diagnostic ignored "-Wstrict-overflow"          // warning: assuming signed overflow does not occur when assuming that (X - c) > X is always false
#pragma GCC diagnostic ignored "-Wclass-memaccess"          // [__GNUC__ >= 8] warning: 'memset/memcpy' clearing/writing an object of type 'xxxx' with no trivial copy-assignment; use assignment or value-initialization instead
#endif

// Debug options
#define IMGUI_DEBUG_NAV_SCORING     0   // Display navigation scoring preview when hovering items. Display last moving direction matches when holding CTRL
#define IMGUI_DEBUG_NAV_RECTS       0   // Display the reference navigation rectangle for each window
#define IMGUI_DEBUG_INI_SETTINGS    0   // Save additional comments in .ini file (particularly helps for Docking, but makes saving slower)

// When using CTRL+TAB (or Gamepad Square+L/R) we delay the visual a little in order to reduce visual noise doing a fast switch.
static const float NAV_WINDOWING_HIGHLIGHT_DELAY            = 0.20f;    // Time before the highlight and screen dimming starts fading in
static const float NAV_WINDOWING_LIST_APPEAR_DELAY          = 0.15f;    // Time before the window list starts to appear

// Window resizing from edges (when io.ConfigWindowsResizeFromEdges = true and ImGuiBackendFlags_HasMouseCursors is set in io.BackendFlags by backend)
static const float WINDOWS_HOVER_PADDING                    = 4.0f;     // Extend outside window for hovering/resizing (maxxed with TouchPadding) and inside windows for borders. Affect FindHoveredWindow().
static const float WINDOWS_RESIZE_FROM_EDGES_FEEDBACK_TIMER = 0.04f;    // Reduce visual noise by only highlighting the border after a certain time.
static const float WINDOWS_MOUSE_WHEEL_SCROLL_LOCK_TIMER    = 2.00f;    // Lock scrolled window (so it doesn't pick child windows that are scrolling through) for a certain time, unless mouse moved.

//-------------------------------------------------------------------------
// [SECTION] FORWARD DECLARATIONS
//-------------------------------------------------------------------------

static void             SetCurrentWindow(ImGuiWindow* window);
static void             FindHoveredWindow();
static ImGuiWindow*     CreateNewWindow(const char* name, ImGuiWindowFlags flags);
static ImVec2           CalcNextScrollFromScrollTargetAndClamp(ImGuiWindow* window);

static void             AddDrawListToDrawData(ImVector<ImDrawList*>* out_list, ImDrawList* draw_list);
static void             AddWindowToSortBuffer(ImVector<ImGuiWindow*>* out_sorted_windows, ImGuiWindow* window);

// Settings
static void             WindowSettingsHandler_ClearAll(ImGuiContext*, ImGuiSettingsHandler*);
static void*            WindowSettingsHandler_ReadOpen(ImGuiContext*, ImGuiSettingsHandler*, const char* name);
static void             WindowSettingsHandler_ReadLine(ImGuiContext*, ImGuiSettingsHandler*, void* entry, const char* line);
static void             WindowSettingsHandler_ApplyAll(ImGuiContext*, ImGuiSettingsHandler*);
static void             WindowSettingsHandler_WriteAll(ImGuiContext*, ImGuiSettingsHandler*, ImGuiTextBuffer* buf);

// Platform Dependents default implementation for IO functions
static const char*      GetClipboardTextFn_DefaultImpl(void* user_data);
static void             SetClipboardTextFn_DefaultImpl(void* user_data, const char* text);
static void             SetPlatformImeDataFn_DefaultImpl(ImGuiViewport* viewport, ImGuiPlatformImeData* data);

namespace ImGui
{
// Navigation
static void             NavUpdate();
static void             NavUpdateWindowing();
static void             NavUpdateWindowingOverlay();
static void             NavUpdateCancelRequest();
static void             NavUpdateCreateMoveRequest();
static void             NavUpdateCreateTabbingRequest();
static float            NavUpdatePageUpPageDown();
static inline void      NavUpdateAnyRequestFlag();
static void             NavUpdateCreateWrappingRequest();
static void             NavEndFrame();
static bool             NavScoreItem(ImGuiNavItemData* result);
static void             NavApplyItemToResult(ImGuiNavItemData* result);
static void             NavProcessItem();
static void             NavProcessItemForTabbingRequest(ImGuiID id);
static ImVec2           NavCalcPreferredRefPos();
static void             NavSaveLastChildNavWindowIntoParent(ImGuiWindow* nav_window);
static ImGuiWindow*     NavRestoreLastChildNavWindow(ImGuiWindow* window);
static void             NavRestoreLayer(ImGuiNavLayer layer);
static void             NavRestoreHighlightAfterMove();
static int              FindWindowFocusIndex(ImGuiWindow* window);

// Error Checking and Debug Tools
static void             ErrorCheckNewFrameSanityChecks();
static void             ErrorCheckEndFrameSanityChecks();
static void             UpdateDebugToolItemPicker();
static void             UpdateDebugToolStackQueries();

// Misc
static void             UpdateSettings();
static void             UpdateKeyboardInputs();
static void             UpdateMouseInputs();
static void             UpdateMouseWheel();
static bool             UpdateWindowManualResize(ImGuiWindow* window, const ImVec2& size_auto_fit, int* border_held, int resize_grip_count, ImU32 resize_grip_col[4], const ImRect& visibility_rect);
static void             RenderWindowOuterBorders(ImGuiWindow* window);
static void             RenderWindowDecorations(ImGuiWindow* window, const ImRect& title_bar_rect, bool title_bar_is_highlight, int resize_grip_count, const ImU32 resize_grip_col[4], float resize_grip_draw_size);
static void             RenderWindowTitleBarContents(ImGuiWindow* window, const ImRect& title_bar_rect, const char* name, bool* p_open);
static void             RenderDimmedBackgroundBehindWindow(ImGuiWindow* window, ImU32 col);
static void             RenderDimmedBackgrounds();
static ImGuiWindow*     FindBlockingModal(ImGuiWindow* window);

// Viewports
static void             UpdateViewportsNewFrame();

}

//-----------------------------------------------------------------------------
// [SECTION] CONTEXT AND MEMORY ALLOCATORS
//-----------------------------------------------------------------------------

// DLL users:
// - Heaps and globals are not shared across DLL boundaries!
// - You will need to call SetCurrentContext() + SetAllocatorFunctions() for each static/DLL boundary you are calling from.
// - Same applies for hot-reloading mechanisms that are reliant on reloading DLL (note that many hot-reloading mechanisms work without DLL).
// - Using Dear ImGui via a shared library is not recommended, because of function call overhead and because we don't guarantee backward nor forward ABI compatibility.
// - Confused? In a debugger: add GImGui to your watch window and notice how its value changes depending on your current location (which DLL boundary you are in).

// Current context pointer. Implicitly used by all Dear ImGui functions. Always assumed to be != NULL.
// - ImGui::CreateContext() will automatically set this pointer if it is NULL.
//   Change to a different context by calling ImGui::SetCurrentContext().
// - Important: Dear ImGui functions are not thread-safe because of this pointer.
//   If you want thread-safety to allow N threads to access N different contexts:
//   - Change this variable to use thread local storage so each thread can refer to a different context, in your imconfig.h:
//         struct ImGuiContext;
//         extern thread_local ImGuiContext* MyImGuiTLS;
//         #define GImGui MyImGuiTLS
//     And then define MyImGuiTLS in one of your cpp files. Note that thread_local is a C++11 keyword, earlier C++ uses compiler-specific keyword.
//   - Future development aims to make this context pointer explicit to all calls. Also read https://github.com/ocornut/imgui/issues/586
//   - If you need a finite number of contexts, you may compile and use multiple instances of the ImGui code from a different namespace.
// - DLL users: read comments above.
#ifndef GImGui
ImGuiContext*   GImGui = NULL;
#endif

// Memory Allocator functions. Use SetAllocatorFunctions() to change them.
// - You probably don't want to modify that mid-program, and if you use global/static e.g. ImVector<> instances you may need to keep them accessible during program destruction.
// - DLL users: read comments above.
#ifndef IMGUI_DISABLE_DEFAULT_ALLOCATORS
static void*   MallocWrapper(size_t size, void* user_data)    { IM_UNUSED(user_data); return malloc(size); }
static void    FreeWrapper(void* ptr, void* user_data)        { IM_UNUSED(user_data); free(ptr); }
#else
static void*   MallocWrapper(size_t size, void* user_data)    { IM_UNUSED(user_data); IM_UNUSED(size); IM_ASSERT(0); return NULL; }
static void    FreeWrapper(void* ptr, void* user_data)        { IM_UNUSED(user_data); IM_UNUSED(ptr); IM_ASSERT(0); }
#endif
static ImGuiMemAllocFunc    GImAllocatorAllocFunc = MallocWrapper;
static ImGuiMemFreeFunc     GImAllocatorFreeFunc = FreeWrapper;
static void*                GImAllocatorUserData = NULL;

//-----------------------------------------------------------------------------
// [SECTION] USER FACING STRUCTURES (ImGuiStyle, ImGuiIO)
//-----------------------------------------------------------------------------

ImGuiStyle::ImGuiStyle()
{
    Alpha                   = 1.0f;             // Global alpha applies to everything in Dear ImGui.
    DisabledAlpha           = 0.60f;            // Additional alpha multiplier applied by BeginDisabled(). Multiply over current value of Alpha.
    WindowPadding           = ImVec2(8,8);      // Padding within a window
    WindowRounding          = 0.0f;             // Radius of window corners rounding. Set to 0.0f to have rectangular windows. Large values tend to lead to variety of artifacts and are not recommended.
    WindowBorderSize        = 1.0f;             // Thickness of border around windows. Generally set to 0.0f or 1.0f. Other values not well tested.
    WindowMinSize           = ImVec2(32,32);    // Minimum window size
    WindowTitleAlign        = ImVec2(0.0f,0.5f);// Alignment for title bar text
    WindowMenuButtonPosition= ImGuiDir_Left;    // Position of the collapsing/docking button in the title bar (left/right). Defaults to ImGuiDir_Left.
    ChildRounding           = 0.0f;             // Radius of child window corners rounding. Set to 0.0f to have rectangular child windows
    ChildBorderSize         = 1.0f;             // Thickness of border around child windows. Generally set to 0.0f or 1.0f. Other values not well tested.
    PopupRounding           = 0.0f;             // Radius of popup window corners rounding. Set to 0.0f to have rectangular child windows
    PopupBorderSize         = 1.0f;             // Thickness of border around popup or tooltip windows. Generally set to 0.0f or 1.0f. Other values not well tested.
    FramePadding            = ImVec2(4,3);      // Padding within a framed rectangle (used by most widgets)
    FrameRounding           = 0.0f;             // Radius of frame corners rounding. Set to 0.0f to have rectangular frames (used by most widgets).
    FrameBorderSize         = 0.0f;             // Thickness of border around frames. Generally set to 0.0f or 1.0f. Other values not well tested.
    ItemSpacing             = ImVec2(8,4);      // Horizontal and vertical spacing between widgets/lines
    ItemInnerSpacing        = ImVec2(4,4);      // Horizontal and vertical spacing between within elements of a composed widget (e.g. a slider and its label)
    CellPadding             = ImVec2(4,2);      // Padding within a table cell
    TouchExtraPadding       = ImVec2(0,0);      // Expand reactive bounding box for touch-based system where touch position is not accurate enough. Unfortunately we don't sort widgets so priority on overlap will always be given to the first widget. So don't grow this too much!
    IndentSpacing           = 21.0f;            // Horizontal spacing when e.g. entering a tree node. Generally == (FontSize + FramePadding.x*2).
    ColumnsMinSpacing       = 6.0f;             // Minimum horizontal spacing between two columns. Preferably > (FramePadding.x + 1).
    ScrollbarSize           = 14.0f;            // Width of the vertical scrollbar, Height of the horizontal scrollbar
    ScrollbarRounding       = 9.0f;             // Radius of grab corners rounding for scrollbar
    GrabMinSize             = 10.0f;            // Minimum width/height of a grab box for slider/scrollbar
    GrabRounding            = 0.0f;             // Radius of grabs corners rounding. Set to 0.0f to have rectangular slider grabs.
    LogSliderDeadzone       = 4.0f;             // The size in pixels of the dead-zone around zero on logarithmic sliders that cross zero.
    TabRounding             = 4.0f;             // Radius of upper corners of a tab. Set to 0.0f to have rectangular tabs.
    TabBorderSize           = 0.0f;             // Thickness of border around tabs.
    TabMinWidthForCloseButton = 0.0f;           // Minimum width for close button to appears on an unselected tab when hovered. Set to 0.0f to always show when hovering, set to FLT_MAX to never show close button unless selected.
    ColorButtonPosition     = ImGuiDir_Right;   // Side of the color button in the ColorEdit4 widget (left/right). Defaults to ImGuiDir_Right.
    ButtonTextAlign         = ImVec2(0.5f,0.5f);// Alignment of button text when button is larger than text.
    SelectableTextAlign     = ImVec2(0.0f,0.0f);// Alignment of selectable text. Defaults to (0.0f, 0.0f) (top-left aligned). It's generally important to keep this left-aligned if you want to lay multiple items on a same line.
    DisplayWindowPadding    = ImVec2(19,19);    // Window position are clamped to be visible within the display area or monitors by at least this amount. Only applies to regular windows.
    DisplaySafeAreaPadding  = ImVec2(3,3);      // If you cannot see the edge of your screen (e.g. on a TV) increase the safe area padding. Covers popups/tooltips as well regular windows.
    MouseCursorScale        = 1.0f;             // Scale software rendered mouse cursor (when io.MouseDrawCursor is enabled). May be removed later.
    AntiAliasedLines        = true;             // Enable anti-aliased lines/borders. Disable if you are really tight on CPU/GPU.
    AntiAliasedLinesUseTex  = true;             // Enable anti-aliased lines/borders using textures where possible. Require backend to render with bilinear filtering (NOT point/nearest filtering).
    AntiAliasedFill         = true;             // Enable anti-aliased filled shapes (rounded rectangles, circles, etc.).
    CurveTessellationTol    = 1.25f;            // Tessellation tolerance when using PathBezierCurveTo() without a specific number of segments. Decrease for highly tessellated curves (higher quality, more polygons), increase to reduce quality.
    CircleTessellationMaxError = 0.30f;         // Maximum error (in pixels) allowed when using AddCircle()/AddCircleFilled() or drawing rounded corner rectangles with no explicit segment count specified. Decrease for higher quality but more geometry.

    // Default theme
    ImGui::StyleColorsDark(this);
}

// To scale your entire UI (e.g. if you want your app to use High DPI or generally be DPI aware) you may use this helper function. Scaling the fonts is done separately and is up to you.
// Important: This operation is lossy because we round all sizes to integer. If you need to change your scale multiples, call this over a freshly initialized ImGuiStyle structure rather than scaling multiple times.
void ImGuiStyle::ScaleAllSizes(float scale_factor)
{
    WindowPadding = ImFloor(WindowPadding * scale_factor);
    WindowRounding = ImFloor(WindowRounding * scale_factor);
    WindowMinSize = ImFloor(WindowMinSize * scale_factor);
    ChildRounding = ImFloor(ChildRounding * scale_factor);
    PopupRounding = ImFloor(PopupRounding * scale_factor);
    FramePadding = ImFloor(FramePadding * scale_factor);
    FrameRounding = ImFloor(FrameRounding * scale_factor);
    ItemSpacing = ImFloor(ItemSpacing * scale_factor);
    ItemInnerSpacing = ImFloor(ItemInnerSpacing * scale_factor);
    CellPadding = ImFloor(CellPadding * scale_factor);
    TouchExtraPadding = ImFloor(TouchExtraPadding * scale_factor);
    IndentSpacing = ImFloor(IndentSpacing * scale_factor);
    ColumnsMinSpacing = ImFloor(ColumnsMinSpacing * scale_factor);
    ScrollbarSize = ImFloor(ScrollbarSize * scale_factor);
    ScrollbarRounding = ImFloor(ScrollbarRounding * scale_factor);
    GrabMinSize = ImFloor(GrabMinSize * scale_factor);
    GrabRounding = ImFloor(GrabRounding * scale_factor);
    LogSliderDeadzone = ImFloor(LogSliderDeadzone * scale_factor);
    TabRounding = ImFloor(TabRounding * scale_factor);
    TabMinWidthForCloseButton = (TabMinWidthForCloseButton != FLT_MAX) ? ImFloor(TabMinWidthForCloseButton * scale_factor) : FLT_MAX;
    DisplayWindowPadding = ImFloor(DisplayWindowPadding * scale_factor);
    DisplaySafeAreaPadding = ImFloor(DisplaySafeAreaPadding * scale_factor);
    MouseCursorScale = ImFloor(MouseCursorScale * scale_factor);
}

ImGuiIO::ImGuiIO()
{
    // Most fields are initialized with zero
    memset(this, 0, sizeof(*this));
    IM_STATIC_ASSERT(IM_ARRAYSIZE(ImGuiIO::MouseDown) == ImGuiMouseButton_COUNT && IM_ARRAYSIZE(ImGuiIO::MouseClicked) == ImGuiMouseButton_COUNT);

    // Settings
    ConfigFlags = ImGuiConfigFlags_None;
    BackendFlags = ImGuiBackendFlags_None;
    DisplaySize = ImVec2(-1.0f, -1.0f);
    DeltaTime = 1.0f / 60.0f;
    IniSavingRate = 5.0f;
    IniFilename = "imgui.ini"; // Important: "imgui.ini" is relative to current working dir, most apps will want to lock this to an absolute path (e.g. same path as executables).
    LogFilename = "imgui_log.txt";
    MouseDoubleClickTime = 0.30f;
    MouseDoubleClickMaxDist = 6.0f;
#ifndef IMGUI_DISABLE_OBSOLETE_KEYIO
    for (int i = 0; i < ImGuiKey_COUNT; i++)
        KeyMap[i] = -1;
#endif
    KeyRepeatDelay = 0.275f;
    KeyRepeatRate = 0.050f;
    UserData = NULL;

    Fonts = NULL;
    FontGlobalScale = 1.0f;
    FontDefault = NULL;
    FontAllowUserScaling = false;
    DisplayFramebufferScale = ImVec2(1.0f, 1.0f);

    // Miscellaneous options
    MouseDrawCursor = false;
#ifdef __APPLE__
    ConfigMacOSXBehaviors = true;  // Set Mac OS X style defaults based on __APPLE__ compile time flag
#else
    ConfigMacOSXBehaviors = false;
#endif
    ConfigInputTrickleEventQueue = true;
    ConfigInputTextCursorBlink = true;
    ConfigWindowsResizeFromEdges = true;
    ConfigWindowsMoveFromTitleBarOnly = false;
    ConfigMemoryCompactTimer = 60.0f;

    // Platform Functions
    BackendPlatformName = BackendRendererName = NULL;
    BackendPlatformUserData = BackendRendererUserData = BackendLanguageUserData = NULL;
    GetClipboardTextFn = GetClipboardTextFn_DefaultImpl;   // Platform dependent default implementations
    SetClipboardTextFn = SetClipboardTextFn_DefaultImpl;
    ClipboardUserData = NULL;
    SetPlatformImeDataFn = SetPlatformImeDataFn_DefaultImpl;

    // Input (NB: we already have memset zero the entire structure!)
    MousePos = ImVec2(-FLT_MAX, -FLT_MAX);
    MousePosPrev = ImVec2(-FLT_MAX, -FLT_MAX);
    MouseDragThreshold = 6.0f;
    for (int i = 0; i < IM_ARRAYSIZE(MouseDownDuration); i++) MouseDownDuration[i] = MouseDownDurationPrev[i] = -1.0f;
    for (int i = 0; i < IM_ARRAYSIZE(KeysData); i++) { KeysData[i].DownDuration = KeysData[i].DownDurationPrev = -1.0f; }
    for (int i = 0; i < IM_ARRAYSIZE(NavInputsDownDuration); i++) NavInputsDownDuration[i] = -1.0f;
    AppAcceptingEvents = true;
    BackendUsingLegacyKeyArrays = (ImS8)-1;
    BackendUsingLegacyNavInputArray = true; // assume using legacy array until proven wrong
}

// Pass in translated ASCII characters for text input.
// - with glfw you can get those from the callback set in glfwSetCharCallback()
// - on Windows you can get those using ToAscii+keyboard state, or via the WM_CHAR message
// FIXME: Should in theory be called "AddCharacterEvent()" to be consistent with new API
void ImGuiIO::AddInputCharacter(unsigned int c)
{
    ImGuiContext& g = *GImGui;
    IM_ASSERT(&g.IO == this && "Can only add events to current context.");
    if (c == 0 || !AppAcceptingEvents)
        return;

    ImGuiInputEvent e;
    e.Type = ImGuiInputEventType_Text;
    e.Source = ImGuiInputSource_Keyboard;
    e.Text.Char = c;
    g.InputEventsQueue.push_back(e);
}

// UTF16 strings use surrogate pairs to encode codepoints >= 0x10000, so
// we should save the high surrogate.
void ImGuiIO::AddInputCharacterUTF16(ImWchar16 c)
{
    if ((c == 0 && InputQueueSurrogate == 0) || !AppAcceptingEvents)
        return;

    if ((c & 0xFC00) == 0xD800) // High surrogate, must save
    {
        if (InputQueueSurrogate != 0)
            AddInputCharacter(IM_UNICODE_CODEPOINT_INVALID);
        InputQueueSurrogate = c;
        return;
    }

    ImWchar cp = c;
    if (InputQueueSurrogate != 0)
    {
        if ((c & 0xFC00) != 0xDC00) // Invalid low surrogate
        {
            AddInputCharacter(IM_UNICODE_CODEPOINT_INVALID);
        }
        else
        {
#if IM_UNICODE_CODEPOINT_MAX == 0xFFFF
            cp = IM_UNICODE_CODEPOINT_INVALID; // Codepoint will not fit in ImWchar
#else
            cp = (ImWchar)(((InputQueueSurrogate - 0xD800) << 10) + (c - 0xDC00) + 0x10000);
#endif
        }

        InputQueueSurrogate = 0;
    }
    AddInputCharacter((unsigned)cp);
}

void ImGuiIO::AddInputCharactersUTF8(const char* utf8_chars)
{
    if (!AppAcceptingEvents)
        return;
    while (*utf8_chars != 0)
    {
        unsigned int c = 0;
        utf8_chars += ImTextCharFromUtf8(&c, utf8_chars, NULL);
        if (c != 0)
            AddInputCharacter(c);
    }
}

void ImGuiIO::ClearInputCharacters()
{
    InputQueueCharacters.resize(0);
}

void ImGuiIO::ClearInputKeys()
{
#ifndef IMGUI_DISABLE_OBSOLETE_KEYIO
    memset(KeysDown, 0, sizeof(KeysDown));
#endif
    for (int n = 0; n < IM_ARRAYSIZE(KeysData); n++)
    {
        KeysData[n].Down             = false;
        KeysData[n].DownDuration     = -1.0f;
        KeysData[n].DownDurationPrev = -1.0f;
    }
    KeyCtrl = KeyShift = KeyAlt = KeySuper = false;
    KeyMods = ImGuiModFlags_None;
    for (int n = 0; n < IM_ARRAYSIZE(NavInputsDownDuration); n++)
        NavInputsDownDuration[n] = NavInputsDownDurationPrev[n] = -1.0f;
}

// Queue a new key down/up event.
// - ImGuiKey key:       Translated key (as in, generally ImGuiKey_A matches the key end-user would use to emit an 'A' character)
// - bool down:          Is the key down? use false to signify a key release.
// - float analog_value: 0.0f..1.0f
void ImGuiIO::AddKeyAnalogEvent(ImGuiKey key, bool down, float analog_value)
{
    //if (e->Down) { IMGUI_DEBUG_LOG("AddKeyEvent() Key='%s' %d, NativeKeycode = %d, NativeScancode = %d\n", ImGui::GetKeyName(e->Key), e->Down, e->NativeKeycode, e->NativeScancode); }
    if (key == ImGuiKey_None || !AppAcceptingEvents)
        return;
    ImGuiContext& g = *GImGui;
    IM_ASSERT(&g.IO == this && "Can only add events to current context.");
    IM_ASSERT(ImGui::IsNamedKey(key)); // Backend needs to pass a valid ImGuiKey_ constant. 0..511 values are legacy native key codes which are not accepted by this API.

    // Verify that backend isn't mixing up using new io.AddKeyEvent() api and old io.KeysDown[] + io.KeyMap[] data.
#ifndef IMGUI_DISABLE_OBSOLETE_KEYIO
    IM_ASSERT((BackendUsingLegacyKeyArrays == -1 || BackendUsingLegacyKeyArrays == 0) && "Backend needs to either only use io.AddKeyEvent(), either only fill legacy io.KeysDown[] + io.KeyMap[]. Not both!");
    if (BackendUsingLegacyKeyArrays == -1)
        for (int n = ImGuiKey_NamedKey_BEGIN; n < ImGuiKey_NamedKey_END; n++)
            IM_ASSERT(KeyMap[n] == -1 && "Backend needs to either only use io.AddKeyEvent(), either only fill legacy io.KeysDown[] + io.KeyMap[]. Not both!");
    BackendUsingLegacyKeyArrays = 0;
#endif
    if (ImGui::IsGamepadKey(key))
        BackendUsingLegacyNavInputArray = false;

    // Partial filter of duplicates (not strictly needed, but makes data neater in particular for key mods and gamepad values which are most commonly spmamed)
    ImGuiKeyData* key_data = ImGui::GetKeyData(key);
    if (key_data->Down == down && key_data->AnalogValue == analog_value)
    {
        bool found = false;
        for (int n = g.InputEventsQueue.Size - 1; n >= 0 && !found; n--)
            if (g.InputEventsQueue[n].Type == ImGuiInputEventType_Key && g.InputEventsQueue[n].Key.Key == key)
                found = true;
        if (!found)
            return;
    }

    // Add event
    ImGuiInputEvent e;
    e.Type = ImGuiInputEventType_Key;
    e.Source = ImGui::IsGamepadKey(key) ? ImGuiInputSource_Gamepad : ImGuiInputSource_Keyboard;
    e.Key.Key = key;
    e.Key.Down = down;
    e.Key.AnalogValue = analog_value;
    g.InputEventsQueue.push_back(e);
}

void ImGuiIO::AddKeyEvent(ImGuiKey key, bool down)
{
    if (!AppAcceptingEvents)
        return;
    AddKeyAnalogEvent(key, down, down ? 1.0f : 0.0f);
}

// [Optional] Call after AddKeyEvent().
// Specify native keycode, scancode + Specify index for legacy <1.87 IsKeyXXX() functions with native indices.
// If you are writing a backend in 2022 or don't use IsKeyXXX() with native values that are not ImGuiKey values, you can avoid calling this.
void ImGuiIO::SetKeyEventNativeData(ImGuiKey key, int native_keycode, int native_scancode, int native_legacy_index)
{
    if (key == ImGuiKey_None)
        return;
    IM_ASSERT(ImGui::IsNamedKey(key)); // >= 512
    IM_ASSERT(native_legacy_index == -1 || ImGui::IsLegacyKey(native_legacy_index)); // >= 0 && <= 511
    IM_UNUSED(native_keycode);  // Yet unused
    IM_UNUSED(native_scancode); // Yet unused

    // Build native->imgui map so old user code can still call key functions with native 0..511 values.
#ifndef IMGUI_DISABLE_OBSOLETE_KEYIO
    const int legacy_key = (native_legacy_index != -1) ? native_legacy_index : native_keycode;
    if (!ImGui::IsLegacyKey(legacy_key))
        return;
    KeyMap[legacy_key] = key;
    KeyMap[key] = legacy_key;
#else
    IM_UNUSED(key);
    IM_UNUSED(native_legacy_index);
#endif
}

// Set master flag for accepting key/mouse/text events (default to true). Useful if you have native dialog boxes that are interrupting your application loop/refresh, and you want to disable events being queued while your app is frozen.
void ImGuiIO::SetAppAcceptingEvents(bool accepting_events)
{
    AppAcceptingEvents = accepting_events;
}

// Queue a mouse move event
void ImGuiIO::AddMousePosEvent(float x, float y)
{
    ImGuiContext& g = *GImGui;
    IM_ASSERT(&g.IO == this && "Can only add events to current context.");
    if (!AppAcceptingEvents)
        return;

    ImGuiInputEvent e;
    e.Type = ImGuiInputEventType_MousePos;
    e.Source = ImGuiInputSource_Mouse;
    e.MousePos.PosX = x;
    e.MousePos.PosY = y;
    g.InputEventsQueue.push_back(e);
}

void ImGuiIO::AddMouseButtonEvent(int mouse_button, bool down)
{
    ImGuiContext& g = *GImGui;
    IM_ASSERT(&g.IO == this && "Can only add events to current context.");
    IM_ASSERT(mouse_button >= 0 && mouse_button < ImGuiMouseButton_COUNT);
    if (!AppAcceptingEvents)
        return;

    ImGuiInputEvent e;
    e.Type = ImGuiInputEventType_MouseButton;
    e.Source = ImGuiInputSource_Mouse;
    e.MouseButton.Button = mouse_button;
    e.MouseButton.Down = down;
    g.InputEventsQueue.push_back(e);
}

// Queue a mouse wheel event (most mouse/API will only have a Y component)
void ImGuiIO::AddMouseWheelEvent(float wheel_x, float wheel_y)
{
    ImGuiContext& g = *GImGui;
    IM_ASSERT(&g.IO == this && "Can only add events to current context.");
    if ((wheel_x == 0.0f && wheel_y == 0.0f) || !AppAcceptingEvents)
        return;

    ImGuiInputEvent e;
    e.Type = ImGuiInputEventType_MouseWheel;
    e.Source = ImGuiInputSource_Mouse;
    e.MouseWheel.WheelX = wheel_x;
    e.MouseWheel.WheelY = wheel_y;
    g.InputEventsQueue.push_back(e);
}

void ImGuiIO::AddFocusEvent(bool focused)
{
    ImGuiContext& g = *GImGui;
    IM_ASSERT(&g.IO == this && "Can only add events to current context.");

    ImGuiInputEvent e;
    e.Type = ImGuiInputEventType_Focus;
    e.AppFocused.Focused = focused;
    g.InputEventsQueue.push_back(e);
}

//-----------------------------------------------------------------------------
// [SECTION] MISC HELPERS/UTILITIES (Geometry functions)
//-----------------------------------------------------------------------------

ImVec2 ImBezierCubicClosestPoint(const ImVec2& p1, const ImVec2& p2, const ImVec2& p3, const ImVec2& p4, const ImVec2& p, int num_segments)
{
    IM_ASSERT(num_segments > 0); // Use ImBezierCubicClosestPointCasteljau()
    ImVec2 p_last = p1;
    ImVec2 p_closest;
    float p_closest_dist2 = FLT_MAX;
    float t_step = 1.0f / (float)num_segments;
    for (int i_step = 1; i_step <= num_segments; i_step++)
    {
        ImVec2 p_current = ImBezierCubicCalc(p1, p2, p3, p4, t_step * i_step);
        ImVec2 p_line = ImLineClosestPoint(p_last, p_current, p);
        float dist2 = ImLengthSqr(p - p_line);
        if (dist2 < p_closest_dist2)
        {
            p_closest = p_line;
            p_closest_dist2 = dist2;
        }
        p_last = p_current;
    }
    return p_closest;
}

// Closely mimics PathBezierToCasteljau() in imgui_draw.cpp
static void ImBezierCubicClosestPointCasteljauStep(const ImVec2& p, ImVec2& p_closest, ImVec2& p_last, float& p_closest_dist2, float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4, float tess_tol, int level)
{
    float dx = x4 - x1;
    float dy = y4 - y1;
    float d2 = ((x2 - x4) * dy - (y2 - y4) * dx);
    float d3 = ((x3 - x4) * dy - (y3 - y4) * dx);
    d2 = (d2 >= 0) ? d2 : -d2;
    d3 = (d3 >= 0) ? d3 : -d3;
    if ((d2 + d3) * (d2 + d3) < tess_tol * (dx * dx + dy * dy))
    {
        ImVec2 p_current(x4, y4);
        ImVec2 p_line = ImLineClosestPoint(p_last, p_current, p);
        float dist2 = ImLengthSqr(p - p_line);
        if (dist2 < p_closest_dist2)
        {
            p_closest = p_line;
            p_closest_dist2 = dist2;
        }
        p_last = p_current;
    }
    else if (level < 10)
    {
        float x12 = (x1 + x2)*0.5f,       y12 = (y1 + y2)*0.5f;
        float x23 = (x2 + x3)*0.5f,       y23 = (y2 + y3)*0.5f;
        float x34 = (x3 + x4)*0.5f,       y34 = (y3 + y4)*0.5f;
        float x123 = (x12 + x23)*0.5f,    y123 = (y12 + y23)*0.5f;
        float x234 = (x23 + x34)*0.5f,    y234 = (y23 + y34)*0.5f;
        float x1234 = (x123 + x234)*0.5f, y1234 = (y123 + y234)*0.5f;
        ImBezierCubicClosestPointCasteljauStep(p, p_closest, p_last, p_closest_dist2, x1, y1, x12, y12, x123, y123, x1234, y1234, tess_tol, level + 1);
        ImBezierCubicClosestPointCasteljauStep(p, p_closest, p_last, p_closest_dist2, x1234, y1234, x234, y234, x34, y34, x4, y4, tess_tol, level + 1);
    }
}

// tess_tol is generally the same value you would find in ImGui::GetStyle().CurveTessellationTol
// Because those ImXXX functions are lower-level than ImGui:: we cannot access this value automatically.
ImVec2 ImBezierCubicClosestPointCasteljau(const ImVec2& p1, const ImVec2& p2, const ImVec2& p3, const ImVec2& p4, const ImVec2& p, float tess_tol)
{
    IM_ASSERT(tess_tol > 0.0f);
    ImVec2 p_last = p1;
    ImVec2 p_closest;
    float p_closest_dist2 = FLT_MAX;
    ImBezierCubicClosestPointCasteljauStep(p, p_closest, p_last, p_closest_dist2, p1.x, p1.y, p2.x, p2.y, p3.x, p3.y, p4.x, p4.y, tess_tol, 0);
    return p_closest;
}

ImVec2 ImLineClosestPoint(const ImVec2& a, const ImVec2& b, const ImVec2& p)
{
    ImVec2 ap = p - a;
    ImVec2 ab_dir = b - a;
    float dot = ap.x * ab_dir.x + ap.y * ab_dir.y;
    if (dot < 0.0f)
        return a;
    float ab_len_sqr = ab_dir.x * ab_dir.x + ab_dir.y * ab_dir.y;
    if (dot > ab_len_sqr)
        return b;
    return a + ab_dir * dot / ab_len_sqr;
}

bool ImTriangleContainsPoint(const ImVec2& a, const ImVec2& b, const ImVec2& c, const ImVec2& p)
{
    bool b1 = ((p.x - b.x) * (a.y - b.y) - (p.y - b.y) * (a.x - b.x)) < 0.0f;
    bool b2 = ((p.x - c.x) * (b.y - c.y) - (p.y - c.y) * (b.x - c.x)) < 0.0f;
    bool b3 = ((p.x - a.x) * (c.y - a.y) - (p.y - a.y) * (c.x - a.x)) < 0.0f;
    return ((b1 == b2) && (b2 == b3));
}

void ImTriangleBarycentricCoords(const ImVec2& a, const ImVec2& b, const ImVec2& c, const ImVec2& p, float& out_u, float& out_v, float& out_w)
{
    ImVec2 v0 = b - a;
    ImVec2 v1 = c - a;
    ImVec2 v2 = p - a;
    const float denom = v0.x * v1.y - v1.x * v0.y;
    out_v = (v2.x * v1.y - v1.x * v2.y) / denom;
    out_w = (v0.x * v2.y - v2.x * v0.y) / denom;
    out_u = 1.0f - out_v - out_w;
}

ImVec2 ImTriangleClosestPoint(const ImVec2& a, const ImVec2& b, const ImVec2& c, const ImVec2& p)
{
    ImVec2 proj_ab = ImLineClosestPoint(a, b, p);
    ImVec2 proj_bc = ImLineClosestPoint(b, c, p);
    ImVec2 proj_ca = ImLineClosestPoint(c, a, p);
    float dist2_ab = ImLengthSqr(p - proj_ab);
    float dist2_bc = ImLengthSqr(p - proj_bc);
    float dist2_ca = ImLengthSqr(p - proj_ca);
    float m = ImMin(dist2_ab, ImMin(dist2_bc, dist2_ca));
    if (m == dist2_ab)
        return proj_ab;
    if (m == dist2_bc)
        return proj_bc;
    return proj_ca;
}

//-----------------------------------------------------------------------------
// [SECTION] MISC HELPERS/UTILITIES (String, Format, Hash functions)
//-----------------------------------------------------------------------------

// Consider using _stricmp/_strnicmp under Windows or strcasecmp/strncasecmp. We don't actually use either ImStricmp/ImStrnicmp in the codebase any more.
int ImStricmp(const char* str1, const char* str2)
{
    int d;
    while ((d = toupper(*str2) - toupper(*str1)) == 0 && *str1) { str1++; str2++; }
    return d;
}

int ImStrnicmp(const char* str1, const char* str2, size_t count)
{
    int d = 0;
    while (count > 0 && (d = toupper(*str2) - toupper(*str1)) == 0 && *str1) { str1++; str2++; count--; }
    return d;
}

void ImStrncpy(char* dst, const char* src, size_t count)
{
    if (count < 1)
        return;
    if (count > 1)
        strncpy(dst, src, count - 1);
    dst[count - 1] = 0;
}

char* ImStrdup(const char* str)
{
    size_t len = strlen(str);
    void* buf = IM_ALLOC(len + 1);
    return (char*)memcpy(buf, (const void*)str, len + 1);
}

char* ImStrdupcpy(char* dst, size_t* p_dst_size, const char* src)
{
    size_t dst_buf_size = p_dst_size ? *p_dst_size : strlen(dst) + 1;
    size_t src_size = strlen(src) + 1;
    if (dst_buf_size < src_size)
    {
        IM_FREE(dst);
        dst = (char*)IM_ALLOC(src_size);
        if (p_dst_size)
            *p_dst_size = src_size;
    }
    return (char*)memcpy(dst, (const void*)src, src_size);
}

const char* ImStrchrRange(const char* str, const char* str_end, char c)
{
    const char* p = (const char*)memchr(str, (int)c, str_end - str);
    return p;
}

int ImStrlenW(const ImWchar* str)
{
    //return (int)wcslen((const wchar_t*)str);  // FIXME-OPT: Could use this when wchar_t are 16-bit
    int n = 0;
    while (*str++) n++;
    return n;
}

// Find end-of-line. Return pointer will point to either first \n, either str_end.
const char* ImStreolRange(const char* str, const char* str_end)
{
    const char* p = (const char*)memchr(str, '\n', str_end - str);
    return p ? p : str_end;
}

const ImWchar* ImStrbolW(const ImWchar* buf_mid_line, const ImWchar* buf_begin) // find beginning-of-line
{
    while (buf_mid_line > buf_begin && buf_mid_line[-1] != '\n')
        buf_mid_line--;
    return buf_mid_line;
}

const char* ImStristr(const char* haystack, const char* haystack_end, const char* needle, const char* needle_end)
{
    if (!needle_end)
        needle_end = needle + strlen(needle);

    const char un0 = (char)toupper(*needle);
    while ((!haystack_end && *haystack) || (haystack_end && haystack < haystack_end))
    {
        if (toupper(*haystack) == un0)
        {
            const char* b = needle + 1;
            for (const char* a = haystack + 1; b < needle_end; a++, b++)
                if (toupper(*a) != toupper(*b))
                    break;
            if (b == needle_end)
                return haystack;
        }
        haystack++;
    }
    return NULL;
}

// Trim str by offsetting contents when there's leading data + writing a \0 at the trailing position. We use this in situation where the cost is negligible.
void ImStrTrimBlanks(char* buf)
{
    char* p = buf;
    while (p[0] == ' ' || p[0] == '\t')     // Leading blanks
        p++;
    char* p_start = p;
    while (*p != 0)                         // Find end of string
        p++;
    while (p > p_start && (p[-1] == ' ' || p[-1] == '\t'))  // Trailing blanks
        p--;
    if (p_start != buf)                     // Copy memory if we had leading blanks
        memmove(buf, p_start, p - p_start);
    buf[p - p_start] = 0;                   // Zero terminate
}

const char* ImStrSkipBlank(const char* str)
{
    while (str[0] == ' ' || str[0] == '\t')
        str++;
    return str;
}

// A) MSVC version appears to return -1 on overflow, whereas glibc appears to return total count (which may be >= buf_size).
// Ideally we would test for only one of those limits at runtime depending on the behavior the vsnprintf(), but trying to deduct it at compile time sounds like a pandora can of worm.
// B) When buf==NULL vsnprintf() will return the output size.
#ifndef IMGUI_DISABLE_DEFAULT_FORMAT_FUNCTIONS

// We support stb_sprintf which is much faster (see: https://github.com/nothings/stb/blob/master/stb_sprintf.h)
// You may set IMGUI_USE_STB_SPRINTF to use our default wrapper, or set IMGUI_DISABLE_DEFAULT_FORMAT_FUNCTIONS
// and setup the wrapper yourself. (FIXME-OPT: Some of our high-level operations such as ImGuiTextBuffer::appendfv() are
// designed using two-passes worst case, which probably could be improved using the stbsp_vsprintfcb() function.)
#ifdef IMGUI_USE_STB_SPRINTF
#define STB_SPRINTF_IMPLEMENTATION
#ifdef IMGUI_STB_SPRINTF_FILENAME
#include IMGUI_STB_SPRINTF_FILENAME
#else
#include "stb_sprintf.h"
#endif
#endif

#if defined(_MSC_VER) && !defined(vsnprintf)
#define vsnprintf _vsnprintf
#endif

int ImFormatString(char* buf, size_t buf_size, const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
#ifdef IMGUI_USE_STB_SPRINTF
    int w = stbsp_vsnprintf(buf, (int)buf_size, fmt, args);
#else
    int w = vsnprintf(buf, buf_size, fmt, args);
#endif
    va_end(args);
    if (buf == NULL)
        return w;
    if (w == -1 || w >= (int)buf_size)
        w = (int)buf_size - 1;
    buf[w] = 0;
    return w;
}

int ImFormatStringV(char* buf, size_t buf_size, const char* fmt, va_list args)
{
#ifdef IMGUI_USE_STB_SPRINTF
    int w = stbsp_vsnprintf(buf, (int)buf_size, fmt, args);
#else
    int w = vsnprintf(buf, buf_size, fmt, args);
#endif
    if (buf == NULL)
        return w;
    if (w == -1 || w >= (int)buf_size)
        w = (int)buf_size - 1;
    buf[w] = 0;
    return w;
}
#endif // #ifdef IMGUI_DISABLE_DEFAULT_FORMAT_FUNCTIONS

// CRC32 needs a 1KB lookup table (not cache friendly)
// Although the code to generate the table is simple and shorter than the table itself, using a const table allows us to easily:
// - avoid an unnecessary branch/memory tap, - keep the ImHashXXX functions usable by static constructors, - make it thread-safe.
static const ImU32 GCrc32LookupTable[256] =
{
    0x00000000,0x77073096,0xEE0E612C,0x990951BA,0x076DC419,0x706AF48F,0xE963A535,0x9E6495A3,0x0EDB8832,0x79DCB8A4,0xE0D5E91E,0x97D2D988,0x09B64C2B,0x7EB17CBD,0xE7B82D07,0x90BF1D91,
    0x1DB71064,0x6AB020F2,0xF3B97148,0x84BE41DE,0x1ADAD47D,0x6DDDE4EB,0xF4D4B551,0x83D385C7,0x136C9856,0x646BA8C0,0xFD62F97A,0x8A65C9EC,0x14015C4F,0x63066CD9,0xFA0F3D63,0x8D080DF5,
    0x3B6E20C8,0x4C69105E,0xD56041E4,0xA2677172,0x3C03E4D1,0x4B04D447,0xD20D85FD,0xA50AB56B,0x35B5A8FA,0x42B2986C,0xDBBBC9D6,0xACBCF940,0x32D86CE3,0x45DF5C75,0xDCD60DCF,0xABD13D59,
    0x26D930AC,0x51DE003A,0xC8D75180,0xBFD06116,0x21B4F4B5,0x56B3C423,0xCFBA9599,0xB8BDA50F,0x2802B89E,0x5F058808,0xC60CD9B2,0xB10BE924,0x2F6F7C87,0x58684C11,0xC1611DAB,0xB6662D3D,
    0x76DC4190,0x01DB7106,0x98D220BC,0xEFD5102A,0x71B18589,0x06B6B51F,0x9FBFE4A5,0xE8B8D433,0x7807C9A2,0x0F00F934,0x9609A88E,0xE10E9818,0x7F6A0DBB,0x086D3D2D,0x91646C97,0xE6635C01,
    0x6B6B51F4,0x1C6C6162,0x856530D8,0xF262004E,0x6C0695ED,0x1B01A57B,0x8208F4C1,0xF50FC457,0x65B0D9C6,0x12B7E950,0x8BBEB8EA,0xFCB9887C,0x62DD1DDF,0x15DA2D49,0x8CD37CF3,0xFBD44C65,
    0x4DB26158,0x3AB551CE,0xA3BC0074,0xD4BB30E2,0x4ADFA541,0x3DD895D7,0xA4D1C46D,0xD3D6F4FB,0x4369E96A,0x346ED9FC,0xAD678846,0xDA60B8D0,0x44042D73,0x33031DE5,0xAA0A4C5F,0xDD0D7CC9,
    0x5005713C,0x270241AA,0xBE0B1010,0xC90C2086,0x5768B525,0x206F85B3,0xB966D409,0xCE61E49F,0x5EDEF90E,0x29D9C998,0xB0D09822,0xC7D7A8B4,0x59B33D17,0x2EB40D81,0xB7BD5C3B,0xC0BA6CAD,
    0xEDB88320,0x9ABFB3B6,0x03B6E20C,0x74B1D29A,0xEAD54739,0x9DD277AF,0x04DB2615,0x73DC1683,0xE3630B12,0x94643B84,0x0D6D6A3E,0x7A6A5AA8,0xE40ECF0B,0x9309FF9D,0x0A00AE27,0x7D079EB1,
    0xF00F9344,0x8708A3D2,0x1E01F268,0x6906C2FE,0xF762575D,0x806567CB,0x196C3671,0x6E6B06E7,0xFED41B76,0x89D32BE0,0x10DA7A5A,0x67DD4ACC,0xF9B9DF6F,0x8EBEEFF9,0x17B7BE43,0x60B08ED5,
    0xD6D6A3E8,0xA1D1937E,0x38D8C2C4,0x4FDFF252,0xD1BB67F1,0xA6BC5767,0x3FB506DD,0x48B2364B,0xD80D2BDA,0xAF0A1B4C,0x36034AF6,0x41047A60,0xDF60EFC3,0xA867DF55,0x316E8EEF,0x4669BE79,
    0xCB61B38C,0xBC66831A,0x256FD2A0,0x5268E236,0xCC0C7795,0xBB0B4703,0x220216B9,0x5505262F,0xC5BA3BBE,0xB2BD0B28,0x2BB45A92,0x5CB36A04,0xC2D7FFA7,0xB5D0CF31,0x2CD99E8B,0x5BDEAE1D,
    0x9B64C2B0,0xEC63F226,0x756AA39C,0x026D930A,0x9C0906A9,0xEB0E363F,0x72076785,0x05005713,0x95BF4A82,0xE2B87A14,0x7BB12BAE,0x0CB61B38,0x92D28E9B,0xE5D5BE0D,0x7CDCEFB7,0x0BDBDF21,
    0x86D3D2D4,0xF1D4E242,0x68DDB3F8,0x1FDA836E,0x81BE16CD,0xF6B9265B,0x6FB077E1,0x18B74777,0x88085AE6,0xFF0F6A70,0x66063BCA,0x11010B5C,0x8F659EFF,0xF862AE69,0x616BFFD3,0x166CCF45,
    0xA00AE278,0xD70DD2EE,0x4E048354,0x3903B3C2,0xA7672661,0xD06016F7,0x4969474D,0x3E6E77DB,0xAED16A4A,0xD9D65ADC,0x40DF0B66,0x37D83BF0,0xA9BCAE53,0xDEBB9EC5,0x47B2CF7F,0x30B5FFE9,
    0xBDBDF21C,0xCABAC28A,0x53B39330,0x24B4A3A6,0xBAD03605,0xCDD70693,0x54DE5729,0x23D967BF,0xB3667A2E,0xC4614AB8,0x5D681B02,0x2A6F2B94,0xB40BBE37,0xC30C8EA1,0x5A05DF1B,0x2D02EF8D,
};

// Known size hash
// It is ok to call ImHashData on a string with known length but the ### operator won't be supported.
// FIXME-OPT: Replace with e.g. FNV1a hash? CRC32 pretty much randomly access 1KB. Need to do proper measurements.
ImGuiID ImHashData(const void* data_p, size_t data_size, ImU32 seed)
{
    ImU32 crc = ~seed;
    const unsigned char* data = (const unsigned char*)data_p;
    const ImU32* crc32_lut = GCrc32LookupTable;
    while (data_size-- != 0)
        crc = (crc >> 8) ^ crc32_lut[(crc & 0xFF) ^ *data++];
    return ~crc;
}

// Zero-terminated string hash, with support for ### to reset back to seed value
// We support a syntax of "label###id" where only "###id" is included in the hash, and only "label" gets displayed.
// Because this syntax is rarely used we are optimizing for the common case.
// - If we reach ### in the string we discard the hash so far and reset to the seed.
// - We don't do 'current += 2; continue;' after handling ### to keep the code smaller/faster (measured ~10% diff in Debug build)
// FIXME-OPT: Replace with e.g. FNV1a hash? CRC32 pretty much randomly access 1KB. Need to do proper measurements.
ImGuiID ImHashStr(const char* data_p, size_t data_size, ImU32 seed)
{
    seed = ~seed;
    ImU32 crc = seed;
    const unsigned char* data = (const unsigned char*)data_p;
    const ImU32* crc32_lut = GCrc32LookupTable;
    if (data_size != 0)
    {
        while (data_size-- != 0)
        {
            unsigned char c = *data++;
            if (c == '#' && data_size >= 2 && data[0] == '#' && data[1] == '#')
                crc = seed;
            crc = (crc >> 8) ^ crc32_lut[(crc & 0xFF) ^ c];
        }
    }
    else
    {
        while (unsigned char c = *data++)
        {
            if (c == '#' && data[0] == '#' && data[1] == '#')
                crc = seed;
            crc = (crc >> 8) ^ crc32_lut[(crc & 0xFF) ^ c];
        }
    }
    return ~crc;
}

//-----------------------------------------------------------------------------
// [SECTION] MISC HELPERS/UTILITIES (File functions)
//-----------------------------------------------------------------------------

// Default file functions
#ifndef IMGUI_DISABLE_DEFAULT_FILE_FUNCTIONS

ImFileHandle ImFileOpen(const char* filename, const char* mode)
{
#if defined(_WIN32) && !defined(IMGUI_DISABLE_WIN32_FUNCTIONS) && !defined(__CYGWIN__) && !defined(__GNUC__)
    // We need a fopen() wrapper because MSVC/Windows fopen doesn't handle UTF-8 filenames.
    // Previously we used ImTextCountCharsFromUtf8/ImTextStrFromUtf8 here but we now need to support ImWchar16 and ImWchar32!
    const int filename_wsize = ::MultiByteToWideChar(CP_UTF8, 0, filename, -1, NULL, 0);
    const int mode_wsize = ::MultiByteToWideChar(CP_UTF8, 0, mode, -1, NULL, 0);
    ImVector<ImWchar> buf;
    buf.resize(filename_wsize + mode_wsize);
    ::MultiByteToWideChar(CP_UTF8, 0, filename, -1, (wchar_t*)&buf[0], filename_wsize);
    ::MultiByteToWideChar(CP_UTF8, 0, mode, -1, (wchar_t*)&buf[filename_wsize], mode_wsize);
    return ::_wfopen((const wchar_t*)&buf[0], (const wchar_t*)&buf[filename_wsize]);
#else
    return fopen(filename, mode);
#endif
}

// We should in theory be using fseeko()/ftello() with off_t and _fseeki64()/_ftelli64() with __int64, waiting for the PR that does that in a very portable pre-C++11 zero-warnings way.
bool    ImFileClose(ImFileHandle f)     { return fclose(f) == 0; }
ImU64   ImFileGetSize(ImFileHandle f)   { long off = 0, sz = 0; return ((off = ftell(f)) != -1 && !fseek(f, 0, SEEK_END) && (sz = ftell(f)) != -1 && !fseek(f, off, SEEK_SET)) ? (ImU64)sz : (ImU64)-1; }
ImU64   ImFileRead(void* data, ImU64 sz, ImU64 count, ImFileHandle f)           { return fread(data, (size_t)sz, (size_t)count, f); }
ImU64   ImFileWrite(const void* data, ImU64 sz, ImU64 count, ImFileHandle f)    { return fwrite(data, (size_t)sz, (size_t)count, f); }
#endif // #ifndef IMGUI_DISABLE_DEFAULT_FILE_FUNCTIONS

// Helper: Load file content into memory
// Memory allocated with IM_ALLOC(), must be freed by user using IM_FREE() == ImGui::MemFree()
// This can't really be used with "rt" because fseek size won't match read size.
void*   ImFileLoadToMemory(const char* filename, const char* mode, size_t* out_file_size, int padding_bytes)
{
    IM_ASSERT(filename && mode);
    if (out_file_size)
        *out_file_size = 0;

    ImFileHandle f;
    if ((f = ImFileOpen(filename, mode)) == NULL)
        return NULL;

    size_t file_size = (size_t)ImFileGetSize(f);
    if (file_size == (size_t)-1)
    {
        ImFileClose(f);
        return NULL;
    }

    void* file_data = IM_ALLOC(file_size + padding_bytes);
    if (file_data == NULL)
    {
        ImFileClose(f);
        return NULL;
    }
    if (ImFileRead(file_data, 1, file_size, f) != file_size)
    {
        ImFileClose(f);
        IM_FREE(file_data);
        return NULL;
    }
    if (padding_bytes > 0)
        memset((void*)(((char*)file_data) + file_size), 0, (size_t)padding_bytes);

    ImFileClose(f);
    if (out_file_size)
        *out_file_size = file_size;

    return file_data;
}

//-----------------------------------------------------------------------------
// [SECTION] MISC HELPERS/UTILITIES (ImText* functions)
//-----------------------------------------------------------------------------

// Convert UTF-8 to 32-bit character, process single character input.
// A nearly-branchless UTF-8 decoder, based on work of Christopher Wellons (https://github.com/skeeto/branchless-utf8).
// We handle UTF-8 decoding error by skipping forward.
int ImTextCharFromUtf8(unsigned int* out_char, const char* in_text, const char* in_text_end)
{
    static const char lengths[32] = { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 2, 2, 2, 2, 3, 3, 4, 0 };
    static const int masks[]  = { 0x00, 0x7f, 0x1f, 0x0f, 0x07 };
    static const uint32_t mins[] = { 0x400000, 0, 0x80, 0x800, 0x10000 };
    static const int shiftc[] = { 0, 18, 12, 6, 0 };
    static const int shifte[] = { 0, 6, 4, 2, 0 };
    int len = lengths[*(const unsigned char*)in_text >> 3];
    int wanted = len + !len;

    if (in_text_end == NULL)
        in_text_end = in_text + wanted; // Max length, nulls will be taken into account.

    // Copy at most 'len' bytes, stop copying at 0 or past in_text_end. Branch predictor does a good job here,
    // so it is fast even with excessive branching.
    unsigned char s[4];
    s[0] = in_text + 0 < in_text_end ? in_text[0] : 0;
    s[1] = in_text + 1 < in_text_end ? in_text[1] : 0;
    s[2] = in_text + 2 < in_text_end ? in_text[2] : 0;
    s[3] = in_text + 3 < in_text_end ? in_text[3] : 0;

    // Assume a four-byte character and load four bytes. Unused bits are shifted out.
    *out_char  = (uint32_t)(s[0] & masks[len]) << 18;
    *out_char |= (uint32_t)(s[1] & 0x3f) << 12;
    *out_char |= (uint32_t)(s[2] & 0x3f) <<  6;
    *out_char |= (uint32_t)(s[3] & 0x3f) <<  0;
    *out_char >>= shiftc[len];

    // Accumulate the various error conditions.
    int e = 0;
    e  = (*out_char < mins[len]) << 6; // non-canonical encoding
    e |= ((*out_char >> 11) == 0x1b) << 7;  // surrogate half?
    e |= (*out_char > IM_UNICODE_CODEPOINT_MAX) << 8;  // out of range?
    e |= (s[1] & 0xc0) >> 2;
    e |= (s[2] & 0xc0) >> 4;
    e |= (s[3]       ) >> 6;
    e ^= 0x2a; // top two bits of each tail byte correct?
    e >>= shifte[len];

    if (e)
    {
        // No bytes are consumed when *in_text == 0 || in_text == in_text_end.
        // One byte is consumed in case of invalid first byte of in_text.
        // All available bytes (at most `len` bytes) are consumed on incomplete/invalid second to last bytes.
        // Invalid or incomplete input may consume less bytes than wanted, therefore every byte has to be inspected in s.
        wanted = ImMin(wanted, !!s[0] + !!s[1] + !!s[2] + !!s[3]);
        *out_char = IM_UNICODE_CODEPOINT_INVALID;
    }

    return wanted;
}

int ImTextStrFromUtf8(ImWchar* buf, int buf_size, const char* in_text, const char* in_text_end, const char** in_text_remaining)
{
    ImWchar* buf_out = buf;
    ImWchar* buf_end = buf + buf_size;
    while (buf_out < buf_end - 1 && (!in_text_end || in_text < in_text_end) && *in_text)
    {
        unsigned int c;
        in_text += ImTextCharFromUtf8(&c, in_text, in_text_end);
        if (c == 0)
            break;
        *buf_out++ = (ImWchar)c;
    }
    *buf_out = 0;
    if (in_text_remaining)
        *in_text_remaining = in_text;
    return (int)(buf_out - buf);
}

int ImTextCountCharsFromUtf8(const char* in_text, const char* in_text_end)
{
    int char_count = 0;
    while ((!in_text_end || in_text < in_text_end) && *in_text)
    {
        unsigned int c;
        in_text += ImTextCharFromUtf8(&c, in_text, in_text_end);
        if (c == 0)
            break;
        char_count++;
    }
    return char_count;
}

// Based on stb_to_utf8() from github.com/nothings/stb/
static inline int ImTextCharToUtf8_inline(char* buf, int buf_size, unsigned int c)
{
    if (c < 0x80)
    {
        buf[0] = (char)c;
        return 1;
    }
    if (c < 0x800)
    {
        if (buf_size < 2) return 0;
        buf[0] = (char)(0xc0 + (c >> 6));
        buf[1] = (char)(0x80 + (c & 0x3f));
        return 2;
    }
    if (c < 0x10000)
    {
        if (buf_size < 3) return 0;
        buf[0] = (char)(0xe0 + (c >> 12));
        buf[1] = (char)(0x80 + ((c >> 6) & 0x3f));
        buf[2] = (char)(0x80 + ((c ) & 0x3f));
        return 3;
    }
    if (c <= 0x10FFFF)
    {
        if (buf_size < 4) return 0;
        buf[0] = (char)(0xf0 + (c >> 18));
        buf[1] = (char)(0x80 + ((c >> 12) & 0x3f));
        buf[2] = (char)(0x80 + ((c >> 6) & 0x3f));
        buf[3] = (char)(0x80 + ((c ) & 0x3f));
        return 4;
    }
    // Invalid code point, the max unicode is 0x10FFFF
    return 0;
}

const char* ImTextCharToUtf8(char out_buf[5], unsigned int c)
{
    int count = ImTextCharToUtf8_inline(out_buf, 5, c);
    out_buf[count] = 0;
    return out_buf;
}

// Not optimal but we very rarely use this function.
int ImTextCountUtf8BytesFromChar(const char* in_text, const char* in_text_end)
{
    unsigned int unused = 0;
    return ImTextCharFromUtf8(&unused, in_text, in_text_end);
}

static inline int ImTextCountUtf8BytesFromChar(unsigned int c)
{
    if (c < 0x80) return 1;
    if (c < 0x800) return 2;
    if (c < 0x10000) return 3;
    if (c <= 0x10FFFF) return 4;
    return 3;
}

int ImTextStrToUtf8(char* out_buf, int out_buf_size, const ImWchar* in_text, const ImWchar* in_text_end)
{
    char* buf_p = out_buf;
    const char* buf_end = out_buf + out_buf_size;
    while (buf_p < buf_end - 1 && (!in_text_end || in_text < in_text_end) && *in_text)
    {
        unsigned int c = (unsigned int)(*in_text++);
        if (c < 0x80)
            *buf_p++ = (char)c;
        else
            buf_p += ImTextCharToUtf8_inline(buf_p, (int)(buf_end - buf_p - 1), c);
    }
    *buf_p = 0;
    return (int)(buf_p - out_buf);
}

int ImTextCountUtf8BytesFromStr(const ImWchar* in_text, const ImWchar* in_text_end)
{
    int bytes_count = 0;
    while ((!in_text_end || in_text < in_text_end) && *in_text)
    {
        unsigned int c = (unsigned int)(*in_text++);
        if (c < 0x80)
            bytes_count++;
        else
            bytes_count += ImTextCountUtf8BytesFromChar(c);
    }
    return bytes_count;
}

//-----------------------------------------------------------------------------
// [SECTION] MISC HELPERS/UTILITIES (Color functions)
// Note: The Convert functions are early design which are not consistent with other API.
//-----------------------------------------------------------------------------

IMGUI_API ImU32 ImAlphaBlendColors(ImU32 col_a, ImU32 col_b)
{
    float t = ((col_b >> IM_COL32_A_SHIFT) & 0xFF) / 255.f;
    int r = ImLerp((int)(col_a >> IM_COL32_R_SHIFT) & 0xFF, (int)(col_b >> IM_COL32_R_SHIFT) & 0xFF, t);
    int g = ImLerp((int)(col_a >> IM_COL32_G_SHIFT) & 0xFF, (int)(col_b >> IM_COL32_G_SHIFT) & 0xFF, t);
    int b = ImLerp((int)(col_a >> IM_COL32_B_SHIFT) & 0xFF, (int)(col_b >> IM_COL32_B_SHIFT) & 0xFF, t);
    return IM_COL32(r, g, b, 0xFF);
}

ImVec4 ImGui::ColorConvertU32ToFloat4(ImU32 in)
{
    float s = 1.0f / 255.0f;
    return ImVec4(
        ((in >> IM_COL32_R_SHIFT) & 0xFF) * s,
        ((in >> IM_COL32_G_SHIFT) & 0xFF) * s,
        ((in >> IM_COL32_B_SHIFT) & 0xFF) * s,
        ((in >> IM_COL32_A_SHIFT) & 0xFF) * s);
}

ImU32 ImGui::ColorConvertFloat4ToU32(const ImVec4& in)
{
    ImU32 out;
    out  = ((ImU32)IM_F32_TO_INT8_SAT(in.x)) << IM_COL32_R_SHIFT;
    out |= ((ImU32)IM_F32_TO_INT8_SAT(in.y)) << IM_COL32_G_SHIFT;
    out |= ((ImU32)IM_F32_TO_INT8_SAT(in.z)) << IM_COL32_B_SHIFT;
    out |= ((ImU32)IM_F32_TO_INT8_SAT(in.w)) << IM_COL32_A_SHIFT;
    return out;
}

// Convert rgb floats ([0-1],[0-1],[0-1]) to hsv floats ([0-1],[0-1],[0-1]), from Foley & van Dam p592
// Optimized http://lolengine.net/blog/2013/01/13/fast-rgb-to-hsv
void ImGui::ColorConvertRGBtoHSV(float r, float g, float b, float& out_h, float& out_s, float& out_v)
{
    float K = 0.f;
    if (g < b)
    {
        ImSwap(g, b);
        K = -1.f;
    }
    if (r < g)
    {
        ImSwap(r, g);
        K = -2.f / 6.f - K;
    }

    const float chroma = r - (g < b ? g : b);
    out_h = ImFabs(K + (g - b) / (6.f * chroma + 1e-20f));
    out_s = chroma / (r + 1e-20f);
    out_v = r;
}

// Convert hsv floats ([0-1],[0-1],[0-1]) to rgb floats ([0-1],[0-1],[0-1]), from Foley & van Dam p593
// also http://en.wikipedia.org/wiki/HSL_and_HSV
void ImGui::ColorConvertHSVtoRGB(float h, float s, float v, float& out_r, float& out_g, float& out_b)
{
    if (s == 0.0f)
    {
        // gray
        out_r = out_g = out_b = v;
        return;
    }

    h = ImFmod(h, 1.0f) / (60.0f / 360.0f);
    int   i = (int)h;
    float f = h - (float)i;
    float p = v * (1.0f - s);
    float q = v * (1.0f - s * f);
    float t = v * (1.0f - s * (1.0f - f));

    switch (i)
    {
    case 0: out_r = v; out_g = t; out_b = p; break;
    case 1: out_r = q; out_g = v; out_b = p; break;
    case 2: out_r = p; out_g = v; out_b = t; break;
    case 3: out_r = p; out_g = q; out_b = v; break;
    case 4: out_r = t; out_g = p; out_b = v; break;
    case 5: default: out_r = v; out_g = p; out_b = q; break;
    }
}

//-----------------------------------------------------------------------------
// [SECTION] ImGuiStorage
// Helper: Key->value storage
//-----------------------------------------------------------------------------

// std::lower_bound but without the bullshit
static ImGuiStorage::ImGuiStoragePair* LowerBound(ImVector<ImGuiStorage::ImGuiStoragePair>& data, ImGuiID key)
{
    ImGuiStorage::ImGuiStoragePair* first = data.Data;
    ImGuiStorage::ImGuiStoragePair* last = data.Data + data.Size;
    size_t count = (size_t)(last - first);
    while (count > 0)
    {
        size_t count2 = count >> 1;
        ImGuiStorage::ImGuiStoragePair* mid = first + count2;
        if (mid->key < key)
        {
            first = ++mid;
            count -= count2 + 1;
        }
        else
        {
            count = count2;
        }
    }
    return first;
}

// For quicker full rebuild of a storage (instead of an incremental one), you may add all your contents and then sort once.
void ImGuiStorage::BuildSortByKey()
{
    struct StaticFunc
    {
        static int IMGUI_CDECL PairComparerByID(const void* lhs, const void* rhs)
        {
            // We can't just do a subtraction because qsort uses signed integers and subtracting our ID doesn't play well with that.
            if (((const ImGuiStoragePair*)lhs)->key > ((const ImGuiStoragePair*)rhs)->key) return +1;
            if (((const ImGuiStoragePair*)lhs)->key < ((const ImGuiStoragePair*)rhs)->key) return -1;
            return 0;
        }
    };
    ImQsort(Data.Data, (size_t)Data.Size, sizeof(ImGuiStoragePair), StaticFunc::PairComparerByID);
}

int ImGuiStorage::GetInt(ImGuiID key, int default_val) const
{
    ImGuiStoragePair* it = LowerBound(const_cast<ImVector<ImGuiStoragePair>&>(Data), key);
    if (it == Data.end() || it->key != key)
        return default_val;
    return it->val_i;
}

bool ImGuiStorage::GetBool(ImGuiID key, bool default_val) const
{
    return GetInt(key, default_val ? 1 : 0) != 0;
}

float ImGuiStorage::GetFloat(ImGuiID key, float default_val) const
{
    ImGuiStoragePair* it = LowerBound(const_cast<ImVector<ImGuiStoragePair>&>(Data), key);
    if (it == Data.end() || it->key != key)
        return default_val;
    return it->val_f;
}

void* ImGuiStorage::GetVoidPtr(ImGuiID key) const
{
    ImGuiStoragePair* it = LowerBound(const_cast<ImVector<ImGuiStoragePair>&>(Data), key);
    if (it == Data.end() || it->key != key)
        return NULL;
    return it->val_p;
}

// References are only valid until a new value is added to the storage. Calling a Set***() function or a Get***Ref() function invalidates the pointer.
int* ImGuiStorage::GetIntRef(ImGuiID key, int default_val)
{
    ImGuiStoragePair* it = LowerBound(Data, key);
    if (it == Data.end() || it->key != key)
        it = Data.insert(it, ImGuiStoragePair(key, default_val));
    return &it->val_i;
}

bool* ImGuiStorage::GetBoolRef(ImGuiID key, bool default_val)
{
    return (bool*)GetIntRef(key, default_val ? 1 : 0);
}

float* ImGuiStorage::GetFloatRef(ImGuiID key, float default_val)
{
    ImGuiStoragePair* it = LowerBound(Data, key);
    if (it == Data.end() || it->key != key)
        it = Data.insert(it, ImGuiStoragePair(key, default_val));
    return &it->val_f;
}

void** ImGuiStorage::GetVoidPtrRef(ImGuiID key, void* default_val)
{
    ImGuiStoragePair* it = LowerBound(Data, key);
    if (it == Data.end() || it->key != key)
        it = Data.insert(it, ImGuiStoragePair(key, default_val));
    return &it->val_p;
}

// FIXME-OPT: Need a way to reuse the result of lower_bound when doing GetInt()/SetInt() - not too bad because it only happens on explicit interaction (maximum one a frame)
void ImGuiStorage::SetInt(ImGuiID key, int val)
{
    ImGuiStoragePair* it = LowerBound(Data, key);
    if (it == Data.end() || it->key != key)
    {
        Data.insert(it, ImGuiStoragePair(key, val));
        return;
    }
    it->val_i = val;
}

void ImGuiStorage::SetBool(ImGuiID key, bool val)
{
    SetInt(key, val ? 1 : 0);
}

void ImGuiStorage::SetFloat(ImGuiID key, float val)
{
    ImGuiStoragePair* it = LowerBound(Data, key);
    if (it == Data.end() || it->key != key)
    {
        Data.insert(it, ImGuiStoragePair(key, val));
        return;
    }
    it->val_f = val;
}

void ImGuiStorage::SetVoidPtr(ImGuiID key, void* val)
{
    ImGuiStoragePair* it = LowerBound(Data, key);
    if (it == Data.end() || it->key != key)
    {
        Data.insert(it, ImGuiStoragePair(key, val));
        return;
    }
    it->val_p = val;
}

void ImGuiStorage::SetAllInt(int v)
{
    for (int i = 0; i < Data.Size; i++)
        Data[i].val_i = v;
}

//-----------------------------------------------------------------------------
// [SECTION] ImGuiTextFilter
//-----------------------------------------------------------------------------

// Helper: Parse and apply text filters. In format "aaaaa[,bbbb][,ccccc]"
ImGuiTextFilter::ImGuiTextFilter(const char* default_filter) //-V1077
{
    InputBuf[0] = 0;
    CountGrep = 0;
    if (default_filter)
    {
        ImStrncpy(InputBuf, default_filter, IM_ARRAYSIZE(InputBuf));
        Build();
    }
}

bool ImGuiTextFilter::Draw(const char* label, float width)
{
    if (width != 0.0f)
        ImGui::SetNextItemWidth(width);
    bool value_changed = ImGui::InputText(label, InputBuf, IM_ARRAYSIZE(InputBuf));
    if (value_changed)
        Build();
    return value_changed;
}

void ImGuiTextFilter::ImGuiTextRange::split(char separator, ImVector<ImGuiTextRange>* out) const
{
    out->resize(0);
    const char* wb = b;
    const char* we = wb;
    while (we < e)
    {
        if (*we == separator)
        {
            out->push_back(ImGuiTextRange(wb, we));
            wb = we + 1;
        }
        we++;
    }
    if (wb != we)
        out->push_back(ImGuiTextRange(wb, we));
}

void ImGuiTextFilter::Build()
{
    Filters.resize(0);
    ImGuiTextRange input_range(InputBuf, InputBuf + strlen(InputBuf));
    input_range.split(',', &Filters);

    CountGrep = 0;
    for (int i = 0; i != Filters.Size; i++)
    {
        ImGuiTextRange& f = Filters[i];
        while (f.b < f.e && ImCharIsBlankA(f.b[0]))
            f.b++;
        while (f.e > f.b && ImCharIsBlankA(f.e[-1]))
            f.e--;
        if (f.empty())
            continue;
        if (Filters[i].b[0] != '-')
            CountGrep += 1;
    }
}

bool ImGuiTextFilter::PassFilter(const char* text, const char* text_end) const
{
    if (Filters.empty())
        return true;

    if (text == NULL)
        text = "";

    for (int i = 0; i != Filters.Size; i++)
    {
        const ImGuiTextRange& f = Filters[i];
        if (f.empty())
            continue;
        if (f.b[0] == '-')
        {
            // Subtract
            if (ImStristr(text, text_end, f.b + 1, f.e) != NULL)
                return false;
        }
        else
        {
            // Grep
            if (ImStristr(text, text_end, f.b, f.e) != NULL)
                return true;
        }
    }

    // Implicit * grep
    if (CountGrep == 0)
        return true;

    return false;
}

//-----------------------------------------------------------------------------
// [SECTION] ImGuiTextBuffer
//-----------------------------------------------------------------------------

// On some platform vsnprintf() takes va_list by reference and modifies it.
// va_copy is the 'correct' way to copy a va_list but Visual Studio prior to 2013 doesn't have it.
#ifndef va_copy
#if defined(__GNUC__) || defined(__clang__)
#define va_copy(dest, src) __builtin_va_copy(dest, src)
#else
#define va_copy(dest, src) (dest = src)
#endif
#endif

char ImGuiTextBuffer::EmptyString[1] = { 0 };

void ImGuiTextBuffer::append(const char* str, const char* str_end)
{
    int len = str_end ? (int)(str_end - str) : (int)strlen(str);

    // Add zero-terminator the first time
    const int write_off = (Buf.Size != 0) ? Buf.Size : 1;
    const int needed_sz = write_off + len;
    if (write_off + len >= Buf.Capacity)
    {
        int new_capacity = Buf.Capacity * 2;
        Buf.reserve(needed_sz > new_capacity ? needed_sz : new_capacity);
    }

    Buf.resize(needed_sz);
    memcpy(&Buf[write_off - 1], str, (size_t)len);
    Buf[write_off - 1 + len] = 0;
}

void ImGuiTextBuffer::appendf(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    appendfv(fmt, args);
    va_end(args);
}

// Helper: Text buffer for logging/accumulating text
void ImGuiTextBuffer::appendfv(const char* fmt, va_list args)
{
    va_list args_copy;
    va_copy(args_copy, args);

    int len = ImFormatStringV(NULL, 0, fmt, args);         // FIXME-OPT: could do a first pass write attempt, likely successful on first pass.
    if (len <= 0)
    {
        va_end(args_copy);
        return;
    }

    // Add zero-terminator the first time
    const int write_off = (Buf.Size != 0) ? Buf.Size : 1;
    const int needed_sz = write_off + len;
    if (write_off + len >= Buf.Capacity)
    {
        int new_capacity = Buf.Capacity * 2;
        Buf.reserve(needed_sz > new_capacity ? needed_sz : new_capacity);
    }

    Buf.resize(needed_sz);
    ImFormatStringV(&Buf[write_off - 1], (size_t)len + 1, fmt, args_copy);
    va_end(args_copy);
}

//-----------------------------------------------------------------------------
// [SECTION] ImGuiListClipper
// This is currently not as flexible/powerful as it should be and really confusing/spaghetti, mostly because we changed
// the API mid-way through development and support two ways to using the clipper, needs some rework (see TODO)
//-----------------------------------------------------------------------------

// FIXME-TABLE: This prevents us from using ImGuiListClipper _inside_ a table cell.
// The problem we have is that without a Begin/End scheme for rows using the clipper is ambiguous.
static bool GetSkipItemForListClipping()
{
    ImGuiContext& g = *GImGui;
    return (g.CurrentTable ? g.CurrentTable->HostSkipItems : g.CurrentWindow->SkipItems);
}

#ifndef IMGUI_DISABLE_OBSOLETE_FUNCTIONS
// Legacy helper to calculate coarse clipping of large list of evenly sized items.
// This legacy API is not ideal because it assume we will return a single contiguous rectangle.
// Prefer using ImGuiListClipper which can returns non-contiguous ranges.
void ImGui::CalcListClipping(int items_count, float items_height, int* out_items_display_start, int* out_items_display_end)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;
    if (g.LogEnabled)
    {
        // If logging is active, do not perform any clipping
        *out_items_display_start = 0;
        *out_items_display_end = items_count;
        return;
    }
    if (GetSkipItemForListClipping())
    {
        *out_items_display_start = *out_items_display_end = 0;
        return;
    }

    // We create the union of the ClipRect and the scoring rect which at worst should be 1 page away from ClipRect
    // We don't include g.NavId's rectangle in there (unless g.NavJustMovedToId is set) because the rectangle enlargement can get costly.
    ImRect rect = window->ClipRect;
    if (g.NavMoveScoringItems)
        rect.Add(g.NavScoringNoClipRect);
    if (g.NavJustMovedToId && window->NavLastIds[0] == g.NavJustMovedToId)
        rect.Add(WindowRectRelToAbs(window, window->NavRectRel[0])); // Could store and use NavJustMovedToRectRel

    const ImVec2 pos = window->DC.CursorPos;
    int start = (int)((rect.Min.y - pos.y) / items_height);
    int end = (int)((rect.Max.y - pos.y) / items_height);

    // When performing a navigation request, ensure we have one item extra in the direction we are moving to
    // FIXME: Verify this works with tabbing
    const bool is_nav_request = (g.NavMoveScoringItems && g.NavWindow && g.NavWindow->RootWindowForNav == window->RootWindowForNav);
    if (is_nav_request && g.NavMoveClipDir == ImGuiDir_Up)
        start--;
    if (is_nav_request && g.NavMoveClipDir == ImGuiDir_Down)
        end++;

    start = ImClamp(start, 0, items_count);
    end = ImClamp(end + 1, start, items_count);
    *out_items_display_start = start;
    *out_items_display_end = end;
}
#endif

static void ImGuiListClipper_SortAndFuseRanges(ImVector<ImGuiListClipperRange>& ranges, int offset = 0)
{
    if (ranges.Size - offset <= 1)
        return;

    // Helper to order ranges and fuse them together if possible (bubble sort is fine as we are only sorting 2-3 entries)
    for (int sort_end = ranges.Size - offset - 1; sort_end > 0; --sort_end)
        for (int i = offset; i < sort_end + offset; ++i)
            if (ranges[i].Min > ranges[i + 1].Min)
                ImSwap(ranges[i], ranges[i + 1]);

    // Now fuse ranges together as much as possible.
    for (int i = 1 + offset; i < ranges.Size; i++)
    {
        IM_ASSERT(!ranges[i].PosToIndexConvert && !ranges[i - 1].PosToIndexConvert);
        if (ranges[i - 1].Max < ranges[i].Min)
            continue;
        ranges[i - 1].Min = ImMin(ranges[i - 1].Min, ranges[i].Min);
        ranges[i - 1].Max = ImMax(ranges[i - 1].Max, ranges[i].Max);
        ranges.erase(ranges.Data + i);
        i--;
    }
}

static void ImGuiListClipper_SeekCursorAndSetupPrevLine(float pos_y, float line_height)
{
    // Set cursor position and a few other things so that SetScrollHereY() and Columns() can work when seeking cursor.
    // FIXME: It is problematic that we have to do that here, because custom/equivalent end-user code would stumble on the same issue.
    // The clipper should probably have a final step to display the last item in a regular manner, maybe with an opt-out flag for data sets which may have costly seek?
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;
    float off_y = pos_y - window->DC.CursorPos.y;
    window->DC.CursorPos.y = pos_y;
    window->DC.CursorMaxPos.y = ImMax(window->DC.CursorMaxPos.y, pos_y - g.Style.ItemSpacing.y);
    window->DC.CursorPosPrevLine.y = window->DC.CursorPos.y - line_height;  // Setting those fields so that SetScrollHereY() can properly function after the end of our clipper usage.
    window->DC.PrevLineSize.y = (line_height - g.Style.ItemSpacing.y);      // If we end up needing more accurate data (to e.g. use SameLine) we may as well make the clipper have a fourth step to let user process and display the last item in their list.
    if (ImGuiOldColumns* columns = window->DC.CurrentColumns)
        columns->LineMinY = window->DC.CursorPos.y;                         // Setting this so that cell Y position are set properly
    if (ImGuiTable* table = g.CurrentTable)
    {
        if (table->IsInsideRow)
            ImGui::TableEndRow(table);
        table->RowPosY2 = window->DC.CursorPos.y;
        const int row_increase = (int)((off_y / line_height) + 0.5f);
        //table->CurrentRow += row_increase; // Can't do without fixing TableEndRow()
        table->RowBgColorCounter += row_increase;
    }
}

static void ImGuiListClipper_SeekCursorForItem(ImGuiListClipper* clipper, int item_n)
{
    // StartPosY starts from ItemsFrozen hence the subtraction
    // Perform the add and multiply with double to allow seeking through larger ranges
    ImGuiListClipperData* data = (ImGuiListClipperData*)clipper->TempData;
    float pos_y = (float)((double)clipper->StartPosY + data->LossynessOffset + (double)(item_n - data->ItemsFrozen) * clipper->ItemsHeight);
    ImGuiListClipper_SeekCursorAndSetupPrevLine(pos_y, clipper->ItemsHeight);
}

ImGuiListClipper::ImGuiListClipper()
{
    memset(this, 0, sizeof(*this));
    ItemsCount = -1;
}

ImGuiListClipper::~ImGuiListClipper()
{
    End();
}

// Use case A: Begin() called from constructor with items_height<0, then called again from Step() in StepNo 1
// Use case B: Begin() called from constructor with items_height>0
// FIXME-LEGACY: Ideally we should remove the Begin/End functions but they are part of the legacy API we still support. This is why some of the code in Step() calling Begin() and reassign some fields, spaghetti style.
void ImGuiListClipper::Begin(int items_count, float items_height)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;

    if (ImGuiTable* table = g.CurrentTable)
        if (table->IsInsideRow)
            ImGui::TableEndRow(table);

    StartPosY = window->DC.CursorPos.y;
    ItemsHeight = items_height;
    ItemsCount = items_count;
    DisplayStart = -1;
    DisplayEnd = 0;

    // Acquire temporary buffer
    if (++g.ClipperTempDataStacked > g.ClipperTempData.Size)
        g.ClipperTempData.resize(g.ClipperTempDataStacked, ImGuiListClipperData());
    ImGuiListClipperData* data = &g.ClipperTempData[g.ClipperTempDataStacked - 1];
    data->Reset(this);
    data->LossynessOffset = window->DC.CursorStartPosLossyness.y;
    TempData = data;
}

void ImGuiListClipper::End()
{
    ImGuiContext& g = *GImGui;
    if (ImGuiListClipperData* data = (ImGuiListClipperData*)TempData)
    {
        // In theory here we should assert that we are already at the right position, but it seems saner to just seek at the end and not assert/crash the user.
        if (ItemsCount >= 0 && ItemsCount < INT_MAX && DisplayStart >= 0)
            ImGuiListClipper_SeekCursorForItem(this, ItemsCount);

        // Restore temporary buffer and fix back pointers which may be invalidated when nesting
        IM_ASSERT(data->ListClipper == this);
        data->StepNo = data->Ranges.Size;
        if (--g.ClipperTempDataStacked > 0)
        {
            data = &g.ClipperTempData[g.ClipperTempDataStacked - 1];
            data->ListClipper->TempData = data;
        }
        TempData = NULL;
    }
    ItemsCount = -1;
}

void ImGuiListClipper::ForceDisplayRangeByIndices(int item_min, int item_max)
{
    ImGuiListClipperData* data = (ImGuiListClipperData*)TempData;
    IM_ASSERT(DisplayStart < 0); // Only allowed after Begin() and if there has not been a specified range yet.
    IM_ASSERT(item_min <= item_max);
    if (item_min < item_max)
        data->Ranges.push_back(ImGuiListClipperRange::FromIndices(item_min, item_max));
}

bool ImGuiListClipper::Step()
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;
    ImGuiListClipperData* data = (ImGuiListClipperData*)TempData;
    IM_ASSERT(data != NULL && "Called ImGuiListClipper::Step() too many times, or before ImGuiListClipper::Begin() ?");

    ImGuiTable* table = g.CurrentTable;
    if (table && table->IsInsideRow)
        ImGui::TableEndRow(table);

    // No items
    if (ItemsCount == 0 || GetSkipItemForListClipping())
        return (void)End(), false;

    // While we are in frozen row state, keep displaying items one by one, unclipped
    // FIXME: Could be stored as a table-agnostic state.
    if (data->StepNo == 0 && table != NULL && !table->IsUnfrozenRows)
    {
        DisplayStart = data->ItemsFrozen;
        DisplayEnd = data->ItemsFrozen + 1;
        if (DisplayStart >= ItemsCount)
            return (void)End(), false;
        data->ItemsFrozen++;
        return true;
    }

    // Step 0: Let you process the first element (regardless of it being visible or not, so we can measure the element height)
    bool calc_clipping = false;
    if (data->StepNo == 0)
    {
        StartPosY = window->DC.CursorPos.y;
        if (ItemsHeight <= 0.0f)
        {
            // Submit the first item (or range) so we can measure its height (generally the first range is 0..1)
            data->Ranges.push_front(ImGuiListClipperRange::FromIndices(data->ItemsFrozen, data->ItemsFrozen + 1));
            DisplayStart = ImMax(data->Ranges[0].Min, data->ItemsFrozen);
            DisplayEnd = ImMin(data->Ranges[0].Max, ItemsCount);
            if (DisplayStart == DisplayEnd)
                return (void)End(), false;
            data->StepNo = 1;
            return true;
        }
        calc_clipping = true;   // If on the first step with known item height, calculate clipping.
    }

    // Step 1: Let the clipper infer height from first range
    if (ItemsHeight <= 0.0f)
    {
        IM_ASSERT(data->StepNo == 1);
        if (table)
            IM_ASSERT(table->RowPosY1 == StartPosY && table->RowPosY2 == window->DC.CursorPos.y);

        ItemsHeight = (window->DC.CursorPos.y - StartPosY) / (float)(DisplayEnd - DisplayStart);
        bool affected_by_floating_point_precision = ImIsFloatAboveGuaranteedIntegerPrecision(StartPosY) || ImIsFloatAboveGuaranteedIntegerPrecision(window->DC.CursorPos.y);
        if (affected_by_floating_point_precision)
            ItemsHeight = window->DC.PrevLineSize.y + g.Style.ItemSpacing.y; // FIXME: Technically wouldn't allow multi-line entries.

        IM_ASSERT(ItemsHeight > 0.0f && "Unable to calculate item height! First item hasn't moved the cursor vertically!");
        calc_clipping = true;   // If item height had to be calculated, calculate clipping afterwards.
    }

    // Step 0 or 1: Calculate the actual ranges of visible elements.
    const int already_submitted = DisplayEnd;
    if (calc_clipping)
    {
        if (g.LogEnabled)
        {
            // If logging is active, do not perform any clipping
            data->Ranges.push_back(ImGuiListClipperRange::FromIndices(0, ItemsCount));
        }
        else
        {
            // Add range selected to be included for navigation
            const bool is_nav_request = (g.NavMoveScoringItems && g.NavWindow && g.NavWindow->RootWindowForNav == window->RootWindowForNav);
            if (is_nav_request)
                data->Ranges.push_back(ImGuiListClipperRange::FromPositions(g.NavScoringNoClipRect.Min.y, g.NavScoringNoClipRect.Max.y, 0, 0));
            if (is_nav_request && (g.NavMoveFlags & ImGuiNavMoveFlags_Tabbing) && g.NavTabbingDir == -1)
                data->Ranges.push_back(ImGuiListClipperRange::FromIndices(ItemsCount - 1, ItemsCount));

            // Add focused/active item
            ImRect nav_rect_abs = ImGui::WindowRectRelToAbs(window, window->NavRectRel[0]);
            if (g.NavId != 0 && window->NavLastIds[0] == g.NavId)
                data->Ranges.push_back(ImGuiListClipperRange::FromPositions(nav_rect_abs.Min.y, nav_rect_abs.Max.y, 0, 0));

            // Add visible range
            const int off_min = (is_nav_request && g.NavMoveClipDir == ImGuiDir_Up) ? -1 : 0;
            const int off_max = (is_nav_request && g.NavMoveClipDir == ImGuiDir_Down) ? 1 : 0;
            data->Ranges.push_back(ImGuiListClipperRange::FromPositions(window->ClipRect.Min.y, window->ClipRect.Max.y, off_min, off_max));
        }

        // Convert position ranges to item index ranges
        // - Very important: when a starting position is after our maximum item, we set Min to (ItemsCount - 1). This allows us to handle most forms of wrapping.
        // - Due to how Selectable extra padding they tend to be "unaligned" with exact unit in the item list,
        //   which with the flooring/ceiling tend to lead to 2 items instead of one being submitted.
        for (int i = 0; i < data->Ranges.Size; i++)
            if (data->Ranges[i].PosToIndexConvert)
            {
                int m1 = (int)(((double)data->Ranges[i].Min - window->DC.CursorPos.y - data->LossynessOffset) / ItemsHeight);
                int m2 = (int)((((double)data->Ranges[i].Max - window->DC.CursorPos.y - data->LossynessOffset) / ItemsHeight) + 0.999999f);
                data->Ranges[i].Min = ImClamp(already_submitted + m1 + data->Ranges[i].PosToIndexOffsetMin, already_submitted, ItemsCount - 1);
                data->Ranges[i].Max = ImClamp(already_submitted + m2 + data->Ranges[i].PosToIndexOffsetMax, data->Ranges[i].Min + 1, ItemsCount);
                data->Ranges[i].PosToIndexConvert = false;
            }
        ImGuiListClipper_SortAndFuseRanges(data->Ranges, data->StepNo);
    }

    // Step 0+ (if item height is given in advance) or 1+: Display the next range in line.
    if (data->StepNo < data->Ranges.Size)
    {
        DisplayStart = ImMax(data->Ranges[data->StepNo].Min, already_submitted);
        DisplayEnd = ImMin(data->Ranges[data->StepNo].Max, ItemsCount);
        if (DisplayStart > already_submitted) //-V1051
            ImGuiListClipper_SeekCursorForItem(this, DisplayStart);
        data->StepNo++;
        return true;
    }

    // After the last step: Let the clipper validate that we have reached the expected Y position (corresponding to element DisplayEnd),
    // Advance the cursor to the end of the list and then returns 'false' to end the loop.
    if (ItemsCount < INT_MAX)
        ImGuiListClipper_SeekCursorForItem(this, ItemsCount);

    End();
    return false;
}

//-----------------------------------------------------------------------------
// [SECTION] STYLING
//-----------------------------------------------------------------------------

ImGuiStyle& ImGui::GetStyle()
{
    IM_ASSERT(GImGui != NULL && "No current context. Did you call ImGui::CreateContext() and ImGui::SetCurrentContext() ?");
    return GImGui->Style;
}

ImU32 ImGui::GetColorU32(ImGuiCol idx, float alpha_mul)
{
    ImGuiStyle& style = GImGui->Style;
    ImVec4 c = style.Colors[idx];
    c.w *= style.Alpha * alpha_mul;
    return ColorConvertFloat4ToU32(c);
}

ImU32 ImGui::GetColorU32(const ImVec4& col)
{
    ImGuiStyle& style = GImGui->Style;
    ImVec4 c = col;
    c.w *= style.Alpha;
    return ColorConvertFloat4ToU32(c);
}

const ImVec4& ImGui::GetStyleColorVec4(ImGuiCol idx)
{
    ImGuiStyle& style = GImGui->Style;
    return style.Colors[idx];
}

ImU32 ImGui::GetColorU32(ImU32 col)
{
    ImGuiStyle& style = GImGui->Style;
    if (style.Alpha >= 1.0f)
        return col;
    ImU32 a = (col & IM_COL32_A_MASK) >> IM_COL32_A_SHIFT;
    a = (ImU32)(a * style.Alpha); // We don't need to clamp 0..255 because Style.Alpha is in 0..1 range.
    return (col & ~IM_COL32_A_MASK) | (a << IM_COL32_A_SHIFT);
}

// FIXME: This may incur a round-trip (if the end user got their data from a float4) but eventually we aim to store the in-flight colors as ImU32
void ImGui::PushStyleColor(ImGuiCol idx, ImU32 col)
{
    ImGuiContext& g = *GImGui;
    ImGuiColorMod backup;
    backup.Col = idx;
    backup.BackupValue = g.Style.Colors[idx];
    g.ColorStack.push_back(backup);
    g.Style.Colors[idx] = ColorConvertU32ToFloat4(col);
}

void ImGui::PushStyleColor(ImGuiCol idx, const ImVec4& col)
{
    ImGuiContext& g = *GImGui;
    ImGuiColorMod backup;
    backup.Col = idx;
    backup.BackupValue = g.Style.Colors[idx];
    g.ColorStack.push_back(backup);
    g.Style.Colors[idx] = col;
}

void ImGui::PopStyleColor(int count)
{
    ImGuiContext& g = *GImGui;
    while (count > 0)
    {
        ImGuiColorMod& backup = g.ColorStack.back();
        g.Style.Colors[backup.Col] = backup.BackupValue;
        g.ColorStack.pop_back();
        count--;
    }
}

struct ImGuiStyleVarInfo
{
    ImGuiDataType   Type;
    ImU32           Count;
    ImU32           Offset;
    void*           GetVarPtr(ImGuiStyle* style) const { return (void*)((unsigned char*)style + Offset); }
};

static const ImGuiStyleVarInfo GStyleVarInfo[] =
{
    { ImGuiDataType_Float, 1, (ImU32)IM_OFFSETOF(ImGuiStyle, Alpha) },               // ImGuiStyleVar_Alpha
    { ImGuiDataType_Float, 1, (ImU32)IM_OFFSETOF(ImGuiStyle, DisabledAlpha) },       // ImGuiStyleVar_DisabledAlpha
    { ImGuiDataType_Float, 2, (ImU32)IM_OFFSETOF(ImGuiStyle, WindowPadding) },       // ImGuiStyleVar_WindowPadding
    { ImGuiDataType_Float, 1, (ImU32)IM_OFFSETOF(ImGuiStyle, WindowRounding) },      // ImGuiStyleVar_WindowRounding
    { ImGuiDataType_Float, 1, (ImU32)IM_OFFSETOF(ImGuiStyle, WindowBorderSize) },    // ImGuiStyleVar_WindowBorderSize
    { ImGuiDataType_Float, 2, (ImU32)IM_OFFSETOF(ImGuiStyle, WindowMinSize) },       // ImGuiStyleVar_WindowMinSize
    { ImGuiDataType_Float, 2, (ImU32)IM_OFFSETOF(ImGuiStyle, WindowTitleAlign) },    // ImGuiStyleVar_WindowTitleAlign
    { ImGuiDataType_Float, 1, (ImU32)IM_OFFSETOF(ImGuiStyle, ChildRounding) },       // ImGuiStyleVar_ChildRounding
    { ImGuiDataType_Float, 1, (ImU32)IM_OFFSETOF(ImGuiStyle, ChildBorderSize) },     // ImGuiStyleVar_ChildBorderSize
    { ImGuiDataType_Float, 1, (ImU32)IM_OFFSETOF(ImGuiStyle, PopupRounding) },       // ImGuiStyleVar_PopupRounding
    { ImGuiDataType_Float, 1, (ImU32)IM_OFFSETOF(ImGuiStyle, PopupBorderSize) },     // ImGuiStyleVar_PopupBorderSize
    { ImGuiDataType_Float, 2, (ImU32)IM_OFFSETOF(ImGuiStyle, FramePadding) },        // ImGuiStyleVar_FramePadding
    { ImGuiDataType_Float, 1, (ImU32)IM_OFFSETOF(ImGuiStyle, FrameRounding) },       // ImGuiStyleVar_FrameRounding
    { ImGuiDataType_Float, 1, (ImU32)IM_OFFSETOF(ImGuiStyle, FrameBorderSize) },     // ImGuiStyleVar_FrameBorderSize
    { ImGuiDataType_Float, 2, (ImU32)IM_OFFSETOF(ImGuiStyle, ItemSpacing) },         // ImGuiStyleVar_ItemSpacing
    { ImGuiDataType_Float, 2, (ImU32)IM_OFFSETOF(ImGuiStyle, ItemInnerSpacing) },    // ImGuiStyleVar_ItemInnerSpacing
    { ImGuiDataType_Float, 1, (ImU32)IM_OFFSETOF(ImGuiStyle, IndentSpacing) },       // ImGuiStyleVar_IndentSpacing
    { ImGuiDataType_Float, 2, (ImU32)IM_OFFSETOF(ImGuiStyle, CellPadding) },         // ImGuiStyleVar_CellPadding
    { ImGuiDataType_Float, 1, (ImU32)IM_OFFSETOF(ImGuiStyle, ScrollbarSize) },       // ImGuiStyleVar_ScrollbarSize
    { ImGuiDataType_Float, 1, (ImU32)IM_OFFSETOF(ImGuiStyle, ScrollbarRounding) },   // ImGuiStyleVar_ScrollbarRounding
    { ImGuiDataType_Float, 1, (ImU32)IM_OFFSETOF(ImGuiStyle, GrabMinSize) },         // ImGuiStyleVar_GrabMinSize
    { ImGuiDataType_Float, 1, (ImU32)IM_OFFSETOF(ImGuiStyle, GrabRounding) },        // ImGuiStyleVar_GrabRounding
    { ImGuiDataType_Float, 1, (ImU32)IM_OFFSETOF(ImGuiStyle, TabRounding) },         // ImGuiStyleVar_TabRounding
    { ImGuiDataType_Float, 2, (ImU32)IM_OFFSETOF(ImGuiStyle, ButtonTextAlign) },     // ImGuiStyleVar_ButtonTextAlign
    { ImGuiDataType_Float, 2, (ImU32)IM_OFFSETOF(ImGuiStyle, SelectableTextAlign) }, // ImGuiStyleVar_SelectableTextAlign
};

static const ImGuiStyleVarInfo* GetStyleVarInfo(ImGuiStyleVar idx)
{
    IM_ASSERT(idx >= 0 && idx < ImGuiStyleVar_COUNT);
    IM_ASSERT(IM_ARRAYSIZE(GStyleVarInfo) == ImGuiStyleVar_COUNT);
    return &GStyleVarInfo[idx];
}

void ImGui::PushStyleVar(ImGuiStyleVar idx, float val)
{
    const ImGuiStyleVarInfo* var_info = GetStyleVarInfo(idx);
    if (var_info->Type == ImGuiDataType_Float && var_info->Count == 1)
    {
        ImGuiContext& g = *GImGui;
        float* pvar = (float*)var_info->GetVarPtr(&g.Style);
        g.StyleVarStack.push_back(ImGuiStyleMod(idx, *pvar));
        *pvar = val;
        return;
    }
    IM_ASSERT(0 && "Called PushStyleVar() float variant but variable is not a float!");
}

void ImGui::PushStyleVar(ImGuiStyleVar idx, const ImVec2& val)
{
    const ImGuiStyleVarInfo* var_info = GetStyleVarInfo(idx);
    if (var_info->Type == ImGuiDataType_Float && var_info->Count == 2)
    {
        ImGuiContext& g = *GImGui;
        ImVec2* pvar = (ImVec2*)var_info->GetVarPtr(&g.Style);
        g.StyleVarStack.push_back(ImGuiStyleMod(idx, *pvar));
        *pvar = val;
        return;
    }
    IM_ASSERT(0 && "Called PushStyleVar() ImVec2 variant but variable is not a ImVec2!");
}

void ImGui::PopStyleVar(int count)
{
    ImGuiContext& g = *GImGui;
    while (count > 0)
    {
        // We avoid a generic memcpy(data, &backup.Backup.., GDataTypeSize[info->Type] * info->Count), the overhead in Debug is not worth it.
        ImGuiStyleMod& backup = g.StyleVarStack.back();
        const ImGuiStyleVarInfo* info = GetStyleVarInfo(backup.VarIdx);
        void* data = info->GetVarPtr(&g.Style);
        if (info->Type == ImGuiDataType_Float && info->Count == 1)      { ((float*)data)[0] = backup.BackupFloat[0]; }
        else if (info->Type == ImGuiDataType_Float && info->Count == 2) { ((float*)data)[0] = backup.BackupFloat[0]; ((float*)data)[1] = backup.BackupFloat[1]; }
        g.StyleVarStack.pop_back();
        count--;
    }
}

const char* ImGui::GetStyleColorName(ImGuiCol idx)
{
    // Create switch-case from enum with regexp: ImGuiCol_{.*}, --> case ImGuiCol_\1: return "\1";
    switch (idx)
    {
    case ImGuiCol_Text: return "Text";
    case ImGuiCol_TextDisabled: return "TextDisabled";
    case ImGuiCol_WindowBg: return "WindowBg";
    case ImGuiCol_ChildBg: return "ChildBg";
    case ImGuiCol_PopupBg: return "PopupBg";
    case ImGuiCol_Border: return "Border";
    case ImGuiCol_BorderShadow: return "BorderShadow";
    case ImGuiCol_FrameBg: return "FrameBg";
    case ImGuiCol_FrameBgHovered: return "FrameBgHovered";
    case ImGuiCol_FrameBgActive: return "FrameBgActive";
    case ImGuiCol_TitleBg: return "TitleBg";
    case ImGuiCol_TitleBgActive: return "TitleBgActive";
    case ImGuiCol_TitleBgCollapsed: return "TitleBgCollapsed";
    case ImGuiCol_MenuBarBg: return "MenuBarBg";
    case ImGuiCol_ScrollbarBg: return "ScrollbarBg";
    case ImGuiCol_ScrollbarGrab: return "ScrollbarGrab";
    case ImGuiCol_ScrollbarGrabHovered: return "ScrollbarGrabHovered";
    case ImGuiCol_ScrollbarGrabActive: return "ScrollbarGrabActive";
    case ImGuiCol_CheckMark: return "CheckMark";
    case ImGuiCol_SliderGrab: return "SliderGrab";
    case ImGuiCol_SliderGrabActive: return "SliderGrabActive";
    case ImGuiCol_Button: return "Button";
    case ImGuiCol_ButtonHovered: return "ButtonHovered";
    case ImGuiCol_ButtonActive: return "ButtonActive";
    case ImGuiCol_Header: return "Header";
    case ImGuiCol_HeaderHovered: return "HeaderHovered";
    case ImGuiCol_HeaderActive: return "HeaderActive";
    case ImGuiCol_Separator: return "Separator";
    case ImGuiCol_SeparatorHovered: return "SeparatorHovered";
    case ImGuiCol_SeparatorActive: return "SeparatorActive";
    case ImGuiCol_ResizeGrip: return "ResizeGrip";
    case ImGuiCol_ResizeGripHovered: return "ResizeGripHovered";
    case ImGuiCol_ResizeGripActive: return "ResizeGripActive";
    case ImGuiCol_Tab: return "Tab";
    case ImGuiCol_TabHovered: return "TabHovered";
    case ImGuiCol_TabActive: return "TabActive";
    case ImGuiCol_TabUnfocused: return "TabUnfocused";
    case ImGuiCol_TabUnfocusedActive: return "TabUnfocusedActive";
    case ImGuiCol_PlotLines: return "PlotLines";
    case ImGuiCol_PlotLinesHovered: return "PlotLinesHovered";
    case ImGuiCol_PlotHistogram: return "PlotHistogram";
    case ImGuiCol_PlotHistogramHovered: return "PlotHistogramHovered";
    case ImGuiCol_TableHeaderBg: return "TableHeaderBg";
    case ImGuiCol_TableBorderStrong: return "TableBorderStrong";
    case ImGuiCol_TableBorderLight: return "TableBorderLight";
    case ImGuiCol_TableRowBg: return "TableRowBg";
    case ImGuiCol_TableRowBgAlt: return "TableRowBgAlt";
    case ImGuiCol_TextSelectedBg: return "TextSelectedBg";
    case ImGuiCol_DragDropTarget: return "DragDropTarget";
    case ImGuiCol_NavHighlight: return "NavHighlight";
    case ImGuiCol_NavWindowingHighlight: return "NavWindowingHighlight";
    case ImGuiCol_NavWindowingDimBg: return "NavWindowingDimBg";
    case ImGuiCol_ModalWindowDimBg: return "ModalWindowDimBg";
    }
    IM_ASSERT(0);
    return "Unknown";
}


//-----------------------------------------------------------------------------
// [SECTION] RENDER HELPERS
// Some of those (internal) functions are currently quite a legacy mess - their signature and behavior will change,
// we need a nicer separation between low-level functions and high-level functions relying on the ImGui context.
// Also see imgui_draw.cpp for some more which have been reworked to not rely on ImGui:: context.
//-----------------------------------------------------------------------------

const char* ImGui::FindRenderedTextEnd(const char* text, const char* text_end)
{
    const char* text_display_end = text;
    if (!text_end)
        text_end = (const char*)-1;

    while (text_display_end < text_end && *text_display_end != '\0' && (text_display_end[0] != '#' || text_display_end[1] != '#'))
        text_display_end++;
    return text_display_end;
}

// Internal ImGui functions to render text
// RenderText***() functions calls ImDrawList::AddText() calls ImBitmapFont::RenderText()
void ImGui::RenderText(ImVec2 pos, const char* text, const char* text_end, bool hide_text_after_hash)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;

    // Hide anything after a '##' string
    const char* text_display_end;
    if (hide_text_after_hash)
    {
        text_display_end = FindRenderedTextEnd(text, text_end);
    }
    else
    {
        if (!text_end)
            text_end = text + strlen(text); // FIXME-OPT
        text_display_end = text_end;
    }

    if (text != text_display_end)
    {
        window->DrawList->AddText(g.Font, g.FontSize, pos, GetColorU32(ImGuiCol_Text), text, text_display_end);
        if (g.LogEnabled)
            LogRenderedText(&pos, text, text_display_end);
    }
}

void ImGui::RenderTextWrapped(ImVec2 pos, const char* text, const char* text_end, float wrap_width)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;

    if (!text_end)
        text_end = text + strlen(text); // FIXME-OPT

    if (text != text_end)
    {
        window->DrawList->AddText(g.Font, g.FontSize, pos, GetColorU32(ImGuiCol_Text), text, text_end, wrap_width);
        if (g.LogEnabled)
            LogRenderedText(&pos, text, text_end);
    }
}

// Default clip_rect uses (pos_min,pos_max)
// Handle clipping on CPU immediately (vs typically let the GPU clip the triangles that are overlapping the clipping rectangle edges)
void ImGui::RenderTextClippedEx(ImDrawList* draw_list, const ImVec2& pos_min, const ImVec2& pos_max, const char* text, const char* text_display_end, const ImVec2* text_size_if_known, const ImVec2& align, const ImRect* clip_rect)
{
    // Perform CPU side clipping for single clipped element to avoid using scissor state
    ImVec2 pos = pos_min;
    const ImVec2 text_size = text_size_if_known ? *text_size_if_known : CalcTextSize(text, text_display_end, false, 0.0f);

    const ImVec2* clip_min = clip_rect ? &clip_rect->Min : &pos_min;
    const ImVec2* clip_max = clip_rect ? &clip_rect->Max : &pos_max;
    bool need_clipping = (pos.x + text_size.x >= clip_max->x) || (pos.y + text_size.y >= clip_max->y);
    if (clip_rect) // If we had no explicit clipping rectangle then pos==clip_min
        need_clipping |= (pos.x < clip_min->x) || (pos.y < clip_min->y);

    // Align whole block. We should defer that to the better rendering function when we'll have support for individual line alignment.
    if (align.x > 0.0f) pos.x = ImMax(pos.x, pos.x + (pos_max.x - pos.x - text_size.x) * align.x);
    if (align.y > 0.0f) pos.y = ImMax(pos.y, pos.y + (pos_max.y - pos.y - text_size.y) * align.y);

    // Render
    if (need_clipping)
    {
        ImVec4 fine_clip_rect(clip_min->x, clip_min->y, clip_max->x, clip_max->y);
        draw_list->AddText(NULL, 0.0f, pos, GetColorU32(ImGuiCol_Text), text, text_display_end, 0.0f, &fine_clip_rect);
    }
    else
    {
        draw_list->AddText(NULL, 0.0f, pos, GetColorU32(ImGuiCol_Text), text, text_display_end, 0.0f, NULL);
    }
}

void ImGui::RenderTextClipped(const ImVec2& pos_min, const ImVec2& pos_max, const char* text, const char* text_end, const ImVec2* text_size_if_known, const ImVec2& align, const ImRect* clip_rect)
{
    // Hide anything after a '##' string
    const char* text_display_end = FindRenderedTextEnd(text, text_end);
    const int text_len = (int)(text_display_end - text);
    if (text_len == 0)
        return;

    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;
    RenderTextClippedEx(window->DrawList, pos_min, pos_max, text, text_display_end, text_size_if_known, align, clip_rect);
    if (g.LogEnabled)
        LogRenderedText(&pos_min, text, text_display_end);
}


// Another overly complex function until we reorganize everything into a nice all-in-one helper.
// This is made more complex because we have dissociated the layout rectangle (pos_min..pos_max) which define _where_ the ellipsis is, from actual clipping of text and limit of the ellipsis display.
// This is because in the context of tabs we selectively hide part of the text when the Close Button appears, but we don't want the ellipsis to move.
void ImGui::RenderTextEllipsis(ImDrawList* draw_list, const ImVec2& pos_min, const ImVec2& pos_max, float clip_max_x, float ellipsis_max_x, const char* text, const char* text_end_full, const ImVec2* text_size_if_known)
{
    ImGuiContext& g = *GImGui;
    if (text_end_full == NULL)
        text_end_full = FindRenderedTextEnd(text);
    const ImVec2 text_size = text_size_if_known ? *text_size_if_known : CalcTextSize(text, text_end_full, false, 0.0f);

    //draw_list->AddLine(ImVec2(pos_max.x, pos_min.y - 4), ImVec2(pos_max.x, pos_max.y + 4), IM_COL32(0, 0, 255, 255));
    //draw_list->AddLine(ImVec2(ellipsis_max_x, pos_min.y-2), ImVec2(ellipsis_max_x, pos_max.y+2), IM_COL32(0, 255, 0, 255));
    //draw_list->AddLine(ImVec2(clip_max_x, pos_min.y), ImVec2(clip_max_x, pos_max.y), IM_COL32(255, 0, 0, 255));
    // FIXME: We could technically remove (last_glyph->AdvanceX - last_glyph->X1) from text_size.x here and save a few pixels.
    if (text_size.x > pos_max.x - pos_min.x)
    {
        // Hello wo...
        // |       |   |
        // min   max   ellipsis_max
        //          <-> this is generally some padding value

        const ImFont* font = draw_list->_Data->Font;
        const float font_size = draw_list->_Data->FontSize;
        const char* text_end_ellipsis = NULL;

        ImWchar ellipsis_char = font->EllipsisChar;
        int ellipsis_char_count = 1;
        if (ellipsis_char == (ImWchar)-1)
        {
            ellipsis_char = font->DotChar;
            ellipsis_char_count = 3;
        }
        const ImFontGlyph* glyph = font->FindGlyph(ellipsis_char);

        float ellipsis_glyph_width = glyph->X1;                 // Width of the glyph with no padding on either side
        float ellipsis_total_width = ellipsis_glyph_width;      // Full width of entire ellipsis

        if (ellipsis_char_count > 1)
        {
            // Full ellipsis size without free spacing after it.
            const float spacing_between_dots = 1.0f * (draw_list->_Data->FontSize / font->FontSize);
            ellipsis_glyph_width = glyph->X1 - glyph->X0 + spacing_between_dots;
            ellipsis_total_width = ellipsis_glyph_width * (float)ellipsis_char_count - spacing_between_dots;
        }

        // We can now claim the space between pos_max.x and ellipsis_max.x
        const float text_avail_width = ImMax((ImMax(pos_max.x, ellipsis_max_x) - ellipsis_total_width) - pos_min.x, 1.0f);
        float text_size_clipped_x = font->CalcTextSizeA(font_size, text_avail_width, 0.0f, text, text_end_full, &text_end_ellipsis).x;
        if (text == text_end_ellipsis && text_end_ellipsis < text_end_full)
        {
            // Always display at least 1 character if there's no room for character + ellipsis
            text_end_ellipsis = text + ImTextCountUtf8BytesFromChar(text, text_end_full);
            text_size_clipped_x = font->CalcTextSizeA(font_size, FLT_MAX, 0.0f, text, text_end_ellipsis).x;
        }
        while (text_end_ellipsis > text && ImCharIsBlankA(text_end_ellipsis[-1]))
        {
            // Trim trailing space before ellipsis (FIXME: Supporting non-ascii blanks would be nice, for this we need a function to backtrack in UTF-8 text)
            text_end_ellipsis--;
            text_size_clipped_x -= font->CalcTextSizeA(font_size, FLT_MAX, 0.0f, text_end_ellipsis, text_end_ellipsis + 1).x; // Ascii blanks are always 1 byte
        }

        // Render text, render ellipsis
        RenderTextClippedEx(draw_list, pos_min, ImVec2(clip_max_x, pos_max.y), text, text_end_ellipsis, &text_size, ImVec2(0.0f, 0.0f));
        float ellipsis_x = pos_min.x + text_size_clipped_x;
        if (ellipsis_x + ellipsis_total_width <= ellipsis_max_x)
            for (int i = 0; i < ellipsis_char_count; i++)
            {
                font->RenderChar(draw_list, font_size, ImVec2(ellipsis_x, pos_min.y), GetColorU32(ImGuiCol_Text), ellipsis_char);
                ellipsis_x += ellipsis_glyph_width;
            }
    }
    else
    {
        RenderTextClippedEx(draw_list, pos_min, ImVec2(clip_max_x, pos_max.y), text, text_end_full, &text_size, ImVec2(0.0f, 0.0f));
    }

    if (g.LogEnabled)
        LogRenderedText(&pos_min, text, text_end_full);
}

// Render a rectangle shaped with optional rounding and borders
void ImGui::RenderFrame(ImVec2 p_min, ImVec2 p_max, ImU32 fill_col, bool border, float rounding)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;
    window->DrawList->AddRectFilled(p_min, p_max, fill_col, rounding);
    const float border_size = g.Style.FrameBorderSize;
    if (border && border_size > 0.0f)
    {
        window->DrawList->AddRect(p_min + ImVec2(1, 1), p_max + ImVec2(1, 1), GetColorU32(ImGuiCol_BorderShadow), rounding, 0, border_size);
        window->DrawList->AddRect(p_min, p_max, GetColorU32(ImGuiCol_Border), rounding, 0, border_size);
    }
}

void ImGui::RenderFrameBorder(ImVec2 p_min, ImVec2 p_max, float rounding)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;
    const float border_size = g.Style.FrameBorderSize;
    if (border_size > 0.0f)
    {
        window->DrawList->AddRect(p_min + ImVec2(1, 1), p_max + ImVec2(1, 1), GetColorU32(ImGuiCol_BorderShadow), rounding, 0, border_size);
        window->DrawList->AddRect(p_min, p_max, GetColorU32(ImGuiCol_Border), rounding, 0, border_size);
    }
}

void ImGui::RenderNavHighlight(const ImRect& bb, ImGuiID id, ImGuiNavHighlightFlags flags)
{
    ImGuiContext& g = *GImGui;
    if (id != g.NavId)
        return;
    if (g.NavDisableHighlight && !(flags & ImGuiNavHighlightFlags_AlwaysDraw))
        return;
    ImGuiWindow* window = g.CurrentWindow;
    if (window->DC.NavHideHighlightOneFrame)
        return;

    float rounding = (flags & ImGuiNavHighlightFlags_NoRounding) ? 0.0f : g.Style.FrameRounding;
    ImRect display_rect = bb;
    display_rect.ClipWith(window->ClipRect);
    if (flags & ImGuiNavHighlightFlags_TypeDefault)
    {
        const float THICKNESS = 2.0f;
        const float DISTANCE = 3.0f + THICKNESS * 0.5f;
        display_rect.Expand(ImVec2(DISTANCE, DISTANCE));
        bool fully_visible = window->ClipRect.Contains(display_rect);
        if (!fully_visible)
            window->DrawList->PushClipRect(display_rect.Min, display_rect.Max);
        window->DrawList->AddRect(display_rect.Min + ImVec2(THICKNESS * 0.5f, THICKNESS * 0.5f), display_rect.Max - ImVec2(THICKNESS * 0.5f, THICKNESS * 0.5f), GetColorU32(ImGuiCol_NavHighlight), rounding, 0, THICKNESS);
        if (!fully_visible)
            window->DrawList->PopClipRect();
    }
    if (flags & ImGuiNavHighlightFlags_TypeThin)
    {
        window->DrawList->AddRect(display_rect.Min, display_rect.Max, GetColorU32(ImGuiCol_NavHighlight), rounding, 0, 1.0f);
    }
}

void ImGui::RenderMouseCursor(ImVec2 base_pos, float base_scale, ImGuiMouseCursor mouse_cursor, ImU32 col_fill, ImU32 col_border, ImU32 col_shadow)
{
    ImGuiContext& g = *GImGui;
    IM_ASSERT(mouse_cursor > ImGuiMouseCursor_None && mouse_cursor < ImGuiMouseCursor_COUNT);
    for (int n = 0; n < g.Viewports.Size; n++)
    {
        ImGuiViewportP* viewport = g.Viewports[n];
        ImDrawList* draw_list = GetForegroundDrawList(viewport);
        ImFontAtlas* font_atlas = draw_list->_Data->Font->ContainerAtlas;
        ImVec2 offset, size, uv[4];
        if (font_atlas->GetMouseCursorTexData(mouse_cursor, &offset, &size, &uv[0], &uv[2]))
        {
            const ImVec2 pos = base_pos - offset;
            const float scale = base_scale;
            ImTextureID tex_id = font_atlas->TexID;
            draw_list->PushTextureID(tex_id);
            draw_list->AddImage(tex_id, pos + ImVec2(1, 0) * scale, pos + (ImVec2(1, 0) + size) * scale, uv[2], uv[3], col_shadow);
            draw_list->AddImage(tex_id, pos + ImVec2(2, 0) * scale, pos + (ImVec2(2, 0) + size) * scale, uv[2], uv[3], col_shadow);
            draw_list->AddImage(tex_id, pos,                        pos + size * scale,                  uv[2], uv[3], col_border);
            draw_list->AddImage(tex_id, pos,                        pos + size * scale,                  uv[0], uv[1], col_fill);
            draw_list->PopTextureID();
        }
    }
}


//-----------------------------------------------------------------------------
// [SECTION] MAIN CODE (most of the code! lots of stuff, needs tidying up!)
//-----------------------------------------------------------------------------

// ImGuiWindow is mostly a dumb struct. It merely has a constructor and a few helper methods
ImGuiWindow::ImGuiWindow(ImGuiContext* context, const char* name) : DrawListInst(NULL)
{
    memset(this, 0, sizeof(*this));
    Name = ImStrdup(name);
    NameBufLen = (int)strlen(name) + 1;
    ID = ImHashStr(name);
    IDStack.push_back(ID);
    MoveId = GetID("#MOVE");
    ScrollTarget = ImVec2(FLT_MAX, FLT_MAX);
    ScrollTargetCenterRatio = ImVec2(0.5f, 0.5f);
    AutoFitFramesX = AutoFitFramesY = -1;
    AutoPosLastDirection = ImGuiDir_None;
    SetWindowPosAllowFlags = SetWindowSizeAllowFlags = SetWindowCollapsedAllowFlags = ImGuiCond_Always | ImGuiCond_Once | ImGuiCond_FirstUseEver | ImGuiCond_Appearing;
    SetWindowPosVal = SetWindowPosPivot = ImVec2(FLT_MAX, FLT_MAX);
    LastFrameActive = -1;
    LastTimeActive = -1.0f;
    FontWindowScale = 1.0f;
    SettingsOffset = -1;
    DrawList = &DrawListInst;
    DrawList->_Data = &context->DrawListSharedData;
    DrawList->_OwnerName = Name;
}

ImGuiWindow::~ImGuiWindow()
{
    IM_ASSERT(DrawList == &DrawListInst);
    IM_DELETE(Name);
    ColumnsStorage.clear_destruct();
}

ImGuiID ImGuiWindow::GetID(const char* str, const char* str_end)
{
    ImGuiID seed = IDStack.back();
    ImGuiID id = ImHashStr(str, str_end ? (str_end - str) : 0, seed);
    ImGuiContext& g = *GImGui;
    if (g.DebugHookIdInfo == id)
        ImGui::DebugHookIdInfo(id, ImGuiDataType_String, str, str_end);
    return id;
}

ImGuiID ImGuiWindow::GetID(const void* ptr)
{
    ImGuiID seed = IDStack.back();
    ImGuiID id = ImHashData(&ptr, sizeof(void*), seed);
    ImGuiContext& g = *GImGui;
    if (g.DebugHookIdInfo == id)
        ImGui::DebugHookIdInfo(id, ImGuiDataType_Pointer, ptr, NULL);
    return id;
}

ImGuiID ImGuiWindow::GetID(int n)
{
    ImGuiID seed = IDStack.back();
    ImGuiID id = ImHashData(&n, sizeof(n), seed);
    ImGuiContext& g = *GImGui;
    if (g.DebugHookIdInfo == id)
        ImGui::DebugHookIdInfo(id, ImGuiDataType_S32, (void*)(intptr_t)n, NULL);
    return id;
}

// This is only used in rare/specific situations to manufacture an ID out of nowhere.
ImGuiID ImGuiWindow::GetIDFromRectangle(const ImRect& r_abs)
{
    ImGuiID seed = IDStack.back();
    ImRect r_rel = ImGui::WindowRectAbsToRel(this, r_abs);
    ImGuiID id = ImHashData(&r_rel, sizeof(r_rel), seed);
    return id;
}

static void SetCurrentWindow(ImGuiWindow* window)
{
    ImGuiContext& g = *GImGui;
    g.CurrentWindow = window;
    g.CurrentTable = window && window->DC.CurrentTableIdx != -1 ? g.Tables.GetByIndex(window->DC.CurrentTableIdx) : NULL;
    if (window)
        g.FontSize = g.DrawListSharedData.FontSize = window->CalcFontSize();
}

void ImGui::GcCompactTransientMiscBuffers()
{
    ImGuiContext& g = *GImGui;
    g.ItemFlagsStack.clear();
    g.GroupStack.clear();
    TableGcCompactSettings();
}

// Free up/compact internal window buffers, we can use this when a window becomes unused.
// Not freed:
// - ImGuiWindow, ImGuiWindowSettings, Name, StateStorage, ColumnsStorage (may hold useful data)
// This should have no noticeable visual effect. When the window reappear however, expect new allocation/buffer growth/copy cost.
void ImGui::GcCompactTransientWindowBuffers(ImGuiWindow* window)
{
    window->MemoryCompacted = true;
    window->MemoryDrawListIdxCapacity = window->DrawList->IdxBuffer.Capacity;
    window->MemoryDrawListVtxCapacity = window->DrawList->VtxBuffer.Capacity;
    window->IDStack.clear();
    window->DrawList->_ClearFreeMemory();
    window->DC.ChildWindows.clear();
    window->DC.ItemWidthStack.clear();
    window->DC.TextWrapPosStack.clear();
}

void ImGui::GcAwakeTransientWindowBuffers(ImGuiWindow* window)
{
    // We stored capacity of the ImDrawList buffer to reduce growth-caused allocation/copy when awakening.
    // The other buffers tends to amortize much faster.
    window->MemoryCompacted = false;
    window->DrawList->IdxBuffer.reserve(window->MemoryDrawListIdxCapacity);
    window->DrawList->VtxBuffer.reserve(window->MemoryDrawListVtxCapacity);
    window->MemoryDrawListIdxCapacity = window->MemoryDrawListVtxCapacity = 0;
}

void ImGui::SetActiveID(ImGuiID id, ImGuiWindow* window)
{
    ImGuiContext& g = *GImGui;
    g.ActiveIdIsJustActivated = (g.ActiveId != id);
    if (g.ActiveIdIsJustActivated)
    {
        g.ActiveIdTimer = 0.0f;
        g.ActiveIdHasBeenPressedBefore = false;
        g.ActiveIdHasBeenEditedBefore = false;
        g.ActiveIdMouseButton = -1;
        if (id != 0)
        {
            g.LastActiveId = id;
            g.LastActiveIdTimer = 0.0f;
        }
    }
    g.ActiveId = id;
    g.ActiveIdAllowOverlap = false;
    g.ActiveIdNoClearOnFocusLoss = false;
    g.ActiveIdWindow = window;
    g.ActiveIdHasBeenEditedThisFrame = false;
    if (id)
    {
        g.ActiveIdIsAlive = id;
        g.ActiveIdSource = (g.NavActivateId == id || g.NavActivateInputId == id || g.NavJustMovedToId == id) ? (ImGuiInputSource)ImGuiInputSource_Nav : ImGuiInputSource_Mouse;
    }

    // Clear declaration of inputs claimed by the widget
    // (Please note that this is WIP and not all keys/inputs are thoroughly declared by all widgets yet)
    g.ActiveIdUsingMouseWheel = false;
    g.ActiveIdUsingNavDirMask = 0x00;
    g.ActiveIdUsingNavInputMask = 0x00;
    g.ActiveIdUsingKeyInputMask.ClearAllBits();
}

void ImGui::ClearActiveID()
{
    SetActiveID(0, NULL); // g.ActiveId = 0;
}

void ImGui::SetHoveredID(ImGuiID id)
{
    ImGuiContext& g = *GImGui;
    g.HoveredId = id;
    g.HoveredIdAllowOverlap = false;
    g.HoveredIdUsingMouseWheel = false;
    if (id != 0 && g.HoveredIdPreviousFrame != id)
        g.HoveredIdTimer = g.HoveredIdNotActiveTimer = 0.0f;
}

ImGuiID ImGui::GetHoveredID()
{
    ImGuiContext& g = *GImGui;
    return g.HoveredId ? g.HoveredId : g.HoveredIdPreviousFrame;
}

// This is called by ItemAdd().
// Code not using ItemAdd() may need to call this manually otherwise ActiveId will be cleared. In IMGUI_VERSION_NUM < 18717 this was called by GetID().
void ImGui::KeepAliveID(ImGuiID id)
{
    ImGuiContext& g = *GImGui;
    if (g.ActiveId == id)
        g.ActiveIdIsAlive = id;
    if (g.ActiveIdPreviousFrame == id)
        g.ActiveIdPreviousFrameIsAlive = true;
}

void ImGui::MarkItemEdited(ImGuiID id)
{
    // This marking is solely to be able to provide info for IsItemDeactivatedAfterEdit().
    // ActiveId might have been released by the time we call this (as in the typical press/release button behavior) but still need need to fill the data.
    ImGuiContext& g = *GImGui;
    IM_ASSERT(g.ActiveId == id || g.ActiveId == 0 || g.DragDropActive);
    IM_UNUSED(id); // Avoid unused variable warnings when asserts are compiled out.
    //IM_ASSERT(g.CurrentWindow->DC.LastItemId == id);
    g.ActiveIdHasBeenEditedThisFrame = true;
    g.ActiveIdHasBeenEditedBefore = true;
    g.LastItemData.StatusFlags |= ImGuiItemStatusFlags_Edited;
}

static inline bool IsWindowContentHoverable(ImGuiWindow* window, ImGuiHoveredFlags flags)
{
    // An active popup disable hovering on other windows (apart from its own children)
    // FIXME-OPT: This could be cached/stored within the window.
    ImGuiContext& g = *GImGui;
    if (g.NavWindow)
        if (ImGuiWindow* focused_root_window = g.NavWindow->RootWindow)
            if (focused_root_window->WasActive && focused_root_window != window->RootWindow)
            {
                // For the purpose of those flags we differentiate "standard popup" from "modal popup"
                // NB: The order of those two tests is important because Modal windows are also Popups.
                if (focused_root_window->Flags & ImGuiWindowFlags_Modal)
                    return false;
                if ((focused_root_window->Flags & ImGuiWindowFlags_Popup) && !(flags & ImGuiHoveredFlags_AllowWhenBlockedByPopup))
                    return false;
            }
    return true;
}

// This is roughly matching the behavior of internal-facing ItemHoverable()
// - we allow hovering to be true when ActiveId==window->MoveID, so that clicking on non-interactive items such as a Text() item still returns true with IsItemHovered()
// - this should work even for non-interactive items that have no ID, so we cannot use LastItemId
bool ImGui::IsItemHovered(ImGuiHoveredFlags flags)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;
    if (g.NavDisableMouseHover && !g.NavDisableHighlight && !(flags & ImGuiHoveredFlags_NoNavOverride))
    {
        if ((g.LastItemData.InFlags & ImGuiItemFlags_Disabled) && !(flags & ImGuiHoveredFlags_AllowWhenDisabled))
            return false;
        if (!IsItemFocused())
            return false;
    }
    else
    {
        // Test for bounding box overlap, as updated as ItemAdd()
        ImGuiItemStatusFlags status_flags = g.LastItemData.StatusFlags;
        if (!(status_flags & ImGuiItemStatusFlags_HoveredRect))
            return false;
        IM_ASSERT((flags & (ImGuiHoveredFlags_AnyWindow | ImGuiHoveredFlags_RootWindow | ImGuiHoveredFlags_ChildWindows | ImGuiHoveredFlags_NoPopupHierarchy)) == 0);   // Flags not supported by this function

        // Test if we are hovering the right window (our window could be behind another window)
        // [2021/03/02] Reworked / reverted the revert, finally. Note we want e.g. BeginGroup/ItemAdd/EndGroup to work as well. (#3851)
        // [2017/10/16] Reverted commit 344d48be3 and testing RootWindow instead. I believe it is correct to NOT test for RootWindow but this leaves us unable
        // to use IsItemHovered() after EndChild() itself. Until a solution is found I believe reverting to the test from 2017/09/27 is safe since this was
        // the test that has been running for a long while.
        if (g.HoveredWindow != window && (status_flags & ImGuiItemStatusFlags_HoveredWindow) == 0)
            if ((flags & ImGuiHoveredFlags_AllowWhenOverlapped) == 0)
                return false;

        // Test if another item is active (e.g. being dragged)
        if ((flags & ImGuiHoveredFlags_AllowWhenBlockedByActiveItem) == 0)
            if (g.ActiveId != 0 && g.ActiveId != g.LastItemData.ID && !g.ActiveIdAllowOverlap && g.ActiveId != window->MoveId)
                return false;

        // Test if interactions on this window are blocked by an active popup or modal.
        // The ImGuiHoveredFlags_AllowWhenBlockedByPopup flag will be tested here.
        if (!IsWindowContentHoverable(window, flags))
            return false;

        // Test if the item is disabled
        if ((g.LastItemData.InFlags & ImGuiItemFlags_Disabled) && !(flags & ImGuiHoveredFlags_AllowWhenDisabled))
            return false;

        // Special handling for calling after Begin() which represent the title bar or tab.
        // When the window is collapsed (SkipItems==true) that last item will never be overwritten so we need to detect the case.
        if (g.LastItemData.ID == window->MoveId && window->WriteAccessed)
            return false;
    }

    return true;
}

// Internal facing ItemHoverable() used when submitting widgets. Differs slightly from IsItemHovered().
bool ImGui::ItemHoverable(const ImRect& bb, ImGuiID id)
{
    ImGuiContext& g = *GImGui;
    if (g.HoveredId != 0 && g.HoveredId != id && !g.HoveredIdAllowOverlap)
        return false;

    ImGuiWindow* window = g.CurrentWindow;
    if (g.HoveredWindow != window)
        return false;
    if (g.ActiveId != 0 && g.ActiveId != id && !g.ActiveIdAllowOverlap)
        return false;
    if (!IsMouseHoveringRect(bb.Min, bb.Max))
        return false;
    if (!IsWindowContentHoverable(window, ImGuiHoveredFlags_None))
    {
        g.HoveredIdDisabled = true;
        return false;
    }

    // We exceptionally allow this function to be called with id==0 to allow using it for easy high-level
    // hover test in widgets code. We could also decide to split this function is two.
    if (id != 0)
        SetHoveredID(id);

    // When disabled we'll return false but still set HoveredId
    ImGuiItemFlags item_flags = (g.LastItemData.ID == id ? g.LastItemData.InFlags : g.CurrentItemFlags);
    if (item_flags & ImGuiItemFlags_Disabled)
    {
        // Release active id if turning disabled
        if (g.ActiveId == id)
            ClearActiveID();
        g.HoveredIdDisabled = true;
        return false;
    }

    if (id != 0)
    {
        // [DEBUG] Item Picker tool!
        // We perform the check here because SetHoveredID() is not frequently called (1~ time a frame), making
        // the cost of this tool near-zero. We can get slightly better call-stack and support picking non-hovered
        // items if we perform the test in ItemAdd(), but that would incur a small runtime cost.
        // #define IMGUI_DEBUG_TOOL_ITEM_PICKER_EX in imconfig.h if you want this check to also be performed in ItemAdd().
        if (g.DebugItemPickerActive && g.HoveredIdPreviousFrame == id)
            GetForegroundDrawList()->AddRect(bb.Min, bb.Max, IM_COL32(255, 255, 0, 255));
        if (g.DebugItemPickerBreakId == id)
            IM_DEBUG_BREAK();
    }

    if (g.NavDisableMouseHover)
        return false;

    return true;
}

bool ImGui::IsClippedEx(const ImRect& bb, ImGuiID id)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;
    if (!bb.Overlaps(window->ClipRect))
        if (id == 0 || (id != g.ActiveId && id != g.NavId))
            if (!g.LogEnabled)
                return true;
    return false;
}

// This is also inlined in ItemAdd()
// Note: if ImGuiItemStatusFlags_HasDisplayRect is set, user needs to set window->DC.LastItemDisplayRect!
void ImGui::SetLastItemData(ImGuiID item_id, ImGuiItemFlags in_flags, ImGuiItemStatusFlags item_flags, const ImRect& item_rect)
{
    ImGuiContext& g = *GImGui;
    g.LastItemData.ID = item_id;
    g.LastItemData.InFlags = in_flags;
    g.LastItemData.StatusFlags = item_flags;
    g.LastItemData.Rect = item_rect;
}

float ImGui::CalcWrapWidthForPos(const ImVec2& pos, float wrap_pos_x)
{
    if (wrap_pos_x < 0.0f)
        return 0.0f;

    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;
    if (wrap_pos_x == 0.0f)
    {
        // We could decide to setup a default wrapping max point for auto-resizing windows,
        // or have auto-wrap (with unspecified wrapping pos) behave as a ContentSize extending function?
        //if (window->Hidden && (window->Flags & ImGuiWindowFlags_AlwaysAutoResize))
        //    wrap_pos_x = ImMax(window->WorkRect.Min.x + g.FontSize * 10.0f, window->WorkRect.Max.x);
        //else
        wrap_pos_x = window->WorkRect.Max.x;
    }
    else if (wrap_pos_x > 0.0f)
    {
        wrap_pos_x += window->Pos.x - window->Scroll.x; // wrap_pos_x is provided is window local space
    }

    return ImMax(wrap_pos_x - pos.x, 1.0f);
}

// IM_ALLOC() == ImGui::MemAlloc()
void* ImGui::MemAlloc(size_t size)
{
    if (ImGuiContext* ctx = GImGui)
        ctx->IO.MetricsActiveAllocations++;
    return (*GImAllocatorAllocFunc)(size, GImAllocatorUserData);
}

// IM_FREE() == ImGui::MemFree()
void ImGui::MemFree(void* ptr)
{
    if (ptr)
        if (ImGuiContext* ctx = GImGui)
            ctx->IO.MetricsActiveAllocations--;
    return (*GImAllocatorFreeFunc)(ptr, GImAllocatorUserData);
}

const char* ImGui::GetClipboardText()
{
    ImGuiContext& g = *GImGui;
    return g.IO.GetClipboardTextFn ? g.IO.GetClipboardTextFn(g.IO.ClipboardUserData) : "";
}

void ImGui::SetClipboardText(const char* text)
{
    ImGuiContext& g = *GImGui;
    if (g.IO.SetClipboardTextFn)
        g.IO.SetClipboardTextFn(g.IO.ClipboardUserData, text);
}

const char* ImGui::GetVersion()
{
    return IMGUI_VERSION;
}

// Internal state access - if you want to share Dear ImGui state between modules (e.g. DLL) or allocate it yourself
// Note that we still point to some static data and members (such as GFontAtlas), so the state instance you end up using will point to the static data within its module
ImGuiContext* ImGui::GetCurrentContext()
{
    return GImGui;
}

void ImGui::SetCurrentContext(ImGuiContext* ctx)
{
#ifdef IMGUI_SET_CURRENT_CONTEXT_FUNC
    IMGUI_SET_CURRENT_CONTEXT_FUNC(ctx); // For custom thread-based hackery you may want to have control over this.
#else
    GImGui = ctx;
#endif
}

void ImGui::SetAllocatorFunctions(ImGuiMemAllocFunc alloc_func, ImGuiMemFreeFunc free_func, void* user_data)
{
    GImAllocatorAllocFunc = alloc_func;
    GImAllocatorFreeFunc = free_func;
    GImAllocatorUserData = user_data;
}

// This is provided to facilitate copying allocators from one static/DLL boundary to another (e.g. retrieve default allocator of your executable address space)
void ImGui::GetAllocatorFunctions(ImGuiMemAllocFunc* p_alloc_func, ImGuiMemFreeFunc* p_free_func, void** p_user_data)
{
    *p_alloc_func = GImAllocatorAllocFunc;
    *p_free_func = GImAllocatorFreeFunc;
    *p_user_data = GImAllocatorUserData;
}

ImGuiContext* ImGui::CreateContext(ImFontAtlas* shared_font_atlas)
{
    ImGuiContext* prev_ctx = GetCurrentContext();
    ImGuiContext* ctx = IM_NEW(ImGuiContext)(shared_font_atlas);
    SetCurrentContext(ctx);
    Initialize();
    if (prev_ctx != NULL)
        SetCurrentContext(prev_ctx); // Restore previous context if any, else keep new one.
    return ctx;
}

void ImGui::DestroyContext(ImGuiContext* ctx)
{
    ImGuiContext* prev_ctx = GetCurrentContext();
    if (ctx == NULL) //-V1051
        ctx = prev_ctx;
    SetCurrentContext(ctx);
    Shutdown();
    SetCurrentContext((prev_ctx != ctx) ? prev_ctx : NULL);
    IM_DELETE(ctx);
}

// No specific ordering/dependency support, will see as needed
ImGuiID ImGui::AddContextHook(ImGuiContext* ctx, const ImGuiContextHook* hook)
{
    ImGuiContext& g = *ctx;
    IM_ASSERT(hook->Callback != NULL && hook->HookId == 0 && hook->Type != ImGuiContextHookType_PendingRemoval_);
    g.Hooks.push_back(*hook);
    g.Hooks.back().HookId = ++g.HookIdNext;
    return g.HookIdNext;
}

// Deferred removal, avoiding issue with changing vector while iterating it
void ImGui::RemoveContextHook(ImGuiContext* ctx, ImGuiID hook_id)
{
    ImGuiContext& g = *ctx;
    IM_ASSERT(hook_id != 0);
    for (int n = 0; n < g.Hooks.Size; n++)
        if (g.Hooks[n].HookId == hook_id)
            g.Hooks[n].Type = ImGuiContextHookType_PendingRemoval_;
}

// Call context hooks (used by e.g. test engine)
// We assume a small number of hooks so all stored in same array
void ImGui::CallContextHooks(ImGuiContext* ctx, ImGuiContextHookType hook_type)
{
    ImGuiContext& g = *ctx;
    for (int n = 0; n < g.Hooks.Size; n++)
        if (g.Hooks[n].Type == hook_type)
            g.Hooks[n].Callback(&g, &g.Hooks[n]);
}

ImGuiIO& ImGui::GetIO()
{
    IM_ASSERT(GImGui != NULL && "No current context. Did you call ImGui::CreateContext() and ImGui::SetCurrentContext() ?");
    return GImGui->IO;
}

// Pass this to your backend rendering function! Valid after Render() and until the next call to NewFrame()
ImDrawData* ImGui::GetDrawData()
{
    ImGuiContext& g = *GImGui;
    ImGuiViewportP* viewport = g.Viewports[0];
    return viewport->DrawDataP.Valid ? &viewport->DrawDataP : NULL;
}

double ImGui::GetTime()
{
    return GImGui->Time;
}

int ImGui::GetFrameCount()
{
    return GImGui->FrameCount;
}

static ImDrawList* GetViewportDrawList(ImGuiViewportP* viewport, size_t drawlist_no, const char* drawlist_name)
{
    // Create the draw list on demand, because they are not frequently used for all viewports
    ImGuiContext& g = *GImGui;
    IM_ASSERT(drawlist_no < IM_ARRAYSIZE(viewport->DrawLists));
    ImDrawList* draw_list = viewport->DrawLists[drawlist_no];
    if (draw_list == NULL)
    {
        draw_list = IM_NEW(ImDrawList)(&g.DrawListSharedData);
        draw_list->_OwnerName = drawlist_name;
        viewport->DrawLists[drawlist_no] = draw_list;
    }

    // Our ImDrawList system requires that there is always a command
    if (viewport->DrawListsLastFrame[drawlist_no] != g.FrameCount)
    {
        draw_list->_ResetForNewFrame();
        draw_list->PushTextureID(g.IO.Fonts->TexID);
        draw_list->PushClipRect(viewport->Pos, viewport->Pos + viewport->Size, false);
        viewport->DrawListsLastFrame[drawlist_no] = g.FrameCount;
    }
    return draw_list;
}

ImDrawList* ImGui::GetBackgroundDrawList(ImGuiViewport* viewport)
{
    return GetViewportDrawList((ImGuiViewportP*)viewport, 0, "##Background");
}

ImDrawList* ImGui::GetBackgroundDrawList()
{
    ImGuiContext& g = *GImGui;
    return GetBackgroundDrawList(g.Viewports[0]);
}

ImDrawList* ImGui::GetForegroundDrawList(ImGuiViewport* viewport)
{
    return GetViewportDrawList((ImGuiViewportP*)viewport, 1, "##Foreground");
}

ImDrawList* ImGui::GetForegroundDrawList()
{
    ImGuiContext& g = *GImGui;
    return GetForegroundDrawList(g.Viewports[0]);
}

ImDrawListSharedData* ImGui::GetDrawListSharedData()
{
    return &GImGui->DrawListSharedData;
}

void ImGui::StartMouseMovingWindow(ImGuiWindow* window)
{
    // Set ActiveId even if the _NoMove flag is set. Without it, dragging away from a window with _NoMove would activate hover on other windows.
    // We _also_ call this when clicking in a window empty space when io.ConfigWindowsMoveFromTitleBarOnly is set, but clear g.MovingWindow afterward.
    // This is because we want ActiveId to be set even when the window is not permitted to move.
    ImGuiContext& g = *GImGui;
    FocusWindow(window);
    SetActiveID(window->MoveId, window);
    g.NavDisableHighlight = true;
    g.ActiveIdClickOffset = g.IO.MouseClickedPos[0] - window->RootWindow->Pos;
    g.ActiveIdNoClearOnFocusLoss = true;
    SetActiveIdUsingNavAndKeys();

    bool can_move_window = true;
    if ((window->Flags & ImGuiWindowFlags_NoMove) || (window->RootWindow->Flags & ImGuiWindowFlags_NoMove))
        can_move_window = false;
    if (can_move_window)
        g.MovingWindow = window;
}

// Handle mouse moving window
// Note: moving window with the navigation keys (Square + d-pad / CTRL+TAB + Arrows) are processed in NavUpdateWindowing()
// FIXME: We don't have strong guarantee that g.MovingWindow stay synched with g.ActiveId == g.MovingWindow->MoveId.
// This is currently enforced by the fact that BeginDragDropSource() is setting all g.ActiveIdUsingXXXX flags to inhibit navigation inputs,
// but if we should more thoroughly test cases where g.ActiveId or g.MovingWindow gets changed and not the other.
void ImGui::UpdateMouseMovingWindowNewFrame()
{
    ImGuiContext& g = *GImGui;
    if (g.MovingWindow != NULL)
    {
        // We actually want to move the root window. g.MovingWindow == window we clicked on (could be a child window).
        // We track it to preserve Focus and so that generally ActiveIdWindow == MovingWindow and ActiveId == MovingWindow->MoveId for consistency.
        KeepAliveID(g.ActiveId);
        IM_ASSERT(g.MovingWindow && g.MovingWindow->RootWindow);
        ImGuiWindow* moving_window = g.MovingWindow->RootWindow;
        if (g.IO.MouseDown[0] && IsMousePosValid(&g.IO.MousePos))
        {
            ImVec2 pos = g.IO.MousePos - g.ActiveIdClickOffset;
            if (moving_window->Pos.x != pos.x || moving_window->Pos.y != pos.y)
            {
                MarkIniSettingsDirty(moving_window);
                SetWindowPos(moving_window, pos, ImGuiCond_Always);
            }
            FocusWindow(g.MovingWindow);
        }
        else
        {
            g.MovingWindow = NULL;
            ClearActiveID();
        }
    }
    else
    {
        // When clicking/dragging from a window that has the _NoMove flag, we still set the ActiveId in order to prevent hovering others.
        if (g.ActiveIdWindow && g.ActiveIdWindow->MoveId == g.ActiveId)
        {
            KeepAliveID(g.ActiveId);
            if (!g.IO.MouseDown[0])
                ClearActiveID();
        }
    }
}

// Initiate moving window when clicking on empty space or title bar.
// Handle left-click and right-click focus.
void ImGui::UpdateMouseMovingWindowEndFrame()
{
    ImGuiContext& g = *GImGui;
    if (g.ActiveId != 0 || g.HoveredId != 0)
        return;

    // Unless we just made a window/popup appear
    if (g.NavWindow && g.NavWindow->Appearing)
        return;

    // Click on empty space to focus window and start moving
    // (after we're done with all our widgets)
    if (g.IO.MouseClicked[0])
    {
        // Handle the edge case of a popup being closed while clicking in its empty space.
        // If we try to focus it, FocusWindow() > ClosePopupsOverWindow() will accidentally close any parent popups because they are not linked together any more.
        ImGuiWindow* root_window = g.HoveredWindow ? g.HoveredWindow->RootWindow : NULL;
        const bool is_closed_popup = root_window && (root_window->Flags & ImGuiWindowFlags_Popup) && !IsPopupOpen(root_window->PopupId, ImGuiPopupFlags_AnyPopupLevel);

        if (root_window != NULL && !is_closed_popup)
        {
            StartMouseMovingWindow(g.HoveredWindow); //-V595

            // Cancel moving if clicked outside of title bar
            if (g.IO.ConfigWindowsMoveFromTitleBarOnly && !(root_window->Flags & ImGuiWindowFlags_NoTitleBar))
                if (!root_window->TitleBarRect().Contains(g.IO.MouseClickedPos[0]))
                    g.MovingWindow = NULL;

            // Cancel moving if clicked over an item which was disabled or inhibited by popups (note that we know HoveredId == 0 already)
            if (g.HoveredIdDisabled)
                g.MovingWindow = NULL;
        }
        else if (root_window == NULL && g.NavWindow != NULL && GetTopMostPopupModal() == NULL)
        {
            // Clicking on void disable focus
            FocusWindow(NULL);
        }
    }

    // With right mouse button we close popups without changing focus based on where the mouse is aimed
    // Instead, focus will be restored to the window under the bottom-most closed popup.
    // (The left mouse button path calls FocusWindow on the hovered window, which will lead NewFrame->ClosePopupsOverWindow to trigger)
    if (g.IO.MouseClicked[1])
    {
        // Find the top-most window between HoveredWindow and the top-most Modal Window.
        // This is where we can trim the popup stack.
        ImGuiWindow* modal = GetTopMostPopupModal();
        bool hovered_window_above_modal = g.HoveredWindow && (modal == NULL || IsWindowAbove(g.HoveredWindow, modal));
        ClosePopupsOverWindow(hovered_window_above_modal ? g.HoveredWindow : modal, true);
    }
}

static bool IsWindowActiveAndVisible(ImGuiWindow* window)
{
    return (window->Active) && (!window->Hidden);
}

static void ImGui::UpdateKeyboardInputs()
{
    ImGuiContext& g = *GImGui;
    ImGuiIO& io = g.IO;

    // Import legacy keys or verify they are not used
#ifndef IMGUI_DISABLE_OBSOLETE_KEYIO
    if (io.BackendUsingLegacyKeyArrays == 0)
    {
        // Backend used new io.AddKeyEvent() API: Good! Verify that old arrays are never written to externally.
        for (int n = 0; n < ImGuiKey_LegacyNativeKey_END; n++)
            IM_ASSERT((io.KeysDown[n] == false || IsKeyDown(n)) && "Backend needs to either only use io.AddKeyEvent(), either only fill legacy io.KeysDown[] + io.KeyMap[]. Not both!");
    }
    else
    {
        if (g.FrameCount == 0)
            for (int n = ImGuiKey_LegacyNativeKey_BEGIN; n < ImGuiKey_LegacyNativeKey_END; n++)
                IM_ASSERT(g.IO.KeyMap[n] == -1 && "Backend is not allowed to write to io.KeyMap[0..511]!");

        // Build reverse KeyMap (Named -> Legacy)
        for (int n = ImGuiKey_NamedKey_BEGIN; n < ImGuiKey_NamedKey_END; n++)
            if (io.KeyMap[n] != -1)
            {
                IM_ASSERT(IsLegacyKey((ImGuiKey)io.KeyMap[n]));
                io.KeyMap[io.KeyMap[n]] = n;
            }

        // Import legacy keys into new ones
        for (int n = ImGuiKey_LegacyNativeKey_BEGIN; n < ImGuiKey_LegacyNativeKey_END; n++)
            if (io.KeysDown[n] || io.BackendUsingLegacyKeyArrays == 1)
            {
                const ImGuiKey key = (ImGuiKey)(io.KeyMap[n] != -1 ? io.KeyMap[n] : n);
                IM_ASSERT(io.KeyMap[n] == -1 || IsNamedKey(key));
                io.KeysData[key].Down = io.KeysDown[n];
                if (key != n)
                    io.KeysDown[key] = io.KeysDown[n]; // Allow legacy code using io.KeysDown[GetKeyIndex()] with old backends
                io.BackendUsingLegacyKeyArrays = 1;
            }
        if (io.BackendUsingLegacyKeyArrays == 1)
        {
            io.KeysData[ImGuiKey_ModCtrl].Down = io.KeyCtrl;
            io.KeysData[ImGuiKey_ModShift].Down = io.KeyShift;
            io.KeysData[ImGuiKey_ModAlt].Down = io.KeyAlt;
            io.KeysData[ImGuiKey_ModSuper].Down = io.KeySuper;
        }
    }
#endif

    // Synchronize io.KeyMods with individual modifiers io.KeyXXX bools
    io.KeyMods = GetMergedModFlags();

    // Clear gamepad data if disabled
    if ((io.BackendFlags & ImGuiBackendFlags_HasGamepad) == 0)
        for (int i = ImGuiKey_Gamepad_BEGIN; i < ImGuiKey_Gamepad_END; i++)
        {
            io.KeysData[i - ImGuiKey_KeysData_OFFSET].Down = false;
            io.KeysData[i - ImGuiKey_KeysData_OFFSET].AnalogValue = 0.0f;
        }

    // Update keys
    for (int i = 0; i < IM_ARRAYSIZE(io.KeysData); i++)
    {
        ImGuiKeyData* key_data = &io.KeysData[i];
        key_data->DownDurationPrev = key_data->DownDuration;
        key_data->DownDuration = key_data->Down ? (key_data->DownDuration < 0.0f ? 0.0f : key_data->DownDuration + io.DeltaTime) : -1.0f;
    }
}

static void ImGui::UpdateMouseInputs()
{
    ImGuiContext& g = *GImGui;
    ImGuiIO& io = g.IO;

    // Round mouse position to avoid spreading non-rounded position (e.g. UpdateManualResize doesn't support them well)
    if (IsMousePosValid(&io.MousePos))
        io.MousePos = g.MouseLastValidPos = ImFloorSigned(io.MousePos);

    // If mouse just appeared or disappeared (usually denoted by -FLT_MAX components) we cancel out movement in MouseDelta
    if (IsMousePosValid(&io.MousePos) && IsMousePosValid(&io.MousePosPrev))
        io.MouseDelta = io.MousePos - io.MousePosPrev;
    else
        io.MouseDelta = ImVec2(0.0f, 0.0f);

    // If mouse moved we re-enable mouse hovering in case it was disabled by gamepad/keyboard. In theory should use a >0.0f threshold but would need to reset in everywhere we set this to true.
    if (io.MouseDelta.x != 0.0f || io.MouseDelta.y != 0.0f)
        g.NavDisableMouseHover = false;

    io.MousePosPrev = io.MousePos;
    for (int i = 0; i < IM_ARRAYSIZE(io.MouseDown); i++)
    {
        io.MouseClicked[i] = io.MouseDown[i] && io.MouseDownDuration[i] < 0.0f;
        io.MouseClickedCount[i] = 0; // Will be filled below
        io.MouseReleased[i] = !io.MouseDown[i] && io.MouseDownDuration[i] >= 0.0f;
        io.MouseDownDurationPrev[i] = io.MouseDownDuration[i];
        io.MouseDownDuration[i] = io.MouseDown[i] ? (io.MouseDownDuration[i] < 0.0f ? 0.0f : io.MouseDownDuration[i] + io.DeltaTime) : -1.0f;
        if (io.MouseClicked[i])
        {
            bool is_repeated_click = false;
            if ((float)(g.Time - io.MouseClickedTime[i]) < io.MouseDoubleClickTime)
            {
                ImVec2 delta_from_click_pos = IsMousePosValid(&io.MousePos) ? (io.MousePos - io.MouseClickedPos[i]) : ImVec2(0.0f, 0.0f);
                if (ImLengthSqr(delta_from_click_pos) < io.MouseDoubleClickMaxDist * io.MouseDoubleClickMaxDist)
                    is_repeated_click = true;
            }
            if (is_repeated_click)
                io.MouseClickedLastCount[i]++;
            else
                io.MouseClickedLastCount[i] = 1;
            io.MouseClickedTime[i] = g.Time;
            io.MouseClickedPos[i] = io.MousePos;
            io.MouseClickedCount[i] = io.MouseClickedLastCount[i];
            io.MouseDragMaxDistanceSqr[i] = 0.0f;
        }
        else if (io.MouseDown[i])
        {
            // Maintain the maximum distance we reaching from the initial click position, which is used with dragging threshold
            float delta_sqr_click_pos = IsMousePosValid(&io.MousePos) ? ImLengthSqr(io.MousePos - io.MouseClickedPos[i]) : 0.0f;
            io.MouseDragMaxDistanceSqr[i] = ImMax(io.MouseDragMaxDistanceSqr[i], delta_sqr_click_pos);
        }

        // We provide io.MouseDoubleClicked[] as a legacy service
        io.MouseDoubleClicked[i] = (io.MouseClickedCount[i] == 2);

        // Clicking any mouse button reactivate mouse hovering which may have been deactivated by gamepad/keyboard navigation
        if (io.MouseClicked[i])
            g.NavDisableMouseHover = false;
    }
}

static void StartLockWheelingWindow(ImGuiWindow* window)
{
    ImGuiContext& g = *GImGui;
    if (g.WheelingWindow == window)
        return;
    g.WheelingWindow = window;
    g.WheelingWindowRefMousePos = g.IO.MousePos;
    g.WheelingWindowTimer = WINDOWS_MOUSE_WHEEL_SCROLL_LOCK_TIMER;
}

void ImGui::UpdateMouseWheel()
{
    ImGuiContext& g = *GImGui;

    // Reset the locked window if we move the mouse or after the timer elapses
    if (g.WheelingWindow != NULL)
    {
        g.WheelingWindowTimer -= g.IO.DeltaTime;
        if (IsMousePosValid() && ImLengthSqr(g.IO.MousePos - g.WheelingWindowRefMousePos) > g.IO.MouseDragThreshold * g.IO.MouseDragThreshold)
            g.WheelingWindowTimer = 0.0f;
        if (g.WheelingWindowTimer <= 0.0f)
        {
            g.WheelingWindow = NULL;
            g.WheelingWindowTimer = 0.0f;
        }
    }

    float wheel_x = g.IO.MouseWheelH;
    float wheel_y = g.IO.MouseWheel;
    if (wheel_x == 0.0f && wheel_y == 0.0f)
        return;

    if ((g.ActiveId != 0 && g.ActiveIdUsingMouseWheel) || (g.HoveredIdPreviousFrame != 0 && g.HoveredIdPreviousFrameUsingMouseWheel))
        return;

    ImGuiWindow* window = g.WheelingWindow ? g.WheelingWindow : g.HoveredWindow;
    if (!window || window->Collapsed)
        return;

    // Zoom / Scale window
    // FIXME-OBSOLETE: This is an old feature, it still works but pretty much nobody is using it and may be best redesigned.
    if (wheel_y != 0.0f && g.IO.KeyCtrl && g.IO.FontAllowUserScaling)
    {
        StartLockWheelingWindow(window);
        const float new_font_scale = ImClamp(window->FontWindowScale + g.IO.MouseWheel * 0.10f, 0.50f, 2.50f);
        const float scale = new_font_scale / window->FontWindowScale;
        window->FontWindowScale = new_font_scale;
        if (window == window->RootWindow)
        {
            const ImVec2 offset = window->Size * (1.0f - scale) * (g.IO.MousePos - window->Pos) / window->Size;
            SetWindowPos(window, window->Pos + offset, 0);
            window->Size = ImFloor(window->Size * scale);
            window->SizeFull = ImFloor(window->SizeFull * scale);
        }
        return;
    }

    // Mouse wheel scrolling
    // If a child window has the ImGuiWindowFlags_NoScrollWithMouse flag, we give a chance to scroll its parent
    if (g.IO.KeyCtrl)
        return;

    // As a standard behavior holding SHIFT while using Vertical Mouse Wheel triggers Horizontal scroll instead
    // (we avoid doing it on OSX as it the OS input layer handles this already)
    const bool swap_axis = g.IO.KeyShift && !g.IO.ConfigMacOSXBehaviors;
    if (swap_axis)
    {
        wheel_x = wheel_y;
        wheel_y = 0.0f;
    }

    // Vertical Mouse Wheel scrolling
    if (wheel_y != 0.0f)
    {
        StartLockWheelingWindow(window);
        while ((window->Flags & ImGuiWindowFlags_ChildWindow) && ((window->ScrollMax.y == 0.0f) || ((window->Flags & ImGuiWindowFlags_NoScrollWithMouse) && !(window->Flags & ImGuiWindowFlags_NoMouseInputs))))
            window = window->ParentWindow;
        if (!(window->Flags & ImGuiWindowFlags_NoScrollWithMouse) && !(window->Flags & ImGuiWindowFlags_NoMouseInputs))
        {
            float max_step = window->InnerRect.GetHeight() * 0.67f;
            float scroll_step = ImFloor(ImMin(5 * window->CalcFontSize(), max_step));
            SetScrollY(window, window->Scroll.y - wheel_y * scroll_step);
        }
    }

    // Horizontal Mouse Wheel scrolling, or Vertical Mouse Wheel w/ Shift held
    if (wheel_x != 0.0f)
    {
        StartLockWheelingWindow(window);
        while ((window->Flags & ImGuiWindowFlags_ChildWindow) && ((window->ScrollMax.x == 0.0f) || ((window->Flags & ImGuiWindowFlags_NoScrollWithMouse) && !(window->Flags & ImGuiWindowFlags_NoMouseInputs))))
            window = window->ParentWindow;
        if (!(window->Flags & ImGuiWindowFlags_NoScrollWithMouse) && !(window->Flags & ImGuiWindowFlags_NoMouseInputs))
        {
            float max_step = window->InnerRect.GetWidth() * 0.67f;
            float scroll_step = ImFloor(ImMin(2 * window->CalcFontSize(), max_step));
            SetScrollX(window, window->Scroll.x - wheel_x * scroll_step);
        }
    }
}

// The reason this is exposed in imgui_internal.h is: on touch-based system that don't have hovering, we want to dispatch inputs to the right target (imgui vs imgui+app)
void ImGui::UpdateHoveredWindowAndCaptureFlags()
{
    ImGuiContext& g = *GImGui;
    ImGuiIO& io = g.IO;
    g.WindowsHoverPadding = ImMax(g.Style.TouchExtraPadding, ImVec2(WINDOWS_HOVER_PADDING, WINDOWS_HOVER_PADDING));

    // Find the window hovered by mouse:
    // - Child windows can extend beyond the limit of their parent so we need to derive HoveredRootWindow from HoveredWindow.
    // - When moving a window we can skip the search, which also conveniently bypasses the fact that window->WindowRectClipped is lagging as this point of the frame.
    // - We also support the moved window toggling the NoInputs flag after moving has started in order to be able to detect windows below it, which is useful for e.g. docking mechanisms.
    bool clear_hovered_windows = false;
    FindHoveredWindow();

    // Modal windows prevents mouse from hovering behind them.
    ImGuiWindow* modal_window = GetTopMostPopupModal();
    if (modal_window && g.HoveredWindow && !IsWindowWithinBeginStackOf(g.HoveredWindow->RootWindow, modal_window))
        clear_hovered_windows = true;

    // Disabled mouse?
    if (io.ConfigFlags & ImGuiConfigFlags_NoMouse)
        clear_hovered_windows = true;

    // We track click ownership. When clicked outside of a window the click is owned by the application and
    // won't report hovering nor request capture even while dragging over our windows afterward.
    const bool has_open_popup = (g.OpenPopupStack.Size > 0);
    const bool has_open_modal = (modal_window != NULL);
    int mouse_earliest_down = -1;
    bool mouse_any_down = false;
    for (int i = 0; i < IM_ARRAYSIZE(io.MouseDown); i++)
    {
        if (io.MouseClicked[i])
        {
            io.MouseDownOwned[i] = (g.HoveredWindow != NULL) || has_open_popup;
            io.MouseDownOwnedUnlessPopupClose[i] = (g.HoveredWindow != NULL) || has_open_modal;
        }
        mouse_any_down |= io.MouseDown[i];
        if (io.MouseDown[i])
            if (mouse_earliest_down == -1 || io.MouseClickedTime[i] < io.MouseClickedTime[mouse_earliest_down])
                mouse_earliest_down = i;
    }
    const bool mouse_avail = (mouse_earliest_down == -1) || io.MouseDownOwned[mouse_earliest_down];
    const bool mouse_avail_unless_popup_close = (mouse_earliest_down == -1) || io.MouseDownOwnedUnlessPopupClose[mouse_earliest_down];

    // If mouse was first clicked outside of ImGui bounds we also cancel out hovering.
    // FIXME: For patterns of drag and drop across OS windows, we may need to rework/remove this test (first committed 311c0ca9 on 2015/02)
    const bool mouse_dragging_extern_payload = g.DragDropActive && (g.DragDropSourceFlags & ImGuiDragDropFlags_SourceExtern) != 0;
    if (!mouse_avail && !mouse_dragging_extern_payload)
        clear_hovered_windows = true;

    if (clear_hovered_windows)
        g.HoveredWindow = g.HoveredWindowUnderMovingWindow = NULL;

    // Update io.WantCaptureMouse for the user application (true = dispatch mouse info to Dear ImGui only, false = dispatch mouse to Dear ImGui + underlying app)
    // Update io.WantCaptureMouseAllowPopupClose (experimental) to give a chance for app to react to popup closure with a drag
    if (g.WantCaptureMouseNextFrame != -1)
    {
        io.WantCaptureMouse = io.WantCaptureMouseUnlessPopupClose = (g.WantCaptureMouseNextFrame != 0);
    }
    else
    {
        io.WantCaptureMouse = (mouse_avail && (g.HoveredWindow != NULL || mouse_any_down)) || has_open_popup;
        io.WantCaptureMouseUnlessPopupClose = (mouse_avail_unless_popup_close && (g.HoveredWindow != NULL || mouse_any_down)) || has_open_modal;
    }

    // Update io.WantCaptureKeyboard for the user application (true = dispatch keyboard info to Dear ImGui only, false = dispatch keyboard info to Dear ImGui + underlying app)
    if (g.WantCaptureKeyboardNextFrame != -1)
        io.WantCaptureKeyboard = (g.WantCaptureKeyboardNextFrame != 0);
    else
        io.WantCaptureKeyboard = (g.ActiveId != 0) || (modal_window != NULL);
    if (io.NavActive && (io.ConfigFlags & ImGuiConfigFlags_NavEnableKeyboard) && !(io.ConfigFlags & ImGuiConfigFlags_NavNoCaptureKeyboard))
        io.WantCaptureKeyboard = true;

    // Update io.WantTextInput flag, this is to allow systems without a keyboard (e.g. mobile, hand-held) to show a software keyboard if possible
    io.WantTextInput = (g.WantTextInputNextFrame != -1) ? (g.WantTextInputNextFrame != 0) : false;
}

// [Internal] Do not use directly (can read io.KeyMods instead)
ImGuiModFlags ImGui::GetMergedModFlags()
{
    ImGuiContext& g = *GImGui;
    ImGuiModFlags key_mods = ImGuiModFlags_None;
    if (g.IO.KeyCtrl)   { key_mods |= ImGuiModFlags_Ctrl; }
    if (g.IO.KeyShift)  { key_mods |= ImGuiModFlags_Shift; }
    if (g.IO.KeyAlt)    { key_mods |= ImGuiModFlags_Alt; }
    if (g.IO.KeySuper)  { key_mods |= ImGuiModFlags_Super; }
    return key_mods;
}

void ImGui::NewFrame()
{
    IM_ASSERT(GImGui != NULL && "No current context. Did you call ImGui::CreateContext() and ImGui::SetCurrentContext() ?");
    ImGuiContext& g = *GImGui;

    // Remove pending delete hooks before frame start.
    // This deferred removal avoid issues of removal while iterating the hook vector
    for (int n = g.Hooks.Size - 1; n >= 0; n--)
        if (g.Hooks[n].Type == ImGuiContextHookType_PendingRemoval_)
            g.Hooks.erase(&g.Hooks[n]);

    CallContextHooks(&g, ImGuiContextHookType_NewFramePre);

    // Check and assert for various common IO and Configuration mistakes
    ErrorCheckNewFrameSanityChecks();

    // Load settings on first frame, save settings when modified (after a delay)
    UpdateSettings();

    g.Time += g.IO.DeltaTime;
    g.WithinFrameScope = true;
    g.FrameCount += 1;
    g.TooltipOverrideCount = 0;
    g.WindowsActiveCount = 0;
    g.MenusIdSubmittedThisFrame.resize(0);

    // Calculate frame-rate for the user, as a purely luxurious feature
    g.FramerateSecPerFrameAccum += g.IO.DeltaTime - g.FramerateSecPerFrame[g.FramerateSecPerFrameIdx];
    g.FramerateSecPerFrame[g.FramerateSecPerFrameIdx] = g.IO.DeltaTime;
    g.FramerateSecPerFrameIdx = (g.FramerateSecPerFrameIdx + 1) % IM_ARRAYSIZE(g.FramerateSecPerFrame);
    g.FramerateSecPerFrameCount = ImMin(g.FramerateSecPerFrameCount + 1, IM_ARRAYSIZE(g.FramerateSecPerFrame));
    g.IO.Framerate = (g.FramerateSecPerFrameAccum > 0.0f) ? (1.0f / (g.FramerateSecPerFrameAccum / (float)g.FramerateSecPerFrameCount)) : FLT_MAX;

    UpdateViewportsNewFrame();

    // Setup current font and draw list shared data
    g.IO.Fonts->Locked = true;
    SetCurrentFont(GetDefaultFont());
    IM_ASSERT(g.Font->IsLoaded());
    ImRect virtual_space(FLT_MAX, FLT_MAX, -FLT_MAX, -FLT_MAX);
    for (int n = 0; n < g.Viewports.Size; n++)
        virtual_space.Add(g.Viewports[n]->GetMainRect());
    g.DrawListSharedData.ClipRectFullscreen = virtual_space.ToVec4();
    g.DrawListSharedData.CurveTessellationTol = g.Style.CurveTessellationTol;
    g.DrawListSharedData.SetCircleTessellationMaxError(g.Style.CircleTessellationMaxError);
    g.DrawListSharedData.InitialFlags = ImDrawListFlags_None;
    if (g.Style.AntiAliasedLines)
        g.DrawListSharedData.InitialFlags |= ImDrawListFlags_AntiAliasedLines;
    if (g.Style.AntiAliasedLinesUseTex && !(g.Font->ContainerAtlas->Flags & ImFontAtlasFlags_NoBakedLines))
        g.DrawListSharedData.InitialFlags |= ImDrawListFlags_AntiAliasedLinesUseTex;
    if (g.Style.AntiAliasedFill)
        g.DrawListSharedData.InitialFlags |= ImDrawListFlags_AntiAliasedFill;
    if (g.IO.BackendFlags & ImGuiBackendFlags_RendererHasVtxOffset)
        g.DrawListSharedData.InitialFlags |= ImDrawListFlags_AllowVtxOffset;

    // Mark rendering data as invalid to prevent user who may have a handle on it to use it.
    for (int n = 0; n < g.Viewports.Size; n++)
    {
        ImGuiViewportP* viewport = g.Viewports[n];
        viewport->DrawDataP.Clear();
    }

    // Drag and drop keep the source ID alive so even if the source disappear our state is consistent
    if (g.DragDropActive && g.DragDropPayload.SourceId == g.ActiveId)
        KeepAliveID(g.DragDropPayload.SourceId);

    // Update HoveredId data
    if (!g.HoveredIdPreviousFrame)
        g.HoveredIdTimer = 0.0f;
    if (!g.HoveredIdPreviousFrame || (g.HoveredId && g.ActiveId == g.HoveredId))
        g.HoveredIdNotActiveTimer = 0.0f;
    if (g.HoveredId)
        g.HoveredIdTimer += g.IO.DeltaTime;
    if (g.HoveredId && g.ActiveId != g.HoveredId)
        g.HoveredIdNotActiveTimer += g.IO.DeltaTime;
    g.HoveredIdPreviousFrame = g.HoveredId;
    g.HoveredIdPreviousFrameUsingMouseWheel = g.HoveredIdUsingMouseWheel;
    g.HoveredId = 0;
    g.HoveredIdAllowOverlap = false;
    g.HoveredIdUsingMouseWheel = false;
    g.HoveredIdDisabled = false;

    // Update ActiveId data (clear reference to active widget if the widget isn't alive anymore)
    if (g.ActiveIdIsAlive != g.ActiveId && g.ActiveIdPreviousFrame == g.ActiveId && g.ActiveId != 0)
        ClearActiveID();
    if (g.ActiveId)
        g.ActiveIdTimer += g.IO.DeltaTime;
    g.LastActiveIdTimer += g.IO.DeltaTime;
    g.ActiveIdPreviousFrame = g.ActiveId;
    g.ActiveIdPreviousFrameWindow = g.ActiveIdWindow;
    g.ActiveIdPreviousFrameHasBeenEditedBefore = g.ActiveIdHasBeenEditedBefore;
    g.ActiveIdIsAlive = 0;
    g.ActiveIdHasBeenEditedThisFrame = false;
    g.ActiveIdPreviousFrameIsAlive = false;
    g.ActiveIdIsJustActivated = false;
    if (g.TempInputId != 0 && g.ActiveId != g.TempInputId)
        g.TempInputId = 0;
    if (g.ActiveId == 0)
    {
        g.ActiveIdUsingNavDirMask = 0x00;
        g.ActiveIdUsingNavInputMask = 0x00;
        g.ActiveIdUsingKeyInputMask.ClearAllBits();
    }

    // Drag and drop
    g.DragDropAcceptIdPrev = g.DragDropAcceptIdCurr;
    g.DragDropAcceptIdCurr = 0;
    g.DragDropAcceptIdCurrRectSurface = FLT_MAX;
    g.DragDropWithinSource = false;
    g.DragDropWithinTarget = false;
    g.DragDropHoldJustPressedId = 0;

    // Close popups on focus lost (currently wip/opt-in)
    //if (g.IO.AppFocusLost)
    //    ClosePopupsExceptModals();

    // Process input queue (trickle as many events as possible)
    g.InputEventsTrail.resize(0);
    UpdateInputEvents(g.IO.ConfigInputTrickleEventQueue);

    // Update keyboard input state
    UpdateKeyboardInputs();

    //IM_ASSERT(g.IO.KeyCtrl == IsKeyDown(ImGuiKey_LeftCtrl) || IsKeyDown(ImGuiKey_RightCtrl));
    //IM_ASSERT(g.IO.KeyShift == IsKeyDown(ImGuiKey_LeftShift) || IsKeyDown(ImGuiKey_RightShift));
    //IM_ASSERT(g.IO.KeyAlt == IsKeyDown(ImGuiKey_LeftAlt) || IsKeyDown(ImGuiKey_RightAlt));
    //IM_ASSERT(g.IO.KeySuper == IsKeyDown(ImGuiKey_LeftSuper) || IsKeyDown(ImGuiKey_RightSuper));

    // Update gamepad/keyboard navigation
    NavUpdate();

    // Update mouse input state
    UpdateMouseInputs();

    // Find hovered window
    // (needs to be before UpdateMouseMovingWindowNewFrame so we fill g.HoveredWindowUnderMovingWindow on the mouse release frame)
    UpdateHoveredWindowAndCaptureFlags();

    // Handle user moving window with mouse (at the beginning of the frame to avoid input lag or sheering)
    UpdateMouseMovingWindowNewFrame();

    // Background darkening/whitening
    if (GetTopMostPopupModal() != NULL || (g.NavWindowingTarget != NULL && g.NavWindowingHighlightAlpha > 0.0f))
        g.DimBgRatio = ImMin(g.DimBgRatio + g.IO.DeltaTime * 6.0f, 1.0f);
    else
        g.DimBgRatio = ImMax(g.DimBgRatio - g.IO.DeltaTime * 10.0f, 0.0f);

    g.MouseCursor = ImGuiMouseCursor_Arrow;
    g.WantCaptureMouseNextFrame = g.WantCaptureKeyboardNextFrame = g.WantTextInputNextFrame = -1;

    // Platform IME data: reset for the frame
    g.PlatformImeDataPrev = g.PlatformImeData;
    g.PlatformImeData.WantVisible = false;

    // Mouse wheel scrolling, scale
    UpdateMouseWheel();

    // Mark all windows as not visible and compact unused memory.
    IM_ASSERT(g.WindowsFocusOrder.Size <= g.Windows.Size);
    const float memory_compact_start_time = (g.GcCompactAll || g.IO.ConfigMemoryCompactTimer < 0.0f) ? FLT_MAX : (float)g.Time - g.IO.ConfigMemoryCompactTimer;
    for (int i = 0; i != g.Windows.Size; i++)
    {
        ImGuiWindow* window = g.Windows[i];
        window->WasActive = window->Active;
        window->BeginCount = 0;
        window->Active = false;
        window->WriteAccessed = false;

        // Garbage collect transient buffers of recently unused windows
        if (!window->WasActive && !window->MemoryCompacted && window->LastTimeActive < memory_compact_start_time)
            GcCompactTransientWindowBuffers(window);
    }

    // Garbage collect transient buffers of recently unused tables
    for (int i = 0; i < g.TablesLastTimeActive.Size; i++)
        if (g.TablesLastTimeActive[i] >= 0.0f && g.TablesLastTimeActive[i] < memory_compact_start_time)
            TableGcCompactTransientBuffers(g.Tables.GetByIndex(i));
    for (int i = 0; i < g.TablesTempData.Size; i++)
        if (g.TablesTempData[i].LastTimeActive >= 0.0f && g.TablesTempData[i].LastTimeActive < memory_compact_start_time)
            TableGcCompactTransientBuffers(&g.TablesTempData[i]);
    if (g.GcCompactAll)
        GcCompactTransientMiscBuffers();
    g.GcCompactAll = false;

    // Closing the focused window restore focus to the first active root window in descending z-order
    if (g.NavWindow && !g.NavWindow->WasActive)
        FocusTopMostWindowUnderOne(NULL, NULL);

    // No window should be open at the beginning of the frame.
    // But in order to allow the user to call NewFrame() multiple times without calling Render(), we are doing an explicit clear.
    g.CurrentWindowStack.resize(0);
    g.BeginPopupStack.resize(0);
    g.ItemFlagsStack.resize(0);
    g.ItemFlagsStack.push_back(ImGuiItemFlags_None);
    g.GroupStack.resize(0);

    // [DEBUG] Update debug features
    UpdateDebugToolItemPicker();
    UpdateDebugToolStackQueries();

    // Create implicit/fallback window - which we will only render it if the user has added something to it.
    // We don't use "Debug" to avoid colliding with user trying to create a "Debug" window with custom flags.
    // This fallback is particularly important as it avoid ImGui:: calls from crashing.
    g.WithinFrameScopeWithImplicitWindow = true;
    SetNextWindowSize(ImVec2(400, 400), ImGuiCond_FirstUseEver);
    Begin("Debug##Default");
    IM_ASSERT(g.CurrentWindow->IsFallbackWindow == true);

    CallContextHooks(&g, ImGuiContextHookType_NewFramePost);
}

void ImGui::Initialize()
{
    ImGuiContext& g = *GImGui;
    IM_ASSERT(!g.Initialized && !g.SettingsLoaded);

    // Add .ini handle for ImGuiWindow type
    {
        ImGuiSettingsHandler ini_handler;
        ini_handler.TypeName = "Window";
        ini_handler.TypeHash = ImHashStr("Window");
        ini_handler.ClearAllFn = WindowSettingsHandler_ClearAll;
        ini_handler.ReadOpenFn = WindowSettingsHandler_ReadOpen;
        ini_handler.ReadLineFn = WindowSettingsHandler_ReadLine;
        ini_handler.ApplyAllFn = WindowSettingsHandler_ApplyAll;
        ini_handler.WriteAllFn = WindowSettingsHandler_WriteAll;
        AddSettingsHandler(&ini_handler);
    }

    // Add .ini handle for ImGuiTable type
    TableSettingsAddSettingsHandler();

    // Create default viewport
    ImGuiViewportP* viewport = IM_NEW(ImGuiViewportP)();
    g.Viewports.push_back(viewport);

#ifdef IMGUI_HAS_DOCK
#endif

    g.Initialized = true;
}

// This function is merely here to free heap allocations.
void ImGui::Shutdown()
{
    // The fonts atlas can be used prior to calling NewFrame(), so we clear it even if g.Initialized is FALSE (which would happen if we never called NewFrame)
    ImGuiContext& g = *GImGui;
    if (g.IO.Fonts && g.FontAtlasOwnedByContext)
    {
        g.IO.Fonts->Locked = false;
        IM_DELETE(g.IO.Fonts);
    }
    g.IO.Fonts = NULL;

    // Cleanup of other data are conditional on actually having initialized Dear ImGui.
    if (!g.Initialized)
        return;

    // Save settings (unless we haven't attempted to load them: CreateContext/DestroyContext without a call to NewFrame shouldn't save an empty file)
    if (g.SettingsLoaded && g.IO.IniFilename != NULL)
        SaveIniSettingsToDisk(g.IO.IniFilename);

    CallContextHooks(&g, ImGuiContextHookType_Shutdown);

    // Clear everything else
    g.Windows.clear_delete();
    g.WindowsFocusOrder.clear();
    g.WindowsTempSortBuffer.clear();
    g.CurrentWindow = NULL;
    g.CurrentWindowStack.clear();
    g.WindowsById.Clear();
    g.NavWindow = NULL;
    g.HoveredWindow = g.HoveredWindowUnderMovingWindow = NULL;
    g.ActiveIdWindow = g.ActiveIdPreviousFrameWindow = NULL;
    g.MovingWindow = NULL;
    g.ColorStack.clear();
    g.StyleVarStack.clear();
    g.FontStack.clear();
    g.OpenPopupStack.clear();
    g.BeginPopupStack.clear();

    g.Viewports.clear_delete();

    g.TabBars.Clear();
    g.CurrentTabBarStack.clear();
    g.ShrinkWidthBuffer.clear();

    g.ClipperTempData.clear_destruct();

    g.Tables.Clear();
    g.TablesTempData.clear_destruct();
    g.DrawChannelsTempMergeBuffer.clear();

    g.ClipboardHandlerData.clear();
    g.MenusIdSubmittedThisFrame.clear();
    g.InputTextState.ClearFreeMemory();

    g.SettingsWindows.clear();
    g.SettingsHandlers.clear();

    if (g.LogFile)
    {
#ifndef IMGUI_DISABLE_TTY_FUNCTIONS
        if (g.LogFile != stdout)
#endif
            ImFileClose(g.LogFile);
        g.LogFile = NULL;
    }
    g.LogBuffer.clear();

    g.Initialized = false;
}

// FIXME: Add a more explicit sort order in the window structure.
static int IMGUI_CDECL ChildWindowComparer(const void* lhs, const void* rhs)
{
    const ImGuiWindow* const a = *(const ImGuiWindow* const *)lhs;
    const ImGuiWindow* const b = *(const ImGuiWindow* const *)rhs;
    if (int d = (a->Flags & ImGuiWindowFlags_Popup) - (b->Flags & ImGuiWindowFlags_Popup))
        return d;
    if (int d = (a->Flags & ImGuiWindowFlags_Tooltip) - (b->Flags & ImGuiWindowFlags_Tooltip))
        return d;
    return (a->BeginOrderWithinParent - b->BeginOrderWithinParent);
}

static void AddWindowToSortBuffer(ImVector<ImGuiWindow*>* out_sorted_windows, ImGuiWindow* window)
{
    out_sorted_windows->push_back(window);
    if (window->Active)
    {
        int count = window->DC.ChildWindows.Size;
        ImQsort(window->DC.ChildWindows.Data, (size_t)count, sizeof(ImGuiWindow*), ChildWindowComparer);
        for (int i = 0; i < count; i++)
        {
            ImGuiWindow* child = window->DC.ChildWindows[i];
            if (child->Active)
                AddWindowToSortBuffer(out_sorted_windows, child);
        }
    }
}

static void AddDrawListToDrawData(ImVector<ImDrawList*>* out_list, ImDrawList* draw_list)
{
    if (draw_list->CmdBuffer.Size == 0)
        return;
    if (draw_list->CmdBuffer.Size == 1 && draw_list->CmdBuffer[0].ElemCount == 0 && draw_list->CmdBuffer[0].UserCallback == NULL)
        return;

    // Draw list sanity check. Detect mismatch between PrimReserve() calls and incrementing _VtxCurrentIdx, _VtxWritePtr etc.
    // May trigger for you if you are using PrimXXX functions incorrectly.
    IM_ASSERT(draw_list->VtxBuffer.Size == 0 || draw_list->_VtxWritePtr == draw_list->VtxBuffer.Data + draw_list->VtxBuffer.Size);
    IM_ASSERT(draw_list->IdxBuffer.Size == 0 || draw_list->_IdxWritePtr == draw_list->IdxBuffer.Data + draw_list->IdxBuffer.Size);
    if (!(draw_list->Flags & ImDrawListFlags_AllowVtxOffset))
        IM_ASSERT((int)draw_list->_VtxCurrentIdx == draw_list->VtxBuffer.Size);

    // Check that draw_list doesn't use more vertices than indexable (default ImDrawIdx = unsigned short = 2 bytes = 64K vertices per ImDrawList = per window)
    // If this assert triggers because you are drawing lots of stuff manually:
    // - First, make sure you are coarse clipping yourself and not trying to draw many things outside visible bounds.
    //   Be mindful that the ImDrawList API doesn't filter vertices. Use the Metrics/Debugger window to inspect draw list contents.
    // - If you want large meshes with more than 64K vertices, you can either:
    //   (A) Handle the ImDrawCmd::VtxOffset value in your renderer backend, and set 'io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset'.
    //       Most example backends already support this from 1.71. Pre-1.71 backends won't.
    //       Some graphics API such as GL ES 1/2 don't have a way to offset the starting vertex so it is not supported for them.
    //   (B) Or handle 32-bit indices in your renderer backend, and uncomment '#define ImDrawIdx unsigned int' line in imconfig.h.
    //       Most example backends already support this. For example, the OpenGL example code detect index size at compile-time:
    //         glDrawElements(GL_TRIANGLES, (GLsizei)pcmd->ElemCount, sizeof(ImDrawIdx) == 2 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT, idx_buffer_offset);
    //       Your own engine or render API may use different parameters or function calls to specify index sizes.
    //       2 and 4 bytes indices are generally supported by most graphics API.
    // - If for some reason neither of those solutions works for you, a workaround is to call BeginChild()/EndChild() before reaching
    //   the 64K limit to split your draw commands in multiple draw lists.
    if (sizeof(ImDrawIdx) == 2)
        IM_ASSERT(draw_list->_VtxCurrentIdx < (1 << 16) && "Too many vertices in ImDrawList using 16-bit indices. Read comment above");

    out_list->push_back(draw_list);
}

static void AddWindowToDrawData(ImGuiWindow* window, int layer)
{
    ImGuiContext& g = *GImGui;
    ImGuiViewportP* viewport = g.Viewports[0];
    g.IO.MetricsRenderWindows++;
    AddDrawListToDrawData(&viewport->DrawDataBuilder.Layers[layer], window->DrawList);
    for (int i = 0; i < window->DC.ChildWindows.Size; i++)
    {
        ImGuiWindow* child = window->DC.ChildWindows[i];
        if (IsWindowActiveAndVisible(child)) // Clipped children may have been marked not active
            AddWindowToDrawData(child, layer);
    }
}

static inline int GetWindowDisplayLayer(ImGuiWindow* window)
{
    return (window->Flags & ImGuiWindowFlags_Tooltip) ? 1 : 0;
}

// Layer is locked for the root window, however child windows may use a different viewport (e.g. extruding menu)
static inline void AddRootWindowToDrawData(ImGuiWindow* window)
{
    AddWindowToDrawData(window, GetWindowDisplayLayer(window));
}

void ImDrawDataBuilder::FlattenIntoSingleLayer()
{
    int n = Layers[0].Size;
    int size = n;
    for (int i = 1; i < IM_ARRAYSIZE(Layers); i++)
        size += Layers[i].Size;
    Layers[0].resize(size);
    for (int layer_n = 1; layer_n < IM_ARRAYSIZE(Layers); layer_n++)
    {
        ImVector<ImDrawList*>& layer = Layers[layer_n];
        if (layer.empty())
            continue;
        memcpy(&Layers[0][n], &layer[0], layer.Size * sizeof(ImDrawList*));
        n += layer.Size;
        layer.resize(0);
    }
}

static void SetupViewportDrawData(ImGuiViewportP* viewport, ImVector<ImDrawList*>* draw_lists)
{
    ImGuiIO& io = ImGui::GetIO();
    ImDrawData* draw_data = &viewport->DrawDataP;
    draw_data->Valid = true;
    draw_data->CmdLists = (draw_lists->Size > 0) ? draw_lists->Data : NULL;
    draw_data->CmdListsCount = draw_lists->Size;
    draw_data->TotalVtxCount = draw_data->TotalIdxCount = 0;
    draw_data->DisplayPos = viewport->Pos;
    draw_data->DisplaySize = viewport->Size;
    draw_data->FramebufferScale = io.DisplayFramebufferScale;
    for (int n = 0; n < draw_lists->Size; n++)
    {
        ImDrawList* draw_list = draw_lists->Data[n];
        draw_list->_PopUnusedDrawCmd();
        draw_data->TotalVtxCount += draw_list->VtxBuffer.Size;
        draw_data->TotalIdxCount += draw_list->IdxBuffer.Size;
    }
}

// Push a clipping rectangle for both ImGui logic (hit-testing etc.) and low-level ImDrawList rendering.
// - When using this function it is sane to ensure that float are perfectly rounded to integer values,
//   so that e.g. (int)(max.x-min.x) in user's render produce correct result.
// - If the code here changes, may need to update code of functions like NextColumn() and PushColumnClipRect():
//   some frequently called functions which to modify both channels and clipping simultaneously tend to use the
//   more specialized SetWindowClipRectBeforeSetChannel() to avoid extraneous updates of underlying ImDrawCmds.
void ImGui::PushClipRect(const ImVec2& clip_rect_min, const ImVec2& clip_rect_max, bool intersect_with_current_clip_rect)
{
    ImGuiWindow* window = GetCurrentWindow();
    window->DrawList->PushClipRect(clip_rect_min, clip_rect_max, intersect_with_current_clip_rect);
    window->ClipRect = window->DrawList->_ClipRectStack.back();
}

void ImGui::PopClipRect()
{
    ImGuiWindow* window = GetCurrentWindow();
    window->DrawList->PopClipRect();
    window->ClipRect = window->DrawList->_ClipRectStack.back();
}

static void ImGui::RenderDimmedBackgroundBehindWindow(ImGuiWindow* window, ImU32 col)
{
    if ((col & IM_COL32_A_MASK) == 0)
        return;

    ImGuiViewportP* viewport = (ImGuiViewportP*)GetMainViewport();
    ImRect viewport_rect = viewport->GetMainRect();

    // Draw behind window by moving the draw command at the FRONT of the draw list
    {
        // We've already called AddWindowToDrawData() which called DrawList->ChannelsMerge() on DockNodeHost windows,
        // and draw list have been trimmed already, hence the explicit recreation of a draw command if missing.
        // FIXME: This is creating complication, might be simpler if we could inject a drawlist in drawdata at a given position and not attempt to manipulate ImDrawCmd order.
        ImDrawList* draw_list = window->RootWindow->DrawList;
        if (draw_list->CmdBuffer.Size == 0)
            draw_list->AddDrawCmd();
        draw_list->PushClipRect(viewport_rect.Min - ImVec2(1, 1), viewport_rect.Max + ImVec2(1, 1), false); // Ensure ImDrawCmd are not merged
        draw_list->AddRectFilled(viewport_rect.Min, viewport_rect.Max, col);
        ImDrawCmd cmd = draw_list->CmdBuffer.back();
        IM_ASSERT(cmd.ElemCount == 6);
        draw_list->CmdBuffer.pop_back();
        draw_list->CmdBuffer.push_front(cmd);
        draw_list->PopClipRect();
        draw_list->AddDrawCmd(); // We need to create a command as CmdBuffer.back().IdxOffset won't be correct if we append to same command.
    }
}

ImGuiWindow* ImGui::FindBottomMostVisibleWindowWithinBeginStack(ImGuiWindow* parent_window)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* bottom_most_visible_window = parent_window;
    for (int i = FindWindowDisplayIndex(parent_window); i >= 0; i--)
    {
        ImGuiWindow* window = g.Windows[i];
        if (window->Flags & ImGuiWindowFlags_ChildWindow)
            continue;
        if (!IsWindowWithinBeginStackOf(window, parent_window))
            break;
        if (IsWindowActiveAndVisible(window) && GetWindowDisplayLayer(window) <= GetWindowDisplayLayer(parent_window))
            bottom_most_visible_window = window;
    }
    return bottom_most_visible_window;
}

static void ImGui::RenderDimmedBackgrounds()
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* modal_window = GetTopMostAndVisiblePopupModal();
    if (g.DimBgRatio <= 0.0f && g.NavWindowingHighlightAlpha <= 0.0f)
        return;
    const bool dim_bg_for_modal = (modal_window != NULL);
    const bool dim_bg_for_window_list = (g.NavWindowingTargetAnim != NULL && g.NavWindowingTargetAnim->Active);
    if (!dim_bg_for_modal && !dim_bg_for_window_list)
        return;

    if (dim_bg_for_modal)
    {
        // Draw dimming behind modal or a begin stack child, whichever comes first in draw order.
        ImGuiWindow* dim_behind_window = FindBottomMostVisibleWindowWithinBeginStack(modal_window);
        RenderDimmedBackgroundBehindWindow(dim_behind_window, GetColorU32(ImGuiCol_ModalWindowDimBg, g.DimBgRatio));
    }
    else if (dim_bg_for_window_list)
    {
        // Draw dimming behind CTRL+Tab target window
        RenderDimmedBackgroundBehindWindow(g.NavWindowingTargetAnim, GetColorU32(ImGuiCol_NavWindowingDimBg, g.DimBgRatio));

        // Draw border around CTRL+Tab target window
        ImGuiWindow* window = g.NavWindowingTargetAnim;
        ImGuiViewport* viewport = GetMainViewport();
        float distance = g.FontSize;
        ImRect bb = window->Rect();
        bb.Expand(distance);
        if (bb.GetWidth() >= viewport->Size.x && bb.GetHeight() >= viewport->Size.y)
            bb.Expand(-distance - 1.0f); // If a window fits the entire viewport, adjust its highlight inward
        if (window->DrawList->CmdBuffer.Size == 0)
            window->DrawList->AddDrawCmd();
        window->DrawList->PushClipRect(viewport->Pos, viewport->Pos + viewport->Size);
        window->DrawList->AddRect(bb.Min, bb.Max, GetColorU32(ImGuiCol_NavWindowingHighlight, g.NavWindowingHighlightAlpha), window->WindowRounding, 0, 3.0f);
        window->DrawList->PopClipRect();
    }
}

// This is normally called by Render(). You may want to call it directly if you want to avoid calling Render() but the gain will be very minimal.
void ImGui::EndFrame()
{
    ImGuiContext& g = *GImGui;
    IM_ASSERT(g.Initialized);

    // Don't process EndFrame() multiple times.
    if (g.FrameCountEnded == g.FrameCount)
        return;
    IM_ASSERT(g.WithinFrameScope && "Forgot to call ImGui::NewFrame()?");

    CallContextHooks(&g, ImGuiContextHookType_EndFramePre);

    ErrorCheckEndFrameSanityChecks();

    // Notify Platform/OS when our Input Method Editor cursor has moved (e.g. CJK inputs using Microsoft IME)
    if (g.IO.SetPlatformImeDataFn && memcmp(&g.PlatformImeData, &g.PlatformImeDataPrev, sizeof(ImGuiPlatformImeData)) != 0)
        g.IO.SetPlatformImeDataFn(GetMainViewport(), &g.PlatformImeData);

    // Hide implicit/fallback "Debug" window if it hasn't been used
    g.WithinFrameScopeWithImplicitWindow = false;
    if (g.CurrentWindow && !g.CurrentWindow->WriteAccessed)
        g.CurrentWindow->Active = false;
    End();

    // Update navigation: CTRL+Tab, wrap-around requests
    NavEndFrame();

    // Drag and Drop: Elapse payload (if delivered, or if source stops being submitted)
    if (g.DragDropActive)
    {
        bool is_delivered = g.DragDropPayload.Delivery;
        bool is_elapsed = (g.DragDropPayload.DataFrameCount + 1 < g.FrameCount) && ((g.DragDropSourceFlags & ImGuiDragDropFlags_SourceAutoExpirePayload) || !IsMouseDown(g.DragDropMouseButton));
        if (is_delivered || is_elapsed)
            ClearDragDrop();
    }

    // Drag and Drop: Fallback for source tooltip. This is not ideal but better than nothing.
    if (g.DragDropActive && g.DragDropSourceFrameCount < g.FrameCount && !(g.DragDropSourceFlags & ImGuiDragDropFlags_SourceNoPreviewTooltip))
    {
        g.DragDropWithinSource = true;
        SetTooltip("...");
        g.DragDropWithinSource = false;
    }

    // End frame
    g.WithinFrameScope = false;
    g.FrameCountEnded = g.FrameCount;

    // Initiate moving window + handle left-click and right-click focus
    UpdateMouseMovingWindowEndFrame();

    // Sort the window list so that all child windows are after their parent
    // We cannot do that on FocusWindow() because children may not exist yet
    g.WindowsTempSortBuffer.resize(0);
    g.WindowsTempSortBuffer.reserve(g.Windows.Size);
    for (int i = 0; i != g.Windows.Size; i++)
    {
        ImGuiWindow* window = g.Windows[i];
        if (window->Active && (window->Flags & ImGuiWindowFlags_ChildWindow))       // if a child is active its parent will add it
            continue;
        AddWindowToSortBuffer(&g.WindowsTempSortBuffer, window);
    }

    // This usually assert if there is a mismatch between the ImGuiWindowFlags_ChildWindow / ParentWindow values and DC.ChildWindows[] in parents, aka we've done something wrong.
    IM_ASSERT(g.Windows.Size == g.WindowsTempSortBuffer.Size);
    g.Windows.swap(g.WindowsTempSortBuffer);
    g.IO.MetricsActiveWindows = g.WindowsActiveCount;

    // Unlock font atlas
    g.IO.Fonts->Locked = false;

    // Clear Input data for next frame
    g.IO.MouseWheel = g.IO.MouseWheelH = 0.0f;
    g.IO.InputQueueCharacters.resize(0);
    memset(g.IO.NavInputs, 0, sizeof(g.IO.NavInputs));

    CallContextHooks(&g, ImGuiContextHookType_EndFramePost);
}

// Prepare the data for rendering so you can call GetDrawData()
// (As with anything within the ImGui:: namspace this doesn't touch your GPU or graphics API at all:
// it is the role of the ImGui_ImplXXXX_RenderDrawData() function provided by the renderer backend)
void ImGui::Render()
{
    ImGuiContext& g = *GImGui;
    IM_ASSERT(g.Initialized);

    if (g.FrameCountEnded != g.FrameCount)
        EndFrame();
    const bool first_render_of_frame = (g.FrameCountRendered != g.FrameCount);
    g.FrameCountRendered = g.FrameCount;
    g.IO.MetricsRenderWindows = 0;

    CallContextHooks(&g, ImGuiContextHookType_RenderPre);

    // Add background ImDrawList (for each active viewport)
    for (int n = 0; n != g.Viewports.Size; n++)
    {
        ImGuiViewportP* viewport = g.Viewports[n];
        viewport->DrawDataBuilder.Clear();
        if (viewport->DrawLists[0] != NULL)
            AddDrawListToDrawData(&viewport->DrawDataBuilder.Layers[0], GetBackgroundDrawList(viewport));
    }

    // Draw modal/window whitening backgrounds
    if (first_render_of_frame)
        RenderDimmedBackgrounds();

    // Add ImDrawList to render
    ImGuiWindow* windows_to_render_top_most[2];
    windows_to_render_top_most[0] = (g.NavWindowingTarget && !(g.NavWindowingTarget->Flags & ImGuiWindowFlags_NoBringToFrontOnFocus)) ? g.NavWindowingTarget->RootWindow : NULL;
    windows_to_render_top_most[1] = (g.NavWindowingTarget ? g.NavWindowingListWindow : NULL);
    for (int n = 0; n != g.Windows.Size; n++)
    {
        ImGuiWindow* window = g.Windows[n];
        IM_MSVC_WARNING_SUPPRESS(6011); // Static Analysis false positive "warning C6011: Dereferencing NULL pointer 'window'"
        if (IsWindowActiveAndVisible(window) && (window->Flags & ImGuiWindowFlags_ChildWindow) == 0 && window != windows_to_render_top_most[0] && window != windows_to_render_top_most[1])
            AddRootWindowToDrawData(window);
    }
    for (int n = 0; n < IM_ARRAYSIZE(windows_to_render_top_most); n++)
        if (windows_to_render_top_most[n] && IsWindowActiveAndVisible(windows_to_render_top_most[n])) // NavWindowingTarget is always temporarily displayed as the top-most window
            AddRootWindowToDrawData(windows_to_render_top_most[n]);

    // Draw software mouse cursor if requested by io.MouseDrawCursor flag
    if (g.IO.MouseDrawCursor && first_render_of_frame && g.MouseCursor != ImGuiMouseCursor_None)
        RenderMouseCursor(g.IO.MousePos, g.Style.MouseCursorScale, g.MouseCursor, IM_COL32_WHITE, IM_COL32_BLACK, IM_COL32(0, 0, 0, 48));

    // Setup ImDrawData structures for end-user
    g.IO.MetricsRenderVertices = g.IO.MetricsRenderIndices = 0;
    for (int n = 0; n < g.Viewports.Size; n++)
    {
        ImGuiViewportP* viewport = g.Viewports[n];
        viewport->DrawDataBuilder.FlattenIntoSingleLayer();

        // Add foreground ImDrawList (for each active viewport)
        if (viewport->DrawLists[1] != NULL)
            AddDrawListToDrawData(&viewport->DrawDataBuilder.Layers[0], GetForegroundDrawList(viewport));

        SetupViewportDrawData(viewport, &viewport->DrawDataBuilder.Layers[0]);
        ImDrawData* draw_data = &viewport->DrawDataP;
        g.IO.MetricsRenderVertices += draw_data->TotalVtxCount;
        g.IO.MetricsRenderIndices += draw_data->TotalIdxCount;
    }

    CallContextHooks(&g, ImGuiContextHookType_RenderPost);
}

// Calculate text size. Text can be multi-line. Optionally ignore text after a ## marker.
// CalcTextSize("") should return ImVec2(0.0f, g.FontSize)
ImVec2 ImGui::CalcTextSize(const char* text, const char* text_end, bool hide_text_after_double_hash, float wrap_width)
{
    ImGuiContext& g = *GImGui;

    const char* text_display_end;
    if (hide_text_after_double_hash)
        text_display_end = FindRenderedTextEnd(text, text_end);      // Hide anything after a '##' string
    else
        text_display_end = text_end;

    ImFont* font = g.Font;
    const float font_size = g.FontSize;
    if (text == text_display_end)
        return ImVec2(0.0f, font_size);
    ImVec2 text_size = font->CalcTextSizeA(font_size, FLT_MAX, wrap_width, text, text_display_end, NULL);

    // Round
    // FIXME: This has been here since Dec 2015 (7b0bf230) but down the line we want this out.
    // FIXME: Investigate using ceilf or e.g.
    // - https://git.musl-libc.org/cgit/musl/tree/src/math/ceilf.c
    // - https://embarkstudios.github.io/rust-gpu/api/src/libm/math/ceilf.rs.html
    text_size.x = IM_FLOOR(text_size.x + 0.99999f);

    return text_size;
}

// Find window given position, search front-to-back
// FIXME: Note that we have an inconsequential lag here: OuterRectClipped is updated in Begin(), so windows moved programmatically
// with SetWindowPos() and not SetNextWindowPos() will have that rectangle lagging by a frame at the time FindHoveredWindow() is
// called, aka before the next Begin(). Moving window isn't affected.
static void FindHoveredWindow()
{
    ImGuiContext& g = *GImGui;

    ImGuiWindow* hovered_window = NULL;
    ImGuiWindow* hovered_window_ignoring_moving_window = NULL;
    if (g.MovingWindow && !(g.MovingWindow->Flags & ImGuiWindowFlags_NoMouseInputs))
        hovered_window = g.MovingWindow;

    ImVec2 padding_regular = g.Style.TouchExtraPadding;
    ImVec2 padding_for_resize = g.IO.ConfigWindowsResizeFromEdges ? g.WindowsHoverPadding : padding_regular;
    for (int i = g.Windows.Size - 1; i >= 0; i--)
    {
        ImGuiWindow* window = g.Windows[i];
        IM_MSVC_WARNING_SUPPRESS(28182); // [Static Analyzer] Dereferencing NULL pointer.
        if (!window->Active || window->Hidden)
            continue;
        if (window->Flags & ImGuiWindowFlags_NoMouseInputs)
            continue;

        // Using the clipped AABB, a child window will typically be clipped by its parent (not always)
        ImRect bb(window->OuterRectClipped);
        if (window->Flags & (ImGuiWindowFlags_ChildWindow | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize))
            bb.Expand(padding_regular);
        else
            bb.Expand(padding_for_resize);
        if (!bb.Contains(g.IO.MousePos))
            continue;

        // Support for one rectangular hole in any given window
        // FIXME: Consider generalizing hit-testing override (with more generic data, callback, etc.) (#1512)
        if (window->HitTestHoleSize.x != 0)
        {
            ImVec2 hole_pos(window->Pos.x + (float)window->HitTestHoleOffset.x, window->Pos.y + (float)window->HitTestHoleOffset.y);
            ImVec2 hole_size((float)window->HitTestHoleSize.x, (float)window->HitTestHoleSize.y);
            if (ImRect(hole_pos, hole_pos + hole_size).Contains(g.IO.MousePos))
                continue;
        }

        if (hovered_window == NULL)
            hovered_window = window;
        IM_MSVC_WARNING_SUPPRESS(28182); // [Static Analyzer] Dereferencing NULL pointer.
        if (hovered_window_ignoring_moving_window == NULL && (!g.MovingWindow || window->RootWindow != g.MovingWindow->RootWindow))
            hovered_window_ignoring_moving_window = window;
        if (hovered_window && hovered_window_ignoring_moving_window)
            break;
    }

    g.HoveredWindow = hovered_window;
    g.HoveredWindowUnderMovingWindow = hovered_window_ignoring_moving_window;
}

bool ImGui::IsItemActive()
{
    ImGuiContext& g = *GImGui;
    if (g.ActiveId)
        return g.ActiveId == g.LastItemData.ID;
    return false;
}

bool ImGui::IsItemActivated()
{
    ImGuiContext& g = *GImGui;
    if (g.ActiveId)
        if (g.ActiveId == g.LastItemData.ID && g.ActiveIdPreviousFrame != g.LastItemData.ID)
            return true;
    return false;
}

bool ImGui::IsItemDeactivated()
{
    ImGuiContext& g = *GImGui;
    if (g.LastItemData.StatusFlags & ImGuiItemStatusFlags_HasDeactivated)
        return (g.LastItemData.StatusFlags & ImGuiItemStatusFlags_Deactivated) != 0;
    return (g.ActiveIdPreviousFrame == g.LastItemData.ID && g.ActiveIdPreviousFrame != 0 && g.ActiveId != g.LastItemData.ID);
}

bool ImGui::IsItemDeactivatedAfterEdit()
{
    ImGuiContext& g = *GImGui;
    return IsItemDeactivated() && (g.ActiveIdPreviousFrameHasBeenEditedBefore || (g.ActiveId == 0 && g.ActiveIdHasBeenEditedBefore));
}

// == GetItemID() == GetFocusID()
bool ImGui::IsItemFocused()
{
    ImGuiContext& g = *GImGui;
    if (g.NavId != g.LastItemData.ID || g.NavId == 0)
        return false;
    return true;
}

// Important: this can be useful but it is NOT equivalent to the behavior of e.g.Button()!
// Most widgets have specific reactions based on mouse-up/down state, mouse position etc.
bool ImGui::IsItemClicked(ImGuiMouseButton mouse_button)
{
    return IsMouseClicked(mouse_button) && IsItemHovered(ImGuiHoveredFlags_None);
}

bool ImGui::IsItemToggledOpen()
{
    ImGuiContext& g = *GImGui;
    return (g.LastItemData.StatusFlags & ImGuiItemStatusFlags_ToggledOpen) ? true : false;
}

bool ImGui::IsItemToggledSelection()
{
    ImGuiContext& g = *GImGui;
    return (g.LastItemData.StatusFlags & ImGuiItemStatusFlags_ToggledSelection) ? true : false;
}

bool ImGui::IsAnyItemHovered()
{
    ImGuiContext& g = *GImGui;
    return g.HoveredId != 0 || g.HoveredIdPreviousFrame != 0;
}

bool ImGui::IsAnyItemActive()
{
    ImGuiContext& g = *GImGui;
    return g.ActiveId != 0;
}

bool ImGui::IsAnyItemFocused()
{
    ImGuiContext& g = *GImGui;
    return g.NavId != 0 && !g.NavDisableHighlight;
}

bool ImGui::IsItemVisible()
{
    ImGuiContext& g = *GImGui;
    return g.CurrentWindow->ClipRect.Overlaps(g.LastItemData.Rect);
}

bool ImGui::IsItemEdited()
{
    ImGuiContext& g = *GImGui;
    return (g.LastItemData.StatusFlags & ImGuiItemStatusFlags_Edited) != 0;
}

// Allow last item to be overlapped by a subsequent item. Both may be activated during the same frame before the later one takes priority.
// FIXME: Although this is exposed, its interaction and ideal idiom with using ImGuiButtonFlags_AllowItemOverlap flag are extremely confusing, need rework.
void ImGui::SetItemAllowOverlap()
{
    ImGuiContext& g = *GImGui;
    ImGuiID id = g.LastItemData.ID;
    if (g.HoveredId == id)
        g.HoveredIdAllowOverlap = true;
    if (g.ActiveId == id)
        g.ActiveIdAllowOverlap = true;
}

void ImGui::SetItemUsingMouseWheel()
{
    ImGuiContext& g = *GImGui;
    ImGuiID id = g.LastItemData.ID;
    if (g.HoveredId == id)
        g.HoveredIdUsingMouseWheel = true;
    if (g.ActiveId == id)
        g.ActiveIdUsingMouseWheel = true;
}

void ImGui::SetActiveIdUsingNavAndKeys()
{
    ImGuiContext& g = *GImGui;
    IM_ASSERT(g.ActiveId != 0);
    g.ActiveIdUsingNavDirMask = ~(ImU32)0;
    g.ActiveIdUsingNavInputMask = ~(ImU32)0;
    g.ActiveIdUsingKeyInputMask.SetAllBits();
    NavMoveRequestCancel();
}

ImVec2 ImGui::GetItemRectMin()
{
    ImGuiContext& g = *GImGui;
    return g.LastItemData.Rect.Min;
}

ImVec2 ImGui::GetItemRectMax()
{
    ImGuiContext& g = *GImGui;
    return g.LastItemData.Rect.Max;
}

ImVec2 ImGui::GetItemRectSize()
{
    ImGuiContext& g = *GImGui;
    return g.LastItemData.Rect.GetSize();
}

bool ImGui::BeginChildEx(const char* name, ImGuiID id, const ImVec2& size_arg, bool border, ImGuiWindowFlags flags)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* parent_window = g.CurrentWindow;

    flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_ChildWindow;
    flags |= (parent_window->Flags & ImGuiWindowFlags_NoMove);  // Inherit the NoMove flag

    // Size
    const ImVec2 content_avail = GetContentRegionAvail();
    ImVec2 size = ImFloor(size_arg);
    const int auto_fit_axises = ((size.x == 0.0f) ? (1 << ImGuiAxis_X) : 0x00) | ((size.y == 0.0f) ? (1 << ImGuiAxis_Y) : 0x00);
    if (size.x <= 0.0f)
        size.x = ImMax(content_avail.x + size.x, 4.0f); // Arbitrary minimum child size (0.0f causing too much issues)
    if (size.y <= 0.0f)
        size.y = ImMax(content_avail.y + size.y, 4.0f);
    SetNextWindowSize(size);

    // Build up name. If you need to append to a same child from multiple location in the ID stack, use BeginChild(ImGuiID id) with a stable value.
    if (name)
        ImFormatString(g.TempBuffer, IM_ARRAYSIZE(g.TempBuffer), "%s/%s_%08X", parent_window->Name, name, id);
    else
        ImFormatString(g.TempBuffer, IM_ARRAYSIZE(g.TempBuffer), "%s/%08X", parent_window->Name, id);

    const float backup_border_size = g.Style.ChildBorderSize;
    if (!border)
        g.Style.ChildBorderSize = 0.0f;
    bool ret = Begin(g.TempBuffer, NULL, flags);
    g.Style.ChildBorderSize = backup_border_size;

    ImGuiWindow* child_window = g.CurrentWindow;
    child_window->ChildId = id;
    child_window->AutoFitChildAxises = (ImS8)auto_fit_axises;

    // Set the cursor to handle case where the user called SetNextWindowPos()+BeginChild() manually.
    // While this is not really documented/defined, it seems that the expected thing to do.
    if (child_window->BeginCount == 1)
        parent_window->DC.CursorPos = child_window->Pos;

    // Process navigation-in immediately so NavInit can run on first frame
    if (g.NavActivateId == id && !(flags & ImGuiWindowFlags_NavFlattened) && (child_window->DC.NavLayersActiveMask != 0 || child_window->DC.NavHasScroll))
    {
        FocusWindow(child_window);
        NavInitWindow(child_window, false);
        SetActiveID(id + 1, child_window); // Steal ActiveId with another arbitrary id so that key-press won't activate child item
        g.ActiveIdSource = ImGuiInputSource_Nav;
    }
    return ret;
}

bool ImGui::BeginChild(const char* str_id, const ImVec2& size_arg, bool border, ImGuiWindowFlags extra_flags)
{
    ImGuiWindow* window = GetCurrentWindow();
    return BeginChildEx(str_id, window->GetID(str_id), size_arg, border, extra_flags);
}

bool ImGui::BeginChild(ImGuiID id, const ImVec2& size_arg, bool border, ImGuiWindowFlags extra_flags)
{
    IM_ASSERT(id != 0);
    return BeginChildEx(NULL, id, size_arg, border, extra_flags);
}

void ImGui::EndChild()
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;

    IM_ASSERT(g.WithinEndChild == false);
    IM_ASSERT(window->Flags & ImGuiWindowFlags_ChildWindow);   // Mismatched BeginChild()/EndChild() calls

    g.WithinEndChild = true;
    if (window->BeginCount > 1)
    {
        End();
    }
    else
    {
        ImVec2 sz = window->Size;
        if (window->AutoFitChildAxises & (1 << ImGuiAxis_X)) // Arbitrary minimum zero-ish child size of 4.0f causes less trouble than a 0.0f
            sz.x = ImMax(4.0f, sz.x);
        if (window->AutoFitChildAxises & (1 << ImGuiAxis_Y))
            sz.y = ImMax(4.0f, sz.y);
        End();

        ImGuiWindow* parent_window = g.CurrentWindow;
        ImRect bb(parent_window->DC.CursorPos, parent_window->DC.CursorPos + sz);
        ItemSize(sz);
        if ((window->DC.NavLayersActiveMask != 0 || window->DC.NavHasScroll) && !(window->Flags & ImGuiWindowFlags_NavFlattened))
        {
            ItemAdd(bb, window->ChildId);
            RenderNavHighlight(bb, window->ChildId);

            // When browsing a window that has no activable items (scroll only) we keep a highlight on the child (pass g.NavId to trick into always displaying)
            if (window->DC.NavLayersActiveMask == 0 && window == g.NavWindow)
                RenderNavHighlight(ImRect(bb.Min - ImVec2(2, 2), bb.Max + ImVec2(2, 2)), g.NavId, ImGuiNavHighlightFlags_TypeThin);
        }
        else
        {
            // Not navigable into
            ItemAdd(bb, 0);
        }
        if (g.HoveredWindow == window)
            g.LastItemData.StatusFlags |= ImGuiItemStatusFlags_HoveredWindow;
    }
    g.WithinEndChild = false;
    g.LogLinePosY = -FLT_MAX; // To enforce a carriage return
}

// Helper to create a child window / scrolling region that looks like a normal widget frame.
bool ImGui::BeginChildFrame(ImGuiID id, const ImVec2& size, ImGuiWindowFlags extra_flags)
{
    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    PushStyleColor(ImGuiCol_ChildBg, style.Colors[ImGuiCol_FrameBg]);
    PushStyleVar(ImGuiStyleVar_ChildRounding, style.FrameRounding);
    PushStyleVar(ImGuiStyleVar_ChildBorderSize, style.FrameBorderSize);
    PushStyleVar(ImGuiStyleVar_WindowPadding, style.FramePadding);
    bool ret = BeginChild(id, size, true, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysUseWindowPadding | extra_flags);
    PopStyleVar(3);
    PopStyleColor();
    return ret;
}

void ImGui::EndChildFrame()
{
    EndChild();
}

static void SetWindowConditionAllowFlags(ImGuiWindow* window, ImGuiCond flags, bool enabled)
{
    window->SetWindowPosAllowFlags       = enabled ? (window->SetWindowPosAllowFlags       | flags) : (window->SetWindowPosAllowFlags       & ~flags);
    window->SetWindowSizeAllowFlags      = enabled ? (window->SetWindowSizeAllowFlags      | flags) : (window->SetWindowSizeAllowFlags      & ~flags);
    window->SetWindowCollapsedAllowFlags = enabled ? (window->SetWindowCollapsedAllowFlags | flags) : (window->SetWindowCollapsedAllowFlags & ~flags);
}

ImGuiWindow* ImGui::FindWindowByID(ImGuiID id)
{
    ImGuiContext& g = *GImGui;
    return (ImGuiWindow*)g.WindowsById.GetVoidPtr(id);
}

ImGuiWindow* ImGui::FindWindowByName(const char* name)
{
    ImGuiID id = ImHashStr(name);
    return FindWindowByID(id);
}

static void ApplyWindowSettings(ImGuiWindow* window, ImGuiWindowSettings* settings)
{
    window->Pos = ImFloor(ImVec2(settings->Pos.x, settings->Pos.y));
    if (settings->Size.x > 0 && settings->Size.y > 0)
        window->Size = window->SizeFull = ImFloor(ImVec2(settings->Size.x, settings->Size.y));
    window->Collapsed = settings->Collapsed;
}

static void UpdateWindowInFocusOrderList(ImGuiWindow* window, bool just_created, ImGuiWindowFlags new_flags)
{
    ImGuiContext& g = *GImGui;

    const bool new_is_explicit_child = (new_flags & ImGuiWindowFlags_ChildWindow) != 0;
    const bool child_flag_changed = new_is_explicit_child != window->IsExplicitChild;
    if ((just_created || child_flag_changed) && !new_is_explicit_child)
    {
        IM_ASSERT(!g.WindowsFocusOrder.contains(window));
        g.WindowsFocusOrder.push_back(window);
        window->FocusOrder = (short)(g.WindowsFocusOrder.Size - 1);
    }
    else if (!just_created && child_flag_changed && new_is_explicit_child)
    {
        IM_ASSERT(g.WindowsFocusOrder[window->FocusOrder] == window);
        for (int n = window->FocusOrder + 1; n < g.WindowsFocusOrder.Size; n++)
            g.WindowsFocusOrder[n]->FocusOrder--;
        g.WindowsFocusOrder.erase(g.WindowsFocusOrder.Data + window->FocusOrder);
        window->FocusOrder = -1;
    }
    window->IsExplicitChild = new_is_explicit_child;
}

static ImGuiWindow* CreateNewWindow(const char* name, ImGuiWindowFlags flags)
{
    ImGuiContext& g = *GImGui;
    //IMGUI_DEBUG_LOG("CreateNewWindow '%s', flags = 0x%08X\n", name, flags);

    // Create window the first time
    ImGuiWindow* window = IM_NEW(ImGuiWindow)(&g, name);
    window->Flags = flags;
    g.WindowsById.SetVoidPtr(window->ID, window);

    // Default/arbitrary window position. Use SetNextWindowPos() with the appropriate condition flag to change the initial position of a window.
    const ImGuiViewport* main_viewport = ImGui::GetMainViewport();
    window->Pos = main_viewport->Pos + ImVec2(60, 60);

    // User can disable loading and saving of settings. Tooltip and child windows also don't store settings.
    if (!(flags & ImGuiWindowFlags_NoSavedSettings))
        if (ImGuiWindowSettings* settings = ImGui::FindWindowSettings(window->ID))
        {
            // Retrieve settings from .ini file
            window->SettingsOffset = g.SettingsWindows.offset_from_ptr(settings);
            SetWindowConditionAllowFlags(window, ImGuiCond_FirstUseEver, false);
            ApplyWindowSettings(window, settings);
        }
    window->DC.CursorStartPos = window->DC.CursorMaxPos = window->DC.IdealMaxPos = window->Pos; // So first call to CalcWindowContentSizes() doesn't return crazy values

    if ((flags & ImGuiWindowFlags_AlwaysAutoResize) != 0)
    {
        window->AutoFitFramesX = window->AutoFitFramesY = 2;
        window->AutoFitOnlyGrows = false;
    }
    else
    {
        if (window->Size.x <= 0.0f)
            window->AutoFitFramesX = 2;
        if (window->Size.y <= 0.0f)
            window->AutoFitFramesY = 2;
        window->AutoFitOnlyGrows = (window->AutoFitFramesX > 0) || (window->AutoFitFramesY > 0);
    }

    if (flags & ImGuiWindowFlags_NoBringToFrontOnFocus)
        g.Windows.push_front(window); // Quite slow but rare and only once
    else
        g.Windows.push_back(window);
    UpdateWindowInFocusOrderList(window, true, window->Flags);

    return window;
}

static ImVec2 CalcWindowSizeAfterConstraint(ImGuiWindow* window, const ImVec2& size_desired)
{
    ImGuiContext& g = *GImGui;
    ImVec2 new_size = size_desired;
    if (g.NextWindowData.Flags & ImGuiNextWindowDataFlags_HasSizeConstraint)
    {
        // Using -1,-1 on either X/Y axis to preserve the current size.
        ImRect cr = g.NextWindowData.SizeConstraintRect;
        new_size.x = (cr.Min.x >= 0 && cr.Max.x >= 0) ? ImClamp(new_size.x, cr.Min.x, cr.Max.x) : window->SizeFull.x;
        new_size.y = (cr.Min.y >= 0 && cr.Max.y >= 0) ? ImClamp(new_size.y, cr.Min.y, cr.Max.y) : window->SizeFull.y;
        if (g.NextWindowData.SizeCallback)
        {
            ImGuiSizeCallbackData data;
            data.UserData = g.NextWindowData.SizeCallbackUserData;
            data.Pos = window->Pos;
            data.CurrentSize = window->SizeFull;
            data.DesiredSize = new_size;
            g.NextWindowData.SizeCallback(&data);
            new_size = data.DesiredSize;
        }
        new_size.x = IM_FLOOR(new_size.x);
        new_size.y = IM_FLOOR(new_size.y);
    }

    // Minimum size
    if (!(window->Flags & (ImGuiWindowFlags_ChildWindow | ImGuiWindowFlags_AlwaysAutoResize)))
    {
        ImGuiWindow* window_for_height = window;
        const float decoration_up_height = window_for_height->TitleBarHeight() + window_for_height->MenuBarHeight();
        new_size = ImMax(new_size, g.Style.WindowMinSize);
        new_size.y = ImMax(new_size.y, decoration_up_height + ImMax(0.0f, g.Style.WindowRounding - 1.0f)); // Reduce artifacts with very small windows
    }
    return new_size;
}

static void CalcWindowContentSizes(ImGuiWindow* window, ImVec2* content_size_current, ImVec2* content_size_ideal)
{
    bool preserve_old_content_sizes = false;
    if (window->Collapsed && window->AutoFitFramesX <= 0 && window->AutoFitFramesY <= 0)
        preserve_old_content_sizes = true;
    else if (window->Hidden && window->HiddenFramesCannotSkipItems == 0 && window->HiddenFramesCanSkipItems > 0)
        preserve_old_content_sizes = true;
    if (preserve_old_content_sizes)
    {
        *content_size_current = window->ContentSize;
        *content_size_ideal = window->ContentSizeIdeal;
        return;
    }

    content_size_current->x = (window->ContentSizeExplicit.x != 0.0f) ? window->ContentSizeExplicit.x : IM_FLOOR(window->DC.CursorMaxPos.x - window->DC.CursorStartPos.x);
    content_size_current->y = (window->ContentSizeExplicit.y != 0.0f) ? window->ContentSizeExplicit.y : IM_FLOOR(window->DC.CursorMaxPos.y - window->DC.CursorStartPos.y);
    content_size_ideal->x = (window->ContentSizeExplicit.x != 0.0f) ? window->ContentSizeExplicit.x : IM_FLOOR(ImMax(window->DC.CursorMaxPos.x, window->DC.IdealMaxPos.x) - window->DC.CursorStartPos.x);
    content_size_ideal->y = (window->ContentSizeExplicit.y != 0.0f) ? window->ContentSizeExplicit.y : IM_FLOOR(ImMax(window->DC.CursorMaxPos.y, window->DC.IdealMaxPos.y) - window->DC.CursorStartPos.y);
}

static ImVec2 CalcWindowAutoFitSize(ImGuiWindow* window, const ImVec2& size_contents)
{
    ImGuiContext& g = *GImGui;
    ImGuiStyle& style = g.Style;
    const float decoration_up_height = window->TitleBarHeight() + window->MenuBarHeight();
    ImVec2 size_pad = window->WindowPadding * 2.0f;
    ImVec2 size_desired = size_contents + size_pad + ImVec2(0.0f, decoration_up_height);
    if (window->Flags & ImGuiWindowFlags_Tooltip)
    {
        // Tooltip always resize
        return size_desired;
    }
    else
    {
        // Maximum window size is determined by the viewport size or monitor size
        const bool is_popup = (window->Flags & ImGuiWindowFlags_Popup) != 0;
        const bool is_menu = (window->Flags & ImGuiWindowFlags_ChildMenu) != 0;
        ImVec2 size_min = style.WindowMinSize;
        if (is_popup || is_menu) // Popups and menus bypass style.WindowMinSize by default, but we give then a non-zero minimum size to facilitate understanding problematic cases (e.g. empty popups)
            size_min = ImMin(size_min, ImVec2(4.0f, 4.0f));

        // FIXME-VIEWPORT-WORKAREA: May want to use GetWorkSize() instead of Size depending on the type of windows?
        ImVec2 avail_size = ImGui::GetMainViewport()->Size;
        ImVec2 size_auto_fit = ImClamp(size_desired, size_min, ImMax(size_min, avail_size - style.DisplaySafeAreaPadding * 2.0f));

        // When the window cannot fit all contents (either because of constraints, either because screen is too small),
        // we are growing the size on the other axis to compensate for expected scrollbar. FIXME: Might turn bigger than ViewportSize-WindowPadding.
        ImVec2 size_auto_fit_after_constraint = CalcWindowSizeAfterConstraint(window, size_auto_fit);
        bool will_have_scrollbar_x = (size_auto_fit_after_constraint.x - size_pad.x - 0.0f                 < size_contents.x && !(window->Flags & ImGuiWindowFlags_NoScrollbar) && (window->Flags & ImGuiWindowFlags_HorizontalScrollbar)) || (window->Flags & ImGuiWindowFlags_AlwaysHorizontalScrollbar);
        bool will_have_scrollbar_y = (size_auto_fit_after_constraint.y - size_pad.y - decoration_up_height < size_contents.y && !(window->Flags & ImGuiWindowFlags_NoScrollbar)) || (window->Flags & ImGuiWindowFlags_AlwaysVerticalScrollbar);
        if (will_have_scrollbar_x)
            size_auto_fit.y += style.ScrollbarSize;
        if (will_have_scrollbar_y)
            size_auto_fit.x += style.ScrollbarSize;
        return size_auto_fit;
    }
}

ImVec2 ImGui::CalcWindowNextAutoFitSize(ImGuiWindow* window)
{
    ImVec2 size_contents_current;
    ImVec2 size_contents_ideal;
    CalcWindowContentSizes(window, &size_contents_current, &size_contents_ideal);
    ImVec2 size_auto_fit = CalcWindowAutoFitSize(window, size_contents_ideal);
    ImVec2 size_final = CalcWindowSizeAfterConstraint(window, size_auto_fit);
    return size_final;
}

static ImGuiCol GetWindowBgColorIdx(ImGuiWindow* window)
{
    if (window->Flags & (ImGuiWindowFlags_Tooltip | ImGuiWindowFlags_Popup))
        return ImGuiCol_PopupBg;
    if (window->Flags & ImGuiWindowFlags_ChildWindow)
        return ImGuiCol_ChildBg;
    return ImGuiCol_WindowBg;
}

static void CalcResizePosSizeFromAnyCorner(ImGuiWindow* window, const ImVec2& corner_target, const ImVec2& corner_norm, ImVec2* out_pos, ImVec2* out_size)
{
    ImVec2 pos_min = ImLerp(corner_target, window->Pos, corner_norm);                // Expected window upper-left
    ImVec2 pos_max = ImLerp(window->Pos + window->Size, corner_target, corner_norm); // Expected window lower-right
    ImVec2 size_expected = pos_max - pos_min;
    ImVec2 size_constrained = CalcWindowSizeAfterConstraint(window, size_expected);
    *out_pos = pos_min;
    if (corner_norm.x == 0.0f)
        out_pos->x -= (size_constrained.x - size_expected.x);
    if (corner_norm.y == 0.0f)
        out_pos->y -= (size_constrained.y - size_expected.y);
    *out_size = size_constrained;
}

// Data for resizing from corner
struct ImGuiResizeGripDef
{
    ImVec2  CornerPosN;
    ImVec2  InnerDir;
    int     AngleMin12, AngleMax12;
};
static const ImGuiResizeGripDef resize_grip_def[4] =
{
    { ImVec2(1, 1), ImVec2(-1, -1), 0, 3 },  // Lower-right
    { ImVec2(0, 1), ImVec2(+1, -1), 3, 6 },  // Lower-left
    { ImVec2(0, 0), ImVec2(+1, +1), 6, 9 },  // Upper-left (Unused)
    { ImVec2(1, 0), ImVec2(-1, +1), 9, 12 }  // Upper-right (Unused)
};

// Data for resizing from borders
struct ImGuiResizeBorderDef
{
    ImVec2 InnerDir;
    ImVec2 SegmentN1, SegmentN2;
    float  OuterAngle;
};
static const ImGuiResizeBorderDef resize_border_def[4] =
{
    { ImVec2(+1, 0), ImVec2(0, 1), ImVec2(0, 0), IM_PI * 1.00f }, // Left
    { ImVec2(-1, 0), ImVec2(1, 0), ImVec2(1, 1), IM_PI * 0.00f }, // Right
    { ImVec2(0, +1), ImVec2(0, 0), ImVec2(1, 0), IM_PI * 1.50f }, // Up
    { ImVec2(0, -1), ImVec2(1, 1), ImVec2(0, 1), IM_PI * 0.50f }  // Down
};

static ImRect GetResizeBorderRect(ImGuiWindow* window, int border_n, float perp_padding, float thickness)
{
    ImRect rect = window->Rect();
    if (thickness == 0.0f)
        rect.Max -= ImVec2(1, 1);
    if (border_n == ImGuiDir_Left)  { return ImRect(rect.Min.x - thickness,    rect.Min.y + perp_padding, rect.Min.x + thickness,    rect.Max.y - perp_padding); }
    if (border_n == ImGuiDir_Right) { return ImRect(rect.Max.x - thickness,    rect.Min.y + perp_padding, rect.Max.x + thickness,    rect.Max.y - perp_padding); }
    if (border_n == ImGuiDir_Up)    { return ImRect(rect.Min.x + perp_padding, rect.Min.y - thickness,    rect.Max.x - perp_padding, rect.Min.y + thickness);    }
    if (border_n == ImGuiDir_Down)  { return ImRect(rect.Min.x + perp_padding, rect.Max.y - thickness,    rect.Max.x - perp_padding, rect.Max.y + thickness);    }
    IM_ASSERT(0);
    return ImRect();
}

// 0..3: corners (Lower-right, Lower-left, Unused, Unused)
ImGuiID ImGui::GetWindowResizeCornerID(ImGuiWindow* window, int n)
{
    IM_ASSERT(n >= 0 && n < 4);
    ImGuiID id = window->ID;
    id = ImHashStr("#RESIZE", 0, id);
    id = ImHashData(&n, sizeof(int), id);
    return id;
}

// Borders (Left, Right, Up, Down)
ImGuiID ImGui::GetWindowResizeBorderID(ImGuiWindow* window, ImGuiDir dir)
{
    IM_ASSERT(dir >= 0 && dir < 4);
    int n = (int)dir + 4;
    ImGuiID id = window->ID;
    id = ImHashStr("#RESIZE", 0, id);
    id = ImHashData(&n, sizeof(int), id);
    return id;
}

// Handle resize for: Resize Grips, Borders, Gamepad
// Return true when using auto-fit (double click on resize grip)
static bool ImGui::UpdateWindowManualResize(ImGuiWindow* window, const ImVec2& size_auto_fit, int* border_held, int resize_grip_count, ImU32 resize_grip_col[4], const ImRect& visibility_rect)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindowFlags flags = window->Flags;

    if ((flags & ImGuiWindowFlags_NoResize) || (flags & ImGuiWindowFlags_AlwaysAutoResize) || window->AutoFitFramesX > 0 || window->AutoFitFramesY > 0)
        return false;
    if (window->WasActive == false) // Early out to avoid running this code for e.g. an hidden implicit/fallback Debug window.
        return false;

    bool ret_auto_fit = false;
    const int resize_border_count = g.IO.ConfigWindowsResizeFromEdges ? 4 : 0;
    const float grip_draw_size = IM_FLOOR(ImMax(g.FontSize * 1.35f, window->WindowRounding + 1.0f + g.FontSize * 0.2f));
    const float grip_hover_inner_size = IM_FLOOR(grip_draw_size * 0.75f);
    const float grip_hover_outer_size = g.IO.ConfigWindowsResizeFromEdges ? WINDOWS_HOVER_PADDING : 0.0f;

    ImVec2 pos_target(FLT_MAX, FLT_MAX);
    ImVec2 size_target(FLT_MAX, FLT_MAX);

    // Resize grips and borders are on layer 1
    window->DC.NavLayerCurrent = ImGuiNavLayer_Menu;

    // Manual resize grips
    PushID("#RESIZE");
    for (int resize_grip_n = 0; resize_grip_n < resize_grip_count; resize_grip_n++)
    {
        const ImGuiResizeGripDef& def = resize_grip_def[resize_grip_n];
        const ImVec2 corner = ImLerp(window->Pos, window->Pos + window->Size, def.CornerPosN);

        // Using the FlattenChilds button flag we make the resize button accessible even if we are hovering over a child window
        bool hovered, held;
        ImRect resize_rect(corner - def.InnerDir * grip_hover_outer_size, corner + def.InnerDir * grip_hover_inner_size);
        if (resize_rect.Min.x > resize_rect.Max.x) ImSwap(resize_rect.Min.x, resize_rect.Max.x);
        if (resize_rect.Min.y > resize_rect.Max.y) ImSwap(resize_rect.Min.y, resize_rect.Max.y);
        ImGuiID resize_grip_id = window->GetID(resize_grip_n); // == GetWindowResizeCornerID()
        KeepAliveID(resize_grip_id);
        ButtonBehavior(resize_rect, resize_grip_id, &hovered, &held, ImGuiButtonFlags_FlattenChildren | ImGuiButtonFlags_NoNavFocus);
        //GetForegroundDrawList(window)->AddRect(resize_rect.Min, resize_rect.Max, IM_COL32(255, 255, 0, 255));
        if (hovered || held)
            g.MouseCursor = (resize_grip_n & 1) ? ImGuiMouseCursor_ResizeNESW : ImGuiMouseCursor_ResizeNWSE;

        if (held && g.IO.MouseClickedCount[0] == 2 && resize_grip_n == 0)
        {
            // Manual auto-fit when double-clicking
            size_target = CalcWindowSizeAfterConstraint(window, size_auto_fit);
            ret_auto_fit = true;
            ClearActiveID();
        }
        else if (held)
        {
            // Resize from any of the four corners
            // We don't use an incremental MouseDelta but rather compute an absolute target size based on mouse position
            ImVec2 clamp_min = ImVec2(def.CornerPosN.x == 1.0f ? visibility_rect.Min.x : -FLT_MAX, def.CornerPosN.y == 1.0f ? visibility_rect.Min.y : -FLT_MAX);
            ImVec2 clamp_max = ImVec2(def.CornerPosN.x == 0.0f ? visibility_rect.Max.x : +FLT_MAX, def.CornerPosN.y == 0.0f ? visibility_rect.Max.y : +FLT_MAX);
            ImVec2 corner_target = g.IO.MousePos - g.ActiveIdClickOffset + ImLerp(def.InnerDir * grip_hover_outer_size, def.InnerDir * -grip_hover_inner_size, def.CornerPosN); // Corner of the window corresponding to our corner grip
            corner_target = ImClamp(corner_target, clamp_min, clamp_max);
            CalcResizePosSizeFromAnyCorner(window, corner_target, def.CornerPosN, &pos_target, &size_target);
        }

        // Only lower-left grip is visible before hovering/activating
        if (resize_grip_n == 0 || held || hovered)
            resize_grip_col[resize_grip_n] = GetColorU32(held ? ImGuiCol_ResizeGripActive : hovered ? ImGuiCol_ResizeGripHovered : ImGuiCol_ResizeGrip);
    }
    for (int border_n = 0; border_n < resize_border_count; border_n++)
    {
        const ImGuiResizeBorderDef& def = resize_border_def[border_n];
        const ImGuiAxis axis = (border_n == ImGuiDir_Left || border_n == ImGuiDir_Right) ? ImGuiAxis_X : ImGuiAxis_Y;

        bool hovered, held;
        ImRect border_rect = GetResizeBorderRect(window, border_n, grip_hover_inner_size, WINDOWS_HOVER_PADDING);
        ImGuiID border_id = window->GetID(border_n + 4); // == GetWindowResizeBorderID()
        KeepAliveID(border_id);
        ButtonBehavior(border_rect, border_id, &hovered, &held, ImGuiButtonFlags_FlattenChildren | ImGuiButtonFlags_NoNavFocus);
        //GetForegroundDrawLists(window)->AddRect(border_rect.Min, border_rect.Max, IM_COL32(255, 255, 0, 255));
        if ((hovered && g.HoveredIdTimer > WINDOWS_RESIZE_FROM_EDGES_FEEDBACK_TIMER) || held)
        {
            g.MouseCursor = (axis == ImGuiAxis_X) ? ImGuiMouseCursor_ResizeEW : ImGuiMouseCursor_ResizeNS;
            if (held)
                *border_held = border_n;
        }
        if (held)
        {
            ImVec2 clamp_min(border_n == ImGuiDir_Right ? visibility_rect.Min.x : -FLT_MAX, border_n == ImGuiDir_Down ? visibility_rect.Min.y : -FLT_MAX);
            ImVec2 clamp_max(border_n == ImGuiDir_Left  ? visibility_rect.Max.x : +FLT_MAX, border_n == ImGuiDir_Up   ? visibility_rect.Max.y : +FLT_MAX);
            ImVec2 border_target = window->Pos;
            border_target[axis] = g.IO.MousePos[axis] - g.ActiveIdClickOffset[axis] + WINDOWS_HOVER_PADDING;
            border_target = ImClamp(border_target, clamp_min, clamp_max);
            CalcResizePosSizeFromAnyCorner(window, border_target, ImMin(def.SegmentN1, def.SegmentN2), &pos_target, &size_target);
        }
    }
    PopID();

    // Restore nav layer
    window->DC.NavLayerCurrent = ImGuiNavLayer_Main;

    // Navigation resize (keyboard/gamepad)
    if (g.NavWindowingTarget && g.NavWindowingTarget->RootWindow == window)
    {
        ImVec2 nav_resize_delta;
        if (g.NavInputSource == ImGuiInputSource_Keyboard && g.IO.KeyShift)
            nav_resize_delta = GetNavInputAmount2d(ImGuiNavDirSourceFlags_RawKeyboard, ImGuiNavReadMode_Down);
        if (g.NavInputSource == ImGuiInputSource_Gamepad)
            nav_resize_delta = GetNavInputAmount2d(ImGuiNavDirSourceFlags_PadDPad, ImGuiNavReadMode_Down);
        if (nav_resize_delta.x != 0.0f || nav_resize_delta.y != 0.0f)
        {
            const float NAV_RESIZE_SPEED = 600.0f;
            nav_resize_delta *= ImFloor(NAV_RESIZE_SPEED * g.IO.DeltaTime * ImMin(g.IO.DisplayFramebufferScale.x, g.IO.DisplayFramebufferScale.y));
            nav_resize_delta = ImMax(nav_resize_delta, visibility_rect.Min - window->Pos - window->Size);
            g.NavWindowingToggleLayer = false;
            g.NavDisableMouseHover = true;
            resize_grip_col[0] = GetColorU32(ImGuiCol_ResizeGripActive);
            // FIXME-NAV: Should store and accumulate into a separate size buffer to handle sizing constraints properly, right now a constraint will make us stuck.
            size_target = CalcWindowSizeAfterConstraint(window, window->SizeFull + nav_resize_delta);
        }
    }

    // Apply back modified position/size to window
    if (size_target.x != FLT_MAX)
    {
        window->SizeFull = size_target;
        MarkIniSettingsDirty(window);
    }
    if (pos_target.x != FLT_MAX)
    {
        window->Pos = ImFloor(pos_target);
        MarkIniSettingsDirty(window);
    }

    window->Size = window->SizeFull;
    return ret_auto_fit;
}

static inline void ClampWindowRect(ImGuiWindow* window, const ImRect& visibility_rect)
{
    ImGuiContext& g = *GImGui;
    ImVec2 size_for_clamping = window->Size;
    if (g.IO.ConfigWindowsMoveFromTitleBarOnly && !(window->Flags & ImGuiWindowFlags_NoTitleBar))
        size_for_clamping.y = window->TitleBarHeight();
    window->Pos = ImClamp(window->Pos, visibility_rect.Min - size_for_clamping, visibility_rect.Max);
}

static void ImGui::RenderWindowOuterBorders(ImGuiWindow* window)
{
    ImGuiContext& g = *GImGui;
    float rounding = window->WindowRounding;
    float border_size = window->WindowBorderSize;
    if (border_size > 0.0f && !(window->Flags & ImGuiWindowFlags_NoBackground))
        window->DrawList->AddRect(window->Pos, window->Pos + window->Size, GetColorU32(ImGuiCol_Border), rounding, 0, border_size);

    int border_held = window->ResizeBorderHeld;
    if (border_held != -1)
    {
        const ImGuiResizeBorderDef& def = resize_border_def[border_held];
        ImRect border_r = GetResizeBorderRect(window, border_held, rounding, 0.0f);
        window->DrawList->PathArcTo(ImLerp(border_r.Min, border_r.Max, def.SegmentN1) + ImVec2(0.5f, 0.5f) + def.InnerDir * rounding, rounding, def.OuterAngle - IM_PI * 0.25f, def.OuterAngle);
        window->DrawList->PathArcTo(ImLerp(border_r.Min, border_r.Max, def.SegmentN2) + ImVec2(0.5f, 0.5f) + def.InnerDir * rounding, rounding, def.OuterAngle, def.OuterAngle + IM_PI * 0.25f);
        window->DrawList->PathStroke(GetColorU32(ImGuiCol_SeparatorActive), 0, ImMax(2.0f, border_size)); // Thicker than usual
    }
    if (g.Style.FrameBorderSize > 0 && !(window->Flags & ImGuiWindowFlags_NoTitleBar))
    {
        float y = window->Pos.y + window->TitleBarHeight() - 1;
        window->DrawList->AddLine(ImVec2(window->Pos.x + border_size, y), ImVec2(window->Pos.x + window->Size.x - border_size, y), GetColorU32(ImGuiCol_Border), g.Style.FrameBorderSize);
    }
}

// Draw background and borders
// Draw and handle scrollbars
void ImGui::RenderWindowDecorations(ImGuiWindow* window, const ImRect& title_bar_rect, bool title_bar_is_highlight, int resize_grip_count, const ImU32 resize_grip_col[4], float resize_grip_draw_size)
{
    ImGuiContext& g = *GImGui;
    ImGuiStyle& style = g.Style;
    ImGuiWindowFlags flags = window->Flags;

    // Ensure that ScrollBar doesn't read last frame's SkipItems
    IM_ASSERT(window->BeginCount == 0);
    window->SkipItems = false;

    // Draw window + handle manual resize
    // As we highlight the title bar when want_focus is set, multiple reappearing windows will have have their title bar highlighted on their reappearing frame.
    const float window_rounding = window->WindowRounding;
    const float window_border_size = window->WindowBorderSize;
    if (window->Collapsed)
    {
        // Title bar only
        float backup_border_size = style.FrameBorderSize;
        g.Style.FrameBorderSize = window->WindowBorderSize;
        ImU32 title_bar_col = GetColorU32((title_bar_is_highlight && !g.NavDisableHighlight) ? ImGuiCol_TitleBgActive : ImGuiCol_TitleBgCollapsed);
        RenderFrame(title_bar_rect.Min, title_bar_rect.Max, title_bar_col, true, window_rounding);
        g.Style.FrameBorderSize = backup_border_size;
    }
    else
    {
        // Window background
        if (!(flags & ImGuiWindowFlags_NoBackground))
        {
            ImU32 bg_col = GetColorU32(GetWindowBgColorIdx(window));
            bool override_alpha = false;
            float alpha = 1.0f;
            if (g.NextWindowData.Flags & ImGuiNextWindowDataFlags_HasBgAlpha)
            {
                alpha = g.NextWindowData.BgAlphaVal;
                override_alpha = true;
            }
            if (override_alpha)
                bg_col = (bg_col & ~IM_COL32_A_MASK) | (IM_F32_TO_INT8_SAT(alpha) << IM_COL32_A_SHIFT);
            window->DrawList->AddRectFilled(window->Pos + ImVec2(0, window->TitleBarHeight()), window->Pos + window->Size, bg_col, window_rounding, (flags & ImGuiWindowFlags_NoTitleBar) ? 0 : ImDrawFlags_RoundCornersBottom);
        }

        // Title bar
        if (!(flags & ImGuiWindowFlags_NoTitleBar))
        {
            ImU32 title_bar_col = GetColorU32(title_bar_is_highlight ? ImGuiCol_TitleBgActive : ImGuiCol_TitleBg);
            window->DrawList->AddRectFilled(title_bar_rect.Min, title_bar_rect.Max, title_bar_col, window_rounding, ImDrawFlags_RoundCornersTop);
        }

        // Menu bar
        if (flags & ImGuiWindowFlags_MenuBar)
        {
            ImRect menu_bar_rect = window->MenuBarRect();
            menu_bar_rect.ClipWith(window->Rect());  // Soft clipping, in particular child window don't have minimum size covering the menu bar so this is useful for them.
            window->DrawList->AddRectFilled(menu_bar_rect.Min + ImVec2(window_border_size, 0), menu_bar_rect.Max - ImVec2(window_border_size, 0), GetColorU32(ImGuiCol_MenuBarBg), (flags & ImGuiWindowFlags_NoTitleBar) ? window_rounding : 0.0f, ImDrawFlags_RoundCornersTop);
            if (style.FrameBorderSize > 0.0f && menu_bar_rect.Max.y < window->Pos.y + window->Size.y)
                window->DrawList->AddLine(menu_bar_rect.GetBL(), menu_bar_rect.GetBR(), GetColorU32(ImGuiCol_Border), style.FrameBorderSize);
        }

        // Scrollbars
        if (window->ScrollbarX)
            Scrollbar(ImGuiAxis_X);
        if (window->ScrollbarY)
            Scrollbar(ImGuiAxis_Y);

        // Render resize grips (after their input handling so we don't have a frame of latency)
        if (!(flags & ImGuiWindowFlags_NoResize))
        {
            for (int resize_grip_n = 0; resize_grip_n < resize_grip_count; resize_grip_n++)
            {
                const ImGuiResizeGripDef& grip = resize_grip_def[resize_grip_n];
                const ImVec2 corner = ImLerp(window->Pos, window->Pos + window->Size, grip.CornerPosN);
                window->DrawList->PathLineTo(corner + grip.InnerDir * ((resize_grip_n & 1) ? ImVec2(window_border_size, resize_grip_draw_size) : ImVec2(resize_grip_draw_size, window_border_size)));
                window->DrawList->PathLineTo(corner + grip.InnerDir * ((resize_grip_n & 1) ? ImVec2(resize_grip_draw_size, window_border_size) : ImVec2(window_border_size, resize_grip_draw_size)));
                window->DrawList->PathArcToFast(ImVec2(corner.x + grip.InnerDir.x * (window_rounding + window_border_size), corner.y + grip.InnerDir.y * (window_rounding + window_border_size)), window_rounding, grip.AngleMin12, grip.AngleMax12);
                window->DrawList->PathFillConvex(resize_grip_col[resize_grip_n]);
            }
        }

        // Borders
        RenderWindowOuterBorders(window);
    }
}

// Render title text, collapse button, close button
void ImGui::RenderWindowTitleBarContents(ImGuiWindow* window, const ImRect& title_bar_rect, const char* name, bool* p_open)
{
    ImGuiContext& g = *GImGui;
    ImGuiStyle& style = g.Style;
    ImGuiWindowFlags flags = window->Flags;

    const bool has_close_button = (p_open != NULL);
    const bool has_collapse_button = !(flags & ImGuiWindowFlags_NoCollapse) && (style.WindowMenuButtonPosition != ImGuiDir_None);

    // Close & Collapse button are on the Menu NavLayer and don't default focus (unless there's nothing else on that layer)
    // FIXME-NAV: Might want (or not?) to set the equivalent of ImGuiButtonFlags_NoNavFocus so that mouse clicks on standard title bar items don't necessarily set nav/keyboard ref?
    const ImGuiItemFlags item_flags_backup = g.CurrentItemFlags;
    g.CurrentItemFlags |= ImGuiItemFlags_NoNavDefaultFocus;
    window->DC.NavLayerCurrent = ImGuiNavLayer_Menu;

    // Layout buttons
    // FIXME: Would be nice to generalize the subtleties expressed here into reusable code.
    float pad_l = style.FramePadding.x;
    float pad_r = style.FramePadding.x;
    float button_sz = g.FontSize;
    ImVec2 close_button_pos;
    ImVec2 collapse_button_pos;
    if (has_close_button)
    {
        pad_r += button_sz;
        close_button_pos = ImVec2(title_bar_rect.Max.x - pad_r - style.FramePadding.x, title_bar_rect.Min.y);
    }
    if (has_collapse_button && style.WindowMenuButtonPosition == ImGuiDir_Right)
    {
        pad_r += button_sz;
        collapse_button_pos = ImVec2(title_bar_rect.Max.x - pad_r - style.FramePadding.x, title_bar_rect.Min.y);
    }
    if (has_collapse_button && style.WindowMenuButtonPosition == ImGuiDir_Left)
    {
        collapse_button_pos = ImVec2(title_bar_rect.Min.x + pad_l - style.FramePadding.x, title_bar_rect.Min.y);
        pad_l += button_sz;
    }

    // Collapse button (submitting first so it gets priority when choosing a navigation init fallback)
    if (has_collapse_button)
        if (CollapseButton(window->GetID("#COLLAPSE"), collapse_button_pos))
            window->WantCollapseToggle = true; // Defer actual collapsing to next frame as we are too far in the Begin() function

    // Close button
    if (has_close_button)
        if (CloseButton(window->GetID("#CLOSE"), close_button_pos))
            *p_open = false;

    window->DC.NavLayerCurrent = ImGuiNavLayer_Main;
    g.CurrentItemFlags = item_flags_backup;

    // Title bar text (with: horizontal alignment, avoiding collapse/close button, optional "unsaved document" marker)
    // FIXME: Refactor text alignment facilities along with RenderText helpers, this is WAY too much messy code..
    const float marker_size_x = (flags & ImGuiWindowFlags_UnsavedDocument) ? button_sz * 0.80f : 0.0f;
    const ImVec2 text_size = CalcTextSize(name, NULL, true) + ImVec2(marker_size_x, 0.0f);

    // As a nice touch we try to ensure that centered title text doesn't get affected by visibility of Close/Collapse button,
    // while uncentered title text will still reach edges correctly.
    if (pad_l > style.FramePadding.x)
        pad_l += g.Style.ItemInnerSpacing.x;
    if (pad_r > style.FramePadding.x)
        pad_r += g.Style.ItemInnerSpacing.x;
    if (style.WindowTitleAlign.x > 0.0f && style.WindowTitleAlign.x < 1.0f)
    {
        float centerness = ImSaturate(1.0f - ImFabs(style.WindowTitleAlign.x - 0.5f) * 2.0f); // 0.0f on either edges, 1.0f on center
        float pad_extend = ImMin(ImMax(pad_l, pad_r), title_bar_rect.GetWidth() - pad_l - pad_r - text_size.x);
        pad_l = ImMax(pad_l, pad_extend * centerness);
        pad_r = ImMax(pad_r, pad_extend * centerness);
    }

    ImRect layout_r(title_bar_rect.Min.x + pad_l, title_bar_rect.Min.y, title_bar_rect.Max.x - pad_r, title_bar_rect.Max.y);
    ImRect clip_r(layout_r.Min.x, layout_r.Min.y, ImMin(layout_r.Max.x + g.Style.ItemInnerSpacing.x, title_bar_rect.Max.x), layout_r.Max.y);
    if (flags & ImGuiWindowFlags_UnsavedDocument)
    {
        ImVec2 marker_pos;
        marker_pos.x = ImClamp(layout_r.Min.x + (layout_r.GetWidth() - text_size.x) * style.WindowTitleAlign.x + text_size.x, layout_r.Min.x, layout_r.Max.x);
        marker_pos.y = (layout_r.Min.y + layout_r.Max.y) * 0.5f;
        if (marker_pos.x > layout_r.Min.x)
        {
            RenderBullet(window->DrawList, marker_pos, GetColorU32(ImGuiCol_Text));
            clip_r.Max.x = ImMin(clip_r.Max.x, marker_pos.x - (int)(marker_size_x * 0.5f));
        }
    }
    //if (g.IO.KeyShift) window->DrawList->AddRect(layout_r.Min, layout_r.Max, IM_COL32(255, 128, 0, 255)); // [DEBUG]
    //if (g.IO.KeyCtrl) window->DrawList->AddRect(clip_r.Min, clip_r.Max, IM_COL32(255, 128, 0, 255)); // [DEBUG]
    RenderTextClipped(layout_r.Min, layout_r.Max, name, NULL, &text_size, style.WindowTitleAlign, &clip_r);
}

void ImGui::UpdateWindowParentAndRootLinks(ImGuiWindow* window, ImGuiWindowFlags flags, ImGuiWindow* parent_window)
{
    window->ParentWindow = parent_window;
    window->RootWindow = window->RootWindowPopupTree = window->RootWindowForTitleBarHighlight = window->RootWindowForNav = window;
    if (parent_window && (flags & ImGuiWindowFlags_ChildWindow) && !(flags & ImGuiWindowFlags_Tooltip))
        window->RootWindow = parent_window->RootWindow;
    if (parent_window && (flags & ImGuiWindowFlags_Popup))
        window->RootWindowPopupTree = parent_window->RootWindowPopupTree;
    if (parent_window && !(flags & ImGuiWindowFlags_Modal) && (flags & (ImGuiWindowFlags_ChildWindow | ImGuiWindowFlags_Popup)))
        window->RootWindowForTitleBarHighlight = parent_window->RootWindowForTitleBarHighlight;
    while (window->RootWindowForNav->Flags & ImGuiWindowFlags_NavFlattened)
    {
        IM_ASSERT(window->RootWindowForNav->ParentWindow != NULL);
        window->RootWindowForNav = window->RootWindowForNav->ParentWindow;
    }
}

// When a modal popup is open, newly created windows that want focus (i.e. are not popups and do not specify ImGuiWindowFlags_NoFocusOnAppearing)
// should be positioned behind that modal window, unless the window was created inside the modal begin-stack.
// In case of multiple stacked modals newly created window honors begin stack order and does not go below its own modal parent.
// - Window             // FindBlockingModal() returns Modal1
//   - Window           //                  .. returns Modal1
//   - Modal1           //                  .. returns Modal2
//      - Window        //                  .. returns Modal2
//          - Window    //                  .. returns Modal2
//          - Modal2    //                  .. returns Modal2
static ImGuiWindow* ImGui::FindBlockingModal(ImGuiWindow* window)
{
    ImGuiContext& g = *GImGui;
    if (g.OpenPopupStack.Size <= 0)
        return NULL;

    // Find a modal that has common parent with specified window. Specified window should be positioned behind that modal.
    for (int i = g.OpenPopupStack.Size - 1; i >= 0; i--)
    {
        ImGuiWindow* popup_window = g.OpenPopupStack.Data[i].Window;
        if (popup_window == NULL || !(popup_window->Flags & ImGuiWindowFlags_Modal))
            continue;
        if (!popup_window->Active && !popup_window->WasActive)      // Check WasActive, because this code may run before popup renders on current frame, also check Active to handle newly created windows.
            continue;
        if (IsWindowWithinBeginStackOf(window, popup_window))       // Window is rendered over last modal, no render order change needed.
            break;
        for (ImGuiWindow* parent = popup_window->ParentWindowInBeginStack->RootWindow; parent != NULL; parent = parent->ParentWindowInBeginStack->RootWindow)
            if (IsWindowWithinBeginStackOf(window, parent))
                return popup_window;                                // Place window above its begin stack parent.
    }
    return NULL;
}

// Push a new Dear ImGui window to add widgets to.
// - A default window called "Debug" is automatically stacked at the beginning of every frame so you can use widgets without explicitly calling a Begin/End pair.
// - Begin/End can be called multiple times during the frame with the same window name to append content.
// - The window name is used as a unique identifier to preserve window information across frames (and save rudimentary information to the .ini file).
//   You can use the "##" or "###" markers to use the same label with different id, or same id with different label. See documentation at the top of this file.
// - Return false when window is collapsed, so you can early out in your code. You always need to call ImGui::End() even if false is returned.
// - Passing 'bool* p_open' displays a Close button on the upper-right corner of the window, the pointed value will be set to false when the button is pressed.
bool ImGui::Begin(const char* name, bool* p_open, ImGuiWindowFlags flags)
{
    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    IM_ASSERT(name != NULL && name[0] != '\0');     // Window name required
    IM_ASSERT(g.WithinFrameScope);                  // Forgot to call ImGui::NewFrame()
    IM_ASSERT(g.FrameCountEnded != g.FrameCount);   // Called ImGui::Render() or ImGui::EndFrame() and haven't called ImGui::NewFrame() again yet

    // Find or create
    ImGuiWindow* window = FindWindowByName(name);
    const bool window_just_created = (window == NULL);
    if (window_just_created)
        window = CreateNewWindow(name, flags);
    else
        UpdateWindowInFocusOrderList(window, window_just_created, flags);

    // Automatically disable manual moving/resizing when NoInputs is set
    if ((flags & ImGuiWindowFlags_NoInputs) == ImGuiWindowFlags_NoInputs)
        flags |= ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize;

    if (flags & ImGuiWindowFlags_NavFlattened)
        IM_ASSERT(flags & ImGuiWindowFlags_ChildWindow);

    const int current_frame = g.FrameCount;
    const bool first_begin_of_the_frame = (window->LastFrameActive != current_frame);
    window->IsFallbackWindow = (g.CurrentWindowStack.Size == 0 && g.WithinFrameScopeWithImplicitWindow);

    // Update the Appearing flag
    bool window_just_activated_by_user = (window->LastFrameActive < current_frame - 1);   // Not using !WasActive because the implicit "Debug" window would always toggle off->on
    if (flags & ImGuiWindowFlags_Popup)
    {
        ImGuiPopupData& popup_ref = g.OpenPopupStack[g.BeginPopupStack.Size];
        window_just_activated_by_user |= (window->PopupId != popup_ref.PopupId); // We recycle popups so treat window as activated if popup id changed
        window_just_activated_by_user |= (window != popup_ref.Window);
    }
    window->Appearing = window_just_activated_by_user;
    if (window->Appearing)
        SetWindowConditionAllowFlags(window, ImGuiCond_Appearing, true);

    // Update Flags, LastFrameActive, BeginOrderXXX fields
    if (first_begin_of_the_frame)
    {
        window->Flags = (ImGuiWindowFlags)flags;
        window->LastFrameActive = current_frame;
        window->LastTimeActive = (float)g.Time;
        window->BeginOrderWithinParent = 0;
        window->BeginOrderWithinContext = (short)(g.WindowsActiveCount++);
    }
    else
    {
        flags = window->Flags;
    }
