/**
 * This file is part of Pretty Prinny.
 *
 * Pretty Prinny is free software : you can redistribute it
 * and/or modify it under the terms of the GNU General Public License
 * as published by The Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 *
 * Pretty Prinny is distributed in the hope that it will be useful,
 *
 * But WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Pretty Prinny.
 *
 *   If not, see <http://www.gnu.org/licenses/>.
 *
**/
#include <Windows.h>
#include <windowsx.h> // GET_X_LPARAM

#include "window.h"
#include "input.h"
#include "render.h"
#include "config.h"
#include "log.h"
#include "hook.h"

pp::window_state_s pp::window;

IsIconic_pfn            IsIconic_Original            = nullptr;
GetForegroundWindow_pfn GetForegroundWindow_Original = nullptr;
GetFocus_pfn            GetFocus_Original            = nullptr;
MoveWindow_pfn          MoveWindow_Original          = nullptr;
SetWindowPos_pfn        SetWindowPos_Original        = nullptr;
SetWindowLongA_pfn      SetWindowLongA_Original      = nullptr;

LRESULT
CALLBACK
DetourWindowProc ( _In_  HWND   hWnd,
                   _In_  UINT   uMsg,
                   _In_  WPARAM wParam,
                   _In_  LPARAM lParam );


bool windowed = false;

#include <dwmapi.h>
#pragma comment (lib, "dwmapi.lib")

BOOL
WINAPI
MoveWindow_Detour(
    _In_ HWND hWnd,
    _In_ int  X,
    _In_ int  Y,
    _In_ int  nWidth,
    _In_ int  nHeight,
    _In_ BOOL bRedraw )
{
  if (config.window.borderless && (! bRedraw) && windowed) {
    SetWindowPos (hWnd, HWND_TOP, X, Y, nWidth, nHeight, 0);
    return TRUE;
  } else {
    return MoveWindow_Original (hWnd, X, Y, nWidth, nHeight, bRedraw);
  }

#if 0
  dll_log.Log (L"[Window Mgr][!] MoveWindow (...)");

  pp::window.window_rect.left = X;
  pp::window.window_rect.top  = Y;

  pp::window.window_rect.right  = X + nWidth;
  pp::window.window_rect.bottom = Y + nHeight;

  if (! config.render.allow_background)
    return MoveWindow_Original (hWnd, X, Y, nWidth, nHeight, bRedraw);
  else
    return TRUE;
#endif
}



