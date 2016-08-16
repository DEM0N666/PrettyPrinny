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
#define _CRT_SECURE_NO_WARNINGS
#include <Windows.h>
#include <cstdint>

#include <comdef.h>

#include <dinput.h>
#pragma comment (lib, "dxguid.lib")

#pragma comment (lib, "dinput8.lib")

#include "log.h"
#include "config.h"
#include "window.h"
#include "render.h"
#include "hook.h"

#include "input.h"

#include <process.h>

#include <mmsystem.h>
#pragma comment (lib, "winmm.lib")


extern HMODULE        hDLLMod;
LPDIRECTINPUTDEVICE8W g_DI8Key = nullptr;



///////////////////////////////////////////////////////////////////////////////
//
// General Utility Functions
//
///////////////////////////////////////////////////////////////////////////////
#if 0
void
PPrinny_ComputeAspectCoeffsEx (float& x, float& y, float& xoff, float& yoff, bool force=false)
{
  yoff = 0.0f;
  xoff = 0.0f;

  x = 1.0f;
  y = 1.0f;

  if (! (config.render.aspect_correction || force))
    return;

  config.render.aspect_ratio = pp::RenderFix::width / pp::RenderFix::height;
  float rescale              = ((16.0f / 9.0f) / config.render.aspect_ratio);

  // Wider
  if (config.render.aspect_ratio > (16.0f / 9.0f)) {
    int width = (16.0f / 9.0f) * pp::RenderFix::height;
    int x_off = (pp::RenderFix::width - width) / 2;

    x    = (float)pp::RenderFix::width / (float)width;
    xoff = x_off;

#if 0
    // Calculated height will be greater than we started with, so work
    //  in the wrong direction here...
    int height = (9.0f / 16.0f) * pp::RenderFix::width;
    y          = (float)pp::RenderFix::height / (float)height;
#endif

    yoff = config.scaling.mouse_y_offset;
  } else {
// No fix is needed in this direction
#if 0
    int height = (9.0f / 16.0f) * pp::RenderFix::width;
    int y_off  = (pp::RenderFix::height - height) / 2;

    y    = (float)pp::RenderFix::height / (float)height;
    yoff = y_off;
#endif
  }
}
#endif

#if 0
// Returns the original cursor position and stores the new one in pPoint
POINT
pp::InputManager::CalcCursorPos (LPPOINT pPoint, bool reverse)
{
  // Bail-out early if aspect ratio correction is disabled, or if the
  //   aspect ratio is less than or equal to 16:9.
  if  (! ( config.render.aspect_correction &&
           config.render.aspect_ratio > (16.0f / 9.0f) ) )
    return *pPoint;

  float xscale, yscale;
  float xoff,   yoff;

  PP_ComputeAspectCoeffsEx (xscale, yscale, xoff, yoff);

  yscale = xscale;

  if (! config.render.center_ui) {
    xscale = 1.0f;
    xoff   = 0.0f;
  }

  // Adjust system coordinates to game's (broken aspect ratio) coordinates
  if (! reverse) {
    pPoint->x = ((float)pPoint->x - xoff) * xscale;
    pPoint->y = ((float)pPoint->y - yoff) * yscale;
  }

  // Adjust game's (broken aspect ratio) coordinates to system coordinates
  else {
    pPoint->x = ((float)pPoint->x / xscale) + xoff;
    pPoint->y = ((float)pPoint->y / yscale) + yoff;
  }

  return *pPoint;
}
#endif

///////////////////////////////////////////////////////////////////////////////
//
// DirectInput 8
//
///////////////////////////////////////////////////////////////////////////////
typedef HRESULT (WINAPI *IDirectInput8_CreateDevice_pfn)(
  IDirectInput8       *This,
  REFGUID              rguid,
  LPDIRECTINPUTDEVICE *lplpDirectInputDevice,
  LPUNKNOWN            pUnkOuter
);

typedef HRESULT (WINAPI *DirectInput8Create_pfn)(
  HINSTANCE  hinst,
  DWORD      dwVersion,
  REFIID     riidltf,
  LPVOID    *ppvOut,
  LPUNKNOWN  punkOuter
);

typedef HRESULT (WINAPI *IDirectInputDevice8_GetDeviceState_pfn)(
  LPDIRECTINPUTDEVICE  This,
  DWORD                cbData,
  LPVOID               lpvData
);

typedef HRESULT (WINAPI *IDirectInputDevice8_SetCooperativeLevel_pfn)(
  LPDIRECTINPUTDEVICE  This,
  HWND                 hwnd,
  DWORD                dwFlags
);

DirectInput8Create_pfn
        DirectInput8Create_Original                      = nullptr;
LPVOID
        DirectInput8Create_Hook                          = nullptr;
IDirectInput8_CreateDevice_pfn
        IDirectInput8_CreateDevice_Original              = nullptr;
IDirectInputDevice8_GetDeviceState_pfn
        IDirectInputDevice8_GetDeviceState_Original      = nullptr;
IDirectInputDevice8_SetCooperativeLevel_pfn
        IDirectInputDevice8_SetCooperativeLevel_Original = nullptr;



#define DINPUT8_CALL(_Ret, _Call) {                                     \
  dll_log.LogEx (true, L"[   Input  ]  Calling original function: ");   \
  (_Ret) = (_Call);                                                     \
  _com_error err ((_Ret));                                              \
  if ((_Ret) != S_OK)                                                   \
    dll_log.LogEx (false, L"(ret=0x%04x - %s)\n", err.WCode (),         \
                                                  err.ErrorMessage ()); \
  else                                                                  \
    dll_log.LogEx (false, L"(ret=S_OK)\n");                             \
}

