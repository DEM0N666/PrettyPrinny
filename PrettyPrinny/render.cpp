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

bool drawing_main_scene = false;

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
#pragma comment (lib, "OpenGL32.lib")

typedef BOOL (WINAPI *wglSwapIntervalEXT_pfn)(int interval);

wglSwapIntervalEXT_pfn wglSwapIntervalEXT_Original = nullptr;

__declspec (noinline)
BOOL
WINAPI
wglSwapIntervalEXT_Detour (int interval)
{
  int override_interval;

  override_interval = config.render.swap_interval/* != 0 ? config.render.swap_interval : interval*/;

  dll_log.LogEx   ( true, L"[ GL VSYNC ] wglSwapIntervalEXT (%li)",
                      interval );

  if (override_interval != interval) {
    dll_log.LogEx ( false, L" => OVERRIDE: %li\n",
                      override_interval );
   } else {
    dll_log.LogEx ( false, L"\n" );
   }

  return wglSwapIntervalEXT_Original (override_interval);
}



typedef GLvoid (WINAPI *glEnable_pfn)(GLenum cap);
typedef GLvoid (WINAPI *glDisable_pfn)(GLenum cap);

glEnable_pfn  glEnable_Original  = nullptr;
glDisable_pfn glDisable_Original = nullptr;

__declspec (noinline)
GLvoid
WINAPI
glEnable_Detour (GLenum cap)
{
  // This invalid state setup is embedded in some display lists
  if (cap == GL_TEXTURE) cap = GL_TEXTURE_2D;

  return glEnable_Original (cap);
}

__declspec (noinline)
GLvoid
WINAPI
glDisable_Detour (GLenum cap)
{
  // This invalid state setup is embedded in some display lists
  if (cap == GL_TEXTURE) cap = GL_TEXTURE_2D;

  return glDisable_Original (cap);
}


typedef GLvoid (WINAPI *glMatrixMode_pfn)(GLenum mode);
glMatrixMode_pfn glMatrixMode_Original = nullptr;

__declspec (noinline)
GLvoid
WINAPI
glMatrixMode_Detour (GLenum mode)
{
  // This is only available if GL_ARB_Imaging is, and that is pretty rare.
  if (mode == GL_COLOR)
    return;

  glMatrixMode_Original (mode);
}


typedef GLvoid (WINAPI *glGetFloatv_pfn)(GLenum pname, GLfloat* pparam);
glGetFloatv_pfn glGetFloatv_Original = nullptr;

__declspec (noinline)
GLvoid
WINAPI
glGetFloatv_Detour (GLenum pname, GLfloat* pparams)
{
#define GL_COLOR_MATRIX_SGI 0x80B1

  // Intel does not support this
  if (pname == GL_COLOR_MATRIX_SGI) {
    const float fZero = 0.0f;
    const int    zero = *(int *)&fZero;

    memset (pparams, zero, sizeof (GLfloat) * 16);
    return;
  }

  glGetFloatv_Original (pname, pparams);
}





typedef GLvoid (WINAPI *glTexImage2D_pfn)(
         GLenum  target,
         GLint   level,
         GLint   internalformat,
         GLsizei width,
         GLsizei height,
         GLint   border,
         GLenum  format,
         GLenum  type,
   const GLvoid* data
);

glTexImage2D_pfn glTexImage2D_Original = nullptr;

__declspec (noinline)
GLvoid
WINAPI
glTexImage2D_Detour ( GLenum  target,
                      GLint   level,
                      GLint   internalformat,
                      GLsizei width,
                      GLsizei height,
                      GLint   border,
                      GLenum  format,
                      GLenum  type,
                const GLvoid* data )
{
  GLint texId;
  glGetIntegerv (GL_TEXTURE_BINDING_2D, &texId);

  if (data == nullptr) {
    dll_log.Log ( L"[GL Texture] Id=%i, LOD: %i, (%ix%i)",
                    texId, level, width, height );

    //dll_log.Log ( L"[GL Texture]  >> Possible Render Target");

    dll_log.Log (L"[GL Texture]  >> Render Target - Internal Format: 0x%X <<", internalformat);

#define GL_DEPTH_COMPONENT24 0x81A6
#define GL_RG                0x8227
#define GL_RG8               0x8051
#define GL_RGBA8             0x8058

    // Avoid driver weirdness from using a non-color renderable format
    if (internalformat == GL_LUMINANCE_ALPHA) {
      internalformat = GL_RG8;
      format         = GL_RG;
    }

    if (internalformat == GL_DEPTH_COMPONENT)
      internalformat = GL_DEPTH_COMPONENT24;

    if (internalformat == GL_RGB)
      internalformat = GL_RGBA8;
  

    if (width == 1280 && height == 720) {
      width  = config.render.scene_res_x;
      height = config.render.scene_res_y;
    }

#if 0
    if (width == 960 && height == 544) {
      width  = config.render.scene_res_x;
      height = config.render.scene_res_y;
    }
#endif

#if 0
    if (width == 1024 && height == 512) {
      width = RES_X; height = RES_Y;
    }
#endif
  }

  glTexImage2D_Original (target, level, internalformat, width, height, border, format, type, data);
}

