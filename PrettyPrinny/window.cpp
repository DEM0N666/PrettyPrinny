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
  dll_log.Log (L"[Window Mgr][!] MoveWindow (...)");

  pp::window.window_rect.left = X;
  pp::window.window_rect.top  = Y;

  pp::window.window_rect.right  = X + nWidth;
  pp::window.window_rect.bottom = Y + nHeight;

  if (! config.render.allow_background)
    return MoveWindow_Original (hWnd, X, Y, nWidth, nHeight, bRedraw);
  else
    return TRUE;
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

// PROBABLY REMOVE ME
#if 0
#include <tchar.h>
#include <tpcshrd.h>
#endif

LONG
WINAPI
SetWindowLongA_Detour (
  _In_ HWND hWnd,
  _In_ int nIndex,
  _In_ LONG dwNewLong
)
{
  if (nIndex == GWL_EXSTYLE || nIndex == GWL_STYLE) {
    dll_log.Log ( L"[Window Mgr] SetWindowLongA (0x%06X, %s, 0x%06X)",
                    hWnd,
              nIndex == GWL_EXSTYLE ? L"GWL_EXSTYLE" :
                                      L" GWL_STYLE ",
                      dwNewLong );
  }

  // Override window styles
  if (nIndex == GWL_STYLE || nIndex == GWL_EXSTYLE) {
    // For proper return behavior
    DWORD dwOldStyle = GetWindowLong (hWnd, nIndex);

    // Allow the game to change its frame
    if (! config.window.borderless)
      return SetWindowLongA_Original (hWnd, nIndex, dwNewLong);

    return dwOldStyle;
  }

  return SetWindowLongA_Original (hWnd, nIndex, dwNewLong);
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

  BringWindowToTop (window.hwnd);
  SetActiveWindow  (window.hwnd);
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

  if (config.render.allow_background) {
    return pp::RenderFix::hWndDevice;
  }

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


LRESULT
CALLBACK
DetourWindowProc ( _In_  HWND   hWnd,
                   _In_  UINT   uMsg,
                   _In_  WPARAM wParam,
                   _In_  LPARAM lParam )
{
  bool last_active = pp::window.active;
  pp::window.active = GetForegroundWindow_Original () == pp::window.hwnd ||
                      GetForegroundWindow_Original () == nullptr;

  bool console_visible   =
    pp::InputManager::Hooker::getInstance ()->isVisible ();

  bool background_render =
    config.render.allow_background && (! pp::window.active);

  //
  // The window activation state is changing, among other things we can take
  //   this opportunity to setup a special framerate limit.
  //
  if (pp::window.active != last_active) {
    eTB_CommandProcessor* pCommandProc =
      SK_GetCommandProcessor           ();

#if 0
    eTB_CommandResult     result       =
      pCommandProc->ProcessCommandLine ("TargetFPS");

    eTB_VarStub <float>* pOriginalLimit = (eTB_VarStub <float>*)result.getVariable ();
#endif
    // Went from active to inactive (enforce background limit)
    if (! pp::window.active)
      pCommandProc->ProcessCommandFormatted ("TargetFPS %f", config.window.background_fps);

    // Went from inactive to active (restore foreground limit)
    else {
      pCommandProc->ProcessCommandFormatted ("TargetFPS %f", config.window.foreground_fps);
    }

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

  // Block the menu key from messing with stuff
  if (config.input.block_left_alt &&
      (uMsg == WM_SYSKEYDOWN || uMsg == WM_SYSKEYUP)) {
    // Make an exception for Alt+Enter, for fullscreen mode toggle.
    //   F4 as well for exit
    if (wParam != VK_RETURN && wParam != VK_F4)
      return DefWindowProc (hWnd, uMsg, wParam, lParam);
  }

  return CallWindowProc (pp::window.WndProc_Original, hWnd, uMsg, wParam, lParam);
}



void
pp::WindowManager::Init (void)
{
#if 0
  if (config.window.borderless)
    window.style = 0x10000000;
  else
    window.style = 0x90CA0000;

  PPrinny_CreateDLLHook ( L"user32.dll", "SetWindowLongA",
                          SetWindowLongA_Detour,
                (LPVOID*)&SetWindowLongA_Original );

  PPrinny_CreateDLLHook ( L"user32.dll", "SetWindowPos",
                          SetWindowPos_Detour,
                (LPVOID*)&SetWindowPos_Original );

 PPrinny_CreateDLLHook ( L"user32.dll", "MoveWindow",
                         MoveWindow_Detour,
               (LPVOID*)&MoveWindow_Original );

// Not used by ToS
  PPrinny_CreateDLLHook ( L"user32.dll", "GetForegroundWindow",
                          GetForegroundWindow_Detour,
                (LPVOID*)&GetForegroundWindow_Original );

// Used, but not for anything important...
  PPrinny_CreateDLLHook ( L"user32.dll", "GetFocus",
                          GetFocus_Detour,
                (LPVOID*)&GetFocus_Original );
#endif


// Not used by ToS
#if 0
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
  foreground_fps_ = new eTB_VarStub <float> (&config.window.foreground_fps, this);
  background_fps_ = new eTB_VarStub <float> (&config.window.background_fps, this);

  eTB_CommandProcessor* pCommandProc = SK_GetCommandProcessor ();

  pCommandProc->AddVariable ("Window.BackgroundFPS", background_fps_);
  pCommandProc->AddVariable ("Window.ForegroundFPS", foreground_fps_);

  // If the user has an FPS limit preference, set it up now...
  pCommandProc->ProcessCommandFormatted ("TargetFPS %f",        config.window.foreground_fps);
  pCommandProc->ProcessCommandFormatted ("LimiterTolerance %f", config.stutter.tolerance);
}

bool
  pp::WindowManager::
    CommandProcessor::OnVarChange (eTB_Variable* var, void* val)
{
  eTB_CommandProcessor* pCommandProc = SK_GetCommandProcessor ();

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
   pp::WindowManager::CommandProcessor::pCommProc;