#define __PTR_SIZE   sizeof LPCVOID 
#define __PAGE_PRIVS PAGE_EXECUTE_READWRITE 
 
#define DI8_VIRTUAL_OVERRIDE(_Base,_Index,_Name,_Override,_Original,_Type) {   \
   void** vftable = *(void***)*_Base;                                          \
                                                                               \
   if (vftable [_Index] != _Override) {                                        \
     DWORD dwProtect;                                                          \
                                                                               \
     VirtualProtect (&vftable [_Index], __PTR_SIZE, __PAGE_PRIVS, &dwProtect); \
                                                                               \
     /*if (_Original == NULL)                                                */\
       _Original = (##_Type)vftable [_Index];                                  \
                                                                               \
     vftable [_Index] = _Override;                                             \
                                                                               \
     VirtualProtect (&vftable [_Index], __PTR_SIZE, dwProtect, &dwProtect);    \
                                                                               \
  }                                                                            \
 }



struct di8_keyboard_s {
  LPDIRECTINPUTDEVICE pDev = nullptr;
  uint8_t             state [512]; // Handle overrun just in case
} _dik;

struct di8_mouse_s {
  LPDIRECTINPUTDEVICE pDev = nullptr;
  DIMOUSESTATE2       state;
} _dim;

// I don't care about joysticks, let them continue working while
//   the window does not have focus...

HRESULT
WINAPI
IDirectInputDevice8_GetDeviceState_Detour ( LPDIRECTINPUTDEVICE        This,
                                            DWORD                      cbData,
                                            LPVOID                     lpvData )
{
  // This seems to be the source of some instability in the game.
  if (/*pp::RenderFix::tracer.log ||*/ This == _dik.pDev && cbData > 256 || lpvData == nullptr)
    dll_log.Log ( L"[   Input  ] Suspicious GetDeviceState - cbData: "
                  L"%lu, lpvData: %p",
                    cbData,
                      lpvData );

  // For input faking (keyboard)
  if ((! pp::window.active) && config.render.allow_background && This == _dik.pDev) {
    memcpy (lpvData, _dik.state, cbData);
    return S_OK;
  }

  // For input faking (mouse)
  if ((! pp::window.active) && config.render.allow_background && This == _dim.pDev) {
    memcpy (lpvData, &_dim.state, cbData);
    return S_OK;
  }

  HRESULT hr;
  hr = IDirectInputDevice8_GetDeviceState_Original ( This,
                                                       cbData,
                                                         lpvData );

  if (SUCCEEDED (hr)) {
    if (pp::window.active && This == _dim.pDev) {
      memcpy (&_dim.state, lpvData, cbData);
    }

    else if (pp::window.active && This == _dik.pDev) {
      memcpy (&_dik.state, lpvData, cbData);
    }
  }

  return hr;
}

HRESULT
WINAPI
IDirectInputDevice8_SetCooperativeLevel_Detour ( LPDIRECTINPUTDEVICE  This,
                                                 HWND                 hwnd,
                                                 DWORD                dwFlags )
{
  if (config.input.block_windows)
    dwFlags |= DISCL_NOWINKEY;

  if (config.render.allow_background) {
    dwFlags &= ~DISCL_EXCLUSIVE;
    dwFlags &= ~DISCL_BACKGROUND;

    dwFlags |= DISCL_NONEXCLUSIVE;
    dwFlags |= DISCL_FOREGROUND;

    return IDirectInputDevice8_SetCooperativeLevel_Original (This, hwnd, dwFlags);
  }

  return IDirectInputDevice8_SetCooperativeLevel_Original (This, hwnd, dwFlags);
}

HRESULT
WINAPI
IDirectInput8_CreateDevice_Detour ( IDirectInput8       *This,
                                    REFGUID              rguid,
                                    LPDIRECTINPUTDEVICE *lplpDirectInputDevice,
                                    LPUNKNOWN            pUnkOuter )
{
  const wchar_t* wszDevice = (rguid == GUID_SysKeyboard) ? L"Default System Keyboard" :
                                (rguid == GUID_SysMouse) ? L"Default System Mouse" :
                                                           L"Other Device";

  dll_log.Log ( L"[   Input  ][!] IDirectInput8::CreateDevice (%08Xh, %s, %08Xh, %08Xh)",
                  This,
                    wszDevice,
                      lplpDirectInputDevice,
                        pUnkOuter );

  HRESULT hr;
  DINPUT8_CALL ( hr,
                  IDirectInput8_CreateDevice_Original ( This,
                                                         rguid,
                                                          lplpDirectInputDevice,
                                                           pUnkOuter ) );

  if (SUCCEEDED (hr)) {
    void** vftable = *(void***)*lplpDirectInputDevice;

    if (rguid == GUID_SysKeyboard) {
#if 1
      PPrinny_CreateFuncHook ( L"IDirectInputDevice8::SetCooperativeLevel",
                               vftable [13],
                               IDirectInputDevice8_SetCooperativeLevel_Detour,
                     (LPVOID*)&IDirectInputDevice8_SetCooperativeLevel_Original );

      PPrinny_EnableHook (vftable [13]);
#else
      DI8_VIRTUAL_OVERRIDE ( lplpDirectInputDevice, 13,
                            L"IDirectInputDevice8::SetCooperativeLevel",
                            IDirectInputDevice8_SetCooperativeLevel_Detour,
                            IDirectInputDevice8_SetCooperativeLevel_Original,
                            IDirectInputDevice8_SetCooperativeLevel_pfn );
#endif
    }

    if (rguid == GUID_SysMouse)
      _dim.pDev = *lplpDirectInputDevice;
    else if (rguid == GUID_SysKeyboard)
      _dik.pDev = *lplpDirectInputDevice;
  }

  return hr;
}