typedef GLvoid (WINAPI *glRenderbufferStorage_pfn)(
  GLenum  target,
  GLenum  internalformat,
  GLsizei width,
  GLsizei height
);

glRenderbufferStorage_pfn glRenderbufferStorage_Original = nullptr;

__declspec (noinline)
GLvoid
WINAPI
glRenderbufferStorage_Detour (
  GLenum  target,
  GLenum  internalformat,
  GLsizei width,
  GLsizei height )
{
  dll_log.Log (L"[ GL Trace ]  >> Render Buffer - Internal Format: 0x%X <<", internalformat);

  if (internalformat == GL_DEPTH_COMPONENT)
    internalformat = GL_DEPTH_COMPONENT24;

  if (internalformat == GL_RGB)
    internalformat = GL_RGBA8;


  if (width == 1280 && height == 720) {
    width = config.render.scene_res_x; height = config.render.scene_res_y;
  }

#if 0
  if (width == 960 && height == 544) {
    width = config.render.scene_res_x; height = config.render.scene_res_y;
  }
#endif


  glRenderbufferStorage_Original (target, internalformat, width, height);
}

typedef GLvoid (WINAPI *glBlitFramebuffer_pfn)(
  GLint      srcX0,
  GLint      srcY0,
  GLint      srcX1,
  GLint      srcY1,
  GLint      dstX0,
  GLint      dstY0,
  GLint      dstX1,
  GLint      dstY1,
  GLbitfield mask,
  GLenum     filter
);

glBlitFramebuffer_pfn glBlitFramebuffer_Original = nullptr;

__declspec (noinline)
GLvoid
WINAPI
glBlitFramebuffer_Detour (
  GLint      srcX0,
  GLint      srcY0,
  GLint      srcX1,
  GLint      srcY1,
  GLint      dstX0,
  GLint      dstY0,
  GLint      dstX1,
  GLint      dstY1,
  GLbitfield mask,
  GLenum     filter )
{
#if 0
  dll_log.Log (L"[GL CallLog] -- glBlitFramebuffer (%li, %li, %li, %li, %li, %li, %li, %li, %li, %X, %X)",
    srcX0, srcY0,
    srcX1, srcY1,
    dstX0, dstY0,
    dstX1, dstY1,
    mask, filter);
#endif

  if (srcX0 == 0    && srcY0 ==   0 &&
      srcX1 == 1280 && srcY1 == 720) {
    srcX1 = config.render.scene_res_x;
    srcY1 = config.render.scene_res_y;
  }

  return glBlitFramebuffer_Original (
    srcX0, srcY0,
    srcX1, srcY1,
    dstX0, dstY0,
    dstX1, dstY1,
    mask, filter
  );
}

typedef GLvoid (WINAPI *glCopyImageSubData_pfn)(
  GLuint  srcName​,
  GLenum  srcTarget​,
  GLint   srcLevel​,
  GLint   srcX​,
  GLint   srcY,
  GLint   srcZ​,
  GLuint  dstName​,
  GLenum  dstTarget​,
  GLint   dstLevel​,
  GLint   dstX​,
  GLint   dstY​,
  GLint   dstZ​,
  GLsizei srcWidth​,
  GLsizei srcHeight​,
  GLsizei srcDepth
);

int HAS_ARB_COPY_IMAGE = -1;
glCopyImageSubData_pfn glCopyImageSubData = nullptr;

typedef GLvoid (WINAPI *glCopyTexSubImage2D_pfn)(
  GLenum  target,
  GLint   level,
  GLint   xoffset,
  GLint   yoffset,
  GLint   x,
  GLint   y,
  GLsizei width, 
  GLsizei height
);

glCopyTexSubImage2D_pfn glCopyTexSubImage2D_Original = nullptr;

