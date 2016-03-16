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

struct {
  bool use           = true;
  bool dirty         = false;
  bool early_resolve = false;
  bool conservative  = false;
} msaa;

bool drawing_main_scene = false;

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

#define GL_MULTISAMPLE                    0x809D

#define GL_FRAMEBUFFER_BINDING            0x8CA6
#define GL_DRAW_FRAMEBUFFER_BINDING       0x8CA6
#define GL_READ_FRAMEBUFFER_BINDING       0x8CAA

#define GL_RENDERBUFFER_BINDING           0x8CA7
#define GL_RENDERBUFFER                   0x8D41

#define GL_READ_FRAMEBUFFER               0x8CA8
#define GL_DRAW_FRAMEBUFFER               0x8CA9
#define GL_FRAMEBUFFER                    0x8D40

#define GL_COLOR_ATTACHMENT0              0x8CE0
#define GL_DEPTH_ATTACHMENT               0x8D00

#define GL_DEPTH_COMPONENT24              0x81A6
#define GL_RG                             0x8227
#define GL_RG8                            0x8051
#define GL_RGBA8                          0x8058
#define GL_RGBA16F                        0x881A

glGenRenderbuffers_pfn               glGenRenderbuffers                 = nullptr;
glBindRenderbuffer_pfn               glBindRenderbuffer                 = nullptr;
glRenderbufferStorageMultisample_pfn glRenderbufferStorageMultisample   = nullptr;
glDeleteRenderbuffers_pfn            glDeleteRenderbuffers_Original     = nullptr;
glRenderbufferStorage_pfn            glRenderbufferStorage_Original     = nullptr;

glDrawBuffers_pfn                    glDrawBuffers                      = nullptr;

glGenFramebuffers_pfn                glGenFramebuffers                  = nullptr;
glBindFramebuffer_pfn                glBindFramebuffer_Original         = nullptr;
glBlitFramebuffer_pfn                glBlitFramebuffer_Original         = nullptr;
glFramebufferTexture2D_pfn           glFramebufferTexture2D_Original    = nullptr;
glFramebufferRenderbuffer_pfn        glFramebufferRenderbuffer_Original = nullptr;
glColorMaski_pfn                     glColorMaski                       = nullptr;

glBindTexture_pfn                    glBindTexture_Original             = nullptr;
glTexImage2D_pfn                     glTexImage2D_Original              = nullptr;
glTexSubImage2D_pfn                  glTexSubImage2D_Original           = nullptr;
glCompressedTexImage2D_pfn           glCompressedTexImage2D_Original    = nullptr;
glCopyTexSubImage2D_pfn              glCopyTexSubImage2D_Original       = nullptr;
glDeleteTextures_pfn                 glDeleteTextures_Original          = nullptr;
glTexParameteri_pfn                  glTexParameteri_Original           = nullptr;

glReadPixels_pfn                     glReadPixels_Original              = nullptr;

glGetFloatv_pfn                      glGetFloatv_Original               = nullptr;

glMatrixMode_pfn                     glMatrixMode_Original              = nullptr;
glLoadMatrixf_pfn                    glLoadMatrixf_Original             = nullptr;
glOrtho_pfn                          glOrtho_Original                   = nullptr;
glScalef_pfn                         glScalef_Original                  = nullptr;
                                                                        
glEnable_pfn                         glEnable_Original                  = nullptr;
glDisable_pfn                        glDisable_Original                 = nullptr;
                                                                        
glAlphaFunc_pfn                      glAlphaFunc_Original               = nullptr;
glBlendFunc_pfn                      glBlendFunc_Original               = nullptr;
                                                                        
glScissor_pfn                        glScissor_Original                 = nullptr;
glViewport_pfn                       glViewport_Original                = nullptr;
                                                                        
glDrawArrays_pfn                     glDrawArrays_Original              = nullptr;
                                                                        
glUseProgram_pfn                     glUseProgram_Original              = nullptr;
glGetShaderiv_pfn                    glGetShaderiv_Original             = nullptr;
glShaderSource_pfn                   glShaderSource_Original            = nullptr;
                                                                        
glUniformMatrix4fv_pfn               glUniformMatrix4fv_Original        = nullptr;
glUniformMatrix3fv_pfn               glUniformMatrix3fv_Original        = nullptr;
                                                                        
glVertexAttrib4f_pfn                 glVertexAttrib4f_Original          = nullptr;
glVertexAttrib4fv_pfn                glVertexAttrib4fv_Original         = nullptr;
                                                                        
glFinish_pfn                         glFinish_Original                  = nullptr;
glFlush_pfn                          glFlush_Original                   = nullptr;
                                                                        
ChoosePixelFormat_pfn                ChoosePixelFormat_Original         = nullptr;
                                                                        
wglGetProcAddress_pfn                wglGetProcAddress_Original         = nullptr;
                                                                        
wglSwapIntervalEXT_pfn               wglSwapIntervalEXT_Original        = nullptr;
wglSwapBuffers_pfn                   wglSwapBuffers                     = nullptr;
                                                                        
wglCreateContextAttribsARB_pfn       wglCreateContextAttribsARB         = nullptr;
wglCreateContext_pfn                 wglCreateContext_Original          = nullptr;
wglCopyContext_pfn                   wglCopyContext_Original            = nullptr;
wglMakeCurrent_pfn                   wglMakeCurrent_Original            = nullptr;
                                                                        
BMF_BeginBufferSwap_pfn              BMF_BeginBufferSwap                = nullptr;
BMF_EndBufferSwap_pfn                BMF_EndBufferSwap                  = nullptr;


extern void
PPrinny_DumpCompressedTexLevel ( uint32_t crc32,
                                 GLint    level,
                                 GLenum   format,
                                 GLsizei  width,
                                 GLsizei  height,

                                 GLsizei  imageSize,
                           const GLvoid*  data );


extern void
PPrinny_DumpUncompressedTexLevel ( uint32_t crc32,
                                   GLint    level,
                                   GLenum   format,
                                   GLsizei  width,
                                   GLsizei  height,

                                   GLsizei  imageSize,
                             const GLvoid*  data );


#include <map>

struct msaa_tex_ref_s {
  bool   dirty      = false;

  GLuint rbo        = GL_NONE;
  GLint  color_loc  = -1;
  GLuint parent_fbo = -1;
};

struct msaa_rbo_ref_s {
  bool   dirty      = false;

  GLuint rbo        = GL_NONE;
  GLint  color_loc  = -1;
  GLuint parent_fbo = -1;
};

GLuint msaa_resolve_fbo = 0;

std::unordered_map <GLuint, msaa_tex_ref_s> msaa_backing_tex;
std::unordered_map <GLuint, msaa_rbo_ref_s> msaa_backing_rbo;

class PP_Framebuffer {
public:
  int getNumAttachments    (void) {
    return getNumRBAttachments  () +
           getNumTexAttachments ();
  }

  int getNumRBAttachments  (void) { return rbos_.size     (); }
  int getNumTexAttachments (void) { return textures_.size (); }

protected:
  std::unordered_map <GLuint, GLuint> rbos_;
  std::unordered_map <GLuint, GLuint> textures_;

private:
};


// These are textures that should always be clamped to edge for proper
//   smoothing behavior
#include <set>
std::set <GLuint> ui_textures;

GLuint active_program = 0;

COM_DECLSPEC_NOTHROW
void
STDMETHODCALLTYPE
OGLEndFrame_Pre (void)
{
  //pp::RenderFix::dwRenderThreadID = GetCurrentThreadId ();

  active_program  = 0;

  void PPrinny_DrawCommandConsole (void);
  PPrinny_DrawCommandConsole ();

  return BMF_BeginBufferSwap ();
}