HRESULT
WINAPI
DirectInput8Create_Detour ( HINSTANCE  hinst,
                            DWORD      dwVersion,
                            REFIID     riidltf,
                            LPVOID    *ppvOut,
                            LPUNKNOWN  punkOuter )
{
  dll_log.Log ( L"[   Input  ][!] DirectInput8Create (0x%X, %lu, ..., %08Xh, %08Xh)",
                  hinst, dwVersion, /*riidltf,*/ ppvOut, punkOuter );

  HRESULT hr;
  DINPUT8_CALL (hr,
    DirectInput8Create_Original ( hinst,
                                    dwVersion,
                                      riidltf,
                                        ppvOut,
                                          punkOuter ));

  if (hinst != GetModuleHandle (nullptr)) {
    dll_log.Log (L"[   Input  ] >> A third-party DLL is manipulating DirectInput 8; will not hook.");
    return hr;
  }

  // Avoid multiple hooks for third-party compatibility
  static bool hooked = false;

  if (SUCCEEDED (hr) && (! hooked)) {
#if 1
    void** vftable = *(void***)*ppvOut;

    PPrinny_CreateFuncHook ( L"IDirectInput8::CreateDevice",
                             vftable [3],
                             IDirectInput8_CreateDevice_Detour,
                   (LPVOID*)&IDirectInput8_CreateDevice_Original );

    PPrinny_EnableHook (vftable [3]);
#else
     DI8_VIRTUAL_OVERRIDE ( ppvOut, 3,
                            L"IDirectInput8::CreateDevice",
                            IDirectInput8_CreateDevice_Detour,
                            IDirectInput8_CreateDevice_Original,
                            IDirectInput8_CreateDevice_pfn );
#endif
    hooked = true;
  }

  return hr;
}





typedef struct _XINPUT_GAMEPAD {
  WORD  wButtons;
  BYTE  bLeftTrigger;
  BYTE  bRightTrigger;
  SHORT sThumbLX;
  SHORT sThumbLY;
  SHORT sThumbRX;
  SHORT sThumbRY;
} XINPUT_GAMEPAD, *PXINPUT_GAMEPAD;

typedef struct _XINPUT_STATE {
  DWORD          dwPacketNumber;
  XINPUT_GAMEPAD Gamepad;
} XINPUT_STATE, *PXINPUT_STATE;

typedef DWORD (WINAPI *XInputGetState_pfn)(
  _In_  DWORD        dwUserIndex,
  _Out_ XINPUT_STATE *pState
);

XInputGetState_pfn XInputGetState = nullptr;

///////////////////////////////////////////////////////////////////////////////
//
// WinMM Input APIs
//
///////////////////////////////////////////////////////////////////////////////

typedef MMRESULT (WINAPI *joyGetDevCapsA_pfn)(
   UINT_PTR   uJoyID,
   LPJOYCAPSA pjc,
   UINT       cbjc
);

typedef UINT (WINAPI *joyGetNumDevs_pfn)(
  void
);

typedef MMRESULT (WINAPI *joyGetPos_pfn) (
   UINT      uJoyID,
   LPJOYINFO pji
);

typedef MMRESULT (WINAPI *joyGetPosEx_pfn)(
   UINT        uJoyID,
   LPJOYINFOEX pji
);

joyGetDevCapsA_pfn joyGetDevCapsA_Original = nullptr;
joyGetNumDevs_pfn  joyGetNumDevs_Original  = nullptr;
joyGetPos_pfn      joyGetPos_Original      = nullptr;
joyGetPosEx_pfn    joyGetPosEx_Original    = nullptr;

#define XINPUT_GAMEPAD_DPAD_UP          0x0001
#define XINPUT_GAMEPAD_DPAD_DOWN        0x0002
#define XINPUT_GAMEPAD_DPAD_LEFT        0x0004
#define XINPUT_GAMEPAD_DPAD_RIGHT       0x0008
#define XINPUT_GAMEPAD_START            0x0010
#define XINPUT_GAMEPAD_BACK             0x0020
#define XINPUT_GAMEPAD_LEFT_THUMB       0x0040
#define XINPUT_GAMEPAD_RIGHT_THUMB      0x0080
#define XINPUT_GAMEPAD_LEFT_SHOULDER    0x0100
#define XINPUT_GAMEPAD_RIGHT_SHOULDER   0x0200
#define XINPUT_GAMEPAD_A                0x1000
#define XINPUT_GAMEPAD_B                0x2000
#define XINPUT_GAMEPAD_X                0x4000
#define XINPUT_GAMEPAD_Y                0x8000

