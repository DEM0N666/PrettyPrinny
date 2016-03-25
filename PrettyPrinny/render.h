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
#ifndef __PPRINNY__RENDER_H__
#define __PPRINNY__RENDER_H__

#include "command.h"

#include <Windows.h>

namespace pp
{
  namespace RenderFix
  {
    struct tracer_s {
      bool log   = false;
      int  count = 0;
    } extern tracer;

    struct pp_draw_states_s {
      bool         has_aniso      = false; // Has he game even once set anisotropy?!
      int          max_aniso      = 4;
      bool         has_msaa       = false;
      bool         use_msaa       = true;  // Allow MSAA toggle via console
                                           //  without changing the swapchain.
      bool         fullscreen     = false;

      uint32_t     depth_tex      = 0; // Avoid costly copy operations by attaching
                                       //   this directly to the primary framebuffer.

      int          draws          = 0; // Number of draw calls
      int          frames         = 0;
      DWORD        thread         = 0;
    } extern draw_state;


    void Init     ();
    void Shutdown ();

    class CommandProcessor : public eTB_iVariableListener {
    public:
      CommandProcessor (void);

      virtual bool OnVarChange (eTB_Variable* var, void* val = NULL);

      static CommandProcessor* getInstance (void)
      {
        if (pCommProc == NULL)
          pCommProc = new CommandProcessor ();

        return pCommProc;
      }

    protected:

    private:
      static CommandProcessor* pCommProc;
    };

    extern HWND                hWndDevice;

    extern uint32_t            width;
    extern uint32_t            height;
    extern bool                fullscreen;

    extern HMODULE             user32_dll;

    extern eTB_VarStub <bool>* high_precision_ssao;
  }
}

typedef void (STDMETHODCALLTYPE *BMF_BeginBufferSwap_pfn)(void);

typedef HRESULT (STDMETHODCALLTYPE *BMF_EndBufferSwap_pfn)
(
  HRESULT   hr,
  IUnknown *device
);

#include <gl/GL.h>
#pragma comment (lib, "OpenGL32.lib")

typedef int (WINAPI *ChoosePixelFormat_pfn)(
        HDC                    hdc,
  const PIXELFORMATDESCRIPTOR *ppfd
);

typedef BOOL (WINAPI *wglSwapIntervalEXT_pfn)(
  INT interval
);

typedef BOOL (WINAPI *wglCopyContext_pfn)(
 HGLRC hglrcSrc,
 HGLRC hglrcDst,
 UINT  mask
);

typedef BOOL (WINAPI *wglMakeCurrent_pfn)(
 HDC   hdc,
 HGLRC hglrc
);

typedef BOOL (WINAPI *wglSwapBuffers_pfn)(
  HDC hdc
);

typedef HGLRC (WINAPI *wglCreateContextAttribsARB_pfn)(
        HDC   hDC,
        HGLRC hShareContext,
  const int * attribList
);

typedef HGLRC (WINAPI *wglCreateContext_pfn)(
  HDC hDC
);

typedef PROC (WINAPI *wglGetProcAddress_pfn)(
  LPCSTR szFuncName
);

extern wglGetProcAddress_pfn wglGetProcAddress_Log;

#define WGL_CONTEXT_MAJOR_VERSION_ARB             0x2091
#define WGL_CONTEXT_MINOR_VERSION_ARB             0x2092
#define WGL_CONTEXT_FLAGS_ARB                     0x2094
#define WGL_CONTEXT_PROFILE_MASK_ARB              0x9126

#define WGL_CONTEXT_DEBUG_BIT_ARB                 0x0001
#define WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB    0x0002

#define WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB 0x0002




typedef GLvoid (WINAPI *glGenerateMipmap_pfn)(
  GLenum target
);

typedef GLvoid (WINAPI *glDepthMask_pfn)(
  GLboolean flag
);

typedef GLvoid (WINAPI *glBindRenderbuffer_pfn)(
  GLenum target,
  GLuint renderbuffer
);