COM_DECLSPEC_NOTHROW
HRESULT
STDMETHODCALLTYPE
OGLEndFrame_Post (HRESULT hr, IUnknown* pDev)
{
  if (pp::RenderFix::tracer.count > 0) {
    if (--pp::RenderFix::tracer.count == 0)
      pp::RenderFix::tracer.log = false;
  }

  pp::RenderFix::draw_state.frames++;
  pp::RenderFix::draw_state.thread = GetCurrentThreadId ();

  HRESULT ret = BMF_EndBufferSwap (S_OK, nullptr);

  if (config.render.msaa_samples > 1) {
    if (msaa.use)
      glEnable (GL_MULTISAMPLE);
    else
      glDisable (GL_MULTISAMPLE);
  }

  return ret;
}


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


bool real_blend_state   = false;
bool real_alpha_state   = false;

GLenum  real_alpha_test = GL_ALWAYS;
GLfloat real_alpha_ref  = 1.0f;

GLenum src_blend, dst_blend;

bool texture_2d = false;

__declspec (noinline)
GLvoid
WINAPI
glUseProgram_Detour (GLuint program)
{
  active_program = program;

  glUseProgram_Original (program);
}

__declspec (noinline)
GLvoid
WINAPI
glBlendFunc_Detour (GLenum srcfactor, GLenum dstfactor)
{
  src_blend = srcfactor;
  dst_blend = dstfactor;

  glBlendFunc_Original (srcfactor, dstfactor);
}

__declspec (noinline)
GLvoid
WINAPI
glAlphaFunc_Detour (GLenum alpha_test, GLfloat ref_val)
{
  real_alpha_test = alpha_test;
  real_alpha_ref  = ref_val;

  glAlphaFunc_Original (alpha_test, ref_val);
}

__declspec (noinline)
GLvoid
WINAPI
glEnable_Detour (GLenum cap)
{
  // This invalid state setup is embedded in some display lists
  if (cap == GL_TEXTURE) cap = GL_TEXTURE_2D;

  if (cap == GL_TEXTURE_2D) texture_2d = true;

  if (cap == GL_BLEND)      real_blend_state = true;
  if (cap == GL_ALPHA_TEST) real_alpha_state = true;

  return glEnable_Original (cap);
}

__declspec (noinline)
GLvoid
WINAPI
glDisable_Detour (GLenum cap)
{
  // This invalid state setup is embedded in some display lists
  if (cap == GL_TEXTURE) cap = GL_TEXTURE_2D;

  if (cap == GL_TEXTURE_2D) texture_2d = false;

  if (cap == GL_BLEND)      real_blend_state = false;
  if (cap == GL_ALPHA_TEST) real_alpha_state = false;

  return glDisable_Original (cap);
}

bool ui_clamp = true;

__declspec (noinline)
GLvoid
WINAPI
glTexParameteri_Detour (
  GLenum target,
  GLenum pname,
  GLint  param
)
{
#if 0
  GLint texture = 0;

  if (target == GL_TEXTURE_2D) {
    glGetIntegerv (GL_TEXTURE_BINDING_2D, &texture);
  }

  //
  // Clamp all UI textures (to edge)
  //
  if ( ui_clamp && (pname == GL_TEXTURE_WRAP_S ||
                    pname == GL_TEXTURE_WRAP_T) ) {
    if (texture != 0 && ui_textures.find (texture) != ui_textures.end ()) {
#define GL_CLAMP_TO_EDGE 0x812F 
      param = GL_CLAMP_TO_EDGE;
    }
  }
#endif

  return glTexParameteri_Original (target, pname, param);
}


__declspec (noinline)
GLvoid
WINAPI
glCompressedTexImage2D_Detour (
        GLenum  target, 
        GLint   level,
        GLenum  internalFormat,
        GLsizei width,
        GLsizei height,
        GLint   border,
        GLsizei imageSize, 
  const GLvoid* data )
{
  uint32_t checksum = crc32 (0, data, imageSize);

  if (config.textures.dump) {
    PPrinny_DumpCompressedTexLevel (checksum, level, internalFormat, width, height, imageSize, data);
  }

#if 0
  dll_log.Log ( L"[GL Texture] Loaded Compressed Texture: Level=%li, (%lix%li) {%5.2f KiB}",
                  level, width, height, (float)imageSize / 1024.0f );
#endif

  // These should be clamped to edge, the game does not repeat these and annoying artifacts are visible
  //   if we don't do this
  if (checksum == 0xD474EABE ||
      checksum == 0xD5B07A99 ||

      checksum == 0xB4F15F92 ||
      checksum == 0x93C585F7 ||

      checksum == 0x6B1AC7B5 ||
      checksum == 0x595868F0 ||

      checksum == 0x79B6E066 ||
      checksum == 0xACE9C61  ||

      (width == 1024 && height == 1024)) {
    GLint texid;
    glGetIntegerv (GL_TEXTURE_BINDING_2D, &texid);
    ui_textures.insert (texid);
  }

  return glCompressedTexImage2D_Original (
    target, level,
      internalFormat,
        width, height,
          border,
            imageSize, data );
}

void
PP_MSAA_ResolveTexture (GLuint tex)
{
  std::unordered_map <GLuint, msaa_tex_ref_s>::iterator it =
    msaa_backing_tex.find (tex);

  if (it != msaa_backing_tex.end ()) {
    if (it->second.dirty) {
      // BLIT from RBO to TEX

      GLuint read_buffer,
             draw_buffer;

      glGetIntegerv (GL_READ_FRAMEBUFFER_BINDING, (GLint *)&read_buffer);
      glGetIntegerv (GL_DRAW_FRAMEBUFFER_BINDING, (GLint *)&draw_buffer);

      // Hard-Coding this makes me nervous, but we will cross that bridge much later
      //   when it is burning.
      glBindFramebuffer_Original (GL_READ_FRAMEBUFFER, 2);

      int buffer = 
        it->second.color_loc >= 0 ?
          it->second.color_loc + GL_COLOR_ATTACHMENT0 :
          GL_NONE;

      int bitmask =
        it->second.color_loc >= 0 ?
          GL_COLOR_BUFFER_BIT :
          GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT;

      glBindFramebuffer_Original (GL_DRAW_FRAMEBUFFER, msaa_resolve_fbo);

      glReadBuffer (buffer);
      glDrawBuffer (buffer);

        glBlitFramebuffer_Original ( 0, 0,
                                       config.render.scene_res_x, config.render.scene_res_y,
                                     0, 0,
                                       config.render.scene_res_x, config.render.scene_res_y,
                                     bitmask,
                                       GL_NEAREST );

      glBindFramebuffer_Original (GL_DRAW_FRAMEBUFFER, draw_buffer);
      glBindFramebuffer_Original (GL_READ_FRAMEBUFFER, read_buffer);

      it->second.dirty = false;
    }
  }
}