__declspec (noinline)
MMRESULT
WINAPI
joyGetDevCapsA_Detour (UINT_PTR uJoyID, LPJOYCAPSA pjc, UINT cbjc)
{
////  dll_log.Log (L"joyGetDevCapsA");

  XINPUT_STATE xstate;
  DWORD dwRet = XInputGetState (uJoyID, &xstate);

  if (dwRet == ERROR_DEVICE_NOT_CONNECTED)
    return MMSYSERR_NODRIVER;

  int num_buttons = 0;

  if (xstate.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_UP)    ++num_buttons;
  if (xstate.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_DOWN)  ++num_buttons;
  if (xstate.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_LEFT)  ++num_buttons;
  if (xstate.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_RIGHT) ++num_buttons;

  if (xstate.Gamepad.wButtons & XINPUT_GAMEPAD_START)      ++num_buttons;
  if (xstate.Gamepad.wButtons & XINPUT_GAMEPAD_BACK)       ++num_buttons;

  if (xstate.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_THUMB)  ++num_buttons;
  if (xstate.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_THUMB) ++num_buttons;

  if (xstate.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER)  ++num_buttons;
  if (xstate.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER) ++num_buttons;

  if (xstate.Gamepad.wButtons & XINPUT_GAMEPAD_A) ++num_buttons;
  if (xstate.Gamepad.wButtons & XINPUT_GAMEPAD_B) ++num_buttons;
  if (xstate.Gamepad.wButtons & XINPUT_GAMEPAD_X) ++num_buttons;
  if (xstate.Gamepad.wButtons & XINPUT_GAMEPAD_Y) ++num_buttons;

  pjc->wMaxButtons = 14;
  pjc->wNumButtons = num_buttons;

  pjc->wMaxAxes = 6; // 6 if you want the left/right trigger
  pjc->wNumAxes = 4;

  pjc->wCaps = JOYCAPS_HASZ | JOYCAPS_HASR;

  // For Left/Right Triggers
  //pjc->wCaps |= JOYCAPS_HAS_U | JOYCAPS_HASV;

  pjc->wMid = 0x666;
  pjc->wPid = 0x666;

  strcpy (pjc->szPname,  "Pretty Prinny XInput Wrapper");
  strcpy (pjc->szRegKey, "");
  strcpy (pjc->szOEMVxD, "");

  // Left Thumb
  pjc->wXmin =  0;//-32768;
  pjc->wXmax =  32768+32767;

  pjc->wYmin =  0;//-32768;
  pjc->wYmax =  32768+32767;

  // Right Thumb
  pjc->wZmin = 0;//-32768;
  pjc->wZmax = 0;// 32767;

  pjc->wRmin = 0;//-32768;
  pjc->wRmax = 0;// 32767;

  // Left Trigger
  pjc->wUmin =  0;//0;
  pjc->wUmax =  0;//255;

  // Right Trigger
  pjc->wVmin =  0;//0;
  pjc->wVmax =  0;//255;

  // DON'T CARE, BUT WHATEVER
  pjc->wPeriodMin = 1;
  pjc->wPeriodMax = UINT_MAX;

  return JOYERR_NOERROR; 

  //return joyGetDevCapsA_Original (uJoyID, pjc, cbjc);
}

__declspec (noinline)
UINT
WINAPI
joyGetNumDevs_Detour (void)
{
////  dll_log.Log (L"joyGetNumDevs");

  return 4;

  //return joyGetNumDevs_Original ();
}

bool
IsControllerPluggedIn (UINT uJoyID)
{
  if (uJoyID == (UINT)-1)
    return true;

  XINPUT_STATE xstate;

  static DWORD last_poll = timeGetTime ();
  static DWORD dwRet     = XInputGetState (uJoyID, &xstate);

  // This function is actually a performance hazzard when no controllers
  //   are plugged in, so ... throttle the sucker.
  if (last_poll < timeGetTime () - 500UL)
    dwRet = XInputGetState (uJoyID, &xstate);

  if (dwRet == ERROR_DEVICE_NOT_CONNECTED)
    return false;

  return true;
}

__declspec (noinline)
MMRESULT
WINAPI
joyGetPos_Detour (UINT uJoyID, LPJOYINFO pji)
{
  //dll_log.Log (L"joyGetPos");

  XINPUT_STATE xstate;
  DWORD dwRet = XInputGetState (uJoyID, &xstate);

  if (dwRet == ERROR_DEVICE_NOT_CONNECTED)
    return JOYERR_UNPLUGGED;

  pji->wXpos = 32768 + xstate.Gamepad.sThumbLX;
  pji->wYpos = 32768 + xstate.Gamepad.sThumbLY;
  pji->wZpos = 0;

  if (xstate.Gamepad.wButtons & XINPUT_GAMEPAD_A) pji->wButtons |= JOY_BUTTON1;
  if (xstate.Gamepad.wButtons & XINPUT_GAMEPAD_B) pji->wButtons |= JOY_BUTTON2;
  if (xstate.Gamepad.wButtons & XINPUT_GAMEPAD_X) pji->wButtons |= JOY_BUTTON3;
  if (xstate.Gamepad.wButtons & XINPUT_GAMEPAD_Y) pji->wButtons |= JOY_BUTTON4;

  return JOYERR_NOERROR;

  return joyGetPos_Original (uJoyID, pji);
}

