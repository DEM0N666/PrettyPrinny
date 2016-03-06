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
#ifndef __PPRINNY__HOOK_H__
#define __PPRINNY__HOOK_H__

#pragma comment (lib, "MinHook/lib/libMinHook.x86.lib")
#include "MinHook/include/MinHook.h"

void
PPrinny_DrawCommandConsole (void);

MH_STATUS
WINAPI
PPrinny_CreateFuncHook ( LPCWSTR pwszFuncName,
                         LPVOID  pTarget,
                         LPVOID  pDetour,
                         LPVOID *ppOriginal );

MH_STATUS
WINAPI
PPrinny_CreateDLLHook ( LPCWSTR pwszModule, LPCSTR  pszProcName,
                        LPVOID  pDetour,    LPVOID *ppOriginal,
                        LPVOID* ppFuncAddr = nullptr );

MH_STATUS
WINAPI
PPrinny_EnableHook (LPVOID pTarget);

MH_STATUS
WINAPI
PPrinny_DisableHook (LPVOID pTarget);

MH_STATUS
WINAPI
PPrinny_RemoveHook (LPVOID pTarget);

MH_STATUS
WINAPI
PPrinny_Init_MinHook (void);

MH_STATUS
WINAPI
PPrinny_UnInit_MinHook (void);

#endif /* __PPRINNY__HOOK_H__ */