BOOL
WINAPI
SetWindowPos_Detour(
    _In_     HWND hWnd,
    _In_opt_ HWND hWndInsertAfter,
    _In_     int  X,
    _In_     int  Y,
    _In_     int  cx,
    _In_     int  cy,
    _In_     UINT uFlags)
{
  BOOL bRet;
  SetWindowLongA (hWnd, GWL_STYLE, 0);

  if (config.window.borderless /*&& windowed*/) {
    HMONITOR hMonitor = 
      MonitorFromWindow ( pp::RenderFix::hWndDevice,
                          MONITOR_DEFAULTTOPRIMARY );//MONITOR_DEFAULTTONEAREST );

    MONITORINFO mi = { 0 };
    mi.cbSize      = sizeof (mi);

    GetMonitorInfo (hMonitor, &mi);

    int left = mi.rcMonitor.left;
    int top  = mi.rcMonitor.top;

    HWND hWndDesktop = GetDesktopWindow ();
    RECT rectDesktop;
    GetWindowRect (hWndDesktop, &rectDesktop);

    if (cx > 1000 || (uFlags & SWP_NOSIZE))// || (uFlags & SWP_NOMOVE))
      cx = config.display.width == 0 ? rectDesktop.right - rectDesktop.left :
                                         config.display.width;

    if (cx <= 0)
      cx = 960;

    if (cy > 600 || (uFlags & SWP_NOSIZE))// || (uFlags & SWP_NOMOVE))
      cy = config.display.height == 0 ? rectDesktop.bottom - rectDesktop.top :
                                           config.display.width;
    if (cy <= 0)
      cy = 540;

    int width;
    int height;

    if (config.display.width > 0)
      width = config.display.width  > cx ? cx : config.display.width;
    else
      width = cx;

    if (config.display.height > 0)
      height = config.display.height > cy ? cy : config.display.height;
    else
      height = cy;

    if (config.window.center) {
      if (width < (mi.rcWork.right - mi.rcWork.left))
        left = ((mi.rcWork.right - mi.rcWork.left) - width) / 2;

      if (height < (mi.rcWork.bottom - mi.rcWork.top))
        top = ((mi.rcWork.bottom - mi.rcWork.top) - height) / 2;
    }

    bRet = SetWindowPos_Original ( pp::RenderFix::hWndDevice,
                                     HWND_TOP,
                                       left, top,
                                         width, height,
                                           SWP_SHOWWINDOW | SWP_FRAMECHANGED |
                                           SWP_DEFERERASE | SWP_ASYNCWINDOWPOS );


    ShowWindow          (pp::RenderFix::hWndDevice, SW_SHOW);
    SetActiveWindow     (pp::RenderFix::hWndDevice);
    //BringWindowToTop    (pp::RenderFix::hWndDevice);
    SetFocus            (pp::RenderFix::hWndDevice);
    SetForegroundWindow (pp::RenderFix::hWndDevice);
  } else {
    bRet = SetWindowPos_Original (hWnd, hWndInsertAfter, X, Y, cx, cy, uFlags);
  }
return bRet;
#if 0
  dll_log.Log ( L"[Window Mgr][!] SetWindowPos (...)");
#endif

  // Ignore this, because it's invalid.
  if (cy == 0 || cx == 0 && (! (uFlags & SWP_NOSIZE))) {
    pp::window.window_rect.right  = pp::window.window_rect.left + pp::RenderFix::width;
    pp::window.window_rect.bottom = pp::window.window_rect.top  + pp::RenderFix::height;

    return TRUE;
  }

#if 0
  dll_log.Log ( L"  >> Before :: Top-Left: [%d/%d], Bottom-Right: [%d/%d]",
                  pp::window.window_rect.left, pp::window.window_rect.top,
                    pp::window.window_rect.right, pp::window.window_rect.bottom );
#endif

  int original_width  = pp::window.window_rect.right -
                        pp::window.window_rect.left;
  int original_height = pp::window.window_rect.bottom -
                        pp::window.window_rect.top;

  if (! (uFlags & SWP_NOMOVE)) {
    pp::window.window_rect.left = X;
    pp::window.window_rect.top  = Y;
  }

  if (! (uFlags & SWP_NOSIZE)) {
    pp::window.window_rect.right  = pp::window.window_rect.left + cx;
    pp::window.window_rect.bottom = pp::window.window_rect.top  + cy;
  } else {
    pp::window.window_rect.right  = pp::window.window_rect.left +
                                       original_width;
    pp::window.window_rect.bottom = pp::window.window_rect.top  +
                                       original_height;
  }

#if 0
  dll_log.Log ( L"  >> After :: Top-Left: [%d/%d], Bottom-Right: [%d/%d]",
                  pp::window.window_rect.left, pp::window.window_rect.top,
                    pp::window.window_rect.right, pp::window.window_rect.bottom );
#endif

  //
  // Fix an invalid scenario that happens for some reason...
  //
  if (pp::window.window_rect.left == pp::window.window_rect.right)
    pp::window.window_rect.left = 0;
  if (pp::window.window_rect.left == pp::window.window_rect.right)
    pp::window.window_rect.right = pp::window.window_rect.left + pp::RenderFix::width;

  if (pp::window.window_rect.top == pp::window.window_rect.bottom)
    pp::window.window_rect.top = 0;
  if (pp::window.window_rect.top == pp::window.window_rect.bottom)
    pp::window.window_rect.bottom = pp::window.window_rect.top + pp::RenderFix::height;



#if 0
  // Let the game manage its window position...
  if (! config.render.borderless)
    return SetWindowPos_Original (hWnd, hWndInsertAfter, X, Y, cx, cy, uFlags);
  else
    return TRUE;
#else
  if (config.window.borderless) {
    if (! pp::RenderFix::fullscreen)
      return SetWindowPos_Original (hWnd, hWndInsertAfter, 0, 0, pp::RenderFix::width, pp::RenderFix::height, uFlags);
    else
      return TRUE;
  } else {
    return SetWindowPos_Original (hWnd, hWndInsertAfter, X, Y, cx, cy, uFlags);
  }
#endif
}

