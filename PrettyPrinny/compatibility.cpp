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

LPVOID __PP_end_img_addr = nullptr;

void*
PP_Scan (uint8_t* pattern, size_t len, uint8_t* mask)
{
  uint8_t* base_addr = (uint8_t *)GetModuleHandle (nullptr);

  MEMORY_BASIC_INFORMATION mem_info;
  VirtualQuery (base_addr, &mem_info, sizeof mem_info);

  //
  // VMProtect kills this, so let's do something else...
  //
#ifdef VMPROTECT_IS_NOT_A_FACTOR
  IMAGE_DOS_HEADER* pDOS =
    (IMAGE_DOS_HEADER *)mem_info.AllocationBase;
  IMAGE_NT_HEADERS* pNT  =
    (IMAGE_NT_HEADERS *)((intptr_t)(pDOS + pDOS->e_lfanew));

  uint8_t* end_addr = base_addr + pNT->OptionalHeader.SizeOfImage;
#else
           base_addr = (uint8_t *)mem_info.BaseAddress;//AllocationBase;
  uint8_t* end_addr  = (uint8_t *)mem_info.BaseAddress + mem_info.RegionSize;

  if (base_addr != (uint8_t *)0x400000) {
    dll_log.Log ( L"[ Sig Scan ] Expected module base addr. 40000h, but got: %ph",
                    base_addr );
  }

  int pages = 0;

// Scan up to 32 MiB worth of data
#define PAGE_WALK_LIMIT (base_addr) + (1 << 26)

  //
  // For practical purposes, let's just assume that all valid games have less than 32 MiB of
  //   committed executable image data.
  //
  while (VirtualQuery (end_addr, &mem_info, sizeof mem_info) && end_addr < PAGE_WALK_LIMIT) {
    if (mem_info.Protect & PAGE_NOACCESS || (! (mem_info.Type & MEM_IMAGE)))
      break;

    pages += VirtualQuery (end_addr, &mem_info, sizeof mem_info);

    end_addr = (uint8_t *)mem_info.BaseAddress + mem_info.RegionSize;
  }

  if (end_addr > PAGE_WALK_LIMIT) {
    dll_log.Log ( L"[ Sig Scan ] Module page walk resulted in end addr. out-of-range: %ph",
                    end_addr );
    dll_log.Log ( L"[ Sig Scan ]  >> Restricting to %ph",
                    PAGE_WALK_LIMIT );
    end_addr = PAGE_WALK_LIMIT;
  }

  dll_log.Log ( L"[ Sig Scan ] Module image consists of %lu pages, from %ph to %ph",
                  pages,
                    base_addr,
                      end_addr );
#endif

  __PP_end_img_addr = end_addr;

  uint8_t*  begin = (uint8_t *)base_addr;
  uint8_t*  it    = begin;
  int       idx   = 0;

  while (it < end_addr)
  {
    VirtualQuery (it, &mem_info, sizeof mem_info);

    // Bail-out once we walk into an address range that is not resident, because
    //   it does not belong to the original executable.
    if (mem_info.RegionSize == 0)
      break;

    uint8_t* next_rgn =
     (uint8_t *)mem_info.BaseAddress + mem_info.RegionSize;

    if ( (! (mem_info.Type    & MEM_IMAGE))  ||
         (! (mem_info.State   & MEM_COMMIT)) ||
             mem_info.Protect & PAGE_NOACCESS ) {
      it    = next_rgn;
      idx   = 0;
      begin = it;
      continue;
    }

    // Do not search past the end of the module image!
    if (next_rgn >= end_addr)
      break;

    while (it < next_rgn) {
      uint8_t* scan_addr = it;

      bool match = (*scan_addr == pattern [idx]);

      // For portions we do not care about... treat them
      //   as matching.
      if (mask != nullptr && (! mask [idx]))
        match = true;

      if (match) {
        if (++idx == len)
          return (void *)begin;

        ++it;
      }

      else {
        // No match?!
        if (it > end_addr - len)
          break;

        it  = ++begin;
        idx = 0;
      }
    }
  }

  return nullptr;
}

void
PP_FlushInstructionCache ( LPCVOID base_addr,
                           size_t  code_size )
{
  FlushInstructionCache ( GetCurrentProcess (),
                            base_addr,
                              code_size );
}

void
PP_InjectMachineCode ( LPVOID   base_addr,
                       uint8_t* new_code,
                       size_t   code_size,
                       DWORD    permissions,
                       uint8_t* old_code = nullptr )
{
  DWORD dwOld;

  if (VirtualProtect (base_addr, code_size, permissions, &dwOld))
  {
    if (old_code != nullptr)
      memcpy (old_code, base_addr, code_size);

    memcpy (base_addr, new_code, code_size);

    VirtualProtect (base_addr, code_size, dwOld, &dwOld);

    PP_FlushInstructionCache (base_addr, code_size);
  }
}

LPVOID lpvDamageHook = nullptr;

__declspec (naked)
void
PP_DamageCrashHandler1 (void)
{
  // Never draw damage text
  __asm {
    pop edi
    pop esi
    pop ebx
    mov esp,ebp
    pop ebp
    ret
  }
}

__declspec (naked)
void
PP_DamageCrashHandler2 (void)
{
  // Bail-out if we were returned a NULL pointer
  __asm {
    pushad
    pushfd

    cmp esi,0
    jne ALL_GOOD

    popfd
    popad

    pop edi
    pop esi
    pop ebx
    mov esp,ebp
    pop ebp

    ret


    // Otherwise, resume normal operation
ALL_GOOD:
    popfd
    popad

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
  uint8_t sig  [] = { 0x8B, 0xF0, 0xFF, 0x15, 00, 00, 00, 00, 0xD9 };
  uint8_t mask [] = { 0xFF, 0xFF, 0xFF, 0xFF, 00, 00, 00, 00, 0xFF };

  intptr_t dwCrashAddr =
    (intptr_t)PP_Scan (sig, sizeof (sig), mask);

  dwCrashAddr += sizeof (sig) - 1;

  if (! config.compatibility.patch_damage_bug)
    return true;

  dll_log.LogEx ( true, L"[Damage Fix] Patching executable (addr=%Xh)... ",
                          dwCrashAddr );

  if (config.compatibility.patch_damage_bug == 1) {
    PPrinny_CreateFuncHook ( L"NISA_Damage_Crash",
                               (LPVOID)dwCrashAddr,
                                 PP_DamageCrashHandler1,
                                   &lpvDamageHook );
    PPrinny_EnableHook ((LPVOID)dwCrashAddr);
  }

  else if (config.compatibility.patch_damage_bug == 2) {
    PPrinny_CreateFuncHook ( L"NISA_Damage_Crash",
                               (LPVOID)dwCrashAddr,
                                 PP_DamageCrashHandler2,
                                   &lpvDamageHook );
    PPrinny_EnableHook ((LPVOID)dwCrashAddr);
  }

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