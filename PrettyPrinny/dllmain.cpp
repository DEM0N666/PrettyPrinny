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

#include "config.h"
#include "log.h"
#include "command.h"

#include "hook.h"

#include "timing.h"
#include "display.h"
#include "render.h"
#include "input.h"
#include "window.h"

#include <comdef.h>

#pragma comment (lib, "kernel32.lib")

HMODULE hDLLMod      = { 0 }; // Handle to SELF
HMODULE hInjectorDLL = { 0 }; // Handle to Special K

typedef HRESULT (__stdcall *SK_UpdateSoftware_pfn)(const wchar_t* wszProduct);
typedef bool    (__stdcall *SK_FetchVersionInfo_pfn)(const wchar_t* wszProduct);

std::wstring injector_dll;

typedef void (__stdcall *SKX_SetPluginName_pfn)(std::wstring name);
SKX_SetPluginName_pfn SKX_SetPluginName = nullptr;

__declspec (dllexport)
BOOL
WINAPI
SKPlugIn_Shutdown (LPVOID* lpReserved)
{
  UNREFERENCED_PARAMETER (lpReserved);

#if 0
  pp::WindowManager::Shutdown ();
  pp::RenderFix::Shutdown     ();
  pp::InputManager::Shutdown  ();
  pp::TimingFix::Shutdown     ();
#endif
  pp::DisplayFix::Shutdown    ();

  PPrinny_UnInit_MinHook ();
  PPrinny_SaveConfig     ();

  dll_log.LogEx ( false, L"============ (Version:  v %s) "
                         L"============\n",
                           PPRINNY_VER_STR.c_str () );
  dll_log.LogEx ( true,  L"End PrettyPrinny.dll\n" );
  dll_log.LogEx ( false, L"-------------- [Pretty Prinny] "
                         L"--------------\n" );

  dll_log.close ();

  return TRUE;
}

DWORD
WINAPI
DllThread (LPVOID user)
{
  std::wstring plugin_name = L"Pretty Prinny v " + PPRINNY_VER_STR;

  //
  // Until we modify the logger to use Kernel32 File I/O, we have to
  //   do this stuff from here... dependency DLLs may not be loaded
  //     until initial DLL startup finishes and threads are allowed
  //       to start.
  //
  dll_log.init  ( "logs/PrettyPrinny.log",
                    "w" );
  dll_log.LogEx ( false, L"-------------- [Pretty Prinny] "
                         L"--------------\n" ); // <--- I was bored ;)
  dll_log.Log   (        L"PrettyPrinny Plug-In\n"
                         L"============ (Version:  v %s) "
                         L"============",
                           PPRINNY_VER_STR.c_str () );

  if (! PPrinny_LoadConfig ()) {
    // Save a new config if none exists
    PPrinny_SaveConfig ();
  }

  hInjectorDLL =
    GetModuleHandle (config.system.injector.c_str ());

  SKX_SetPluginName = 
    (SKX_SetPluginName_pfn)
      GetProcAddress (hInjectorDLL, "SKX_SetPluginName");
  SK_GetCommandProcessor =
    (SK_GetCommandProcessor_pfn)
      GetProcAddress (hInjectorDLL, "SK_GetCommandProcessor");

  //
  // If this is NULL, the injector system isn't working right!!!
  //
  if (SKX_SetPluginName != nullptr)
    SKX_SetPluginName (plugin_name);

  // Plugin State
  if (PPrinny_Init_MinHook () == MH_OK) {

    // This also locates the highest image address, so we can
    //   factor out the Steam overlay from certain hooks...
    extern bool PPrinny_PatchDamageCrash (void);
    PPrinny_PatchDamageCrash ();

    pp::DisplayFix::Init    ();
    pp::RenderFix::Init     ();
    pp::InputManager::Init  ();

    pp::TimingFix::Init     ();

    pp::WindowManager::Init ();
  }

  return 0;
}

__declspec (dllexport)
BOOL
WINAPI
SKPlugIn_Init (HMODULE hModSpecialK)
{
  wchar_t wszSKFileName [ MAX_PATH + 2] = { L'\0' };
          wszSKFileName [   MAX_PATH  ] =   L'\0';

  GetModuleFileName (hModSpecialK, wszSKFileName, MAX_PATH - 1);

  injector_dll = wszSKFileName;

  hInjectorDLL = hModSpecialK;

#if 1
  DllThread (nullptr);
#else
  _beginthreadex ( nullptr, 0, DllThread, nullptr, 0x00, nullptr );
#endif

  return TRUE;
}


BOOL
APIENTRY
DllMain (HMODULE hModule,
         DWORD   ul_reason_for_call,
         LPVOID  /* lpReserved */)
{
  switch (ul_reason_for_call)
  {
    case DLL_PROCESS_ATTACH:
    {
      hDLLMod = hModule;
    } break;

    case DLL_PROCESS_DETACH:
    {
    } break;
  }

  return TRUE;
}