__declspec (noinline)
LONG
WINAPI
SetWindowLongA_Detour (
  _In_ HWND hWnd,
  _In_ int nIndex,
  _In_ LONG dwNewLong
)
{
  // Setup window message detouring as soon as a window is created..
  if (pp::window.WndProc_Original == nullptr && nIndex == GWL_STYLE) {
    pp::window.hwnd           = hWnd;//GetForegroundWindow_Original ();
    pp::RenderFix::hWndDevice = hWnd;//GetForegroundWindow_Original ();

    pp::window.WndProc_Original =
      (WNDPROC)GetWindowLongPtr (pp::RenderFix::hWndDevice, GWLP_WNDPROC);

    extern LRESULT
    CALLBACK
    DetourWindowProc ( _In_  HWND   hWnd,
                       _In_  UINT   uMsg,
                       _In_  WPARAM wParam,
                       _In_  LPARAM lParam );

    //SetWindowLongPtrW ( pp::RenderFix::hWndDevice,
    SetWindowLongA_Original ( pp::RenderFix::hWndDevice,
                                GWLP_WNDPROC,
                                  (LONG_PTR)DetourWindowProc );
  }

  if (nIndex == GWL_EXSTYLE || nIndex == GWL_STYLE) {
    dll_log.Log ( L"[Window Mgr] SetWindowLongA (0x%06X, %s, 0x%06X)",
                    hWnd,
              nIndex == GWL_EXSTYLE ? L"GWL_EXSTYLE" :
                                      L" GWL_STYLE ",
                      dwNewLong );
    if (nIndex == GWL_STYLE && dwNewLong == 0xCF0000)
      windowed = true;
    else
      windowed = false;

    if (config.window.borderless) {
      if (nIndex == GWL_STYLE)
        dwNewLong = WS_POPUP | WS_MINIMIZEBOX;
      if (nIndex == GWL_EXSTYLE)
        dwNewLong = WS_EX_APPWINDOW;
    }

    SetWindowLongA_Original (hWnd, nIndex, dwNewLong);
  }

#if 0
  if (config.window.borderless && nIndex == GWL_STYLE && windowed) {
    HMONITOR hMonitor = 
      MonitorFromWindow ( pp::RenderFix::hWndDevice,
                          MONITOR_DEFAULTTONEAREST );

    MONITORINFO mi = { 0 };
    mi.cbSize      = sizeof (mi);

    GetMonitorInfo (hMonitor, &mi);

    SetWindowPos_Original ( pp::RenderFix::hWndDevice,
                             HWND_TOP,
                              mi.rcMonitor.left,
                              mi.rcMonitor.top,
                                config.display.width <= 0 ? mi.rcMonitor.right  - mi.rcMonitor.left :
                                                            config.display.width,
                                config.display.height <= 0 ? mi.rcMonitor.bottom - mi.rcMonitor.top :
                                                             config.display.height,
                                  SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOSENDCHANGING );

  }

  ShowWindow          (pp::RenderFix::hWndDevice, SW_SHOW);
  SetActiveWindow     (pp::RenderFix::hWndDevice);
  BringWindowToTop    (pp::RenderFix::hWndDevice);
  SetFocus            (pp::RenderFix::hWndDevice);
  SetForegroundWindow (pp::RenderFix::hWndDevice);
#endif

  DwmEnableMMCSS (TRUE);

// TODO: Restore this functionality
#if 0
  // Override window styles
  if (nIndex == GWL_STYLE || nIndex == GWL_EXSTYLE) {
    // For proper return behavior
    DWORD dwOldStyle = GetWindowLong (hWnd, nIndex);

    // Allow the game to change its frame
    if (! config.window.borderless)
      return SetWindowLongA_Original (hWnd, nIndex, dwNewLong);

    return dwOldStyle;
  }
#endif

  if (nIndex != GWL_STYLE && nIndex != GWL_EXSTYLE)
    return SetWindowLongA_Original (hWnd, nIndex, dwNewLong);

  return dwNewLong;
}