__declspec (noinline)
GLvoid
WINAPI
glBindFramebuffer_Detour (GLenum target, GLuint framebuffer)
{
  if (config.render.msaa_samples < 2)
    return glBindFramebuffer_Original (target, framebuffer);


  //dll_log.Log ( L"[GL CallLog] glBindFramebuffer (0x%X, %lu)",
                  //target, framebuffer );

  if (framebuffer == 2) {
    if (! msaa.use) {
      framebuffer = msaa_resolve_fbo;
    } else {
    std::unordered_map <GLuint, msaa_tex_ref_s>::iterator tex_it =
      msaa_backing_tex.begin ();

    while (tex_it != msaa_backing_tex.end ()) {
      tex_it->second.dirty = true;

      ++tex_it;
    }

    std::unordered_map <GLuint, msaa_rbo_ref_s>::iterator rbo_it =
      msaa_backing_rbo.begin ();

    while (rbo_it != msaa_backing_rbo.end ()) {
      rbo_it->second.dirty = true;

      ++rbo_it;
    }

    msaa.dirty = true;
    }
  }


  else {
    GLuint current_fbo;
    glGetIntegerv (GL_FRAMEBUFFER_BINDING, (GLint *)&current_fbo);

    if (current_fbo == 2) {
      std::unordered_map <GLuint, msaa_tex_ref_s>::iterator tex_it =
        msaa_backing_tex.begin ();

      while (tex_it != msaa_backing_tex.end ()) {
        if (! tex_it->second.dirty) {
          ++tex_it;
          continue;
        }

         // Deferring the resolve until first texture use was tried,
         //   however it did not work because of the inefficient way
         //     that the game uses SSAO color buffers.
         PP_MSAA_ResolveTexture (tex_it->first);

        ++tex_it;
      }

      std::unordered_map <GLuint, msaa_rbo_ref_s>::iterator rbo_it =
        msaa_backing_rbo.begin ();

      while (rbo_it != msaa_backing_rbo.end ()) {
        ++rbo_it;
      }
    }
  }

  glBindFramebuffer_Original (target, framebuffer);


  // Simple pass-through to handle scenarios were MSAA rasterization
  //   is skipped.
  if (framebuffer == msaa_resolve_fbo && msaa_resolve_fbo != 0) {
    GLenum buffers [] = { GL_COLOR_ATTACHMENT0,
                          GL_COLOR_ATTACHMENT0 + 1,
                          GL_COLOR_ATTACHMENT0 + 2 };
    glDrawBuffers (3, buffers);
  }
}

std::wstring
PP_GL_InternalFormat_ToStr (GLenum format)
{
  switch (format)
  {
    case GL_DEPTH_COMPONENT:
      return L"Depth     (Unsized)";

    case GL_RGB:
      return L"RGB       (Unsized)";

    case GL_RGBA:
      return L"RGBA      (Unsized)";

    case GL_LUMINANCE8:
      return L"Luminance (8-bit UNORM)       { Attention NIS:  INVALID RENDER FORMAT in GL 3.X !! }";

    case GL_RGBA16F:
      return L"RGBA      (16-bit Half-Float)";
  }

  wchar_t wszUnknown [32];
  wsprintf (wszUnknown, L"Unknown (0x%X)", format);

  return wszUnknown;
}

std::wstring
PP_FBO_AttachPoint_ToStr (GLenum attach)
{
  std::wstring attach_name;

  if (attach >= GL_COLOR_ATTACHMENT0 && attach <= GL_COLOR_ATTACHMENT0 + 15) {
    wchar_t wszColorLoc [16];
    swprintf (wszColorLoc, L"Color%li", attach - GL_COLOR_ATTACHMENT0);
    return wszColorLoc;
  }

  // Generic catch-all, this is probably wrong but I don't care.
  return L"Depth/Stencil";
}

__declspec (noinline)
GLvoid
WINAPI
glFramebufferRenderbuffer_Detour ( GLenum target,
                                   GLenum attachment,
                                   GLenum renderbuffertarget,
                                   GLuint renderbuffer )
{
  if (config.render.msaa_samples < 2)
    return
      glFramebufferRenderbuffer_Original ( target,
                                             attachment,
                                               renderbuffertarget,
                                                 renderbuffer );

  GLuint fbo;
  glGetIntegerv (GL_FRAMEBUFFER_BINDING, (GLint *)&fbo);

  std::unordered_map <GLuint, msaa_rbo_ref_s>::iterator it =
    msaa_backing_rbo.find (renderbuffer);

  if (fbo == 2 && it != msaa_backing_rbo.end ()) {
    if (msaa_resolve_fbo == 0)
      glGenFramebuffers (1, &msaa_resolve_fbo);

    it->second.parent_fbo = fbo;
    it->second.color_loc  = (attachment-GL_COLOR_ATTACHMENT0) < 16 ?
                              (attachment-GL_COLOR_ATTACHMENT0) : -1;
    it->second.dirty      = true;

    dll_log.Log ( L"[   MSAA   ] Attaching RBuffer (id=%5lu) to FBO "
                               L"(id=%lu) via %s",
                    it->second.rbo,
                      fbo,
                        PP_FBO_AttachPoint_ToStr (attachment).c_str () );

    glFramebufferRenderbuffer_Original ( target,
                                           attachment,
                                             GL_RENDERBUFFER,
                                               it->second.rbo );

    glBindFramebuffer_Original (GL_DRAW_FRAMEBUFFER, msaa_resolve_fbo);

    glFramebufferRenderbuffer_Original ( target,
                                           attachment,
                                             renderbuffertarget,
                                               renderbuffer );

    glBindFramebuffer_Original (GL_DRAW_FRAMEBUFFER, fbo);

    return;
  }

  else if (it != msaa_backing_rbo.end ()) {
    // Remove this RBO, we only want to multisample stuff attached to FBO #2
    glDeleteRenderbuffers_Original (1, &it->second.rbo);
    msaa_backing_rbo.erase         (it);
  }

  return
    glFramebufferRenderbuffer_Original ( target,
                                           attachment,
                                             renderbuffertarget,
                                               renderbuffer );
}

__declspec (noinline)
GLvoid
WINAPI
glFramebufferTexture2D_Detour ( GLenum target,
                                GLenum attachment,
                                GLenum textarget,
                                GLuint texture, 
                                GLint  level )
{
  if (config.render.msaa_samples < 2) {
    return
      glFramebufferTexture2D_Original ( target,
                                          attachment,
                                            textarget,
                                              texture,
                                                level );
  }


  GLuint fbo;
  glGetIntegerv (GL_FRAMEBUFFER_BINDING, (GLint *)&fbo);

  std::unordered_map <GLuint, msaa_tex_ref_s>::iterator it =
    msaa_backing_tex.find (texture);

  if (fbo == 2 && it != msaa_backing_tex.end ()) {
    if (msaa_resolve_fbo == 0)
      glGenFramebuffers (1, &msaa_resolve_fbo);

    it->second.parent_fbo = fbo;
    it->second.color_loc  = (attachment-GL_COLOR_ATTACHMENT0) < 16 ?
                            (attachment-GL_COLOR_ATTACHMENT0) : -1;
    it->second.dirty      = true;

    dll_log.Log ( L"[   MSAA   ] Attaching Texture (id=%5lu) to FBO "
                               L"(id=%lu) via %s =>"
                               L" Proxy RBO (id=%lu, %lu-sample)",
                    texture,
                      fbo,
                        PP_FBO_AttachPoint_ToStr (attachment).c_str (),
                          it->second.rbo,
                            config.render.msaa_samples );

    glFramebufferRenderbuffer_Original ( target,
                                           attachment,
                                             GL_RENDERBUFFER,
                                               it->second.rbo );

    glBindFramebuffer_Original (GL_FRAMEBUFFER, msaa_resolve_fbo);

    glFramebufferTexture2D_Original ( target,
                                        attachment,
                                          textarget,
                                            texture,
                                              level );

    glBindFramebuffer_Original (GL_FRAMEBUFFER, fbo);

    return;
  }

  else if (it != msaa_backing_tex.end ()) {
    // Remove this RBO, we only want to multisample stuff attached to FBO #2
    glDeleteRenderbuffers_Original (1, &it->second.rbo);
    msaa_backing_tex.erase         (it);
  }


  return
    glFramebufferTexture2D_Original ( target,
                                        attachment,
                                          textarget,
                                            texture,
                                              level );
}