__declspec (noinline)
GLvoid
WINAPI
glCopyTexSubImage2D_Detour (
  GLenum  target,
  GLint   level,
  GLint   xoffset,
  GLint   yoffset,
  GLint   x,
  GLint   y,
  GLsizei width, 
  GLsizei height
)
{
  /*
  dll_log.Log ( L"[ GL Trace ] -- glCopyTexSubImage2D (%x, %li, %li, %li, %li, %li, %li, %li)",
                  target, level,
                    xoffset, yoffset,
                      x, y,
                        width, height );
  */

  if (width == 1280 && height == 720) {
    width  = config.render.scene_res_x;
    height = config.render.scene_res_y;
  }

#if 0
  if (width == 960 && height == 544) {
    width  = config.render.scene_res_x;
    height = config.render.scene_res_y;
  }
#endif

#if 0
  if (width == 1024 && height == 512) {
    width = RES_X; height = RES_Y;
  }
#endif

  switch (HAS_ARB_COPY_IMAGE)
  {
    case -1:
    {
      glCopyImageSubData = 
        (glCopyImageSubData_pfn)
          wglGetProcAddress ("glCopyImageSubDataARB");
      if (glCopyImageSubData != nullptr)
        HAS_ARB_COPY_IMAGE = 1;
      else
        HAS_ARB_COPY_IMAGE = 0;
    } //break;

    case 0:
    case 1:
      //int internal_fmt;
      //glGetTexLevelParameteriv (target, level, GL_TEXTURE_INTERNAL_FORMAT, &internal_fmt);
      //dll_log.Log (L"[ GL Trace ] Destination internal format: 0x%X", internal_fmt);
      glCopyTexSubImage2D_Original ( target, level,
                                       xoffset, yoffset,
                                         x, y,
                                           width, height );
      break;

  }
}

typedef GLvoid (WINAPI *glScissor_pfn)(
  GLint   x,
  GLint   y,
  GLsizei width,
  GLsizei height
);

glScissor_pfn glScissor_Original = nullptr;

__declspec (noinline)
GLvoid
WINAPI
glScissor_Detour (GLint x, GLint y, GLsizei width, GLsizei height)
{
  //dll_log.Log (L"[GL CallLog] -- glScissor (%li, %li, %li, %li)",
    //x, y,
    //width, height);

  return glScissor_Original (x, y, width, height);
}

typedef GLvoid (WINAPI *glViewport_pfn)(
  GLint x,
  GLint y,
  GLsizei width,
  GLsizei heights
);

glViewport_pfn glViewport_Original = nullptr;

__declspec (noinline)
GLvoid
WINAPI
glViewport_Detour (GLint x, GLint y, GLsizei width, GLsizei height)
{
  if ((width == 1280 && height == 720) /*||
      (width == 960  && height == 544)*/) {
    width  = config.render.scene_res_x;
    height = config.render.scene_res_y;

    drawing_main_scene = true;
  } else {
    drawing_main_scene = false;
  }

#if 0
  if (width == 1024 && height == 512) {
    width = RES_X; height = RES_Y;
  }
#endif

  glViewport_Original (x, y, width, height);
}

typedef GLvoid (WINAPI *glUniformMatrix4fv_pfn)(
        GLint      location,
        GLsizei    count,
        GLboolean  transpose,
  const GLfloat   *value
);

glUniformMatrix4fv_pfn glUniformMatrix4fv_Original = nullptr;

__declspec (noinline)
GLvoid
WINAPI
glUniformMatrix4fv_Detour (
        GLint      location,
        GLsizei    count,
        GLboolean  transpose,
  const GLfloat   *value )
{
#if 0
  dll_log.Log ( L"[ GL Trace ] -- glUniformMatrix4fv (Uniform Loc=%li, count=%li, transpose=%s)",
                    location, count, transpose ? L"yes" : L"no" );
  dll_log.Log ( L"[ GL Trace ]        %11.6f, %11.6f, %11.6f, %11.6f",
                    value [ 0], value [ 1], value [ 2], value [ 3] );
  dll_log.Log ( L"[ GL Trace ]        %11.6f, %11.6f, %11.6f, %11.6f",
                    value [ 4], value [ 5], value [ 6], value [ 7] );
  dll_log.Log ( L"[ GL Trace ]        %11.6f, %11.6f, %11.6f, %11.6f",
                    value [ 8], value [ 9], value [10], value [11] );
  dll_log.Log ( L"[ GL Trace ]        %11.6f, %11.6f, %11.6f, %11.6f",
                    value [12], value [13], value [14], value [15] );
#endif

  glUniformMatrix4fv_Original (location, count, transpose, value);
}