void
pp::WindowManager::BorderManager::Disable (void)
{
  //dll_log.Log (L"BorderManager::Disable");

  window.borderless = true;

  //BringWindowToTop (window.hwnd);
  //SetActiveWindow  (window.hwnd);

  DWORD dwNewLong = window.style;

#if 0
  dwNewLong &= ~( WS_BORDER           | WS_CAPTION  | WS_THICKFRAME |
                  WS_OVERLAPPEDWINDOW | WS_MINIMIZE | WS_MAXIMIZE   |
                  WS_SYSMENU          | WS_GROUP );
#endif

  dwNewLong = WS_POPUP | WS_MINIMIZEBOX;

  SetWindowLongW (window.hwnd, GWL_STYLE,   dwNewLong);

  dwNewLong = window.style_ex;

#if 0
  dwNewLong &= ~( WS_EX_DLGMODALFRAME    | WS_EX_CLIENTEDGE    |
                  WS_EX_STATICEDGE       | WS_EX_WINDOWEDGE    |
                  WS_EX_OVERLAPPEDWINDOW | WS_EX_PALETTEWINDOW |
                  WS_EX_MDICHILD );
#endif

  dwNewLong = WS_EX_APPWINDOW;

  SetWindowLongW (window.hwnd, GWL_EXSTYLE, dwNewLong);

  AdjustWindow ();
}

void
pp::WindowManager::BorderManager::Enable (void)
{
  window.borderless = false;

  SetWindowLongW (window.hwnd, GWL_STYLE,   WS_OVERLAPPEDWINDOW);
  SetWindowLongW (window.hwnd, GWL_EXSTYLE, WS_EX_APPWINDOW);

  AdjustWindow ();
}

void
pp::WindowManager::BorderManager::AdjustWindow (void)
{
  HMONITOR hMonitor =
    MonitorFromWindow ( pp::RenderFix::hWndDevice,
                          MONITOR_DEFAULTTONEAREST );

  MONITORINFO mi = { 0 };
  mi.cbSize      = sizeof (mi);

  GetMonitorInfo (hMonitor, &mi);

  BringWindowToTop (window.hwnd);
  SetActiveWindow  (window.hwnd);

  if (pp::RenderFix::fullscreen) {
    //dll_log.Log (L"BorderManager::AdjustWindow - Fullscreen");

    SetWindowPos_Original ( pp::RenderFix::hWndDevice,
                              HWND_TOP,
                                mi.rcMonitor.left,
                                mi.rcMonitor.top,
                                  mi.rcMonitor.right  - mi.rcMonitor.left,
                                  mi.rcMonitor.bottom - mi.rcMonitor.top,
                                    SWP_FRAMECHANGED );
  } else {
    //dll_log.Log (L"BorderManager::AdjustWindow - Windowed");

    if (config.window.x_offset >= 0)
      window.window_rect.left  = mi.rcWork.left  + config.window.x_offset;
    else
      window.window_rect.right = mi.rcWork.right + config.window.x_offset + 1;


    if (config.window.y_offset >= 0)
      window.window_rect.top    = mi.rcWork.top    + config.window.y_offset;
    else
      window.window_rect.bottom = mi.rcWork.bottom + config.window.y_offset + 1;


    if (config.window.center && config.window.x_offset == 0 && config.window.y_offset == 0) {
      window.window_rect.left = max (0, ((mi.rcWork.right  - mi.rcWork.left) - pp::RenderFix::width)  / 2);
      window.window_rect.top  = max (0, ((mi.rcWork.bottom - mi.rcWork.top)  - pp::RenderFix::height) / 2);
    }


    if (config.window.x_offset >= 0)
      window.window_rect.right = window.window_rect.left  + pp::RenderFix::width;
    else
      window.window_rect.left  = window.window_rect.right - pp::RenderFix::width;


    if (config.window.y_offset >= 0)
      window.window_rect.bottom = window.window_rect.top    + pp::RenderFix::height;
    else
      window.window_rect.top    = window.window_rect.bottom - pp::RenderFix::height;


    AdjustWindowRect (&window.window_rect, GetWindowLong (pp::RenderFix::hWndDevice, GWL_STYLE), FALSE);


    SetWindowPos_Original ( pp::RenderFix::hWndDevice,
                              HWND_TOP,
                                window.window_rect.left, window.window_rect.top,
                                  window.window_rect.right  - window.window_rect.left,
                                  window.window_rect.bottom - window.window_rect.top,
                                    SWP_FRAMECHANGED );
  }

  ShowWindow (window.hwnd, SW_SHOW);
}

void
pp::WindowManager::BorderManager::Toggle (void)
{
  if (window.borderless)
    BorderManager::Enable ();
  else
    BorderManager::Disable ();
}

