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

#include "display.h"
#include "log.h"
#include "config.h"
#include "hook.h"

typedef int (WINAPI *GetSystemMetrics_pfn)(
  _In_ int nIndex
);
GetSystemMetrics_pfn GetSystemMetrics_Original = nullptr;


typedef BOOL (WINAPI *EnumDisplaySettingsA_pfn)(
  _In_  LPCSTR    lpszDeviceName,
  _In_  DWORD     iModeNum,
  _Out_ DEVMODEA *lpDevMode
);
EnumDisplaySettingsA_pfn EnumDisplaySettingsA_Original = nullptr;

BOOL
WINAPI
EnumDisplaySettingsA_Detour ( _In_  LPCSTR    lpszDeviceName,
                              _In_  DWORD     iModeNum,
                              _Out_ DEVMODEA* lpDevMode )
{
  //dll_log.Log ( L"[Resolution] EnumDisplaySettingsA (%hs, %lu, %ph)",
                  //lpszDeviceName, iModeNum, lpDevMode );

  if (config.display.match_desktop) {
    if (iModeNum == 0)
      return EnumDisplaySettingsA_Original (lpszDeviceName, ENUM_CURRENT_SETTINGS, lpDevMode);

    EnumDisplaySettingsA_Original (lpszDeviceName, ENUM_CURRENT_SETTINGS, lpDevMode);

    return 0;
  }

  return EnumDisplaySettingsA_Original (lpszDeviceName, iModeNum, lpDevMode);
}

typedef LONG (WINAPI *ChangeDisplaySettingsA_pfn)(DEVMODEA* dontcare, DWORD dwFlags);
ChangeDisplaySettingsA_pfn ChangeDisplaySettingsA_Original = nullptr;

LONG
WINAPI
ChangeDisplaySettingsA_Detour (DEVMODEA* dontcare, DWORD dwFlags)
{
  bool override = false;

  if (dontcare != nullptr)
    dll_log.LogEx ( true, L"[Resolution] ChangeDisplaySettingsA - %4lux%-4lu@%#2lu Hz",
                      dontcare->dmPelsWidth, dontcare->dmPelsHeight,
                        dontcare->dmDisplayFrequency );
  else {
    dll_log.LogEx (true, L"[Resolution] ChangeDisplaySettingsA - RESET");
  }

  if (config.display.match_desktop) {
    override = true;

    dll_log.LogEx (false, L" { Override: Desktop }\n");

    //
    // Fix Display Scaling Problems
    //
    if ( dontcare != nullptr && dwFlags & CDS_TEST &&
         (dontcare->dmPelsWidth  != GetSystemMetrics_Original (SM_CXSCREEN) ||
          dontcare->dmPelsHeight != GetSystemMetrics_Original (SM_CYSCREEN)) ) {
      return DISP_CHANGE_FAILED;
    }

    return DISP_CHANGE_SUCCESSFUL;
  }

  if ( dontcare != nullptr && ( config.display.height  > 0 ||
                                config.display.width   > 0 ||
                                config.display.refresh > 0 ||
                                config.display.monitor > 0 ) ) {
    override = true;

    LONG ret = -1;

    int width   = dontcare->dmPelsWidth;
    int height  = dontcare->dmPelsHeight;
    int refresh = dontcare->dmDisplayFrequency;
    int monitor = 0;

    int test_width  = width;
    int test_height = height;

    if (config.display.height > 0) {
      height                 = config.display.height;
      dontcare->dmFields    |= DM_PELSHEIGHT;
      dontcare->dmPelsHeight = height;
    }

    if (config.display.width > 0) {
      width                 = config.display.width;
      dontcare->dmFields   |= DM_PELSWIDTH;
      dontcare->dmPelsWidth = width;
    }

    if (config.display.width > 0) {
      refresh                      = config.display.refresh;
      dontcare->dmFields          |= DM_DISPLAYFREQUENCY;
      dontcare->dmDisplayFrequency = refresh;
    }

    if (config.display.monitor > 0) {
      monitor = config.display.monitor;

      DISPLAY_DEVICEA dev = { 0 };
      dev.cb = sizeof DISPLAY_DEVICEA;

      if (EnumDisplayDevicesA (nullptr, monitor, &dev, 0x00)) {
        ret =
          ChangeDisplaySettingsExA ( dev.DeviceString,
                                       dontcare,
                                         NULL,
                                           dwFlags,
                                             nullptr );

#if 0
       DEVMODEA settings;
       settings.dmSize = sizeof DEVMODEA;
       EnumDisplaySettingsExA (dev.DeviceName,
                               ENUM_CURRENT_SETTINGS,
                               &settings,
                               EDS_ROTATEDMODE);
#endif
      }
    }

    dll_log.LogEx ( false, L" { Override: %4lux%-4lu@%#2luHz on"
                           L" Monitor %lu }\n",
                      width, height, refresh, monitor );

    //
    // Fix Display Scaling Problems
    //
    if ( dontcare != nullptr && dwFlags & CDS_TEST &&
         (test_width  != width ||
          test_height != height) ) {
      return DISP_CHANGE_FAILED;
    }

    if (! ret)
      return ret;
  }

  if (! override)
    dll_log.LogEx (false, L"\n");

  return ChangeDisplaySettingsA_Original (dontcare, dwFlags);
}

int
WINAPI
GetSystemMetrics_Detour (_In_ int nIndex)
{
  int nRet = GetSystemMetrics_Original (nIndex);

  if (config.display.width > 0 && nIndex == SM_CXSCREEN)
    return config.display.width;

  if (config.display.height > 0 && nIndex == SM_CYSCREEN)
    return config.display.height;

  if (config.display.width > 0 && nIndex == SM_CXFULLSCREEN) {
    return config.display.width;
  }

  if (config.display.height > 0 && nIndex == SM_CYFULLSCREEN) {
    return config.display.height;
  }

  if (config.window.borderless) {
    if (nIndex == SM_CYCAPTION)
      return 0;
    if (nIndex == SM_CXBORDER)
      return 0;
    if (nIndex == SM_CYBORDER)
      return 0;
    if (nIndex == SM_CXDLGFRAME)
      return 0;
    if (nIndex == SM_CYDLGFRAME)
      return 0;
  }

  //dll_log.Log ( L"[Resolution] GetSystemMetrics (0x%04X) : %lu",
                  //nIndex, nRet );

  return nRet;
}

void
pp::DisplayFix::Init (void)
{
  PPrinny_CreateDLLHook ( L"user32.dll",
                          "ChangeDisplaySettingsA",
                          ChangeDisplaySettingsA_Detour,
               (LPVOID *)&ChangeDisplaySettingsA_Original );

  PPrinny_CreateDLLHook ( L"user32.dll",
                          "EnumDisplaySettingsA",
                          EnumDisplaySettingsA_Detour,
               (LPVOID *)&EnumDisplaySettingsA_Original );

  PPrinny_CreateDLLHook ( L"user32.dll",
                          "GetSystemMetrics",
                          GetSystemMetrics_Detour,
               (LPVOID *)&GetSystemMetrics_Original );
}

void
pp::DisplayFix::Shutdown (void)
{
}