typedef GLvoid (WINAPI *glUniformMatrix3fv_pfn)(
        GLint      location,
        GLsizei    count,
        GLboolean  transpose,
  const GLfloat   *value
);

glUniformMatrix3fv_pfn glUniformMatrix3fv_Original = nullptr;

__declspec (noinline)
GLvoid
WINAPI
glUniformMatrix3fv_Detour (
        GLint      location,
        GLsizei    count,
        GLboolean  transpose,
  const GLfloat   *value )
{
  dll_log.Log ( L"[ GL Trace ] -- glUniformMatrix3fv (Uniform Loc=%li, count=%li, transpose=%s)",
                    location, count, transpose ? L"yes" : L"no" );
  dll_log.Log ( L"[ GL Trace ]        %11.6f, %11.6f, %11.6f",
                    value [ 0], value [ 1], value [ 2] );
  dll_log.Log ( L"[ GL Trace ]        %11.6f, %11.6f, %11.6f",
                    value [ 3], value [ 4], value [ 5] );
  dll_log.Log ( L"[ GL Trace ]        %11.6f, %11.6f, %11.6f",
                    value [ 6], value [ 7], value [ 8] );

  glUniformMatrix3fv_Original (location, count, transpose, value);
}

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
  dll_log.Log ( L"[GL Shaders] Shader Obj Id=%lu, count=%li, Src=%hs",
                  shader, count, *strg );
  glShaderSource_Original (shader, count, string, length);
#else
  char* szShaderSrc = strdup ((const char *)*string);
  char* szTexWidth  = strstr (szShaderSrc, "const float texWidth = 720.0;  ");

  if (szTexWidth != nullptr) {
    char szNewWidth [33];
    sprintf (szNewWidth, "const float texWidth = %7.1f;", (float)config.render.scene_res_y);
    memcpy (szTexWidth, szNewWidth, 31);
  }

  glShaderSource_Original (shader, count, (const GLubyte **)&szShaderSrc, length);

#if 0
  dll_log.Log ( L"[GL Shaders] Shader Obj Id=%lu, count=%li, Src=%hs",
                  shader, count, szShaderSrc );
#endif

  free (szShaderSrc);
#endif
}

typedef PROC (APIENTRY *wglGetProcAddress_pfn)(LPCSTR);

wglGetProcAddress_pfn wglGetProcAddress_Original = nullptr;


__declspec (noinline)
PROC
APIENTRY
wglGetProcAddress_Detour (LPCSTR szFuncName)
{
  // Setup a VSYNC override immediately
  if (! wglSwapIntervalEXT_Original) {
    wglSwapIntervalEXT_Original =
      (wglSwapIntervalEXT_pfn)
        wglGetProcAddress_Original ("wglSwapIntervalEXT");

    if (wglSwapIntervalEXT_Original != nullptr)
      wglSwapIntervalEXT_Detour (1);
  }

  PROC ret = wglGetProcAddress_Original (szFuncName);

  bool detoured = false;

  if (! strcmp (szFuncName, "glShaderSource") && (ret != nullptr)) {
    glShaderSource_Original = (glShaderSource_pfn)ret;
    ret                     = (PROC)glShaderSource_Detour;
    detoured                = true;
  }

#if 0
  if (! strcmp (szFuncName, "glUniformMatrix3fv") && (ret != nullptr)) {
    glUniformMatrix3fv_Original = (glUniformMatrix3fv_pfn)ret;
    ret                         = (PROC)glUniformMatrix3fv_Detour;
    detoured                    = true;
  }

  if (! strcmp (szFuncName, "glUniformMatrix4fv") && (ret != nullptr)) {
    glUniformMatrix4fv_Original = (glUniformMatrix4fv_pfn)ret;
    ret                         = (PROC)glUniformMatrix4fv_Detour;
    detoured                    = true;
  }
#endif

  if (! strcmp (szFuncName, "glBlitFramebuffer") && (ret != nullptr)) {
    glBlitFramebuffer_Original = (glBlitFramebuffer_pfn)ret;
    ret                        = (PROC)glBlitFramebuffer_Detour;
    detoured                   = true;
  }

  if (! strcmp (szFuncName, "glRenderbufferStorage") && (ret != nullptr)) {
    glRenderbufferStorage_Original = (glRenderbufferStorage_pfn)ret;
    ret                            = (PROC)glRenderbufferStorage_Detour;
    detoured                       = true;
  }

  if (! strcmp (szFuncName, "wglSwapIntervalEXT") && (ret != nullptr)) {
    wglSwapIntervalEXT_Original = (wglSwapIntervalEXT_pfn)ret;
    ret                         = (PROC)wglSwapIntervalEXT_Detour;
    detoured                    = true;
  }

  if (! strcmp (szFuncName, "glDebugMessageCallbackARB") && ret != nullptr) {
    extern void PP_Init_glDebug (void);
    PP_Init_glDebug ();

    ret     = nullptr;
    detoured = true;
  }

  dll_log.LogEx (true, L"[OpenGL Ext] %#32hs => %ph", szFuncName, ret);

  if (detoured)
    dll_log.LogEx (false, L"    {Detoured}\n");
  else
    dll_log.LogEx (false, L"\n");

  return ret;
}