__declspec (noinline)
MMRESULT
WINAPI
joyGetPosEx_Detour (UINT uJoyID, LPJOYINFOEX pji)
{
  //dll_log.Log (L"joyGetPosEx");
  //return MMSYSERR_NODRIVER;

  XINPUT_STATE xstate;
  DWORD dwRet = XInputGetState (uJoyID, &xstate);

  if (dwRet == ERROR_DEVICE_NOT_CONNECTED)
    return JOYERR_UNPLUGGED;

  pji->dwXpos = 32768 + xstate.Gamepad.sThumbLX;
  pji->dwYpos = 32768 + xstate.Gamepad.sThumbLY;

  pji->dwZpos = 0;//xstate.Gamepad.sThumbRX;
  pji->dwRpos = 0;// xstate.Gamepad.sThumbRY;

  pji->dwUpos = 0;//xstate.Gamepad.bLeftTrigger;
  pji->dwVpos = 0;//xstate.Gamepad.bRightTrigger;

  if (xstate.Gamepad.wButtons & XINPUT_GAMEPAD_A) pji->dwButtons |= JOY_BUTTON1;
  if (xstate.Gamepad.wButtons & XINPUT_GAMEPAD_B) pji->dwButtons |= JOY_BUTTON2;
  if (xstate.Gamepad.wButtons & XINPUT_GAMEPAD_X) pji->dwButtons |= JOY_BUTTON3;
  if (xstate.Gamepad.wButtons & XINPUT_GAMEPAD_Y) pji->dwButtons |= JOY_BUTTON4;

  if (xstate.Gamepad.wButtons & XINPUT_GAMEPAD_START) pji->dwButtons |= JOY_BUTTON5;
  if (xstate.Gamepad.wButtons & XINPUT_GAMEPAD_BACK)  pji->dwButtons |= JOY_BUTTON6;

  if (xstate.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_THUMB)  pji->dwButtons |= JOY_BUTTON7;
  if (xstate.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_THUMB) pji->dwButtons |= JOY_BUTTON8;

  if (xstate.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER)  pji->dwButtons |= JOY_BUTTON9;
  if (xstate.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER) pji->dwButtons |= JOY_BUTTON10;

  pji->dwPOV = 0;

  if (xstate.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_UP) {
    pji->dwPOV     |= JOY_POVFORWARD;
    pji->dwButtons |= JOY_BUTTON11;
  }

  if (xstate.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_DOWN) {
    pji->dwPOV     |= JOY_POVBACKWARD;
    pji->dwButtons |= JOY_BUTTON12;
  }

  if (xstate.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_LEFT) {
    pji->dwPOV     |= JOY_POVLEFT;
    pji->dwButtons |= JOY_BUTTON13;
  }

  if (xstate.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_RIGHT) {
    pji->dwPOV     |= JOY_POVRIGHT;
    pji->dwButtons |= JOY_BUTTON14;
  }

  if (pji->dwPOV == 0)
    pji->dwPOV = JOY_POVCENTERED;

  pji->dwFlags = pji->dwPOV == JOY_POVCENTERED ? JOY_RETURNCENTERED : JOY_RETURNPOV | JOY_RETURNX | JOY_RETURNY | JOY_RETURNBUTTONS;

  return JOYERR_NOERROR;

  //return joyGetPosEx_Original (uJoyID, pji);
}

///////////////////////////////////////////////////////////////////////////////
//
// User32 Input APIs
//
///////////////////////////////////////////////////////////////////////////////
typedef UINT (WINAPI *GetRawInputData_pfn)(
  _In_      HRAWINPUT hRawInput,
  _In_      UINT      uiCommand,
  _Out_opt_ LPVOID    pData,
  _Inout_   PUINT     pcbSize,
  _In_      UINT      cbSizeHeader
);

typedef BOOL (WINAPI *GetCursorInfo_pfn)(
  _Inout_ PCURSORINFO pci
);

typedef BOOL (WINAPI *GetCursorPos_pfn)(
  _Out_ LPPOINT lpPoint
);

GetAsyncKeyState_pfn GetAsyncKeyState_Original = nullptr;
GetRawInputData_pfn  GetRawInputData_Original  = nullptr;

GetCursorInfo_pfn    GetCursorInfo_Original    = nullptr;
GetCursorPos_pfn     GetCursorPos_Original     = nullptr;
SetCursorPos_pfn     SetCursorPos_Original     = nullptr;

ClipCursor_pfn       ClipCursor_Original       = nullptr;



UINT
WINAPI
GetRawInputData_Detour (_In_      HRAWINPUT hRawInput,
                        _In_      UINT      uiCommand,
                        _Out_opt_ LPVOID    pData,
                        _Inout_   PUINT     pcbSize,
                        _In_      UINT      cbSizeHeader)
{
  if (config.input.block_all_keys) {
    *pcbSize = 0;
    return 0;
  }

  int size = GetRawInputData_Original (hRawInput, uiCommand, pData, pcbSize, cbSizeHeader);

  // Block keyboard input to the game while the console is active
  if (pp::InputManager::Hooker::getInstance ()->isVisible () && uiCommand == RID_INPUT) {
    *pcbSize = 0;
    return 0;
  }

  return size;
}

SHORT
WINAPI
GetAsyncKeyState_Detour (_In_ int vKey)
{
#define PPrinny_ConsumeVKey(vKey) { GetAsyncKeyState_Original(vKey); return 0; }

#if 0
  // Window is not active, but we are faking it...
  if ((! pp::window.active) && config.render.allow_background)
    PPrinny_ConsumeVKey (vKey);
#endif

  // Block keyboard input to the game while the console is active
  if (pp::InputManager::Hooker::getInstance ()->isVisible ()) {
    PPrinny_ConsumeVKey (vKey);
  }

#if 0
  // Block Left Alt
  if (vKey == VK_LMENU)
    if (config.input.block_left_alt)
      PPrinny_ConsumeVKey (vKey);

  // Block Left Ctrl
  if (vKey == VK_LCONTROL)
    if (config.input.block_left_ctrl)
      PPrinny_ConsumeVKey (vKey);
#endif

  return GetAsyncKeyState_Original (vKey);
}

BOOL
WINAPI
ClipCursor_Detour (const RECT *lpRect)
{
  if (lpRect != nullptr)
    pp::window.cursor_clip = *lpRect;

  if (pp::window.active) {
    return ClipCursor_Original (lpRect);
  } else {
    return ClipCursor_Original (nullptr);
  }
}

#if 0
BOOL
WINAPI
GetCursorInfo_Detour (PCURSORINFO pci)
{
  BOOL ret = GetCursorInfo_Original (pci);

  // Correct the cursor position for Aspect Ratio
  if (config.render.aspect_correction && config.render.aspect_ratio > (16.0f / 9.0f)) {
    POINT pt;

    pt.x = pci->ptScreenPos.x;
    pt.y = pci->ptScreenPos.y;

    pp::InputManager::CalcCursorPos (&pt);

    pci->ptScreenPos.x = pt.x;
    pci->ptScreenPos.y = pt.y;
  }

  return ret;
}
#endif