__declspec (noinline)
GLvoid
WINAPI
glReadPixels_Detour ( GLint   x,
                      GLint   y,
                      GLsizei width,
                      GLsizei height,
                      GLenum  format,
                      GLenum  type,
                      GLvoid* data )
{
  // This is designed to read a 10x10 region at the center
  //   of the screen. That of course only works if 720p is
  //     forced.
  if (x == 635 && y == 355) {
    x = config.render.scene_res_x/2 - 5;
    y = config.render.scene_res_y/2 - 5;
  }


// For Depth of Field  (color buffers should alreaby be resolved)
#if 1
  if (config.render.msaa_samples > 1) {
    GLuint read_fbo, draw_fbo;
    glGetIntegerv (GL_READ_FRAMEBUFFER_BINDING, (GLint *)&read_fbo);
    glGetIntegerv (GL_DRAW_FRAMEBUFFER_BINDING, (GLint *)&draw_fbo);

    if (read_fbo == 2) {
      GLuint read_buffer;
      glGetIntegerv (GL_READ_BUFFER, (GLint *)&read_buffer);

      if (msaa.use && msaa.dirty) {
        glBindFramebuffer_Original (GL_READ_FRAMEBUFFER, 2);
        glBindFramebuffer_Original (GL_DRAW_FRAMEBUFFER, msaa_resolve_fbo);

          glBlitFramebuffer_Original ( x, y,
                                         x+width, y+height,
                                       x, y,
                                         x+width, y+height,
                                       GL_DEPTH_BUFFER_BIT,
                                         GL_NEAREST );

        glBindFramebuffer_Original (GL_DRAW_FRAMEBUFFER, draw_fbo);
      }

      glBindFramebuffer_Original (GL_READ_FRAMEBUFFER, msaa_resolve_fbo);
      glReadBuffer               (read_buffer);
    }
  }
#endif


  return glReadPixels_Original (x, y, width, height, format, type, data);
}