typedef GLvoid (WINAPI *glFinish_pfn)(void);
glFinish_pfn glFinish_Original = nullptr;

__declspec (noinline)
GLvoid
WINAPI
glFinish_Detour (void)
{
  if (config.compatibility.allow_gl_cpu_sync)
    glFinish_Original ();
}

typedef GLvoid (WINAPI *glFlush_pfn)(void);
glFlush_pfn glFlush_Original = nullptr;


__declspec (noinline)
GLvoid
WINAPI
glFlush_Detour (void)
{
  //if (config.compatibility.allow_gl_cpu_sync)
    glFlush_Original ();
}



typedef GLvoid (WINAPI *glLoadMatrixf_pfn)(
  const GLfloat* m
);

glLoadMatrixf_pfn glLoadMatrixf_Original = nullptr;

__declspec (noinline)
GLvoid
WINAPI
glLoadMatrixf_Detour (const GLfloat* m)
{
#if 0
  //if (drawing_main_scene) {
    dll_log.Log ( L"[ GL Trace ] glLoadMatrixf -- %11.6f %11.6f %11.6f %11.6f",
                    m [ 0], m [ 1], m [ 2], m [ 3] );
    dll_log.Log ( L"                           -- %11.6f %11.6f %11.6f %11.6f",
                    m [ 4], m [ 5], m [ 6], m [ 7] );
    dll_log.Log ( L"                           -- %11.6f %11.6f %11.6f %11.6f",
                    m [ 8], m [ 9], m [10], m [11] );
    dll_log.Log ( L"                           -- %11.6f %11.6f %11.6f %11.6f",
                    m [12], m [13], m [14], m [15] );
  //}
#endif

#if 0
  if (drawing_main_scene) {
    float scale [16] = { RES_X / 1280.0f, 0.0f,           0.0f, 0.0f,
                         0.0f,            RES_Y / 720.0f, 0.0f, 0.0f,
                         0.0f,            0.0f,           1.0f, 0.0f,
                         0.0f,            0.0f,           0.0f, 1.0f };

    float out [16];

    for (int i = 0; i < 16; i += 4)
      for (int j = 0; j < 4; j++)
        out [i+j] = scale [j] * m [i] + scale [j+4] * m [i+1] + scale [j+8] * m [i+2] + scale [j+12] * m [i+3];

      return glLoadMatrixf_Original (out);
    //}
  }
#endif

  glLoadMatrixf_Original (m);
}


typedef GLvoid (WINAPI *glOrtho_pfn)(
  GLdouble left,    GLdouble right,
  GLdouble bottom,  GLdouble top,
  GLdouble nearVal, GLdouble farVal
);

glOrtho_pfn glOrtho_Original = nullptr;

__declspec (noinline)
GLvoid
WINAPI
glOrtho_Detour
( GLdouble left,    GLdouble right,
  GLdouble bottom,  GLdouble top,
  GLdouble nearVal, GLdouble farVal )
{
  if (left == right)
    right = left + DBL_EPSILON;

  if (bottom == top)
    bottom = top + DBL_EPSILON;

  // Stop calling glOrtho with Near and Far set the same!
  if (nearVal == farVal)
    farVal = nearVal + DBL_EPSILON;

  glOrtho_Original ( left, right,
                       bottom, top,
                         nearVal, farVal );
}

typedef GLvoid (WINAPI *glScalef_pfn)(
  GLfloat x,
  GLfloat y,
  GLfloat z
);