BOOL
WINAPI
IsIconic_Detour (HWND hWnd)
{
  if (config.render.allow_background)
    return FALSE;
  else
    return IsIconic_Original (hWnd);
}

HWND
WINAPI
GetForegroundWindow_Detour (void)
{
  //dll_log.Log (L"[Window Mgr][!] GetForegroundWindow (...)");

#if 0
  if (config.render.allow_background) {
    return pp::RenderFix::hWndDevice;
  }
#endif

  return GetForegroundWindow_Original ();
}

HWND
WINAPI
GetFocus_Detour (void)
{
  //dll_log.Log (L"[Window Mgr][!] GetFocus (...)");

  if (config.render.allow_background) {
    return pp::RenderFix::hWndDevice;
  }

  return GetFocus_Original ();
}

typedef VOID (WINAPI *keybd_event_pfn)(
  _In_ BYTE      bVk,
  _In_ BYTE      bScan,
  _In_ DWORD     dwFlags,
  _In_ ULONG_PTR dwExtraInfo
);

keybd_event_pfn keybd_event_Original = nullptr;

VOID
WINAPI
keybd_event_Detour (
  _In_ BYTE      bVk,
  _In_ BYTE      bScan,
  _In_ DWORD     dwFlags,
  _In_ ULONG_PTR dwExtraInfo )
{
#if 0
  if (! (dwFlags & KEYEVENTF_KEYUP)) {
    dll_log.Log (L"[ InputFix ] Killing NISA's keyboard flood!");
    return;
  }
#else
  return;
#endif

  keybd_event_Original (bVk, bScan, dwFlags, dwExtraInfo);
}