__declspec (noinline)
GLvoid
WINAPI
glDrawArrays_Detour (GLenum mode, GLint first, GLsizei count)
{
  bool fringed = false;

  //
  // The stuff this does, holy crap ... it will torture the GL state machine, so focus heavily on
  //   client-side detection heuristics.
  //
  if (config.render.fringe_removal) {
    bool world  = (mode == GL_TRIANGLES && count < 180);//      && count == 6);
    bool sprite = (mode == GL_TRIANGLE_STRIP && count == 4);

     if (drawing_main_scene && (world || sprite)) {
       GLboolean depth_mask, depth_test;
       glGetBooleanv (GL_DEPTH_WRITEMASK, &depth_mask);
       glGetBooleanv (GL_DEPTH_TEST,      &depth_test);

       if (depth_mask && depth_test && texture_2d) {

       GLint filter;
       glGetTexParameteriv (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, &filter);

       if (filter == GL_LINEAR) {
       glPushAttrib (GL_COLOR_BUFFER_BIT);

       glDepthMask   (GL_FALSE);

       //
       // SSAO Hacks
       //
       glColorMaski (1, GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
       glColorMaski (2, GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

       {
         glEnable_Original (GL_ALPHA_TEST);

         if (sprite)
           glAlphaFunc_Original (GL_LESS, 0.425f);
         else
           glAlphaFunc_Original (GL_LESS, 0.35f);

         glDrawArrays_Original (mode, first, count);
       }

       glColorMaski (2, GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
       glColorMaski (1, GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

       glDepthMask (GL_TRUE);

       if (sprite) {
         glAlphaFunc_Original (GL_GEQUAL, 0.425f);
       }

       else {
         glAlphaFunc_Original (GL_GEQUAL, 0.35f);
       }

       fringed = true;

       if (pp::RenderFix::tracer.log)
         dll_log.Log (L"[ GL Trace ] Fringed draw: %X, %d, %d -- GLSL program: %lu", mode, first, count, active_program);
      }
    }
    }
  }

  glDrawArrays_Original (mode, first, count);

  // Technically we might have issued an extra draw above, but this stat is
  //   intended to reflect the APPLICATION's drawcall count, not the mod's.
  pp::RenderFix::draw_state.draws++;

  if (fringed) {
    glPopAttrib ();
  }
}

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


__declspec (noinline)
GLvoid
WINAPI
glDeleteTextures_Detour (GLsizei n, GLuint* textures)
{
  for (int i = 0; i < n; i++) {
    if (ui_textures.find (textures [i]) != ui_textures.end ()) {
      //dll_log.Log (L"Deleted UI texture");
      ui_textures.erase (textures [i]);
    }

    if (msaa_backing_tex.find (textures [i]) != msaa_backing_tex.end ()) {
      dll_log.Log ( L"[   MSAA   ] Game deleted an MSAA backed Texture, "
                                 L"cleaning up... (TexID=%lu, RBO=%lu)",
                      textures [i], msaa_backing_tex [textures [i]].rbo );

      glDeleteRenderbuffers_Original (1, &msaa_backing_tex [textures [i]].rbo);

      msaa_backing_tex.erase (textures [i]);
    }

    if (textures [i] == pp::RenderFix::draw_state.depth_tex) {
      GLuint fbo;
      glGetIntegerv                   (GL_FRAMEBUFFER_BINDING, (GLint *)&fbo);
      glBindFramebuffer_Original      (GL_FRAMEBUFFER, msaa_resolve_fbo);
      glFramebufferTexture2D_Original (GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, 0, 0);
      glBindFramebuffer_Original      (GL_FRAMEBUFFER, fbo);
      pp::RenderFix::draw_state.depth_tex = 0;
    }
  }

  glDeleteTextures_Original (n, textures);
}

__declspec (noinline)
GLvoid
WINAPI
glTexSubImage2D_Detour ( GLenum  target,
                         GLint   level,
                         GLint   xoffset,
                         GLint   yoffset,
                         GLsizei width,
                         GLsizei height,
                         GLenum  format,
                         GLenum  type,
                   const GLvoid* pixels )
{
  dll_log.Log (L"[ Tex Dump ] SubImage2D - Level: %li, <%li,%li>", level, xoffset, yoffset);

  glTexSubImage2D_Original (
    target, level,
      xoffset, yoffset,
        width, height,
          format, type,
            pixels );
}

__declspec (noinline)
GLvoid
WINAPI
glBindTexture_Detour ( GLenum target,
                       GLuint texture )
{
  if (config.render.msaa_samples < 2)
    return glBindTexture_Original (target, texture);

  return glBindTexture_Original (target, texture);
}

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
  if (data != nullptr) {
  if (internalformat == GL_RGBA) {
    const int imageSize = width * height * 4;
    uint32_t  checksum  = crc32 (0, data, imageSize);

    if (config.textures.dump) {
      //dll_log.Log (L"[ Tex Dump ] Format: 0x%X, Internal: 0x%X", format, internalformat);

      PPrinny_DumpUncompressedTexLevel (
        checksum,
          level,
            format,
              width, height,
                imageSize,
                  data );
    }

    //
    // Gamepad Button Texture
    // 
    if (checksum == 0x7BE1DE43) {
      int      size, width, height;
      int      bpp;
      uint8_t  type_ [4];
      uint8_t  info_ [7];
      uint8_t* tex;

      FILE* fTGA;

      if ((fTGA = fopen ("pad.tga", "rb")) != nullptr) {
        fread (&type_, 1, 3, fTGA);
        fseek (fTGA,   12,   SEEK_SET);
        fread (&info_, 1, 6, fTGA);

        if(! (type_[1] != 0 || (type_[2] != 2 && type_[2] != 3))) {
          width  = info_ [0] + info_ [1] * 256;
          height = info_ [2] + info_ [3] * 256;
          bpp    = info_ [4];

          size = width * height * (bpp >> 3);

          tex =
            (uint8_t*) malloc (size);

          fread  (tex, size, 1, fTGA);
          fclose (fTGA);

          GLint texId;
          glGetIntegerv (GL_TEXTURE_BINDING_2D, &texId);
          ui_textures.insert (texId);

          glTexImage2D_Original ( target,
                                    level,
                                      internalformat,
                                        width, height,
                                          border,
                                            format,
                                              type,
                                                tex );

          free (tex);
          return;
        }
      }
    }
  }

  if (format == GL_COLOR_INDEX) {
    GLint texId;
    glGetIntegerv (GL_TEXTURE_BINDING_2D, &texId);
    ui_textures.insert (texId);
  }

  if (config.textures.dump && format == GL_COLOR_INDEX) {
    // This sort of palette is easy, anything else will require a lot of work.
    if (type == GL_UNSIGNED_BYTE) {
      if (config.textures.dump) {
        const int imageSize = width * height;
        uint32_t  checksum  = crc32 (0, data, imageSize);

       //dll_log.Log (L"[ Tex Dump ] Format: 0x%X, Internal: 0x%X", format, internalformat);

        PPrinny_DumpUncompressedTexLevel (
          checksum,
            level,
              format,
                width, height,
                  imageSize,
                    data );
      }
    } else {
      dll_log.Log ( L"[ Tex Dump ] Color Index Format: (%lux%lu), Data Type: 0x%X",
                      width, height, type );
    }
  }

  }

  GLint texId;
  glGetIntegerv (GL_TEXTURE_BINDING_2D, &texId);

  if (data == nullptr) {
    //ui_textures.insert (texId);

    dll_log.Log ( L"[GL Texture] Id=%i, LOD: %i, (%ix%i)",
                    texId, level, width, height );

    dll_log.Log ( L"[GL Texture]  >> Render Target - "
                  L"Internal Format: %s",
                    PP_GL_InternalFormat_ToStr (internalformat).c_str () );

#if 0
    // Avoid driver weirdness from using a non-color renderable format
    if (internalformat == GL_LUMINANCE_ALPHA) {
      internalformat = GL_RG8;
      format         = GL_RG;
    }
#endif

#if 0
    if (internalformat == GL_DEPTH_COMPONENT)
      internalformat = GL_DEPTH_COMPONENT24;
#endif

#if 0
    // For the sake of memory alignment (the driver is going to do this anyway),
    //   just pad all RGB stuff out to RGBA (where A is implicitly ignored)
    if (internalformat == GL_RGB) {
      internalformat = GL_RGBA8;
      format         = GL_RGBA;
    }
#endif

    if (width == 1280 && height == 720) {
      width  = config.render.scene_res_x;
      height = config.render.scene_res_y;

      if (config.render.msaa_samples > 1) {
        GLint original_rbo;
        glGetIntegerv (GL_RENDERBUFFER_BINDING, &original_rbo);

        std::unordered_map <GLuint, msaa_tex_ref_s>::iterator it =
          msaa_backing_tex.find (texId);

        if (it != msaa_backing_tex.end ()) {
          // Renderbuffer "Orphaning" (reconfigure existing datastore)
          glBindRenderbuffer               (GL_RENDERBUFFER, it->second.rbo);
          glRenderbufferStorageMultisample (GL_RENDERBUFFER, config.render.msaa_samples,
                                                             internalformat, width, height);
          glBindRenderbuffer               (GL_RENDERBUFFER, original_rbo);
        } else {
          GLuint rbo;
          glGenRenderbuffers               (1, &rbo);
          glBindRenderbuffer               (GL_RENDERBUFFER, rbo);
          glRenderbufferStorageMultisample (GL_RENDERBUFFER, config.render.msaa_samples, 
                                                             internalformat, width, height);

          msaa_tex_ref_s ref;
          ref.rbo        = rbo;
          ref.parent_fbo = -1; // Not attached to anything yet

          msaa_backing_tex.insert (std::pair <GLuint, msaa_tex_ref_s> (texId, ref));

          glBindRenderbuffer (GL_RENDERBUFFER, original_rbo);
        }
      }
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
  } else {
    // Font Textures
    //if (width == 1024 && height == 2048)
    ui_textures.insert (texId);
  }

  glTexImage2D_Original ( target, level,
                            internalformat,
                              width, height,
                                border,
                                  format, type,
                                    data );
}

__declspec (noinline)
GLvoid
WINAPI
glDeleteRenderbuffers_Detour (       GLsizei n,
                               const GLuint* renderbuffers )
{
  // Run through the list and look for any MSAA-backed RBOs so we do not
  //   leak VRAM.
  for (int i = 0; i < n; i++) {
    if (msaa_backing_rbo.find (renderbuffers [i]) != msaa_backing_rbo.end ()) {
      dll_log.Log ( L"[   MSAA   ] Game deleted an MSAA backed RBuffer, "
                                 L"cleaning up... (RBO_ss=%lu, RBO_ms=%lu)",
                      renderbuffers [i],
                        msaa_backing_rbo [renderbuffers [i]].rbo );

      glDeleteRenderbuffers_Original (
                  1,
                    &msaa_backing_rbo [renderbuffers [i]].rbo
      );

      msaa_backing_rbo.erase (renderbuffers [i]);
    }
  }

  return glDeleteRenderbuffers_Original (n, renderbuffers);
}

__declspec (noinline)
GLvoid
WINAPI
glRenderbufferStorage_Detour (
  GLenum  target,
  GLenum  internalformat,
  GLsizei width,
  GLsizei height )
{
  dll_log.Log ( L"[GL Surface]  >> Render Buffer - Internal Format: %s",
                  PP_GL_InternalFormat_ToStr (internalformat).c_str () );

#if 0
  if (internalformat == GL_DEPTH_COMPONENT)
    internalformat = GL_DEPTH_COMPONENT24;
#endif

#if 0
  if (internalformat == GL_RGB)
    internalformat = GL_RGBA8;
#endif


  if (width == 1280 && height == 720) {
    width = config.render.scene_res_x; height = config.render.scene_res_y;

    if (config.render.msaa_samples > 1) {
      GLuint rboId;
      glGetIntegerv (GL_RENDERBUFFER_BINDING, (GLint *)&rboId);

      std::unordered_map <GLuint, msaa_rbo_ref_s>::iterator it =
        msaa_backing_rbo.find (rboId);

      if (it != msaa_backing_rbo.end ()) {
        // Renderbuffer "Orphaning" (reconfigure existing datastore)
        glBindRenderbuffer               (GL_RENDERBUFFER, it->second.rbo);
        glRenderbufferStorageMultisample (GL_RENDERBUFFER, config.render.msaa_samples,
                                                           internalformat, width, height);
        glBindRenderbuffer               (GL_RENDERBUFFER, rboId);
      } else {
        GLuint rbo;
        glGenRenderbuffers               (1, &rbo);
        glBindRenderbuffer               (GL_RENDERBUFFER, rbo);
        glRenderbufferStorageMultisample (GL_RENDERBUFFER, config.render.msaa_samples,
                                                           internalformat, width, height);

        msaa_rbo_ref_s ref;
        ref.rbo        = rbo;
        ref.parent_fbo = -1;  // Not attached to anything yet

        msaa_backing_rbo.insert (std::pair <GLuint, msaa_rbo_ref_s> (rboId, ref));

        glBindRenderbuffer (GL_RENDERBUFFER, rboId);
      }
    }
  }

#if 0
  if (width == 960 && height == 544) {
    width = config.render.scene_res_x; height = config.render.scene_res_y;
  }
#endif

  glRenderbufferStorage_Original   (target, internalformat, width, height);
}

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

int HAS_ARB_COPY_IMAGE = -1;
glCopyImageSubData_pfn  glCopyImageSubData           = nullptr;

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
  if (pp::RenderFix::tracer.log) {
    dll_log.Log ( L"[ GL Trace ] -- glCopyTexSubImage2D (%x, %li, %li, %li, %li, %li, %li, %li)",
                    target, level,
                      xoffset, yoffset,
                        x, y,
                          width, height );
   }

  if (width == 1280 && height == 720) {
    width  = config.render.scene_res_x;
    height = config.render.scene_res_y;

    // This has to be done every time the window changes resolution
    if (pp::RenderFix::draw_state.depth_tex == 0) {
      GLuint texId;
      glGetIntegerv (GL_TEXTURE_BINDING_2D, (GLint *)&texId);

      pp::RenderFix::draw_state.depth_tex = texId;

      GLuint draw_buffer;

      glGetIntegerv (GL_FRAMEBUFFER_BINDING, (GLint *)&draw_buffer);

      if (config.render.msaa_samples > 1)
        glBindFramebuffer_Original    (GL_FRAMEBUFFER, msaa_resolve_fbo);
      else
        glBindFramebuffer_Original    (GL_FRAMEBUFFER, 2);

      glFramebufferTexture2D_Original (GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, texId, 0);

      pp::RenderFix::draw_state.window_changed = false;

      glBindFramebuffer_Original (GL_FRAMEBUFFER, draw_buffer);
    }

// For Depth of Field to Work
#if 1
    if (config.render.msaa_samples > 1 && msaa.use && msaa.dirty) {
      GLuint read_buffer,
             draw_buffer;

      glGetIntegerv (GL_READ_FRAMEBUFFER_BINDING, (GLint *)&read_buffer);
      glGetIntegerv (GL_DRAW_FRAMEBUFFER_BINDING, (GLint *)&draw_buffer);

      // Hard-Coding this makes me nervous, but we will cross that bridge much later
      //   when it is burning.
      glBindFramebuffer_Original (GL_READ_FRAMEBUFFER, 2);
      glBindFramebuffer_Original (GL_DRAW_FRAMEBUFFER, msaa_resolve_fbo);

        glBlitFramebuffer_Original ( 0, 0,
                                       config.render.scene_res_x, config.render.scene_res_y,
                                     0, 0,
                                       config.render.scene_res_x, config.render.scene_res_y,
                                     GL_DEPTH_BUFFER_BIT,
                                       GL_NEAREST );

      glBindFramebuffer_Original (GL_DRAW_FRAMEBUFFER, draw_buffer);
      glBindFramebuffer_Original (GL_READ_FRAMEBUFFER, read_buffer);

      msaa.dirty = false;
    }
#endif
  }

  if (pp::RenderFix::draw_state.depth_tex != 0 && width != 32 && height != 2048)
    return;

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
          wglGetProcAddress ("glCopyImageSubData");
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
*  }
#endif

  glViewport_Original (x, y, width, height);
}

__declspec (noinline)
GLvoid
WINAPI
glUniformMatrix4fv_Detour (
        GLint      location,
        GLsizei    count,
        GLboolean  transpose,
  const GLfloat   *value )
{
  if (pp::RenderFix::tracer.log) {
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
  }

  glUniformMatrix4fv_Original (location, count, transpose, value);
}

__declspec (noinline)
GLvoid
WINAPI
glUniformMatrix3fv_Detour (
        GLint      location,
        GLsizei    count,
        GLboolean  transpose,
  const GLfloat   *value )
{
  if (pp::RenderFix::tracer.log) {
    dll_log.Log ( L"[ GL Trace ] -- glUniformMatrix3fv (Uniform Loc=%li, count=%li, transpose=%s)",
                      location, count, transpose ? L"yes" : L"no" );
    dll_log.Log ( L"[ GL Trace ]        %11.6f, %11.6f, %11.6f",
                      value [ 0], value [ 1], value [ 2] );
    dll_log.Log ( L"[ GL Trace ]        %11.6f, %11.6f, %11.6f",
                      value [ 3], value [ 4], value [ 5] );
    dll_log.Log ( L"[ GL Trace ]        %11.6f, %11.6f, %11.6f",
                      value [ 6], value [ 7], value [ 8] );
  }

  glUniformMatrix3fv_Original (location, count, transpose, value);
}

__declspec (noinline)
GLvoid
WINAPI
glVertexAttrib4f_Detour ( GLuint  index,
                          GLfloat v0,
                          GLfloat v1,
                          GLfloat v2,
                          GLfloat v3 )
{
  if (pp::RenderFix::tracer.log) {
    dll_log.Log ( L"VertexAttrib4f: idx=%lu, %f, %f, %f, %f",
                  index, v0, v1, v2, v3 );
  }

  return glVertexAttrib4f_Original (index, v0, v1, v2, v3);
}

__declspec (noinline)
GLvoid
WINAPI
glVertexAttrib4fv_Detour (       GLuint   index,
                           const GLfloat* v )
{
  if (pp::RenderFix::tracer.log) {
    dll_log.Log ( L"VertexAttrib4fv: idx=%lu, %f, %f, %f, %f",
                    index, v [0], v [1], v [2], v [3] );
  }

  return glVertexAttrib4fv_Original (index, v);
}

__declspec (noinline)
GLvoid
WINAPI
glGetShaderiv_Detour ( GLuint shader,
                       GLenum pname,
                       GLint* params )
{
  return glGetShaderiv_Original (shader, pname, params);
}

#define GL_SHADER_TYPE                      0x8B4F
#define GL_FRAGMENT_SHADER                  0x8B30
#define GL_VERTEX_SHADER                    0x8B31


void
PPrinny_DumpShader ( uint32_t    checksum,
                     GLuint      shader,
                     const char* string )
{
  if (GetFileAttributes (L"dump") == INVALID_FILE_ATTRIBUTES)
    CreateDirectoryW (L"dump", nullptr);

  if (GetFileAttributes (L"dump/shaders") == INVALID_FILE_ATTRIBUTES)
    CreateDirectoryW (L"dump/shaders", nullptr);


  GLint type;
  glGetShaderiv_Original (shader, GL_SHADER_TYPE, &type);

  char szShaderDump [MAX_PATH];
  sprintf ( szShaderDump, "dump/shaders/%s_%08X.glsl",
              type == GL_VERTEX_SHADER ? "Vertex" : "Fragment",
                checksum );


  if (GetFileAttributesA (szShaderDump) == INVALID_FILE_ATTRIBUTES) {
    FILE* fShader = fopen (szShaderDump, "wb");
    fwrite ((const void *)string, strlen (string + 1), 1, fShader);
    fclose (fShader);
  }
}

char*
PPrinny_EvaluateShaderVariables (char* szInput)
{
  char* szRedText  = strstr (szInput, "%PrettyPrinny.Colors.RedText%");
  char* szBlueText = strstr (szInput, "%PrettyPrinny.Colors.BlueText%");

  if (szRedText != nullptr || szBlueText != nullptr) {
    if (szRedText) {
      int len = snprintf ( szRedText, 29,
                             "vec3 (%3.2f,%3.2f,%3.2f)",
                               config.colors.red_text.r,
                                 config.colors.red_text.g,
                                   config.colors.red_text.b );
      *(szRedText + len) = *(szRedText + 29);

      // Add spaces for all missing characters
      for (int i = len+1; i < 30; i++) {
        *(szRedText + i) = ' ';
      }
    }

    if (szBlueText) {
      int len = snprintf ( szBlueText, 30,
                             "vec3 (%3.2f,%3.2f,%3.2f)",
                               config.colors.blue_text.r,
                                 config.colors.blue_text.g,
                                   config.colors.blue_text.b );
      *(szBlueText + len) = *(szBlueText + 30);

      // Add spaces for all missing characters
      for (int i = len+1; i < 31; i++) {
        *(szBlueText + i) = ' ';
      }
    }

    //dll_log.Log (L"%hs", szInput);

    // TODO: Allocate a new buffer to fit the replaced variables
    return szInput;
  }

  return szInput;
}

__declspec (noinline)
GLvoid
WINAPI
glShaderSource_Detour (        GLuint   shader,
                               GLsizei  count,
                        const GLubyte **string,
                        const  GLint   *length )
{
  uint32_t checksum = crc32 (0x00, (const void *)*string, strlen ((const char *)*string));

  PPrinny_DumpShader (checksum, shader, (const char *)*string);

  char szShaderName [MAX_PATH];
  sprintf (szShaderName, "shaders/%X.glsl", checksum);

  char* szShaderSrc = _strdup ((const char *)*string);
  char* szTexWidth  =  strstr (szShaderSrc, "const float texWidth = 720.0;  ");

  if (szTexWidth != nullptr) {
    char szNewWidth [33];
    sprintf (szNewWidth, "const float texWidth = %7.1f;", (float)config.render.scene_res_y);
    memcpy (szTexWidth, szNewWidth, 31);
  }


  if (GetFileAttributesA (szShaderName) != INVALID_FILE_ATTRIBUTES) {
    FILE* fShader = fopen (szShaderName, "rb");

    if (fShader != nullptr) {
                    fseek  (fShader, 0, SEEK_END);
      size_t size = ftell  (fShader);
                    rewind (fShader);

      char* szSrc = (char *)malloc (size + 1);
      szSrc [size] = '\0';
      fread (szSrc, size, 1, fShader);
      fclose (fShader);

      szSrc = PPrinny_EvaluateShaderVariables (szSrc);

      glShaderSource_Original (shader, count, (const GLubyte **)&szSrc, length);

      free (szShaderSrc);
      free (szSrc);

      return;
    }
  }

  glShaderSource_Original (shader, count, (const GLubyte **)&szShaderSrc, length);

  free (szShaderSrc);
}

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

  static bool init_self = false;

  if (! init_self) {
    glBindRenderbuffer =
      (glBindRenderbuffer_pfn)
        wglGetProcAddress_Original ("glBindRenderbuffer");

    glRenderbufferStorageMultisample =
      (glRenderbufferStorageMultisample_pfn)
        wglGetProcAddress_Original ("glRenderbufferStorageMultisample");

    glGenRenderbuffers =
      (glGenRenderbuffers_pfn)
        wglGetProcAddress_Original ("glGenRenderbuffers");

    glDeleteRenderbuffers_Original =
      (glDeleteRenderbuffers_pfn)
        wglGetProcAddress_Original ("glDeleteRenderbuffers");

    glGenFramebuffers =
      (glGenFramebuffers_pfn)
        wglGetProcAddress_Original ("glGenFramebuffers");

    glBlitFramebuffer_Original =
      (glBlitFramebuffer_pfn)
        wglGetProcAddress_Original ("glBlitFramebuffer");

    glDrawBuffers =
      (glDrawBuffers_pfn)
        wglGetProcAddress_Original ("glDrawBuffers");

    glColorMaski =
      (glColorMaski_pfn)
        wglGetProcAddress_Original ("glColorMaski");

    init_self = true;
  }

  PROC ret = wglGetProcAddress_Original (szFuncName);

  bool detoured = false;

  if (! strcmp (szFuncName, "glUseProgram") && (ret != nullptr)) {
    glUseProgram_Original = (glUseProgram_pfn)ret;
    ret                   = (PROC)glUseProgram_Detour;
    detoured              = true;
  }

  if (! strcmp (szFuncName, "glShaderSource") && (ret != nullptr)) {
    glShaderSource_Original = (glShaderSource_pfn)ret;
    ret                     = (PROC)glShaderSource_Detour;
    detoured                = true;
  }

  if (! strcmp (szFuncName, "glGetShaderiv") && (ret != nullptr)) {
    glGetShaderiv_Original = (glGetShaderiv_pfn)ret;
    ret                    = (PROC)glGetShaderiv_Detour;
    detoured               = true;
  }

  if (! strcmp (szFuncName, "glVertexAttrib4f") && (ret != nullptr)) {
    glVertexAttrib4f_Original = (glVertexAttrib4f_pfn)ret;
    ret                       = (PROC)glVertexAttrib4f_Detour;
    detoured                  = true;
  }

  if (! strcmp (szFuncName, "glVertexAttrib4fv") && (ret != nullptr)) {
    glVertexAttrib4fv_Original = (glVertexAttrib4fv_pfn)ret;
    ret                        = (PROC)glVertexAttrib4fv_Detour;
    detoured                   = true;
  }

  if (! strcmp (szFuncName, "glCompressedTexImage2D") && (ret != nullptr)) {
    glCompressedTexImage2D_Original = (glCompressedTexImage2D_pfn)ret;
    ret                             = (PROC)glCompressedTexImage2D_Detour;
    detoured                        = true;
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

  if (! strcmp (szFuncName, "glBindFramebuffer") && (ret != nullptr)) {
    glBindFramebuffer_Original = (glBindFramebuffer_pfn)ret;
    ret                        = (PROC)glBindFramebuffer_Detour;
    detoured                   = true;
  }

  if (! strcmp (szFuncName, "glFramebufferTexture2D") && (ret != nullptr)) {
    glFramebufferTexture2D_Original = (glFramebufferTexture2D_pfn)ret;
    ret                             = (PROC)glFramebufferTexture2D_Detour;
    detoured                        = true;
  }

  if (! strcmp (szFuncName, "glFramebufferRenderbuffer") && (ret != nullptr)) {
    glFramebufferRenderbuffer_Original = (glFramebufferRenderbuffer_pfn)ret;
    ret                                = (PROC)glFramebufferRenderbuffer_Detour;
    detoured                           = true;
  }

  if (! strcmp (szFuncName, "glBlitFramebuffer") && (ret != nullptr)) {
    glBlitFramebuffer_Original = (glBlitFramebuffer_pfn)ret;
    ret                        = (PROC)glBlitFramebuffer_Detour;
    detoured                   = true;
  }

  if (! strcmp (szFuncName, "glDeleteRenderbuffers") && (ret != nullptr)) {
    glDeleteRenderbuffers_Original = (glDeleteRenderbuffers_pfn)ret;
    ret                            = (PROC)glDeleteRenderbuffers_Detour;
    detoured                       = true;
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

__declspec (noinline)
GLvoid
WINAPI
glFinish_Detour (void)
{
  if (config.compatibility.allow_gl_cpu_sync)
    glFinish_Original ();
}

__declspec (noinline)
GLvoid
WINAPI
glFlush_Detour (void)
{
  if (config.compatibility.allow_gl_cpu_sync)
    glFlush_Original ();
}

__declspec (noinline)
GLvoid
WINAPI
glLoadMatrixf_Detour (const GLfloat* m)
{
  if (pp::RenderFix::tracer.log) {
    dll_log.Log ( L"[ GL Trace ] glLoadMatrixf -- %11.6f %11.6f %11.6f %11.6f",
                    m [ 0], m [ 1], m [ 2], m [ 3] );
     dll_log.Log ( L"                           -- %11.6f %11.6f %11.6f %11.6f",
                    m [ 4], m [ 5], m [ 6], m [ 7] );
    dll_log.Log ( L"                           -- %11.6f %11.6f %11.6f %11.6f",
                    m [ 8], m [ 9], m [10], m [11] );
    dll_log.Log ( L"                           -- %11.6f %11.6f %11.6f %11.6f",
                    m [12], m [13], m [14], m [15] );
  }

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

__declspec (noinline)
GLvoid
WINAPI
glScalef_Detour ( GLfloat x,
                  GLfloat y,
                  GLfloat z )
{
  if (pp::RenderFix::tracer.log) {
    dll_log.Log ( L"[ GL Trace ] glScalef -- %11.6f %11.6f %11.6f",
                    x, y, z );
  }

  glScalef_Original (x, y, z);
}

LPVOID lpvDontCare = nullptr;

BOOL
__stdcall
SwapBuffers_Detour (HDC hdc)
{
  if (pp::RenderFix::tracer.log) {
    dll_log.Log ( L"[ GL Trace] SwapBuffers (0x%X)",
                    hdc );
  }

  return wglSwapBuffers (hdc);
}

LPVOID ChoosePixelFormat_Hook = nullptr;

int
WINAPI
ChoosePixelFormat_Detour
(
         HDC                   hdc,
   const PIXELFORMATDESCRIPTOR *ppfd
)
{
  dll_log.Log (L"[  WinGDI  ] ChoosePixelFormat - Flags: 0x%X", ppfd->dwFlags);

  PIXELFORMATDESCRIPTOR pfd =
  {
    sizeof (PIXELFORMATDESCRIPTOR),
    1,
    PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER | PFD_SWAP_EXCHANGE,
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

  PPrinny_DisableHook (ChoosePixelFormat_Hook);

  int pf = ChoosePixelFormat_Original (hdc, &pfd);

  PPrinny_EnableHook (ChoosePixelFormat_Hook);

  return pf;
}


LPVOID wglCreateContext_Hook = nullptr;

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


    if (ret != 0) {
      wglMakeCurrent           (hDC, ret);
      wglGetProcAddress_Detour ("glDebugMessageCallbackARB");
      wglMakeCurrent           (NULL, NULL);

      return ret;
    }

    dll_log.Log (L"[   XXX   ] wglCreateContextAttribsARB (...) failed?!\n");
  }

  HGLRC ret = wglCreateContext_Original (hDC);

  if (ret != 0) {
    wglMakeCurrent           (hDC, ret);
    wglGetProcAddress_Detour ("glDebugMessageCallbackARB");
    wglMakeCurrent           (NULL, NULL);
  }

  return ret;
}

__declspec (noinline)
BOOL
WINAPI
wglCopyContext_Detour ( HGLRC hglrcSrc,
                        HGLRC hglrcDst,
                        UINT  mask )
{
  if (config.compatibility.optimize_ctx_mgmt)
    return TRUE;

  return wglCopyContext_Original (hglrcSrc, hglrcDst, mask);
}

__declspec (thread) HDC   current_dc;
__declspec (thread) HGLRC current_glrc;

__declspec (noinline)
BOOL
WINAPI
wglMakeCurrent_Detour ( HDC   hDC,
                        HGLRC hGLRC )
{
  if (! config.compatibility.optimize_ctx_mgmt) {
    return wglMakeCurrent_Original (hDC, hGLRC);
  }

  //dll_log.Log ( L"[ GL Trace ] wglMakeCurrent (%Xh, %Xh) <tid=%X>",
                  //hDC,
                    //hGLRC,
                      //GetCurrentThreadId () );

  if (current_dc != hDC/* || current_glrc != hGLRC*/) {
    if (wglMakeCurrent_Original (hDC, hGLRC)) {
      current_dc   = hDC;
      current_glrc = hGLRC;

      return TRUE;
    }

    return FALSE;
  }

  return TRUE;
}


LRESULT CALLBACK Dummy_WndProc (HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

void
PP_InitGLHooks (void)
{
      PPrinny_CreateDLLHook ( config.system.injector.c_str (),
                              "BMF_BeginBufferSwap",
                               OGLEndFrame_Pre,
                     (LPVOID*)&BMF_BeginBufferSwap );

      PPrinny_CreateDLLHook ( config.system.injector.c_str (),
                              "BMF_EndBufferSwap",
                               OGLEndFrame_Post,
                     (LPVOID*)&BMF_EndBufferSwap );

      PPrinny_CreateDLLHook ( config.system.injector.c_str (),
                              "glDeleteTextures",
                              glDeleteTextures_Detour,
                   (LPVOID *)&glDeleteTextures_Original );

      PPrinny_CreateDLLHook ( config.system.injector.c_str (),
                              "glTexSubImage2D",
                              glTexSubImage2D_Detour,
                   (LPVOID *)&glTexSubImage2D_Original );

      PPrinny_CreateDLLHook ( config.system.injector.c_str (),
                              "glTexImage2D",
                              glTexImage2D_Detour,
                   (LPVOID *)&glTexImage2D_Original );

      PPrinny_CreateDLLHook ( config.system.injector.c_str (),
                              "glCopyTexSubImage2D",
                              glCopyTexSubImage2D_Detour,
                   (LPVOID *)&glCopyTexSubImage2D_Original );

      PPrinny_CreateDLLHook ( config.system.injector.c_str (),
                              "glBindTexture",
                              glBindTexture_Detour,
                   (LPVOID *)&glBindTexture_Original );

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


      PPrinny_CreateDLLHook ( config.system.injector.c_str (),
                              "glTexParameteri",
                               glTexParameteri_Detour,
                    (LPVOID *)&glTexParameteri_Original );

      PPrinny_CreateDLLHook ( config.system.injector.c_str (),
                              "glBlendFunc",
                               glBlendFunc_Detour,
                    (LPVOID *)&glBlendFunc_Original );

      PPrinny_CreateDLLHook ( config.system.injector.c_str (),
                              "glReadPixels",
                               glReadPixels_Detour,
                    (LPVOID *)&glReadPixels_Original );

      PPrinny_CreateDLLHook ( config.system.injector.c_str (),
                              "glDrawArrays",
                               glDrawArrays_Detour,
                    (LPVOID *)&glDrawArrays_Original );

      PPrinny_CreateDLLHook ( config.system.injector.c_str (),
                              "glAlphaFunc",
                               glAlphaFunc_Detour,
                    (LPVOID *)&glAlphaFunc_Original );

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
                   (LPVOID *)&ChoosePixelFormat_Original,
                   (LPVOID *)&ChoosePixelFormat_Hook );
      PPrinny_EnableHook    ( ChoosePixelFormat_Hook );

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
                              "wglMakeCurrent",
                              wglMakeCurrent_Detour,
                   (LPVOID *)&wglMakeCurrent_Original );

      PPrinny_CreateDLLHook ( config.system.injector.c_str (),
                              "wglCopyContext",
                              wglCopyContext_Detour,
                   (LPVOID *)&wglCopyContext_Original );

      PPrinny_CreateDLLHook ( config.system.injector.c_str (),
                              "wglGetProcAddress",
                              wglGetProcAddress_Detour,
                   (LPVOID *)&wglGetProcAddress_Original );
}

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

  if (config.compatibility.support_old_drivers || (! RegisterClass (&wc))) {
    dll_log.Log (L"[OGL Driver] Opting out of OpenGL 3.2+ Profile");
    PP_InitGLHooks ();
    return;
  }

  CreateWindowW ( wc.lpszClassName,
                    L"Dummy_Pretty_Prinny",
                      WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                        0,0,
                          0,0,
                            0,0,
                              hInstance,0);
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

      PP_InitGLHooks ();

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
  pCommandProc->AddVariable ("Render.MSAA",             new eTB_VarStub <bool> (&msaa.use));

  pCommandProc->AddVariable ("Render.FringeRemoval",    new eTB_VarStub <bool> (&config.render.fringe_removal));

  pCommandProc->AddVariable ("Trace.NumFrames",         new eTB_VarStub <int>  (&tracer.count));
  pCommandProc->AddVariable ("Trace.Enable",            new eTB_VarStub <bool> (&tracer.log));

  pCommandProc->AddVariable ("Render.ConservativeMSAA", new eTB_VarStub <bool> (&config.render.conservative_msaa));
  pCommandProc->AddVariable ("Render.EarlyResolve",     new eTB_VarStub <bool> (&msaa.early_resolve));
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