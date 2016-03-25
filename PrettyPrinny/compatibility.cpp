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

#include "hook.h"
#include "log.h"

typedef HMODULE (WINAPI *LoadLibraryA_pfn)(LPCSTR  lpFileName);
typedef HMODULE (WINAPI *LoadLibraryW_pfn)(LPCWSTR lpFileName);

LoadLibraryA_pfn LoadLibraryA_Original = nullptr;
LoadLibraryW_pfn LoadLibraryW_Original = nullptr;

typedef HMODULE (WINAPI *LoadLibraryExA_pfn)
( _In_       LPCSTR  lpFileName,
  _Reserved_ HANDLE  hFile,
  _In_       DWORD   dwFlags
);

typedef HMODULE (WINAPI *LoadLibraryExW_pfn)
( _In_       LPCWSTR lpFileName,
  _Reserved_ HANDLE  hFile,
  _In_       DWORD   dwFlags
);

LoadLibraryExA_pfn LoadLibraryExA_Original = nullptr;
LoadLibraryExW_pfn LoadLibraryExW_Original = nullptr;

extern HMODULE hModSelf;

#include <Shlwapi.h>
#pragma comment (lib, "Shlwapi.lib")

BOOL
BlacklistLibraryW (LPCWSTR lpFileName)
{
  if (StrStrIW (lpFileName, L"ltc_help32") ||
      StrStrIW (lpFileName, L"ltc_game32")) {
    dll_log.Log (L"[Black List] Preventing Raptr's overlay, evil little thing must die!");
    return TRUE;
  }

  if (StrStrIW (lpFileName, L"PlayClaw")) {
    dll_log.Log (L"[Black List] Incompatible software: PlayClaw disabled");
    return TRUE;
  }

  if (StrStrIW (lpFileName, L"fraps")) {
    dll_log.Log (L"[Black List] FRAPS is not compatible with this software");
    return TRUE;
  }

#if 0
  if (StrStrIW (lpFileName, L"ig75icd32")) {
    dll_log.Log (L"[Black List] Preventing Intel Integrated OpenGL driver from activating...");
    return TRUE;
  }
#endif

  return FALSE;
}

BOOL
BlacklistLibraryA (LPCSTR lpFileName)
{
  wchar_t wszWideLibName [MAX_PATH];

  MultiByteToWideChar (CP_OEMCP, 0x00, lpFileName, -1, wszWideLibName, MAX_PATH);

  return BlacklistLibraryW (wszWideLibName);
}

HMODULE
WINAPI
LoadLibraryA_Detour (LPCSTR lpFileName)
{
  if (lpFileName == nullptr)
    return NULL;

  HMODULE hModEarly = GetModuleHandleA (lpFileName);

  if (hModEarly == NULL && BlacklistLibraryA (lpFileName))
    return NULL;

  HMODULE hMod = LoadLibraryA_Original (lpFileName);

  if (hModEarly != hMod)
    dll_log.Log (L"[DLL Loader] Game loaded '%#64hs' <LoadLibraryA>", lpFileName);

  return hMod;
}

HMODULE
WINAPI
LoadLibraryW_Detour (LPCWSTR lpFileName)
{
  if (lpFileName == nullptr)
    return NULL;

  HMODULE hModEarly = GetModuleHandleW (lpFileName);


  if (hModEarly == NULL && config.compatibility.bypass_intel_gl && StrStrIW (lpFileName, L"ig75icd32")) {
    dll_log.LogEx (true, L"[OGL Driver] Bypassing Intel driver (ig75icd32.dll)... ");


    HMODULE hModGL_AMD = LoadLibraryW_Original (L"atioglxx.dll");

    if (hModGL_AMD != nullptr) {
      dll_log.LogEx (false, L"success! (atioglxx.dll [AMD])\n");
      return hModGL_AMD;
    }


    HMODULE hModGL_NV = LoadLibraryW_Original (L"nvoglv32.dll");

    if (hModGL_NV != nullptr) {
      dll_log.LogEx (false, L"success! (nvoglv32.dll [NVIDIA])\n");
      return hModGL_NV;
    }


    dll_log.LogEx (false, L"failed! (You are stuck with Intel)\n");
  }


  if (hModEarly == NULL && BlacklistLibraryW (lpFileName))
    return NULL;

  HMODULE hMod = LoadLibraryW_Original (lpFileName);

  if (hModEarly != hMod)
    dll_log.Log (L"[DLL Loader] Game loaded '%#64s' <LoadLibraryW>", lpFileName);

  return hMod;
}