BOOL
WINAPI
SetCursorPos_Detour (_In_ int X, _In_ int Y)
{
  //POINT pt { X, Y };
  //pp::InputManager::CalcCursorPos (&pt);

  // DO NOT let this stupid game capture the cursor while
  //   it is not active. UGH!
  if (pp::window.active)
    return SetCursorPos_Original (X, Y);
  else
    return TRUE;
}

// Don't care
LPVOID SetPhysicalCursorPos_Original = nullptr;

BOOL
WINAPI
SetPhysicalCursorPos_Detour
  (
    _In_ int X,
    _In_ int Y
  )
{
  return TRUE;
}

#if 0
BOOL
WINAPI
GetCursorPos_Detour (LPPOINT lpPoint)
{
  BOOL ret = GetCursorPos_Original (lpPoint);

  // Correct the cursor position for Aspect Ratio
  if (config.render.aspect_correction && config.render.aspect_ratio > (16.0f / 9.0f))
    pp::InputManager::CalcCursorPos (lpPoint);

  return ret;
}
#endif

void
HookRawInput (void)
{
#if 0
  // Defer installation of this hook until DirectInput8 is setup
  if (GetRawInputData_Original == nullptr) {
    dll_log.LogEx (true, L"[   Input  ] Installing Deferred Hook: \"GetRawInputData (...)\"... ");
    MH_STATUS status =
      PPrinny_CreateDLLHook ( L"user32.dll", "GetRawInputData",
                              GetRawInputData_Detour,
                    (LPVOID*)&GetRawInputData_Original );
   dll_log.LogEx (false, L"%hs\n", MH_StatusToString (status));
  }
#endif
}



void
pp::InputManager::Init (void)
{
  //
  // Win32 API Input Hooks
  //

  HookRawInput ();

  PPrinny_CreateDLLHook ( L"user32.dll", "GetAsyncKeyState",
                          GetAsyncKeyState_Detour,
                (LPVOID*)&GetAsyncKeyState_Original );

  HMODULE hModXInput13 = LoadLibraryW (L"XInput1_3.dll");

  XInputGetState =
    (XInputGetState_pfn)
      GetProcAddress (hModXInput13, "XInputGetState");

  if (config.input.wrap_xinput) {
    //
    // MMSystem Input Hooks
    //
    PPrinny_CreateDLLHook ( L"winmm.dll", "joyGetDevCapsA",
                            joyGetDevCapsA_Detour,
                  (LPVOID*)&joyGetDevCapsA_Original );

    PPrinny_CreateDLLHook ( L"winmm.dll", "joyGetNumDevs",
                            joyGetNumDevs_Detour,
                  (LPVOID*)&joyGetNumDevs_Original );

    PPrinny_CreateDLLHook ( L"winmm.dll", "joyGetPos",
                            joyGetPos_Detour,
                  (LPVOID*)&joyGetPos_Original );

    PPrinny_CreateDLLHook ( L"winmm.dll", "joyGetPosEx",
                            joyGetPosEx_Detour,
                  (LPVOID*)&joyGetPosEx_Original );
  }

  pp::InputManager::Hooker* pHook =
    pp::InputManager::Hooker::getInstance ();

  pHook->Start ();
}

void
pp::InputManager::Shutdown (void)
{
  pp::InputManager::Hooker* pHook = pp::InputManager::Hooker::getInstance ();

  pHook->End ();

  if (g_DI8Key != nullptr) {
    g_DI8Key->Release ();
    g_DI8Key = nullptr;
  }
}


void
pp::InputManager::Hooker::Start (void)
{
  hMsgPump =
    (HANDLE)
      _beginthreadex ( nullptr,
                         0,
                           Hooker::MessagePump,
                             &hooks,
                               0x00,
                                 nullptr );
}

void
pp::InputManager::Hooker::End (void)
{
  TerminateThread     (hMsgPump, 0);
  UnhookWindowsHookEx (hooks.keyboard);
  UnhookWindowsHookEx (hooks.mouse);
}

std::string console_text;
std::string mod_text ("");

void
pp::InputManager::Hooker::Draw (void)
{
  typedef BOOL (__stdcall *SK_DrawExternalOSD_pfn)(std::string app_name, std::string text);

  static HMODULE               hMod =
    GetModuleHandle (config.system.injector.c_str ());
  static SK_DrawExternalOSD_pfn SK_DrawExternalOSD
    =
    (SK_DrawExternalOSD_pfn)GetProcAddress (hMod, "SK_DrawExternalOSD");

  std::string output;

  static DWORD last_time = timeGetTime ();
  static bool  carret    = true;

  if (visible) {
    output += text;

    // Blink the Carret
    if (timeGetTime () - last_time > 333) {
      carret = ! carret;

      last_time = timeGetTime ();
    }

    if (carret)
      output += "-";

    // Show Command Results
    if (command_issued) {
      output += "\n";
      output += result_str;
    }
  }

  console_text = output;

  output += "\n";
  output += mod_text;

  SK_DrawExternalOSD ("Pretty Prinny", output.c_str ());
}

