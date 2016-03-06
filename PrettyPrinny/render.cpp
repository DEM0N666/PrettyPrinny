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
#include "render.h"
#include "config.h"
#include "log.h"
#include "window.h"
#include "timing.h"
#include "hook.h"

#include <cstdint>

pp::RenderFix::tracer_s
  pp::RenderFix::tracer;

pp::RenderFix::pp_draw_states_s
  pp::RenderFix::draw_state;

extern uint32_t
crc32 (uint32_t crc, const void *buf, size_t size);

bool use_msaa      = false;
bool early_resolve = false;

BMF_BeginBufferSwap_pfn BMF_BeginBufferSwap = nullptr;

COM_DECLSPEC_NOTHROW
void
STDMETHODCALLTYPE
OGLEndFrame_Pre (void)
{
  //pp::RenderFix::dwRenderThreadID = GetCurrentThreadId ();

  void PPrinny_DrawCommandConsole (void);
  PPrinny_DrawCommandConsole ();

  return BMF_BeginBufferSwap ();
}


#include <gl/GL.h>

typedef GLvoid (WINAPI *glShaderSource_pfn)(
         GLuint    shader,
         GLsizei   count,
 const   GLubyte **string,
 const   GLint    *length
);

glShaderSource_pfn glShaderSource_Original = nullptr;

__declspec (noinline)
GLvoid
WINAPI
glShaderSource_Detour (        GLuint   shader,
                               GLsizei  count,
                        const GLubyte **string,
                        const  GLint   *length )
{
#if 0
  dll_log.Log ( L"[GL Shaders] Shader Obj Id=%lu, Src=%hs",
                  shader, *string );
#endif

  glShaderSource_Original (shader, count, string, length);
}

typedef PROC (APIENTRY *wglGetProcAddress_pfn)(LPCSTR);

wglGetProcAddress_pfn wglGetProcAddress_Original = nullptr;


__declspec (noinline)
PROC
APIENTRY
wglGetProcAddress_Detour (LPCSTR szFuncName)
{
  PROC ret = wglGetProcAddress_Original (szFuncName);

#if 0
  if (! stricmp (szFuncName, "glShaderSource")) {
    glShaderSource_Original = (glShaderSource_pfn)ret;
    ret                     = (PROC)glShaderSource_Detour;
  }
#endif

  dll_log.Log (L"[OpenGL Ext] %#32hs => %ph", szFuncName, ret);

  return ret;
}

void
pp::RenderFix::Init (void)
{
  PPrinny_CreateDLLHook ( config.system.injector.c_str (),
                          "BMF_BeginBufferSwap",
                           OGLEndFrame_Pre,
                 (LPVOID*)&BMF_BeginBufferSwap );

#if 0
  PPrinny_CreateDLLHook ( config.system.injector.c_str (),
                          "BMF_EndBufferSwap",
                           D3D9EndFrame_Post,
                 (LPVOID*)&BMF_EndBufferSwap );
#endif

  PPrinny_CreateDLLHook ( config.system.injector.c_str (),
                          "wglGetProcAddress",
                          wglGetProcAddress_Detour,
               (LPVOID *)&wglGetProcAddress_Original );

  CommandProcessor*     comm_proc    = CommandProcessor::getInstance ();
  eTB_CommandProcessor* pCommandProc = SK_GetCommandProcessor        ();

  // Don't directly modify this state, switching mid-frame would be disasterous...
  pCommandProc->AddVariable ("Render.MSAA",             new eTB_VarStub <bool> (&use_msaa));

  pCommandProc->AddVariable ("Trace.NumFrames", new eTB_VarStub <int>  (&tracer.count));
  pCommandProc->AddVariable ("Trace.Enable",    new eTB_VarStub <bool> (&tracer.log));

  pCommandProc->AddVariable ("Render.ConservativeMSAA", new eTB_VarStub <bool> (&config.render.conservative_msaa));
  pCommandProc->AddVariable ("Render.EarlyResolve",     new eTB_VarStub <bool> (&early_resolve)); 
}

void
pp::RenderFix::Shutdown (void)
{
}

float ar;

pp::RenderFix::CommandProcessor::CommandProcessor (void)
{
  eTB_CommandProcessor* pCommandProc =
    SK_GetCommandProcessor ();

  pCommandProc->AddVariable ("Render.AllowBG",   new eTB_VarStub <bool>  (&config.render.allow_background));
}

bool
pp::RenderFix::CommandProcessor::OnVarChange (eTB_Variable* var, void* val)
{
  return true;
}

pp::RenderFix::CommandProcessor* 
           pp::RenderFix::CommandProcessor::pCommProc;

HWND     pp::RenderFix::hWndDevice       = NULL;

bool     pp::RenderFix::fullscreen       = false;
uint32_t pp::RenderFix::width            = 0UL;
uint32_t pp::RenderFix::height           = 0UL;

HMODULE  pp::RenderFix::user32_dll       = 0;