HMODULE
WINAPI
LoadLibraryExA_Detour (
  _In_       LPCSTR lpFileName,
  _Reserved_ HANDLE hFile,
  _In_       DWORD  dwFlags )
{
  if (lpFileName == nullptr)
    return NULL;

  HMODULE hModEarly = GetModuleHandleA (lpFileName);

  if (hModEarly == NULL && BlacklistLibraryA (lpFileName))
    return NULL;

  HMODULE hMod = LoadLibraryExA_Original (lpFileName, hFile, dwFlags);

  if (hModEarly != hMod && (! ((dwFlags & LOAD_LIBRARY_AS_DATAFILE_EXCLUSIVE) ||
                               (dwFlags & LOAD_LIBRARY_AS_IMAGE_RESOURCE))))
    dll_log.Log (L"[DLL Loader] Game loaded '%#64hs' <LoadLibraryExA>", lpFileName);

  return hMod;
}

HMODULE
WINAPI
LoadLibraryExW_Detour (
  _In_       LPCWSTR lpFileName,
  _Reserved_ HANDLE  hFile,
  _In_       DWORD   dwFlags )
{
  if (lpFileName == nullptr)
    return NULL;

  HMODULE hModEarly = GetModuleHandleW (lpFileName);

  if (hModEarly == NULL && BlacklistLibraryW (lpFileName))
    return NULL;

  HMODULE hMod = LoadLibraryExW_Original (lpFileName, hFile, dwFlags);

  if (hModEarly != hMod && (! ((dwFlags & LOAD_LIBRARY_AS_DATAFILE_EXCLUSIVE) ||
                               (dwFlags & LOAD_LIBRARY_AS_IMAGE_RESOURCE))))
    dll_log.Log (L"[DLL Loader] Game loaded '%#64s' <LoadLibraryExW>", lpFileName);

  return hMod;
}

void
PPrinny_InitCompatBlacklist (void)
{
  PPrinny_CreateDLLHook ( L"kernel32.dll", "LoadLibraryA",
                          LoadLibraryA_Detour,
                (LPVOID*)&LoadLibraryA_Original );

  PPrinny_CreateDLLHook ( L"kernel32.dll", "LoadLibraryW",
                          LoadLibraryW_Detour,
                (LPVOID*)&LoadLibraryW_Original );

  PPrinny_CreateDLLHook ( L"kernel32.dll", "LoadLibraryExA",
                          LoadLibraryExA_Detour,
                (LPVOID*)&LoadLibraryExA_Original );

  PPrinny_CreateDLLHook ( L"kernel32.dll", "LoadLibraryExW",
                          LoadLibraryExW_Detour,
                (LPVOID*)&LoadLibraryExW_Original );

  if (GetModuleHandleW (L"fraps.dll") != NULL) {
    dll_log.Log (L"[Black List] FRAPS detected; expect the game to crash.");
    FreeLibrary (GetModuleHandleW (L"fraps.dll"));
  }
}

LPVOID lpvDamageHook = nullptr;

__declspec (naked)
void
PP_DamageCrashHandler (void)
{
  // Bail-out if we were returned a NULL pointer
  __asm {
    cmp esi,0
    jne ALL_GOOD
    pop edi
    pop esi
    pop ebx
    mov esp,ebp
    pop ebp
    ret 

    // Otherwise, resume normal operation
ALL_GOOD:
    jmp lpvDamageHook
  }
}