unsigned int
__stdcall
pp::InputManager::Hooker::MessagePump (LPVOID hook_ptr)
{
  hooks_s* pHooks = (hooks_s *)hook_ptr;

  ZeroMemory (text, 4096);

  text [0] = '>';

  extern    HMODULE hDLLMod;

  DWORD dwThreadId;

  int hits = 0;

  DWORD dwTime = timeGetTime ();

  while (true) {
    DWORD dwProc;

    dwThreadId =
      GetWindowThreadProcessId (pp::window.hwnd, &dwProc);

    // Ugly hack, but a different window might be in the foreground...
    if (pp::window.hwnd == nullptr || (! pp::RenderFix::draw_state.frames) || dwProc != GetCurrentProcessId ()) {
      //dll_log.Log (L" *** Tried to hook the wrong process!!!");
      Sleep (83UL);
      continue;
    }

    break;
  }

  // Defer initialization of the Window Message redirection stuff until
  //   we have an actual window!
  eTB_CommandProcessor* pCommandProc = SK_GetCommandProcessor ();
  pCommandProc->ProcessCommandFormatted ("TargetFPS %f", config.window.foreground_fps);


  dll_log.Log ( L"[   Input  ] # Found window in %03.01f seconds, "
                    L"installing keyboard hook...",
                  (float)(timeGetTime () - dwTime) / 1000.0f );

  dwTime = timeGetTime ();
  hits   = 1;

  while (! (pHooks->keyboard = SetWindowsHookEx ( WH_KEYBOARD,
                                                    KeyboardProc,
                                                      hDLLMod,
                                                        dwThreadId ))) {
    _com_error err (HRESULT_FROM_WIN32 (GetLastError ()));

    dll_log.Log ( L"[   Input  ] @ SetWindowsHookEx failed: 0x%04X (%s)",
                  err.WCode (), err.ErrorMessage () );

    ++hits;

    if (hits >= 5) {
      dll_log.Log ( L"[   Input  ] * Failed to install keyboard hook after %lu tries... "
        L"bailing out!",
        hits );
      return 0;
    }

    Sleep (1);
  }

  dll_log.Log ( L"[   Input  ] * Installed keyboard hook for command console... "
                      L"%lu %s (%lu ms!)",
                hits,
                  hits > 1 ? L"tries" : L"try",
                    timeGetTime () - dwTime );

  CoInitializeEx (nullptr, COINIT_MULTITHREADED);

  IDirectInput8W* pDInput8 = nullptr;

  HRESULT hr =
    CoCreateInstance ( CLSID_DirectInput8,
                         nullptr,
                           CLSCTX_INPROC_SERVER,
                             IID_IDirectInput8,
                               (LPVOID *)&pDInput8 );

  if (SUCCEEDED (hr)) {
    void** vftable = *(void***)*&pDInput8;

    pDInput8->Initialize (GetModuleHandle (nullptr), DIRECTINPUT_VERSION);

    PPrinny_CreateFuncHook ( L"IDirectInput8::CreateDevice",
                           vftable [3],
                           IDirectInput8_CreateDevice_Detour,
                 (LPVOID*)&IDirectInput8_CreateDevice_Original );

    PPrinny_EnableHook (vftable [3]);

    if (FAILED (pDInput8->CreateDevice (GUID_SysKeyboard, &g_DI8Key, nullptr))) {
      g_DI8Key = nullptr;
    }

    if (g_DI8Key != nullptr) {
      g_DI8Key->SetDataFormat (&c_dfDIKeyboard);

      g_DI8Key->SetCooperativeLevel (
        pp::window.hwnd,
          DISCL_FOREGROUND | DISCL_NONEXCLUSIVE | DISCL_NOWINKEY );

      g_DI8Key->Release ();
    }

    pDInput8->Release ();
  }

  Sleep (INFINITE);
  _endthreadex (0);

  return 0;
}

LRESULT
CALLBACK
pp::InputManager::Hooker::MouseProc (int nCode, WPARAM wParam, LPARAM lParam)
{
  MOUSEHOOKSTRUCT* pmh = (MOUSEHOOKSTRUCT *)lParam;

  return CallNextHookEx (Hooker::getInstance ()->hooks.mouse, nCode, wParam, lParam);
}