typedef GLvoid (WINAPI *glGenRenderbuffers_pfn)(
  GLsizei  n, 
  GLuint  *renderbuffers
);

typedef GLvoid (WINAPI *glDeleteRenderbuffers_pfn)(
        GLsizei  n,
  const GLuint  *renderbuffers
);

typedef GLvoid (WINAPI *glBindTexture_pfn)(
  GLenum target,
  GLuint texture
);

typedef GLvoid (WINAPI *glFramebufferTexture2D_pfn)(
  GLenum target,
  GLenum attachment,
  GLenum textarget,
  GLuint texture,
  GLint level
);

typedef GLvoid (WINAPI *glFramebufferRenderbuffer_pfn)(
  GLenum target,
  GLenum attachment,
  GLenum renderbuffertarget,
  GLuint renderbuffer
);

typedef GLvoid (WINAPI *glGenFramebuffers_pfn)(
  GLsizei  n,
  GLuint  *framebuffers
);

typedef GLenum (WINAPI *glCheckFramebufferStatus_pfn)(
  GLenum target
);

typedef GLvoid (WINAPI *glReadPixels_pfn)(
  GLint   x,
  GLint   y,
  GLsizei width,
  GLsizei height,
  GLenum  format,
  GLenum  type,
  GLvoid* data
);

typedef GLvoid (WINAPI *glDrawBuffers_pfn)(
        GLsizei n,
  const GLenum* bufs
);

typedef GLvoid (WINAPI *glUseProgram_pfn)(
  GLuint program
);

typedef GLvoid (WINAPI *glBlendFunc_pfn)(
  GLenum srcfactor,
  GLenum dstfactor
);

typedef GLvoid (WINAPI *glAlphaFunc_pfn)(
  GLenum  alpha_test,
  GLfloat ref_val
);

typedef GLvoid (WINAPI *glEnable_pfn)(
  GLenum cap
);

typedef GLvoid (WINAPI *glDisable_pfn)(
  GLenum cap
);

typedef GLvoid (WINAPI *glTexParameteri_pfn)(
  GLenum target,
  GLenum pname,
  GLint  param
);

typedef GLvoid (WINAPI *glCompressedTexImage2D_pfn)(
        GLenum  target, 
        GLint   level,
        GLenum  internalformat,
        GLsizei width,
        GLsizei height,
        GLint   border,
        GLsizei imageSize, 
  const GLvoid* data
);

typedef GLvoid (WINAPI *glBindFramebuffer_pfn)(
  GLenum target,
  GLuint framebuffer
);

typedef GLvoid (WINAPI *glColorMaski_pfn)(
  GLuint    buf,
  GLboolean red,
  GLboolean green,
  GLboolean blue,
  GLboolean alpha
);

typedef GLvoid (WINAPI *glDrawArrays_pfn)(
  GLenum  mode,
  GLint   first,
  GLsizei count
);

typedef GLvoid (WINAPI *glMatrixMode_pfn)(
  GLenum mode
);

typedef GLvoid (WINAPI *glGetFloatv_pfn)(
  GLenum   pname,
  GLfloat* pparam
);

typedef GLvoid (WINAPI *glDeleteTextures_pfn)(
  GLsizei n,
  GLuint* textures
);

typedef GLvoid (WINAPI *glTexSubImage2D_pfn)(
        GLenum   target,
        GLint    level,
        GLint    xoffset,
        GLint    yoffset,
        GLsizei  width,
        GLsizei  height,
        GLenum   format,
        GLenum   type,
  const GLvoid  *pixels
);

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

typedef GLvoid (WINAPI *glRenderbufferStorageMultisample_pfn)(
  GLenum  target,
  GLsizei samples,
  GLenum  internalformat,
  GLsizei width,
  GLsizei height
);

typedef GLvoid (WINAPI *glRenderbufferStorage_pfn)(
  GLenum  target,
  GLenum  internalformat,
  GLsizei width,
  GLsizei height
);

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