#if 0
typedef void* (__cdecl *memcpy_pfn)(
  _Out_writes_bytes_all_(_Size) void* _Dst,
  _In_reads_bytes_(_Size)       void const* _Src,
  _In_                          size_t      _Size
);

memcpy_pfn memcpy_Original = nullptr;

__declspec (nothrow, noinline)
void*
__cdecl
memcpy_Detour (void* _Dst, void const* _Src, size_t _Size)
{
  if (_Dst == nullptr)
    return (void *)_Src;

  if (_Src == nullptr)
    return _Dst;

  return memcpy_Original (_Dst, _Src, _Size);
}
#endif

#if 0
typedef void* (__cdecl *memccpy_pfn)(
        _Out_writes_bytes_opt_(_Size) void*       _Dst,
        _In_reads_bytes_opt_(_Size)   void const* _Src,
        _In_                          int         _Val,
        _In_                          size_t      _Size
);

memccpy_pfn memccpy_Original = nullptr;

__declspec (nothrow, noinline, naked)
void*
__cdecl
memccpy_Detour ( _Out_writes_bytes_opt_(_Size) void*       _Dst,
                 _In_reads_bytes_opt_(_Size)   void const* _Src,
                 _In_                          int         _Val,
                 _In_                          size_t      _Size)
{
  if (_Dst == nullptr && _Size > 0) {
    dll_log.Log (L"[Damage Fix] Sanity check failure (_Dst == nullptr)!");
    fflush (dll_log.fLog);
    return nullptr;//memccpy_Original ((void *)_Dst, _Src, _Val, _Size);//(void *)_Dst;
  }

//  if (_Src == nullptr) {
//    dll_log.Log (L"[Damage Fix] Sanity check failure (_Src == nullptr)!");
//    return _Dst;
//  }

//  dll_log.Log (L"[Damage Fix] Sanity check success!");

  return memccpy_Original (_Dst, _Src, _Val, _Size);
}
#endif

//typedef __cdecl
bool
PPrinny_PatchDamageCrash (void)
{
  return true;

  intptr_t dwCrashAddr = 0x00EECA45;

  dll_log.LogEx ( true, L"[Damage Fix] Patching executable (addr=%Xh)... ",
                          dwCrashAddr );

  PPrinny_CreateFuncHook ( L"NISA_Damage_Crash",
                             (LPVOID)dwCrashAddr,
                               PP_DamageCrashHandler,
                                 &lpvDamageHook );
  PPrinny_EnableHook ((LPVOID)dwCrashAddr);

  dll_log.LogEx ( false, L"done!\n");

#if 0
#if 0
  dll_log.LogEx ( true, L"[Damage Fix] Adding sanity checks to msvcr120.memcpy ()... ",
                          dwCrashAddr );

  PPrinny_CreateDLLHook ( L"msvcr120.dll",
                           "memcpy",
                            memcpy_Detour,
                  (LPVOID *)&memcpy_Original );
#else
  intptr_t memccpy_entry =
    (intptr_t)GetProcAddress (
      GetModuleHandle (L"msvcr120.dll"),
        "_memccpy" );

  memccpy_entry += 0xB3;

  dll_log.LogEx ( true, L"[Damage Fix] Adding sanity checks to msvcr120._memccpy (_Dst,_Src,_Val,_Size)... ",
                          dwCrashAddr );

  PPrinny_CreateFuncHook ( L"_memccpy.fast",
                    (LPVOID)memccpy_entry,
                            memccpy_Detour,
                  (LPVOID *)&memccpy_Original );
  PPrinny_EnableHook ((LPVOID)memccpy_entry);
#endif

  dll_log.LogEx ( false, L"done!\n");
#endif

  return true;
}