LRESULT
CALLBACK
DetourWindowProc ( _In_  HWND   hWnd,
                   _In_  UINT   uMsg,
                   _In_  WPARAM wParam,
                   _In_  LPARAM lParam )
{
  bool last_active = pp::window.active;

  pp::window.active = GetForegroundWindow_Original () == pp::window.hwnd/* ||
                      GetForegroundWindow_Original () == nullptr*/;

  bool console_visible   =
    pp::InputManager::Hooker::getInstance ()->isVisible ();

  bool background_render =
    config.render.allow_background && (! pp::window.active);

  //
  // The window activation state is changing, among other things we can take
  //   this opportunity to setup a special framerate limit.
  //
  if (pp::window.active != last_active) {
    SK_ICommandProcessor* pCommandProc =
      SK_GetCommandProcessor           ();

#if 0
    eTB_CommandResult     result       =
      pCommandProc->ProcessCommandLine ("TargetFPS");

    eTB_VarStub <float>* pOriginalLimit = (eTB_VarStub <float>*)result.getVariable ();
#endif

#if 0
    // Went from active to inactive (enforce background limit)
    if (! pp::window.active)
      pCommandProc->ProcessCommandFormatted ("TargetFPS %f", config.window.background_fps);

    // Went from inactive to active (restore foreground limit)
    else {
      pCommandProc->ProcessCommandFormatted ("TargetFPS %f", config.window.foreground_fps);
    }
#endif

#if 0
    // Unrestrict the mouse when the app is deactivated
    if ((! pp::window.active) && config.render.allow_background) {
      ClipCursor_Original (nullptr);
      SetCursorPos        (pp::window.cursor_pos.x, pp::window.cursor_pos.y);
      ShowCursor          (TRUE);
    }

    // Restore it when the app is activated
    else {
      GetCursorPos        (&pp::window.cursor_pos);
      ShowCursor          (FALSE);
      ClipCursor_Original (&pp::window.cursor_clip);
    }
#endif
  }

#if 0
  // Ignore this event
  if (uMsg == WM_MOUSEACTIVATE && config.render.allow_background) {
    return DefWindowProc (hWnd, uMsg, wParam, lParam);
  }

  // Allow the game to run in the background
  if (uMsg == WM_ACTIVATEAPP || uMsg == WM_ACTIVATE || uMsg == WM_NCACTIVATE /*|| uMsg == WM_MOUSEACTIVATE*/) {
    if (config.render.allow_background) {
      // We must fully consume one of these messages or audio will stop playing
      //   when the game loses focus, so do not simply pass this through to the
      //     default window procedure.
      return 0;//DefWindowProc (hWnd, uMsg, wParam, lParam);
    }
  }
#endif

  // Block keyboard input to the game while the console is visible
  if (console_visible/* || background_render*/) {
    // Only prevent the mouse from working while the window is in the bg
    //if (background_render && uMsg >= WM_MOUSEFIRST && uMsg <= WM_MOUSELAST)
      //return DefWindowProc (hWnd, uMsg, wParam, lParam);

    if (uMsg >= WM_KEYFIRST && uMsg <= WM_KEYLAST)
      return DefWindowProc (hWnd, uMsg, wParam, lParam);

    // Block RAW Input
    if (uMsg == WM_INPUT)
      return DefWindowProc (hWnd, uMsg, wParam, lParam);
  }

#if 0
  // Block the menu key from messing with stuff*
  if (config.input.block_left_alt &&
      (uMsg == WM_SYSKEYDOWN || uMsg == WM_SYSKEYUP)) {
    // Make an exception for Alt+Enter, for fullscreen mode toggle.
    //   F4 as well for exit
    if (wParam != VK_RETURN && wParam != VK_F4)
      return DefWindowProc (hWnd, uMsg, wParam, lParam);
  }
#endif

  // What an ugly mess, this is crazy :)
  if (config.input.cursor_mgmt) {
    extern bool IsControllerPluggedIn (UINT uJoyID);

    struct {
      POINTS pos      = { 0 }; // POINT (Short) - Not POINT plural ;)
      DWORD  sampled  = 0UL;
      bool   cursor   = true;

      int    init     = false;
      int    timer_id = 0x68992;
    } static last_mouse;

   auto ActivateCursor = [](bool changed = false)->
    bool
     {
       bool was_active = last_mouse.cursor;

       if (! last_mouse.cursor) {
         while (ShowCursor (TRUE) < 0) ;
         last_mouse.cursor = true;
       }

       if (changed)
         last_mouse.sampled = timeGetTime ();

       return (last_mouse.cursor != was_active);
     };

   auto DeactivateCursor = []()->
    bool
     {
       if (! last_mouse.cursor)
         return false;

       bool was_active = last_mouse.cursor;

       if (last_mouse.sampled <= timeGetTime () - config.input.cursor_timeout) {
         while (ShowCursor (FALSE) >= 0) ;
         last_mouse.cursor = false;
       }

       return (last_mouse.cursor != was_active);
     };

    if (! last_mouse.init) {
      SetTimer (hWnd, last_mouse.timer_id, config.input.cursor_timeout / 2, nullptr);
      last_mouse.init = true;
    }

    bool activation_event =
      (uMsg == WM_MOUSEMOVE);

    // Don't blindly accept that WM_MOUSEMOVE actually means the mouse moved...
    if (activation_event) {
      const short threshold = 2;

      // Filter out small movements
      if ( abs (last_mouse.pos.x - GET_X_LPARAM (lParam)) < threshold &&
           abs (last_mouse.pos.y - GET_Y_LPARAM (lParam)) < threshold )
        activation_event = false;

      last_mouse.pos = MAKEPOINTS (lParam);
    }

    // We cannot use WM_KEYDOWN as a condition, because the game is constantly
    //   flooding the message pump with this message !!!
    if (config.input.activate_on_kbd)
      activation_event |= ( uMsg == WM_CHAR       ||
                            uMsg == WM_SYSKEYDOWN ||
                            uMsg == WM_SYSKEYUP );

    if (activation_event)
      ActivateCursor (true);

    else if (uMsg == WM_TIMER && wParam == last_mouse.timer_id) {
      if (IsControllerPluggedIn (config.input.gamepad_slot))
        DeactivateCursor ();

      else
        ActivateCursor ();
    }
  }

#if 0
  // The game can process WM_CHAR, rather than these events that
  //   fire at a ridiculous rate...
  if (uMsg >= WM_KEYFIRST && uMsg <= WM_KEYLAST && uMsg != WM_CHAR)
    return 0;
#endif

  //
  // This isn't much better than what NISA was already doing
  //   with their keybd_event spam.
  //
  if (config.input.alias_wasd && (uMsg == WM_KEYDOWN || uMsg == WM_KEYUP))
  {
    DWORD dwFlags = (uMsg == WM_KEYUP ? KEYEVENTF_KEYUP :
                                        0UL);
    switch (wParam)
    {
      case VK_UP:
        keybd_event_Original ('W', 0, dwFlags, (ULONG_PTR)nullptr);
        break;
      case VK_DOWN:
        keybd_event_Original ('S', 0, dwFlags, (ULONG_PTR)nullptr);
        break;
      case VK_LEFT:
        keybd_event_Original ('A', 0, dwFlags, (ULONG_PTR)nullptr);
        break;
      case VK_RIGHT:
        keybd_event_Original ('D', 0, dwFlags, (ULONG_PTR)nullptr);
        break;
    }
  }

  return CallWindowProc (pp::window.WndProc_Original, hWnd, uMsg, wParam, lParam);
}