LRESULT
CALLBACK
pp::InputManager::Hooker::KeyboardProc (int nCode, WPARAM wParam, LPARAM lParam)
{
  if (nCode >= 0 /*nCode >= 0 && (nCode != HC_NOREMOVE)*/) {
    BYTE    vkCode   = LOWORD (wParam) & 0xFF;
    BYTE    scanCode = HIWORD (lParam) & 0x7F;
    SHORT   repeated = LOWORD (lParam);
    bool    keyDown  = ! (lParam & 0x80000000);

    if (visible && vkCode == VK_BACK) {
      if (keyDown) {
        size_t len = strlen (text);
                len--;

        if (len < 1)
          len = 1;

        text [len] = '\0';
      }
    }

    // We don't want to distinguish between left and right on these keys, so alias the stuff
    else if ((vkCode == VK_SHIFT || vkCode == VK_LSHIFT || vkCode == VK_RSHIFT)) {
      if (keyDown) keys_ [VK_SHIFT] = 0x81; else keys_ [VK_SHIFT] = 0x00;
    }

    else if ((vkCode == VK_MENU || vkCode == VK_LMENU || vkCode == VK_MENU)) {
      if (keyDown) keys_ [VK_MENU] = 0x81; else keys_ [VK_MENU] = 0x00;
    }

    else if ((!repeated) && vkCode == VK_CAPITAL) {
      if (keyDown) if (keys_ [VK_CAPITAL] == 0x00) keys_ [VK_CAPITAL] = 0x81; else keys_ [VK_CAPITAL] = 0x00;
    }

    else if ((vkCode == VK_CONTROL || vkCode == VK_LCONTROL || vkCode == VK_RCONTROL)) {
      if (keyDown) keys_ [VK_CONTROL] = 0x81; else keys_ [VK_CONTROL] = 0x00;
    }

    else if ((vkCode == VK_UP) || (vkCode == VK_DOWN)) {
      if (keyDown && visible) {
        if (vkCode == VK_UP)
          commands.idx--;
        else
          commands.idx++;

        // Clamp the index
        if (commands.idx < 0)
          commands.idx = 0;
        else if (commands.idx >= commands.history.size ())
          commands.idx = commands.history.size () - 1;

        if (commands.history.size ()) {
          strcpy (&text [1], commands.history [commands.idx].c_str ());
          command_issued = false;
        }
      }
    }

    else if (visible && vkCode == VK_RETURN) {
      if (keyDown && LOWORD (lParam) < 2) {
        size_t len = strlen (text+1);
        // Don't process empty or pure whitespace command lines
        if (len > 0 && strspn (text+1, " ") != len) {
          eTB_CommandResult result = SK_GetCommandProcessor ()->ProcessCommandLine (text+1);

          if (result.getStatus ()) {
            // Don't repeat the same command over and over
            if (commands.history.size () == 0 ||
                commands.history.back () != &text [1]) {
              commands.history.push_back (&text [1]);
            }

            commands.idx = commands.history.size ();

            text [1] = '\0';

            command_issued = true;
          }
          else {
            command_issued = false;
          }

          result_str = result.getWord   () + std::string (" ")   +
                       result.getArgs   () + std::string (":  ") +
                       result.getResult ();
        }
      }
    }

    else if (keyDown) {
      eTB_CommandProcessor* pCommandProc = SK_GetCommandProcessor ();

      bool new_press = keys_ [vkCode] != 0x81;

      keys_ [vkCode] = 0x81;

      if (keys_ [VK_CONTROL] && keys_ [VK_SHIFT]) {
        if ( keys_ [VK_TAB]         && 
               ( vkCode == VK_CONTROL ||
                 vkCode == VK_SHIFT   ||
                 vkCode == VK_TAB ) &&
             new_press ) {
          visible = ! visible;

          // Avoid duplicating a SK feature
          static HMODULE hOpenGL = GetModuleHandle (config.system.injector.c_str ());

          typedef void (__stdcall *SK_SteamAPI_SetOverlayState_pfn)(bool);
          static SK_SteamAPI_SetOverlayState_pfn SK_SteamAPI_SetOverlayState =
              (SK_SteamAPI_SetOverlayState_pfn)
                GetProcAddress ( hOpenGL,
                                    "SK_SteamAPI_SetOverlayState" );

          SK_SteamAPI_SetOverlayState (visible);

          // Prevent the Steam Overlay from being a real pain
          return -1;
        }

        if (keys_ [VK_MENU] && vkCode == 'L' && new_press) {
          pCommandProc->ProcessCommandLine ("Trace.NumFrames 1");
          pCommandProc->ProcessCommandLine ("Trace.Enable true");
        }

        else if (vkCode == 'H' && new_press) {
          pCommandProc->ProcessCommandLine ("Render.HighPrecisionSSAO toggle");
        }

        else if (vkCode == VK_OEM_COMMA && new_press) {
          pCommandProc->ProcessCommandLine ("Render.MSAA toggle");
        }

        else if (vkCode == 'U' && new_press) {
          extern bool ui_clamp;
          ui_clamp = (! ui_clamp);
        }

        else if (vkCode == 'Z' && new_press) {
          pCommandProc->ProcessCommandLine ("Render.TaskTiming toggle");
        }

        else if (vkCode == 'X' && new_press) {
          pCommandProc->ProcessCommandLine ("Render.AggressiveOpt toggle");
        }

        else if (vkCode == '1' && new_press) {
          pCommandProc->ProcessCommandLine ("Window.ForegroundFPS 60.0");
        }

        else if (vkCode == '2' && new_press) {
          pCommandProc->ProcessCommandLine ("Window.ForegroundFPS 30.0");
        }

        else if (vkCode == '3' && new_press) {
          pCommandProc->ProcessCommandLine ("Window.ForegroundFPS 0.0");
        }

        else if (vkCode == VK_OEM_PERIOD && new_press) {
          pCommandProc->ProcessCommandLine ("Render.FringeRemoval toggle");
        }
      }

      // Don't print the tab character, it's pretty useless.
      if (visible && vkCode != VK_TAB) {
        char key_str [2];
        key_str [1] = '\0';

        if (1 == ToAsciiEx ( vkCode,
                              scanCode,
                              keys_,
                            (LPWORD)key_str,
                              0,
                              GetKeyboardLayout (0) ) &&
             isprint (*key_str) ) {
          strncat (text, key_str, 1);
          command_issued = false;
        }
      }
    }

    else if ((! keyDown)) {
      bool new_release = keys_ [vkCode] != 0x00;

      keys_ [vkCode] = 0x00;
    }

    if (visible) return -1;
  }

  return CallNextHookEx (Hooker::getInstance ()->hooks.keyboard, nCode, wParam, lParam);
};


void
PPrinny_DrawCommandConsole (void)
{
  static int draws = 0;

  // Skip the first several frames, so that the console appears below the
  //  other OSD.
  if (draws++ > 20) {
    pp::InputManager::Hooker* pHook = pp::InputManager::Hooker::getInstance ();
    pHook->Draw ();
  }
}


pp::InputManager::Hooker* pp::InputManager::Hooker::pInputHook = nullptr;

char                      pp::InputManager::Hooker::text [4096];

BYTE                      pp::InputManager::Hooker::keys_ [256]    = { 0 };
bool                      pp::InputManager::Hooker::visible        = false;

bool                      pp::InputManager::Hooker::command_issued = false;
std::string               pp::InputManager::Hooker::result_str;

pp::InputManager::Hooker::command_history_s
                          pp::InputManager::Hooker::commands;