glScalef_pfn glScalef_Original = nullptr;

__declspec (noinline)
GLvoid
WINAPI
glScalef_Detour ( GLfloat x,
                  GLfloat y,
                  GLfloat z )
{
  if (drawing_main_scene) {
    dll_log.Log ( L"[ GL Trace ] glScalef -- %11.6f %11.6f %11.6f",
                    x, y, z );
  }

  glScalef_Original (x, y, z);
}


typedef BOOL (__stdcall *wglSwapBuffers_pfn)(HDC hdc);
static wglSwapBuffers_pfn wglSwapBuffers = nullptr;

LPVOID lpvDontCare = nullptr;

BOOL
__stdcall
SwapBuffers_Detour (HDC hdc)
{
  return wglSwapBuffers (hdc);
}


//
// May wish to override this later for MSAA (though most MSAA stuff would be better done with an FBO)
//
typedef int (WINAPI *ChoosePixelFormat_pfn)(HDC hdc, const PIXELFORMATDESCRIPTOR *ppfd);
ChoosePixelFormat_pfn ChoosePixelFormat_Original = nullptr;

int
WINAPI
ChoosePixelFormat_Detour
(
         HDC                   hdc,
   const PIXELFORMATDESCRIPTOR *ppfd
)
{
  PIXELFORMATDESCRIPTOR pfd = { 0 };

  dll_log.Log (L"[  WinGDI  ] ChoosePixelFormat - Flags: 0x%X", ppfd->dwFlags);

  pfd.nSize          = sizeof pfd;

  pfd.nVersion       = 1;
  pfd.dwFlags        = PFD_DOUBLEBUFFER | PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_SWAP_EXCHANGE;

  pfd.iPixelType      = PFD_TYPE_RGBA;
  pfd.iLayerType      = PFD_MAIN_PLANE;

  pfd.cAccumBits      = 0;
  pfd.cAccumAlphaBits = 0;
  pfd.cAccumBlueBits  = 0;
  pfd.cAccumGreenBits = 0;
  pfd.cAccumRedBits   = 0;

  pfd.cAlphaBits      = 0;
  pfd.cAlphaShift     = 0;

  pfd.cRedBits        = 8;
  pfd.cRedShift       = 0;

  pfd.cBlueBits       = 8;
  pfd.cBlueShift      = 0;

  pfd.cGreenBits      = 8;
  pfd.cGreenShift     = 0;

  pfd.cAuxBuffers     = 0;

  pfd.cColorBits      = 24;
  pfd.cDepthBits      = 24;
  pfd.cStencilBits    = 8;

  return ChoosePixelFormat_Original (hdc, &pfd);
}




typedef HGLRC (WINAPI *wglCreateContextAttribsARB_pfn)(
        HDC   hDC,
        HGLRC hShareContext,
  const int * attribList
);

wglCreateContextAttribsARB_pfn wglCreateContextAttribsARB = nullptr;

LPVOID wglCreateContext_Hook = nullptr;

typedef HGLRC (WINAPI *wglCreateContext_pfn)(HDC hDC);
wglCreateContext_pfn wglCreateContext_Original = nullptr;

__declspec (noinline)
HGLRC
WINAPI
wglCreateContext_Detour (HDC hDC)
{
#define WGL_CONTEXT_MAJOR_VERSION_ARB             0x2091
#define WGL_CONTEXT_MINOR_VERSION_ARB             0x2092
#define WGL_CONTEXT_FLAGS_ARB                     0x2094
#define WGL_CONTEXT_PROFILE_MASK_ARB              0x9126

#define WGL_CONTEXT_DEBUG_BIT_ARB                 0x0001
#define WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB    0x0002

#define WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB 0x0002

  if ((! config.compatibility.support_old_drivers) && wglCreateContextAttribsARB != nullptr) {
    GLint debug_bits = config.compatibility.debug_mode ? WGL_CONTEXT_DEBUG_BIT_ARB : 0;

    GLint attribs [] = {
      WGL_CONTEXT_MAJOR_VERSION_ARB, 3,
      WGL_CONTEXT_MINOR_VERSION_ARB, 2,
      WGL_CONTEXT_FLAGS_ARB,         debug_bits,

      // Not my idea of fun, but I didn't write this software
      WGL_CONTEXT_PROFILE_MASK_ARB,  WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB,

      0
    };

    HGLRC ret = 0;

    //
    // Intel Hack:  Prevent infinite recursion
    //
    PPrinny_DisableHook (wglCreateContext_Hook);
    {
      ret = wglCreateContextAttribsARB (hDC, 0, attribs);
    }
    PPrinny_EnableHook (wglCreateContext_Hook);


    if (ret != 0)
      return ret;

    dll_log.Log (L"[   XXX   ] wglCreateContextAttribsARB (...) failed?!\n");
  }

  return wglCreateContext_Original (hDC);
}