void
pp::WindowManager::Init (void)
{
  if (config.window.borderless)
    window.style = 0x10000000;
  else
    window.style = 0x90CA0000;

#if 0
  PPrinny_CreateDLLHook ( L"user32.dll", "SetWindowLongA",
                          SetWindowLongA_Detour,
                (LPVOID*)&SetWindowLongA_Original );
#endif

  PPrinny_CreateDLLHook ( L"user32.dll", "keybd_event",
                          keybd_event_Detour,
                (LPVOID*)&keybd_event_Original );

#if 0
  PPrinny_CreateDLLHook ( L"user32.dll", "SetWindowPos",
                          SetWindowPos_Detour,
                (LPVOID*)&SetWindowPos_Original );

 PPrinny_CreateDLLHook ( L"user32.dll", "MoveWindow",
                         MoveWindow_Detour,
               (LPVOID*)&MoveWindow_Original );

  PPrinny_CreateDLLHook ( L"user32.dll", "GetForegroundWindow",
                          GetForegroundWindow_Detour,
                (LPVOID*)&GetForegroundWindow_Original );
#endif

#if 0
// Used, but not for anything important...
  PPrinny_CreateDLLHook ( L"user32.dll", "GetFocus",
                          GetFocus_Detour,
                (LPVOID*)&GetFocus_Original );

  PPrinny_CreateDLLHook ( L"user32.dll", "IsIconic",
                          IsIconic_Detour,
                (LPVOID*)&IsIconic_Original );
#endif

  CommandProcessor* comm_proc = CommandProcessor::getInstance ();
}

void
pp::WindowManager::Shutdown (void)
{
}


pp::WindowManager::
  CommandProcessor::CommandProcessor (void)
{
  foreground_fps_ = PP_CreateVar (SK_IVariable::Float, &config.window.foreground_fps, this);
  background_fps_ = PP_CreateVar (SK_IVariable::Float, &config.window.background_fps, this);

  SK_ICommandProcessor* pCommandProc = SK_GetCommandProcessor ();

  pCommandProc->AddVariable ("Window.BackgroundFPS", background_fps_);
  pCommandProc->AddVariable ("Window.ForegroundFPS", foreground_fps_);

  // If the user has an FPS limit preference, set it up now...
  pCommandProc->ProcessCommandFormatted ("TargetFPS %f",        config.window.foreground_fps);
  pCommandProc->ProcessCommandFormatted ("LimiterTolerance %f", config.stutter.tolerance);
}

bool
  pp::WindowManager::
    CommandProcessor::OnVarChange (SK_IVariable* var, void* val)
{
  SK_ICommandProcessor* pCommandProc = SK_GetCommandProcessor ();

  bool known = false;

  if (var == background_fps_) {
    known = true;

    // Range validation
    if (val != nullptr && *(float *)val >= 0.0f) {
      config.window.background_fps = *(float *)val;

      // How this was changed while the window was inactive is a bit of a
      //   mystery, but whatever :P
      if ((! window.active))
        pCommandProc->ProcessCommandFormatted ("TargetFPS %f", *(float *)val);

      return true;
    }
  }

  if (var == foreground_fps_) {
    known = true;

    // Range validation
    if (val != nullptr && *(float *)val >= 0.0f) {
      config.window.foreground_fps = *(float *)val;

      // Immediately apply limiter changes
      if (window.active)
        pCommandProc->ProcessCommandFormatted ("TargetFPS %f", *(float *)val);

      return true;
    }
  }

  if (! known) {
    dll_log.Log ( L"[Window Mgr] UNKNOWN Variable Changed (%p --> %p)",
                    var,
                      val );
  }

  return false;
}

pp::WindowManager::CommandProcessor*
   pp::WindowManager::CommandProcessor::pCommProc = nullptr;