typedef GLvoid (WINAPI *glScissor_pfn)(
  GLint   x,
  GLint   y,
  GLsizei width,
  GLsizei height
);

typedef GLvoid (WINAPI *glViewport_pfn)(
  GLint x,
  GLint y,
  GLsizei width,
  GLsizei heights
);

typedef GLvoid (WINAPI *glUniformMatrix4fv_pfn)(
        GLint      location,
        GLsizei    count,
        GLboolean  transpose,
  const GLfloat   *value
);

typedef GLvoid (WINAPI *glUniformMatrix3fv_pfn)(
        GLint      location,
        GLsizei    count,
        GLboolean  transpose,
  const GLfloat   *value
);

typedef GLvoid (WINAPI *glVertexAttrib4f_pfn)(
  GLuint  index,
  GLfloat v0,
  GLfloat v1,
  GLfloat v2,
  GLfloat v3
);

typedef GLvoid (WINAPI *glVertexAttrib4fv_pfn)(
        GLuint   index,
  const GLfloat* v
);

typedef GLvoid (WINAPI *glGetShaderiv_pfn)(
  GLuint shader,
  GLenum pname,
  GLint *params
);

typedef GLvoid (WINAPI *glShaderSource_pfn)(
         GLuint    shader,
         GLsizei   count,
 const   GLubyte **string,
 const   GLint    *length
);

typedef GLvoid (WINAPI *glAttachShader_pfn)(
  GLuint program,
  GLuint shader
);

typedef GLvoid (WINAPI *glFinish_pfn)(
  void
);

typedef GLvoid (WINAPI *glFlush_pfn)(
  void
);

typedef GLvoid (WINAPI *glLoadMatrixf_pfn)(
  const GLfloat* m
);

typedef GLvoid (WINAPI *glOrtho_pfn)(
  GLdouble left,    GLdouble right,
  GLdouble bottom,  GLdouble top,
  GLdouble nearVal, GLdouble farVal
);


typedef GLvoid (WINAPI *glScalef_pfn)(
  GLfloat x,
  GLfloat y,
  GLfloat z
);

#define GL_MULTISAMPLE                    0x809D

#define GL_FRAMEBUFFER_BINDING            0x8CA6
#define GL_DRAW_FRAMEBUFFER_BINDING       0x8CA6
#define GL_READ_FRAMEBUFFER_BINDING       0x8CAA

#define GL_FRAMEBUFFER_COMPLETE           0x8CD5

#define GL_RENDERBUFFER_BINDING           0x8CA7
#define GL_RENDERBUFFER                   0x8D41

#define GL_READ_FRAMEBUFFER               0x8CA8
#define GL_DRAW_FRAMEBUFFER               0x8CA9
#define GL_FRAMEBUFFER                    0x8D40

#define GL_DRAW_BUFFER0                   0x8825

#define GL_COLOR_ATTACHMENT0              0x8CE0
#define GL_DEPTH_ATTACHMENT               0x8D00

#define GL_DEPTH_COMPONENT24              0x81A6
#define GL_RG                             0x8227
#define GL_R8                             0x8229
#define GL_RG8                            0x8051
#define GL_RGBA8                          0x8058
#define GL_R16F                           0x822D
#define GL_RG16F                          0x822F
#define GL_RGB16F                         0x881B
#define GL_RGBA16F                        0x881A
#define GL_RGB32F                         0x8815

#define GL_R11F_G11F_B10F                 0x8C3A
#define GL_RGB10_A2                       0x8059

#define GL_CLAMP_TO_EDGE                  0x812F
#define GL_MIRRORED_REPEAT                0x8370

#define GL_TEXTURE_SWIZZLE_RGBA           0x8E46

#define GL_COLOR_MATRIX_SGI               0x80B1

#define GL_SHADER_TYPE                    0x8B4F
#define GL_FRAGMENT_SHADER                0x8B30
#define GL_VERTEX_SHADER                  0x8B31

#endif /* __PPRINNY__RENDER_H__ */