LRESULT CALLBACK Dummy_WndProc (HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

void
PP_InitGL (void)
{
  HINSTANCE hInstance = GetModuleHandle (NULL);

  MSG msg          = { 0 };
  WNDCLASS wc      = { 0 }; 
  wc.lpfnWndProc   = Dummy_WndProc;
  wc.hInstance     = hInstance;
  wc.hbrBackground = (HBRUSH)(COLOR_BACKGROUND);
  wc.lpszClassName = L"Dumy_Pretty_Prinny";
  wc.style         = CS_OWNDC;

  if (! RegisterClass (&wc))
    return;

  CreateWindowW ( wc.lpszClassName,
                    L"Dummy_Pretty_Prinny",
                      WS_OVERLAPPEDWINDOW/* | WS_VISIBLE*/,
                        0,0,
                          0,0,
                            0,0,
                              hInstance,0);

  if (PeekMessage (&msg, NULL, 0, 0, 1) > 0)
    DispatchMessage (&msg);
}

LRESULT
CALLBACK
Dummy_WndProc ( HWND   hWnd,
                UINT   message,
                WPARAM wParam,
                LPARAM lParam )
{
  switch (message)
  {
    case WM_CREATE:
    {
      PIXELFORMATDESCRIPTOR pfd =
      {
        sizeof (PIXELFORMATDESCRIPTOR),
        1,
        PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
        PFD_TYPE_RGBA,
        32,
        0, 0, 0, 0, 0, 0,
        0,
        0,
        0,
        0, 0, 0, 0,
        24,
        8,
        0,
        PFD_MAIN_PLANE,
        0,
        0, 0, 0
      };

      HDC dummy_dc     = GetDC             (hWnd);
      int dummy_pf_idx = ChoosePixelFormat (dummy_dc, &pfd); 

      SetPixelFormat (dummy_dc, dummy_pf_idx, &pfd);

      HGLRC dummy_ctx = wglCreateContext (dummy_dc);
                        wglMakeCurrent   (dummy_dc, dummy_ctx);

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
                              "glTexImage2D",
                              glTexImage2D_Detour,
                   (LPVOID *)&glTexImage2D_Original );

      PPrinny_CreateDLLHook ( config.system.injector.c_str (),
                              "glCopyTexSubImage2D",
                              glCopyTexSubImage2D_Detour,
                   (LPVOID *)&glCopyTexSubImage2D_Original );

      PPrinny_CreateDLLHook ( config.system.injector.c_str (),
                              "glScissor",
                              glScissor_Detour,
                   (LPVOID *)&glScissor_Original );

      PPrinny_CreateDLLHook ( config.system.injector.c_str (),
                              "glViewport",
                              glViewport_Detour,
                   (LPVOID *)&glViewport_Original );

      PPrinny_CreateDLLHook ( config.system.injector.c_str (),
                              "glFlush",
                              glFlush_Detour,
                   (LPVOID *)&glFlush_Original );

      PPrinny_CreateDLLHook ( config.system.injector.c_str (),
                              "glFinish",
                              glFinish_Detour,
                   (LPVOID *)&glFinish_Original );


      // It sucks to have to hook THESE functions, but there are display lists
      //   that have wrong state setup.
      PPrinny_CreateDLLHook ( config.system.injector.c_str (),
                              "glEnable",
                              glEnable_Detour,
                   (LPVOID *)&glEnable_Original );

      PPrinny_CreateDLLHook ( config.system.injector.c_str (),
                              "glDisable",
                              glDisable_Detour,
                   (LPVOID *)&glDisable_Original );



//#define FIX_ARB_IMAGING_PROBLEMS
#ifdef FIX_ARB_IMAGING_PROBLEMS
      PPrinny_CreateDLLHook ( config.system.injector.c_str (),
                              "glMatrixMode",
                              glMatrixMode_Detour,
                   (LPVOID *)&glMatrixMode_Original );

      PPrinny_CreateDLLHook ( config.system.injector.c_str (),
                              "glGetFloatv",
                              glGetFloatv_Detour,
                   (LPVOID *)&glGetFloatv_Original );
#endif



#if 0
      wglSwapBuffers =
        (wglSwapBuffers_pfn)
          GetProcAddress ( GetModuleHandle ( config.system.injector.c_str () ),
                             "wglSwapBuffers" );

      PPrinny_CreateDLLHook ( L"gdi32.dll",
                              "SwapBuffers",
                              SwapBuffers_Detour,
                   (LPVOID *)&lpvDontCare );
#endif

      PPrinny_CreateDLLHook ( L"gdi32.dll",
                              "ChoosePixelFormat",
                              ChoosePixelFormat_Detour,
                   (LPVOID *)&ChoosePixelFormat_Original );

#if 0
      PPrinny_CreateDLLHook ( config.system.injector.c_str (),
                              "glLoadMatrixf",
                               glLoadMatrixf_Detour,
                   (LPVOID *)&glLoadMatrixf_Original );
#endif

       PPrinny_CreateDLLHook ( config.system.injector.c_str (),
                               "glOrtho",
                               glOrtho_Detour,
                    (LPVOID *)&glOrtho_Original );

#if 0
      PPrinny_CreateDLLHook ( config.system.injector.c_str (),
                              "glScalef",
                              glScalef_Detour,
                   (LPVOID *)&glScalef_Original );
#endif

      PPrinny_CreateDLLHook ( config.system.injector.c_str (),
                              "wglGetProcAddress",
                              wglGetProcAddress_Detour,
                   (LPVOID *)&wglGetProcAddress_Original );

      PPrinny_CreateDLLHook ( config.system.injector.c_str (),
                              "wglCreateContext",
                              wglCreateContext_Detour,
                   (LPVOID *)&wglCreateContext_Original,
                   (LPVOID *)&wglCreateContext_Hook );
      PPrinny_EnableHook    ( wglCreateContext_Hook );

      dll_log.LogEx ( false,
                      L"-------------------------------------------------------------------"
                      L"------------------------------------------------------------------\n" );

      dll_log.Log ( L"[OGL Driver] Dummy OpenGL Context for Init. Purposes" );
      dll_log.Log ( L"[OGL Driver] ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~" );
      dll_log.Log ( L"[OGL Driver]  Vendor...: %hs\n                         "
                    L"[OGL Driver]  Renderer.: %hs\n                         "
                    L"[OGL Driver]  Version..: %hs",
                      glGetString (GL_VENDOR),
                        glGetString (GL_RENDERER),
                          glGetString (GL_VERSION) );

      wglCreateContextAttribsARB =
        (wglCreateContextAttribsARB_pfn)
          wglGetProcAddress_Original ("wglCreateContextAttribsARB");

      if (wglCreateContextAttribsARB != nullptr)
        dll_log.Log ( L"[OGL Driver]    * Has wglCreateContextAttribsARB");

      if (wglGetProcAddress_Original ("glDebugMessageCallbackARB") != nullptr)
        dll_log.Log ( L"[OGL Driver]    * Has  glDebugMessageCallbackARB");

      // INIT EXTENSIONS TO CREATE CONTEXT HERE

      dll_log.LogEx ( false,
                      L"-------------------------------------------------------------------"
                      L"------------------------------------------------------------------\n" );

      wglDeleteContext (dummy_ctx);
      DestroyWindow    (hWnd);
    } break;

    default:
      return DefWindowProc (hWnd, message, wParam, lParam);
  }

  return 0;
}



void
pp::RenderFix::Init (void)
{
  PP_InitGL ();

  CommandProcessor*     comm_proc    = CommandProcessor::getInstance ();
  eTB_CommandProcessor* pCommandProc = SK_GetCommandProcessor        ();

  // Don't directly modify this state, switching mid-frame would be disasterous...
  pCommandProc->AddVariable ("Render.MSAA",             new eTB_VarStub <bool> (&use_msaa));

  pCommandProc->AddVariable ("Trace.NumFrames",        new eTB_VarStub <int>  (&tracer.count));
  pCommandProc->AddVariable ("Trace.Enable",           new eTB_VarStub <bool> (&tracer.log));

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
           pp::RenderFix::CommandProcessor::pCommProc = nullptr;

HWND     pp::RenderFix::hWndDevice       = NULL;

bool     pp::RenderFix::fullscreen       = false;
uint32_t pp::RenderFix::width            = 0UL;
uint32_t pp::RenderFix::height           = 0UL;

HMODULE  pp::RenderFix::user32_dll       = 0;