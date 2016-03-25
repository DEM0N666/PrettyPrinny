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

#include "render/gl_perf.h"

#include <cstdint>

//
// Optimizations that may turn out to break stuff
//
bool aggressive_optimization = true;
int  program33_draws         = 0;

pp::RenderFix::tracer_s
  pp::RenderFix::tracer;

pp::RenderFix::pp_draw_states_s
  pp::RenderFix::draw_state;

extern uint32_t
crc32 (uint32_t crc, const void *buf, size_t size);

bool drawing_main_scene = false;

GLsizei game_width  = 1280;
GLsizei game_height = 720;

GLsizei fb_width    = config.render.scene_res_x;
GLsizei fb_height   = config.render.scene_res_y;

typedef GLvoid (WINAPI *glCompressedTexSubImage2D_pfn)(
        GLenum  target,
        GLint   level,
        GLint   xoffset,
        GLint   yoffset,
        GLsizei width,
        GLsizei height,
        GLenum  format,
        GLsizei imageSize,
  const GLvoid* data
);

#define GL_TEXTURE_BASE_LEVEL           0x813C
#define GL_TEXTURE_MAX_LEVEL            0x813D

#define GL_TEXTURE0                     0x84C0
#define GL_ACTIVE_TEXTURE               0x84E0

#define GL_TEXTURE_RECTANGLE            0x84F5

#define GL_UNSIGNED_INT_8_8_8_8_REV     0x8367
#define GL_BGRA                         0x80E1


static GLfloat pixel_maps [10][256];


glGenRenderbuffers_pfn               glGenRenderbuffers                 = nullptr;
glBindRenderbuffer_pfn               glBindRenderbuffer                 = nullptr;
glRenderbufferStorageMultisample_pfn glRenderbufferStorageMultisample   = nullptr;
glDeleteRenderbuffers_pfn            glDeleteRenderbuffers_Original     = nullptr;
glRenderbufferStorage_pfn            glRenderbufferStorage_Original     = nullptr;

glDrawBuffers_pfn                    glDrawBuffers_Original             = nullptr;

glGenFramebuffers_pfn                glGenFramebuffers                  = nullptr;
glBindFramebuffer_pfn                glBindFramebuffer_Original         = nullptr;
glBlitFramebuffer_pfn                glBlitFramebuffer_Original         = nullptr;
glFramebufferTexture2D_pfn           glFramebufferTexture2D_Original    = nullptr;
glFramebufferRenderbuffer_pfn        glFramebufferRenderbuffer_Original = nullptr;
glColorMaski_pfn                     glColorMaski                       = nullptr;
glCheckFramebufferStatus_pfn         glCheckFramebufferStatus           = nullptr;

glBindTexture_pfn                    glBindTexture_Original             = nullptr;
glTexImage2D_pfn                     glTexImage2D_Original              = nullptr;
glTexSubImage2D_pfn                  glTexSubImage2D_Original           = nullptr;
glCompressedTexImage2D_pfn           glCompressedTexImage2D_Original    = nullptr;
glCompressedTexSubImage2D_pfn        glCompressedTexSubImage2D          = nullptr;
glCopyTexSubImage2D_pfn              glCopyTexSubImage2D_Original       = nullptr;
glDeleteTextures_pfn                 glDeleteTextures_Original          = nullptr;
glTexParameteri_pfn                  glTexParameteri_Original           = nullptr;
glGenerateMipmap_pfn                 glGenerateMipmap                   = nullptr;

// Fast-path to avoid redundant CPU->GPU data transfers (there are A LOT of them)
glCopyImageSubData_pfn               glCopyImageSubData                 = nullptr;


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
glDepthMask_pfn                      glDepthMask_Original               = nullptr;
                                                                        
glScissor_pfn                        glScissor_Original                 = nullptr;
glViewport_pfn                       glViewport_Original                = nullptr;
                                                                        
glDrawArrays_pfn                     glDrawArrays_Original              = nullptr;
                                                                        
glUseProgram_pfn                     glUseProgram_Original              = nullptr;
glGetShaderiv_pfn                    glGetShaderiv_Original             = nullptr;
glShaderSource_pfn                   glShaderSource_Original            = nullptr;
glAttachShader_pfn                   glAttachShader_Original            = nullptr;
                                                                        
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

typedef GLvoid (WINAPI *glClear_pfn)(
  GLbitfield mask
);

glClear_pfn glClear_Original = nullptr;

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
#include <unordered_set>

// Technically, we could have 80, but this game only uses two stages...
struct {
  struct tex1d_s {
    GLuint    name;
    GLboolean enable;
  } tex1d;

  struct texRect_s {
    GLuint    name;
    GLboolean enable;
  } texRect;

  struct tex2d_s {
    GLuint    name;
    GLboolean enable;
  } tex2d;

  struct tex3d_s {
    GLuint    name;
    GLboolean enable;
  } tex3d;
} current_textures [32] = { { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 } };

GLuint active_texture = 0;

std::unordered_set <GLuint> opaque_texids;
std::unordered_set <GLuint> translucent_texids;

bool deferred_msaa = false;

// The game hard-codes GL object names, yet another thing that will make porting
//   difficult for NIS... but it makes modding easy :)
struct {
  GLuint shadow       = 67552UL; // Shadow   (FBO=1)
  GLuint color        = 67553UL; // Color    (FBO=2)
  GLuint depth        = 67554UL; // Depth    (FBO=2)
  GLuint normal       = 67555UL; // Normal   (FBO=2)
  GLuint position     = 67556UL; // Position (FBO=2)
  GLuint unknown      = 67557UL; // Unknown  (FBO=3)
} render_targets;

struct msaa_ref_s {
  bool   dirty      = false;
  bool   active     = false;

  GLuint  rbo        = GL_NONE;

  GLsizei width      = 0;
  GLsizei height     = 0;
  GLenum  format     = GL_INVALID_ENUM;

  GLint   color_loc  = -1;
  GLuint  parent_fbo = -1;
};

class MSAABackend {
public:
  GLuint resolve_fbo = 0;

  std::unordered_map <GLuint, msaa_ref_s*> backing_textures;
  std::unordered_map <GLuint, msaa_ref_s*> backing_rbos;

  bool use           = true;
  bool dirty         = false;
  bool early_resolve = false;
  bool conservative  = false;

  bool   deleteTexture      ( GLuint  texId );
  bool   createTexture      ( GLuint  texId,   GLenum  internalformat,
                              GLsizei width,   GLsizei height );
  bool   attachTexture      ( GLenum  target,  GLenum  attachment,
                              GLuint  texture, GLenum textarget,
                              GLuint  level );

  bool   deleteRenderbuffer ( GLuint  rboId );
  bool   createRenderbuffer (                  GLenum  internalformat,
                              GLsizei width,   GLsizei height );

  bool   attachRenderbuffer ( GLenum  target,  GLenum  attachment,
                              GLuint  renderbuffer );

  bool   resolveTexture     ( GLuint texId );

  GLuint bindFramebuffer    ( GLenum target, GLuint framebuffer );
} msaa;

GLuint active_fbo       = 0;

// Not used yet
#if 0
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
#endif


// These are textures that should always be clamped to edge for proper
//   smoothing behavior
#include <unordered_set>
std::unordered_set <GLuint> ui_textures;

using namespace pp;

GLuint active_program = 0;
bool   postprocessing = false;
bool   postprocessed  = false;

COM_DECLSPEC_NOTHROW
void
STDMETHODCALLTYPE
OGLEndFrame_Pre (void)
{
  postprocessing = false;
  postprocessed  = false;

BEGIN_TASK_TIMING
  if (GLPerf::timers [GLPerf::TASK_END_UI]->isReady ()) {
    GLPerf::timers [GLPerf::TASK_END_UI]->requestTimestamp ();
  }
END_TASK_TIMING

  //pp::RenderFix::dwRenderThreadID = GetCurrentThreadId ();

  active_program  = 0;
  program33_draws = 0;

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


  // Additional text that we can print below the command console
  extern std::string mod_text;

BEGIN_TASK_TIMING
  if (GLPerf::timers [GLPerf::TASK_BEGIN_FRAME]->isReady ()) {
    GLPerf::timers [GLPerf::TASK_BEGIN_FRAME]->requestTimestamp ();
  } else {
    if (GLPerf::timers [GLPerf::TASK_END_FRAME]->isReady ())
      GLPerf::timers [GLPerf::TASK_END_FRAME]->requestTimestamp ();

    if (GLPerf::timers [GLPerf::TASK_END_FRAME]->isFinished ()) {
      GLuint64 start,end;

      struct {
        GLuint64 ts0, ts1,
                 ts2, ts3;

        GLuint64 ts_start = 0ULL,
                 ts_end   = 0ULL;
      } msaa_times;

      GLuint64 pp_start, pp_end;
      GLuint64 ui_start, ui_end;

     static DWORD         dwTime = timeGetTime ();
     static unsigned char spin    = 193;

     if (dwTime < timeGetTime ()-100UL)
       spin++;

     if (spin > 199)
       spin = 193;

      if (config.render.msaa_samples > 1 && msaa.use) {
        GLPerf::timers [GLPerf::TASK_MSAA0]->getResulIfFinished (&msaa_times.ts0);
        GLPerf::timers [GLPerf::TASK_MSAA1]->getResulIfFinished (&msaa_times.ts1);
        GLPerf::timers [GLPerf::TASK_MSAA2]->getResulIfFinished (&msaa_times.ts2);
        GLPerf::timers [GLPerf::TASK_MSAA3]->getResulIfFinished (&msaa_times.ts3);

        msaa_times.ts_start = min ( msaa_times.ts0,
                                      min ( msaa_times.ts1,
                                              min ( msaa_times.ts2, 
                                                      msaa_times.ts3 ) ) );
        msaa_times.ts_end   = max ( msaa_times.ts0,
                                      max ( msaa_times.ts1,
                                              max ( msaa_times.ts2,
                                                      msaa_times.ts3 ) ) );
      }

      GLPerf::timers [GLPerf::TASK_BEGIN_POSTPROC]->getResulIfFinished (&pp_start);
      GLPerf::timers [GLPerf::TASK_END_POSTPROC]->getResulIfFinished   (&pp_end);

      GLPerf::timers [GLPerf::TASK_BEGIN_UI]->getResulIfFinished       (&ui_start);
      GLPerf::timers [GLPerf::TASK_END_UI]->getResulIfFinished         (&ui_end);

      GLPerf::timers [GLPerf::TASK_BEGIN_FRAME]->getResulIfFinished    (&start);
      GLPerf::timers [GLPerf::TASK_END_FRAME]->getResulIfFinished      (&end);

      GLuint64 msaa_time     = (msaa_times.ts_end - msaa_times.ts_start) * 2,
               postproc_time = (pp_end            - pp_start),
               ui_time       = (ui_end            - ui_start),
               ex_time       = (end               - ui_end);

      GLuint64 max_time = max ( msaa_time,
                                  max ( ui_time,
                                          postproc_time ) );

      unsigned char main_status = ' ';
      unsigned char msaa_status = msaa_time     == max_time ? spin : ' ';
      unsigned char pp_status   = postproc_time == max_time ? spin : ' ';
      unsigned char ui_status   = ui_time       == max_time ? spin : ' ';
      unsigned char ex_status   = ex_time       == max_time ? spin : ' ';

      static char szFrameTimes [512];
      sprintf ( szFrameTimes, "* Full Frame.: %06.03f ms\n"
                              "------------------------\n"
                              //"%c Main Scene.: AA.AAA ms\n"
                              "%c MSAA.......: %06.03f ms\n"
                              "%c Post-Proc..: %06.03f ms\n"
                              "%c UI.........: %06.03f ms\n"
                              //"%c Other......: %#05.03f ms\n",
                              ,
                  (double)(end - start) / 1000000.0,
          //main_status,
            msaa_status, (double)(msaa_time) / 1000000.0,
              pp_status, (double)(postproc_time) / 1000000.0,
                ui_status, (double)(ui_time) / 1000000.0//,
                  );//ex_status, (double)(ex_time) / 1000000.0 );

      mod_text = szFrameTimes;
    }
  }
IF_NO_TASK_TIMING
  // Clear the OSD if this is turned off...
  mod_text = "";
END_TASK_TIMING

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


GLboolean real_blend_state   = GL_FALSE;
GLboolean real_alpha_state   = GL_FALSE;
GLboolean real_depth_test    = GL_FALSE;
GLboolean real_depth_mask    = GL_FALSE;

GLenum  real_alpha_test = GL_ALWAYS;
GLfloat real_alpha_ref  = 1.0f;

GLenum src_blend, dst_blend;

bool texture_2d = false;

__declspec (noinline)
GLvoid
WINAPI
glUseProgram_Detour (GLuint program)
{
  bool redundant = (program == 0 && active_fbo != 0) ||
                   (program == active_program);

  if (pp::RenderFix::tracer.log) {
    const bool log_redundant = false;
    if (log_redundant || (! redundant)) {
      dll_log.LogEx ( true, L"[ GL Trace ] glUseProgram (%lu)",
                        program );

      if (redundant)
        dll_log.LogEx ( false, L" { Redundant }\n" );
      else
        dll_log.LogEx ( false, L"\n" );
    }
  }

  if (aggressive_optimization) {
    //
    // Performance Optimization: STOP invalidating shader cache between draws
    //
    if (redundant)
      return;

#if 0
    if (active_fbo == 2 || (active_fbo == msaa.resolve_fbo && msaa.resolve_fbo != 0)) {
      if (program == 33) {
        GLuint buffers [3] = { GL_COLOR_ATTACHMENT0,
                               GL_COLOR_ATTACHMENT0 + 1,
                               GL_COLOR_ATTACHMENT0 + 2 };
        glDrawBuffers_Original (3, buffers);
        //glColorMaski (1, GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        //glColorMask (2, GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
      }

      // Ignore the other two color attachments
      else if (active_program == 33) {
        //glColorMaski (1, GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
        //glColorMaski (2, GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
        glDrawBuffer (GL_COLOR_ATTACHMENT0);
      }
    }
#endif
  }

  active_program = program;

  glUseProgram_Original (program);
}

__declspec (noinline)
GLvoid
WINAPI
glBlendFunc_Detour (GLenum srcfactor, GLenum dstfactor)
{
  if (pp::RenderFix::tracer.log) {
    dll_log.Log ( L"[ GL Trace ] glBlendFunc (0x%X, 0x%X)",
                    srcfactor, dstfactor );
  }

  src_blend = srcfactor;
  dst_blend = dstfactor;

  glBlendFunc_Original (srcfactor, dstfactor);
}

__declspec (noinline)
GLvoid
WINAPI
glAlphaFunc_Detour (GLenum alpha_test, GLfloat ref_val)
{
  if (pp::RenderFix::tracer.log) {
    dll_log.Log ( L"[ GL Trace ] glAlphaFunc (0x%X, %f)",
                    alpha_test, ref_val );
  }

  real_alpha_test = alpha_test;
  real_alpha_ref  = ref_val;

  glAlphaFunc_Original (alpha_test, ref_val);
}

__declspec (noinline)
GLvoid
WINAPI
glDepthMask_Detour (GLboolean flag)
{
  real_depth_mask = flag;

  return glDepthMask_Original (flag);
}

__declspec (noinline)
GLvoid
WINAPI
glEnable_Detour (GLenum cap)
{
  // This invalid state setup is embedded in some display lists
  if (cap == GL_TEXTURE) cap = GL_TEXTURE_2D;

  if (cap == GL_TEXTURE_2D) {
    texture_2d = true;

    current_textures [active_texture].tex2d.enable = true;
  }

  if (cap == GL_BLEND)      {
    bool opaque = true;

#if 0
    for (int i = 0; i < 32; i++) {
      // Handle the Steam overlay
      if ( current_textures [i].texRect.name != 0 ) {
        opaque = false;
        break;
      }

      if ( (active_program != 0 || current_textures [i].tex2d.enable) &&
           translucent_texids.count ( 
             current_textures [i].tex2d.name ) ) {
        opaque = false;
        break;
      }
    }

    if (! opaque)
      real_blend_state = true;
    else {
      //dll_log.Log (L"[GL SpeedUp] Overriding Blend State... [All Opaque]");
      real_blend_state = false;
      glDisable_Original (GL_BLEND);
      return;
    }
#else
    real_blend_state = true;
#endif
  }
  if (cap == GL_ALPHA_TEST) real_alpha_state = true;

  if (cap == GL_DEPTH_TEST) real_depth_test  = true;

  return glEnable_Original (cap);
}

__declspec (noinline)
GLvoid
WINAPI
glDisable_Detour (GLenum cap)
{
  // This invalid state setup is embedded in some display lists
  if (cap == GL_TEXTURE) cap = GL_TEXTURE_2D;

  if (cap == GL_TEXTURE_2D) {
    texture_2d = false;

    current_textures [active_texture].tex2d.enable = false;
  }

  if (cap == GL_BLEND)      real_blend_state = false;
  if (cap == GL_ALPHA_TEST) real_alpha_state = false;

  if (cap == GL_DEPTH_TEST) real_depth_test  = false;

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
  GLuint texId = 0;

  if (target == GL_TEXTURE_2D) {
    texId = current_textures [active_texture].tex2d.name;
  }

  // We cannot mipmap normal maps, because that would require re-normalization
  if (active_texture != 0) {
    glTexParameteri_Original (GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
    glTexParameteri_Original (GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL,  0);
  }

  if (texId >= 67552 && texId <= 67557) {
    if (texId >= render_targets.color && (pname == GL_TEXTURE_WRAP_S || pname == GL_TEXTURE_WRAP_T))
      return;

#if 0
    dll_log.Log ( L"[ GL Trace ] glTexParameteri (GL_TEXTURE_2D, 0x%X, 0x%X) - TexID: %lu",
                   pname, param, texId );
#endif

    //if (pname == GL_TEXTURE_MAG_FILTER || pname == GL_TEXTURE_MIN_FILTER)
      //param = GL_NEAREST;

#if 0
#define GL_DEPTH_TEXTURE_MODE 0x884B

    if (pname == GL_DEPTH_TEXTURE_MODE)
      param = GL_INTENSITY;
#endif
  } else {
    bool force_nearest = config.textures.pixelate > 0;

    GLint fmt;
    glGetTexLevelParameteriv (GL_TEXTURE_2D, 0, GL_TEXTURE_INTERNAL_FORMAT, &fmt);

    if (force_nearest) {
      // Don't force nearest-neighbor sampling on UI textures
      if (/*fmt != GL_RGBA &&*/ (pname == GL_TEXTURE_MAG_FILTER || pname == GL_TEXTURE_MIN_FILTER))
        param = GL_NEAREST;
    }

    if (config.textures.force_mipmaps)
    {
      if (pname == GL_TEXTURE_MIN_FILTER) {
        if (param == GL_LINEAR || param == GL_NEAREST) {
          GLint width;
          glGetTexLevelParameteriv (GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &width);

          if (param == GL_NEAREST)
          {
            // Pixelation is enhanced when mipmaps are forced, 2 == UBER pixelated ;)
            if (config.textures.pixelate)
            {
              if (config.textures.pixelate == 1)
                param = GL_NEAREST_MIPMAP_LINEAR;
              else
                param = GL_NEAREST_MIPMAP_NEAREST;
            }
          } else {
            param = GL_LINEAR_MIPMAP_LINEAR;
          }
        }
      }
    }
  }

  return glTexParameteri_Original (target, pname, param);
}

typedef GLvoid (WINAPI *glTexStorage2D_pfn)(
  GLenum  target,
  GLsizei levels,
  GLenum  internalformat,
  GLsizei width,
  GLsizei height
);

glTexStorage2D_pfn glTexStorage2D = nullptr;

std::unordered_multimap <uint32_t, GLuint> texture_checksums;
std::unordered_map      <GLuint, uint32_t> texture_checksums_rev;

bool
PPrinny_TEX_RemoveTextureRecord (GLuint texId)
{
  std::unordered_map <GLuint, uint32_t>::iterator it =
    texture_checksums_rev.find (texId);

  if (it != texture_checksums_rev.end ()) {
    std::pair < std::unordered_multimap <uint32_t, GLuint>::iterator,
                std::unordered_multimap <uint32_t, GLuint>::iterator > keys =
      texture_checksums.equal_range (it->second);

    std::unordered_multimap <uint32_t, GLuint>::iterator key;

    for (key = texture_checksums.begin (); key != texture_checksums.end (); ++key) {// keys.first; key != keys.second; ++key) {
      if (key->second == texId) {
        texture_checksums.erase (key);
      }
    }

    texture_checksums_rev.erase (it);

    return true;
  }

  return false;
}

void
PPrinny_TEX_AddTextureRecord (GLuint texId, uint32_t checksum)
{
  std::unordered_map <GLuint, uint32_t>::iterator it =
    texture_checksums_rev.find (texId);

  if (it != texture_checksums_rev.end ()) {
    if (it->second != checksum) {
      // First, we need to clear any pre-existing record
      PPrinny_TEX_RemoveTextureRecord (texId);
    } else {
      return;
    }
  }

  texture_checksums_rev.insert (std::pair <GLuint, uint32_t> (texId, checksum));
  texture_checksums.insert     (std::pair <uint32_t, GLuint> (checksum, texId));
}

void
PPrinny_TEX_GenerateMipmap (GLsizei width, GLsizei height)
{
  //glTexParameteri ( GL_TEXTURE_2D,
                      //GL_TEXTURE_MAX_LEVEL,
                        //max (0, (int)floor (log2 (max (width,height)))));

  glGenerateMipmap ( GL_TEXTURE_2D );

  if (config.textures.pixelate > 2) {
    int lod_bias = 1 - config.textures.pixelate;

#define GL_TEXTURE_LOD_BIAS                0x8501

    glTexParameterf ( GL_TEXTURE_2D,
                        GL_TEXTURE_LOD_BIAS,
                          (float)lod_bias );
  }
}

bool
PPrinny_TEX_TestOpaque ( GLuint  texId,
                         GLsizei width,          GLsizei height,
                         GLenum  internalFormat, GLenum  format,
                   const GLvoid* data )
{
#if 0
  if (internalFormat == GL_RGBA && format == GL_RGBA) {
    int first_alpha = 3;
    int       alpha = first_alpha;

    for (int alpha = first_alpha; alpha < width * height * 4; alpha += 4)
      if (((uint8_t *)data) [alpha] != 0xff)
        return false;
  } else {
    // Haven't bothered decoding DXT stuff yet...
    return false;
  }

  dll_log.Log ( L"Identified texId (%lu) as opaque",
                  texId );
  return true;
#else
  return false;

  static float fTexData [4*2048*2048];

  bool opaque = true;

  glGetTexImage ( GL_TEXTURE_2D, 0, GL_ALPHA, GL_FLOAT, &fTexData );

  for (int i = 0; i < width * height; i++) {
    if (fTexData [i] != 1.0f) {
      opaque = false;
      break;
    }
  }

  if (opaque) {
    if (translucent_texids.count (texId))
      translucent_texids.erase (texId);

    opaque_texids.insert (texId);

    //dll_log.Log (L"Opaque Texture Detected... (texId=%lu)", texId);

    return true;
  }

  else {
    if (opaque_texids.count (texId))
      opaque_texids.erase (texId);

    translucent_texids.insert (texId);

    //dll_log.Log (L"Translucent Texture Detected... (texId=%lu)", texId);

    return false;
  }
#endif
}

bool
PPrinny_TEX_AvoidRedundantCPUtoGPU (
  GLenum   target,
  //GLint    level, -- NIS does not know what mipmap LODs are ;)
  GLenum   internalFormat,
  GLsizei  width,
  GLsizei  height,

  uint32_t checksum,
  GLuint   texId,
  void*    data_addr )
{
  if (! config.textures.caching)
    return false;

  static DWORD last_purge     = 0;
  static void* last_data_addr = 0;

#if 0
  // Cache coherency is strangely a problem, so purge this once per-second.
  if (timeGetTime () - last_purge > 1000UL) {
    texture_checksums_rev.clear ();
    texture_checksums.clear     ();
    last_purge = timeGetTime    ();
  }
#endif
  // It is likely that some texture manipulation function was not hooked :-\

#if 0
  // While not technically an error to upload data to the "default" texture, I have my doubts that
  //   NIS actually intended to do this...
  if (texId == 0)
    return false;
#endif

  if (checksum != 0x00UL) {
    std::unordered_map <GLuint, uint32_t>::iterator it =
      texture_checksums_rev.find (texId);

    if (it != texture_checksums_rev.end ()) {
      if (it->second == checksum && last_data_addr == data_addr)
        return true;
    }
  }

  last_data_addr = data_addr;

  std::unordered_multimap <uint32_t, GLuint>::iterator srcTex =
      texture_checksums.begin ();//

  std::pair < std::unordered_multimap <uint32_t, GLuint>::iterator,
              std::unordered_multimap <uint32_t, GLuint>::iterator > range =
    texture_checksums.equal_range (checksum);

  srcTex = range.first;

  while (srcTex != texture_checksums.end ()) {
    if (((texId < 67558 && srcTex->second > 67557) ||
         (texId > 67557 && srcTex->second < 67558)) && srcTex->first == checksum)
      break;
    ++srcTex;
  }

  if (srcTex->second != texId)
    PPrinny_TEX_AddTextureRecord (texId, checksum);

#if 1
  if (glCopyImageSubData != nullptr) {
    // glCopyImageSubData does not like this format...
    //if (internalFormat == 0x83f3 || width <= 1024)
      //return false;

    if (srcTex != texture_checksums.end ()) {
      GLint tex_levels;

      glBindTexture       (GL_TEXTURE_2D, srcTex->second);
      glGetTexParameteriv (GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, &tex_levels);

      GLint srcFormat, srcWidth, srcHeight;
      glGetTexLevelParameteriv (GL_TEXTURE_2D, 0, GL_TEXTURE_INTERNAL_FORMAT, &srcFormat);
      glGetTexLevelParameteriv (GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH,           &srcWidth);
      glGetTexLevelParameteriv (GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT,          &srcHeight);

      if (srcFormat != internalFormat || srcHeight != height || srcWidth != width) {
//        dll_log.Log (L"[GL SpeedUp] Invalid match detected (%lu)?!", srcTex->second);
        PPrinny_TEX_RemoveTextureRecord (srcTex->second);
        glBindTexture (GL_TEXTURE_2D, texId);
        return false;
      }

      // GL's default is muy unhelpful
      if (tex_levels == 1000)
        tex_levels = 0;

      //dll_log.Log (L"tex_levels: %lu  <== (%lux%lu)", tex_levels, width, height);

      glBindTexture   (GL_TEXTURE_2D, texId);
      glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, tex_levels);

      int w = width;
      int h = height;

      tex_levels = 0;

      //if (tex_levels > floor (log2 (max (width, height))))
        //tex_levels = 0;

      //glTexStorage2D (GL_TEXTURE_2D, tex_levels+1, internalFormat, width, height);
      for (int i = 0; i <= tex_levels; i++) {
//        dll_log.Log ( L"[GL SpeedUp] srcTexId: %lu ==> dstTexId: %lu :: (%lux%lu), GL_RGBA (%x), (level: %lu)",
//                        srcTex->second, texId, w, h, internalFormat, i );
        glTexImage2D_Original ( GL_TEXTURE_2D, i, internalFormat, w, h, 0, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, nullptr );
        glCopyImageSubData    ( srcTex->second, GL_TEXTURE_2D, i, 0, 0, 0,
                                texId,          GL_TEXTURE_2D, i, 0, 0, 0,
                                w, h, 1 );

        if (w > 1) w /= 2;
        if (h > 1) h /= 2;
      }

      if (config.textures.force_mipmaps && tex_levels == 0)
        PPrinny_TEX_GenerateMipmap (width, height);

      return true;
    }
  }
#endif

  return false;
}

typedef GLvoid (WINAPI *glPixelTransferi_pfn)(
  GLenum pname,
  GLint  param
);

glPixelTransferi_pfn glPixelTransferi_Original = nullptr;

__declspec (noinline)
GLvoid
WINAPI
glPixelTransferi_Detour (
  GLenum pname,
  GLint  param )
{
//  dll_log.Log ( L"[ GL Trace ] PixelTransferi: %x, %lu",
//                  pname, param );

//  glPixelTransferi_Original (pname, param);
}

typedef GLvoid (WINAPI *glPixelMapfv_pfn)(
        GLenum   map,
        GLsizei  mapsize,
  const GLfloat* values
);

glPixelMapfv_pfn glPixelMapfv_Original = nullptr;

__declspec (noinline)
GLvoid
WINAPI
glPixelMapfv_Detour (
        GLenum   map,
        GLsizei  mapsize,
  const GLfloat* values )
{
  //if ((! aggressive_optimization) || memcmp (pixel_maps [map - GL_PIXEL_MAP_I_TO_I], values, mapsize * sizeof (GLfloat))) {
    //dll_log.Log (L"Map Change");
    memcpy (pixel_maps [map - GL_PIXEL_MAP_I_TO_I], values, mapsize * sizeof (GLfloat));
    //dll_log.Log (L"Pixel Map Size: %lu, idx: %lu", mapsize, map - GL_PIXEL_MAP_I_TO_I);
    //glPixelMapfv_Original (map, mapsize, values);
  //} else {
    //dll_log.Log (L"Redundant glPixelMapfv call cancelled");
    //return;
  //}
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

  GLuint texId = current_textures [active_texture].tex2d.name;

  //dll_log.Log ( L"texId: %lu, crc32: %x, Level: %li, target: %x, border: %lu, data: %ph",
                  //texId, checksum, level, target, border, data );

  if ( PPrinny_TEX_AvoidRedundantCPUtoGPU ( target,
                                              internalFormat,
                                                width,
                                                  height,
                                                    checksum,
                                                      texId, (void *)data ) ) {
  //dll_log.Log (L"crc32: %x, Level: %li, target: %x, border: %lu, imageSize: %lu, data: %ph",
               //checksum, level, target, border, imageSize, data);

    if (pp::RenderFix::tracer.log)
      dll_log.Log (L"[GL SpeedUp] Avoided redundant compressed texture upload...");

    // Happy day! We just skipped a lot of framerate problems.
    return;
  }

  if (config.textures.dump) {
    PPrinny_DumpCompressedTexLevel ( checksum,
                                       level,
                                         internalFormat,
                                           width,
                                             height,
                                               imageSize,
                                                 data );
  }

#if 0
    dll_log.Log ( L"[GL Texture] Loaded Compressed Texture: Level=%li, (%lix%li) {%5.2f KiB}",
                    level, width, height, (float)imageSize / 1024.0f );
#endif

  glCompressedTexImage2D_Original (
    target, level,
      internalFormat,
        width, height,
          border,
            imageSize, data );

  GLuint cacheTexId;
  glGenTextures (1, &cacheTexId);

  PPrinny_TEX_AvoidRedundantCPUtoGPU ( target,
                                         internalFormat,
                                           width,
                                             height,
                                               checksum,
                                                 cacheTexId, nullptr );

  glBindTexture (GL_TEXTURE_2D, texId);

  PPrinny_TEX_TestOpaque ( texId,
                           width,          height,
                           internalFormat, GL_RGBA,
                           data );

#if 0
  // In the event that we are re-specifying a texture, remove its old checksum.
  if (texture_checksums_rev.count (texId))
    texture_checksums_rev.erase (texId);

  texture_checksums_rev.insert (std::pair <GLuint, uint32_t> (texId, checksum));
#endif

  //
  // Mipmap Generation
  //
  if (config.textures.force_mipmaps) {
    PPrinny_TEX_GenerateMipmap (width, height);
  }
}

GLuint
MSAABackend::bindFramebuffer (GLenum target, GLuint framebuffer)
{
  if (config.render.msaa_samples < 2)
    return framebuffer;

  // On repeated failures, stop reporting errors and turn MSAA off completely...
  static int failures = 0;

  if (! use) {
    framebuffer = resolve_fbo;
  } else if (target == GL_FRAMEBUFFER || target == GL_DRAW_FRAMEBUFFER) {
    // On failure to validate FBO, fallback to resolve_fbo
    if (glCheckFramebufferStatus (target) != GL_FRAMEBUFFER_COMPLETE) {
      dll_log.Log ( L"[   MSAA   ] Framebuffer failed completeness check, "
                    L"falling back to single-sampling (chance=%lu) !!!",
                      failures++ );

      framebuffer = resolve_fbo;

      if (failures > 5) {
        dll_log.Log ( L"[   MSAA   ]  -- Too many failures, "
                      L"disabling MSAA!" );
        use = false;
      }
    }

    std::unordered_map <GLuint, msaa_ref_s*>::iterator tex_it =
      backing_textures.begin ();

    while (tex_it != backing_textures.end ()) {
      if (/*tex_it->second->color_loc >= 0 &&*/tex_it->second->color_loc <= 2)
        tex_it->second->dirty = true;

      ++tex_it;
    }

    std::unordered_map <GLuint, msaa_ref_s*>::iterator rbo_it =
      backing_rbos.begin ();

    while (rbo_it != backing_rbos.end ()) {
      rbo_it->second->dirty = true;

      ++rbo_it;
    }

    //dirty = true;
  }

  // Simple pass-through to handle scenarios were MSAA rasterization
  //   is skipped.
  if (framebuffer == resolve_fbo && resolve_fbo != 0) {
    glBindFramebuffer_Original (target, framebuffer);

    GLenum buffers [] = { GL_COLOR_ATTACHMENT0,
                          GL_COLOR_ATTACHMENT0 + 1,
                          GL_COLOR_ATTACHMENT0 + 2 };
    glDrawBuffers_Original (3, buffers);
  }

  return framebuffer;
}

bool
MSAABackend::resolveTexture (GLuint texId)
{
  std::unordered_map <GLuint, msaa_ref_s*>::iterator it =
    backing_textures.find (texId);

  if (it != backing_textures.end ()) {
    if (it->second->dirty) {
      switch (texId) {
        case 67554:
BEGIN_TASK_TIMING
          if (GLPerf::timers [GLPerf::TASK_MSAA0]->isReady ())
            GLPerf::timers [GLPerf::TASK_MSAA0]->requestTimestamp ();
END_TASK_TIMING
          break;
        case 67553:
BEGIN_TASK_TIMING
           if (GLPerf::timers [GLPerf::TASK_MSAA1]->isReady ())
            GLPerf::timers [GLPerf::TASK_MSAA1]->requestTimestamp ();
END_TASK_TIMING
          break;

        // Normals
        case 67555:
BEGIN_TASK_TIMING
           if (GLPerf::timers [GLPerf::TASK_MSAA2]->isReady ())
            GLPerf::timers [GLPerf::TASK_MSAA2]->requestTimestamp ();
END_TASK_TIMING

           //if (aggressive_optimization && program33_draws == 0) {
             //it->second->dirty = false;
             //return true;
           //}
          break;

        // Position
        case 67556:
BEGIN_TASK_TIMING
           if (GLPerf::timers [GLPerf::TASK_MSAA3]->isReady ())
            GLPerf::timers [GLPerf::TASK_MSAA3]->requestTimestamp ();
END_TASK_TIMING

           //if (aggressive_optimization && program33_draws == 0) {
             //it->second->dirty = false;
             //return true;
           //}
          break;
      }

      if (pp::RenderFix::tracer.log) {
        dll_log.Log ( L"[ GL Trace ] Resolving Dirty Multisampled Texture..."
                      L" (tid=%lu)",
                        texId );
      }

      // BLIT from RBO to TEX

      GLuint read_buffer,
             draw_buffer;

      glGetIntegerv (GL_READ_FRAMEBUFFER_BINDING, (GLint *)&read_buffer);
      glGetIntegerv (GL_DRAW_FRAMEBUFFER_BINDING, (GLint *)&draw_buffer);

      glBindFramebuffer_Original (GL_READ_FRAMEBUFFER, 2);

      int buffer = 
        it->second->color_loc >= 0 ?
          it->second->color_loc + GL_COLOR_ATTACHMENT0 :
          GL_NONE;

      int bitmask =
        it->second->color_loc >= 0 ?
          GL_COLOR_BUFFER_BIT :
          GL_DEPTH_BUFFER_BIT/* | GL_STENCIL_BUFFER_BIT*/;

      glBindFramebuffer_Original (GL_DRAW_FRAMEBUFFER, resolve_fbo);

      glReadBuffer (buffer);
      glDrawBuffer (buffer);

        glBlitFramebuffer_Original ( 0, 0,
                                       fb_width, fb_height,
                                     0, 0,
                                       fb_width, fb_height,
                                     bitmask,
                                       GL_NEAREST );

      glBindFramebuffer_Original (GL_DRAW_FRAMEBUFFER, draw_buffer);
      glBindFramebuffer_Original (GL_READ_FRAMEBUFFER, read_buffer);

      it->second->dirty = false;

      return true;
    }
  }

  return false;
}

__declspec (noinline)
GLvoid
WINAPI
glBindFramebuffer_Detour (GLenum target, GLuint framebuffer)
{
  GLuint previous_fbo = active_fbo;
         active_fbo   = framebuffer;

  if (pp::RenderFix::tracer.log)
    dll_log.Log ( L"[ GL Trace ] glBindFramebuffer (0x%X, %lu)",
                    target, framebuffer );

  if (framebuffer == 2) {
    // First time binding the scene framebuffer after postprocessing starts
    //   signals the end of postprocessing...
    if (postprocessing) {
      postprocessing = false;
      postprocessed  = true;

BEGIN_TASK_TIMING
      if (GLPerf::timers [GLPerf::TASK_END_POSTPROC]->isReady ())
        GLPerf::timers [GLPerf::TASK_END_POSTPROC]->requestTimestamp ();
END_TASK_TIMING
    }

    framebuffer = msaa.bindFramebuffer (target, framebuffer);
  }

  // Start of UI rendering
  if (framebuffer == 0 && postprocessed) {
BEGIN_TASK_TIMING
    if (GLPerf::timers [GLPerf::TASK_BEGIN_UI]->isReady ())
      GLPerf::timers [GLPerf::TASK_BEGIN_UI]->requestTimestamp ();
END_TASK_TIMING
  }

  glBindFramebuffer_Original (target, framebuffer);


  if (config.render.msaa_samples > 1) {
    if (msaa.use && framebuffer == 2 && framebuffer != msaa.resolve_fbo)
      glEnable  (GL_MULTISAMPLE);
    else
      glDisable (GL_MULTISAMPLE);
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

    case GL_RGB16F:
      return L"RGB       (16-bit Half-Float)";

    case GL_RGB32F:
      return L"RGB       (32-bit Float)";
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
    _swprintf (wszColorLoc, L"Color%li", attach - GL_COLOR_ATTACHMENT0);
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
  if (config.render.msaa_samples > 1)
    if (msaa.attachRenderbuffer (target, attachment, renderbuffer))
      return;

  if (attachment == GL_DEPTH_ATTACHMENT) {
    if (active_fbo == 2) {
      glFramebufferTexture2D_Original ( target,
                                          attachment,
                                            GL_TEXTURE_2D,
                                              render_targets.depth,
                                                0 );
      return;
    } /*else if (active_fbo == 1) {
      glFramebufferTexture2D_Original ( target,
                                          attachment,
                                            GL_TEXTURE_2D,
                                              render_targets.shadow,
                                                0 );
      return;
    }*/
  }

  //dll_log.Log (L" Attaching RBO %lu to FBO %lu...", renderbuffer, active_fbo);

  glFramebufferRenderbuffer_Original ( target,
                                         attachment,
                                           renderbuffertarget,
                                             renderbuffer );
}

__declspec (noinline)
GLvoid
WINAPI
glDrawBuffers_Detour
(
        GLsizei n,
  const GLenum* bufs
)
{
  if (pp::RenderFix::tracer.log) {
    dll_log.LogEx ( true, L"[ GL Trace ] glDrawBuffers (%lu, {",
                      n );
    for (int i = 0; i < n; i++) {
      dll_log.LogEx (false, L"0x%X", bufs [i]);

      if (i < n-1)
        dll_log.LogEx (false, L", ");
    }

    dll_log.LogEx (false, L"})\n");
  }

  return glDrawBuffers_Original (n, bufs);
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
  if (config.render.msaa_samples > 1)
    if (msaa.attachTexture (target, attachment, texture, textarget, level))
      return;

  glFramebufferTexture2D_Original ( target,
                                      attachment,
                                        textarget,
                                          texture,
                                            level );
}

#define GL_PIXEL_PACK_BUFFER         0x88EB
#define GL_PIXEL_PACK_BUFFER_BINDING 0x88ED
#define GL_DYNAMIC_READ              0x88E9
#define GL_STREAM_READ               0x88E1

typedef GLvoid (WINAPI *glGenBuffers_pfn)(
  GLsizei n,
  GLuint *buffers
);

typedef GLvoid (WINAPI *glBindBuffer_pfn)(
  GLenum target,
  GLuint buffer
);

typedef GLvoid (WINAPI *glBufferData_pfn)(
        GLenum  target,
        GLsizei size,
  const GLvoid *data,
        GLenum  usage
);

typedef GLvoid (WINAPI *glGetBufferSubData_pfn)(
  GLenum  target,
  GLsizei offset,
  GLsizei size,
  GLvoid *data
);

typedef struct __GLsync *GLsync;

typedef GLsync (WINAPI *glFenceSync_pfn)(
  GLenum     condition,
  GLbitfield flags
);

typedef GLenum (WINAPI *glClientWaitSync_pfn)(
  GLsync     sync,
  GLbitfield flags,
  GLuint64   timeout
);

typedef GLvoid (WINAPI *glDeleteSync_pfn)(
  GLsync sync
);

glGenBuffers_pfn       glGenBuffers       = nullptr;
glBindBuffer_pfn       glBindBuffer       = nullptr;
glBufferData_pfn       glBufferData       = nullptr;
glGetBufferSubData_pfn glGetBufferSubData = nullptr;

glFenceSync_pfn      glFenceSync      = nullptr;
glClientWaitSync_pfn glClientWaitSync = nullptr;
glDeleteSync_pfn     glDeleteSync     = nullptr;

#define GL_SYNC_FLUSH_COMMANDS_BIT    0x0001
#define GL_SYNC_GPU_COMMANDS_COMPLETE 0x9117
#define GL_ALREADY_SIGNALED           0x911A
#define GL_CONDITION_SATISFIED        0x911C


bool wait_one_extra = false;//true;
bool waited         = false;


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
  if (pp::RenderFix::tracer.log) {
    dll_log.Log ( L"[ GL Trace ] glReadPixels (%li, %li, %li, %li, 0x%X, 0x%X, %ph) "
                  L"[Calling Thread: tid=%x]",
                    x,y,
                      width,height,
                        format,type,
                          data,
                            GetCurrentThreadId () );
  }

  // This is designed to read a 10x10 region at the center
  //   of the screen. That of course only works if 720p is
  //     forced.
  if (x == game_width / 2 - 5 && y == game_height / 2 - 5) {
    x = fb_width  / 2 - 5;
    y = fb_height / 2 - 5;

    static GLubyte cached_data [10 * 10 * 4] = { 0 };
    static GLuint  pbo = 0;

    if (pbo == 0) {
      glGenBuffers (1, &pbo);

      GLuint original_pbo;
      glGetIntegerv (GL_PIXEL_PACK_BUFFER_BINDING, (GLint *)&original_pbo);

      glBindBuffer (GL_PIXEL_PACK_BUFFER, pbo);
      glBufferData (GL_PIXEL_PACK_BUFFER, 10 * 10 * 4, nullptr, GL_DYNAMIC_READ);

      glBindBuffer (GL_PIXEL_PACK_BUFFER, original_pbo);
    }

    //
    // Even on systems that do not support sync objects, the way this is written
    //   it will introduce at least a 1 frame latency between issuing a read
    //     and actually trying to get the result, which helps performance.
    //
    //  On a system with sync object support, this is very efficient.
    //
    static GLsync fence = nullptr;
    if (fence == nullptr) {
// For Depth of Field  (color buffers should alreaby be resolved)
#if 1
      if (config.render.msaa_samples > 1) {
        GLuint read_fbo, draw_fbo;
        glGetIntegerv (GL_READ_FRAMEBUFFER_BINDING, (GLint *)&read_fbo);
        glGetIntegerv (GL_DRAW_FRAMEBUFFER_BINDING, (GLint *)&draw_fbo);

        if (read_fbo == 2 && msaa.use) {
          GLuint read_buffer;
          glGetIntegerv (GL_READ_BUFFER, (GLint *)&read_buffer);

          msaa.resolveTexture (pp::RenderFix::draw_state.depth_tex);

          glBindFramebuffer_Original (GL_READ_FRAMEBUFFER, msaa.resolve_fbo);
          glReadBuffer               (read_buffer);
        }
      }
#endif

      GLuint original_pbo;
      glGetIntegerv         (GL_PIXEL_PACK_BUFFER_BINDING, (GLint *)&original_pbo);

      glBindBuffer          (GL_PIXEL_PACK_BUFFER, pbo);
      glReadPixels_Original (x, y, width, height, format, type, nullptr);

      if (glFenceSync != nullptr)
        fence = glFenceSync (GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
      else
        fence = (GLsync)0x01;

      //dll_log.Log ( L"[GL SyncOp] Inserted fence sync (%ph) into command stream "
                    //L"for glReadPixels (...)", fence );

      glBindBuffer          (GL_PIXEL_PACK_BUFFER, original_pbo);

    } else {

      GLenum status = 
        glClientWaitSync != nullptr ?
          glClientWaitSync ( fence, 0x00/*GL_SYNC_FLUSH_COMMANDS_BIT*/, 0ULL ) :
            GL_CONDITION_SATISFIED;

      if (status == GL_CONDITION_SATISFIED || status == GL_ALREADY_SIGNALED) {
        if (wait_one_extra && (! waited))
          waited = true;
        else {
          waited = false;
          //dll_log.Log (L"[GL SyncOp] ReadPixels complete...");

          if (glDeleteSync != nullptr)
            glDeleteSync (fence);

          fence = nullptr;

          GLuint original_pbo;
          glGetIntegerv (GL_PIXEL_PACK_BUFFER_BINDING, (GLint *)&original_pbo);

          glBindBuffer       (GL_PIXEL_PACK_BUFFER, pbo);
          glGetBufferSubData (GL_PIXEL_PACK_BUFFER, 0, 10 * 10 * 4, cached_data);
          glBindBuffer       (GL_PIXEL_PACK_BUFFER, original_pbo);
        }
      } else {
        //dll_log.Log (L"[GL SyncOp] glClientWaitSync --> %x", status);
      }
    }

    memcpy (data, cached_data, 10 * 10 * 4);
    return;
  }

  return glReadPixels_Original (x, y, width, height, format, type, data);
}

__declspec (noinline)
GLvoid
WINAPI
glDrawArrays_Detour (GLenum mode, GLint first, GLsizei count)
{
  bool fringed = false;

  if (config.render.fringe_removal) {
    bool world  = (mode == GL_TRIANGLES);// && count < 180);//      && count == 6);
    bool sprite = (mode == GL_TRIANGLE_STRIP && count == 4);

     if (drawing_main_scene && (world || sprite)) {
       if ( real_depth_mask &&
            real_depth_test &&
            current_textures [active_texture].tex2d.enable ) {

       GLint filter;
       glGetTexParameteriv (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, &filter);

       if (filter == GL_LINEAR || filter == GL_LINEAR_MIPMAP_LINEAR || filter == GL_LINEAR_MIPMAP_NEAREST) {
       glDepthMask_Original (GL_FALSE);

       //if (active_program == 33 || (! aggressive_optimization)) {
         //
         // SSAO Hacks
         //
         glColorMaski (1, GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
         glColorMaski (2, GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
       //}

       {
         glEnable_Original (GL_ALPHA_TEST);

         if (sprite)
           glAlphaFunc_Original (GL_LESS, 0.425f);
         else
           glAlphaFunc_Original (GL_LESS, 0.35f);

         glDrawArrays_Original (mode, first, count);
       }

       //if (active_program == 33 || (! aggressive_optimization)) {
         glColorMaski (2, GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
         glColorMaski (1, GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
       //}

       glDepthMask_Original (GL_TRUE);

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
      } //else {
        //dll_log.Log (L" Fringe Failure: Mask: %d, Test: %d, Tex2D: %d",
                       //real_depth_mask, real_depth_test, texture_2d);
      //}
    }
  }

  glDrawArrays_Original (mode, first, count);

  // Technically we might have issued an extra draw above, but this stat is
  //   intended to reflect the APPLICATION's drawcall count, not the mod's.
  pp::RenderFix::draw_state.draws++;

  // Tracking whether or not the normal / position drawbuffers are dirty
  if (active_program == 33)
    program33_draws++;

  // Restore the original alpha function
  if (fringed)
    glAlphaFunc_Original (GL_GREATER, 0.01f);
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
  // Don't log the craziness that happens at shutdown...
  if (n != 67551) {
    dll_log.LogEx ( true, L"[ GL Trace ] glDeleteTextures (%lu, { ",
                      n );

    for (int i = 0; i < n; i++) {
      dll_log.LogEx ( false, L"%lu", textures [i]);

      if (i < n-1)
        dll_log.LogEx (false, L", ");
    }

    dll_log.LogEx (false, L" })\n");
  }

  for (int i = 0; i < n; i++) {
    if (ui_textures.find (textures [i]) != ui_textures.end ()) {
      //dll_log.Log (L"Deleted UI texture");
      ui_textures.erase (textures [i]);
    }

    msaa.deleteTexture (textures [i]);

    if (texture_checksums_rev.count (textures [i])) {
      texture_checksums_rev.erase (textures [i]);
    }

#if 0
    /// XXX: This probably is not necessary, we will just attach something new there anyway.
    if (textures [i] == pp::RenderFix::draw_state.depth_tex) {
      if (config.render.msaa_samples > 1) {
        GLuint fbo;
        glGetIntegerv                   (GL_FRAMEBUFFER_BINDING, (GLint *)&fbo);
        glBindFramebuffer_Original      (GL_FRAMEBUFFER, msaa.resolve_fbo);
        glFramebufferTexture2D_Original (GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, 0, 0);
        glBindFramebuffer_Original      (GL_FRAMEBUFFER, fbo);
      }

      pp::RenderFix::draw_state.depth_tex = 0;
    }
#endif
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

typedef GLvoid (WINAPI *glActiveTexture_pfn)(
  GLenum texture
);

glActiveTexture_pfn glActiveTexture_Original = nullptr;

__declspec (noinline)
GLvoid
WINAPI
glActiveTexture_Detour ( GLenum texture )
{
  active_texture = texture-GL_TEXTURE0;

  glActiveTexture_Original (texture);
}

__declspec (noinline)
GLvoid
WINAPI
glBindTexture_Detour ( GLenum target,
                       GLuint texture )
{
  if (texture == 0)
    return;//texture = 131072;

  if (target == GL_PROXY_TEXTURE_2D)
    dll_log.Log (L"Oh noes!");

  // TODO: Cubemaps and various others (rectangle for instance)
  if (target == GL_TEXTURE_2D)
    current_textures [active_texture].tex2d.name = texture;
  else if (target == GL_TEXTURE_1D)
    current_textures [active_texture].tex1d.name = texture;
  else if (target == GL_TEXTURE_RECTANGLE)
    current_textures [active_texture].texRect.name = texture;


  if (target == GL_TEXTURE_2D && config.render.msaa_samples > 1) {
    if (msaa.use)
      msaa.resolveTexture (texture);
  }


  return glBindTexture_Original (target, texture);
}

#define GL_PIXEL_UNPACK_BUFFER_BINDING 0x88EF

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
  bool depaletted = false;

  uint32_t checksum = 0x0UL;

  GLuint texId = current_textures [active_texture].tex2d.name;

  //dll_log.Log ( L"[GL Texture] Target: %X, InternalFmt: %X, Format: %X, Type: %X, (%lux%lu @ %lu), data: %ph  [texId: %lu, unit: %lu]",
                 //target, internalformat, format, type, width, height, level, data, texId, active_texture );

  if (data != nullptr) {
  if (internalformat == GL_RGBA) {
    const int imageSize = width * height * 4;
              checksum  = crc32 (0, data, imageSize);

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

          GLint texId = current_textures [active_texture].tex2d.name;
          ui_textures.insert (texId);

          glTexImage2D_Original ( target,
                                    level,
                                      internalformat,
                                        width, height,
                                          border,
                                            format,
                                              type,
                                                tex );

          PPrinny_TEX_GenerateMipmap (width, height);

          free (tex);
          return;
        }
      }
    }
  }

  if (format == GL_COLOR_INDEX) {
    // This sort of palette is easy, anything else will require a lot of work.
    if (type == GL_UNSIGNED_BYTE) {
      const int imageSize = width * height;
                checksum  = crc32 (0, data, imageSize);

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

        uint8_t* out_img = new uint8_t [width * height * 4];

        for (int i = 0, j = 0 ; j < width * height ; j++, i += 4) {
          out_img [ i + 2 ] = 255 * pixel_maps [2][((uint8_t *)data) [j]];
          out_img [ i + 1 ] = 255 * pixel_maps [3][((uint8_t *)data) [j]];
          out_img [ i     ] = 255 * pixel_maps [4][((uint8_t *)data) [j]];
          out_img [ i + 3 ] = 255 * pixel_maps [5][((uint8_t *)data) [j]];
        }

        checksum = crc32 (0, out_img, width * height * 4);

        if ( PPrinny_TEX_AvoidRedundantCPUtoGPU ( target,
                                                    GL_RGBA,
                                                      width,
                                                        height,
                                                          checksum,
                                                            texId, (void *)out_img )) {
          delete [] out_img;
//          dll_log.Log (L"[GL SpeedUp] Avoided redundant paletted texture upload...");
          // Happy day! We just skipped a lot of framerate problems.
          return;
        }
        //glPixelTransferi_Original (GL_MAP_COLOR, 0);

        glTexImage2D_Original (target, level, GL_RGBA, width, height, border, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, out_img);
        delete [] out_img;

        //PPrinny_TEX_RemoveTextureRecord (texId);

        GLuint cacheTexId;
        glGenTextures (1, &cacheTexId);

        //glBindTexture  (GL_TEXTURE_2D, cacheTexId);
        ////glTexImage2D_Original (GL_TEXTURE_2D, level, GL_RGBA, width, height, border, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        //glTexStorage2D (GL_TEXTURE_2D, 1/*log2 (max (width, height))*/, GL_RGBA, width, height);

        ////glCopyImageSubData ( texId,      GL_TEXTURE_2D, 0, 0, 0, 0,
                             ////cacheTexId, GL_TEXTURE_2D, 0, 0, 0, 0,
                             ////width, height, 0 );

        PPrinny_TEX_AvoidRedundantCPUtoGPU ( target,
                                                    GL_RGBA,
                                                      width,
                                                        height,
                                                          checksum,
                                                            cacheTexId, (void *)nullptr );

        glBindTexture  (GL_TEXTURE_2D, texId);

        //dll_log.Log (L"Depaletted!");

        ///////format = GL_RGBA;
        depaletted = true;
    } else {
      dll_log.Log ( L"[ Tex Dump ] Color Index Format: (%lux%lu), Data Type: 0x%X",
                      width, height, type );
    }
  }
  }


  // data == nullptr
  else {
    dll_log.Log ( L"[GL Texture] Id=%i, LOD: %i, (%ix%i)",
                    texId, level, width, height );

    dll_log.Log ( L"[GL Texture]  >> Render Target - "
                  L"Internal Format: %s",
                    PP_GL_InternalFormat_ToStr (internalformat).c_str () );

    if (internalformat == GL_RGBA16F) {
      internalformat = config.render.high_precision_ssao ? GL_RGB32F : GL_RGB16F;
      format         = GL_RGB;

      glTexParameteri_Original ( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT );
      glTexParameteri_Original ( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT );
    }

    // Avoid driver weirdness from using a non-color renderable format
    else if (internalformat == GL_LUMINANCE8) {
      internalformat = GL_R8;
      format         = GL_RED;

      glTexParameteri_Original ( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
      glTexParameteri_Original ( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );

      dll_log.Log ( L"[Compliance] Setting up swizzle mask (r,r,r,1) on "
                    L"GL_R8 instead of rendering to GL_LUMINANCE8" );

      GLint swizzleMeTimbers [] = { GL_RED, GL_RED, GL_RED,
                                    GL_ONE };
      glTexParameteriv ( GL_TEXTURE_2D,
                           GL_TEXTURE_SWIZZLE_RGBA,
                             swizzleMeTimbers );
    } else if (internalformat != GL_DEPTH_COMPONENT) {
      glTexParameteri_Original ( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT );
      glTexParameteri_Original ( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT );
    }

    if (texId >= render_targets.color && texId <= render_targets.position+1) {

      //pp::RenderFix::fullscreen = true;

      game_height = height;
      game_width  = width;

      if (game_height >= 720 && game_width >= 1280) {
        fb_width  = config.render.scene_res_x;
        fb_height = config.render.scene_res_y;
      } else {
        fb_width  = game_width;
        fb_height = game_height;
      }

      width  = fb_width;
      height = fb_height;

      if (config.render.msaa_samples > 1)
        msaa.createTexture (texId, internalformat, width, height);
    }
  }


  if ( data != nullptr ) {
    //dll_log.Log ( L"texId: %lu, crc32: %x, Level: %li, target: %x, border: %lu, data: %ph",
                   //texId, checksum, level, target, border, data );

    if ( format != GL_COLOR_INDEX &&
       PPrinny_TEX_AvoidRedundantCPUtoGPU ( target,
                                                internalformat,
                                                  width,
                                                    height,
                                                      checksum,
                                                        texId, (void *)data ) ) {
      if (pp::RenderFix::tracer.log)
        dll_log.Log (L"[GL SpeedUp] Avoided redundant texture upload...");
      // Happy day! We just skipped a lot of framerate problems.
      return;
    } //else if (format == GL_COLOR_INDEX) {
      //dll_log.Log (L"Removing texture because it is paletted");
      //PPrinny_TEX_RemoveTextureRecord (texId);
    //}
  }

  if (! depaletted) {
    glTexImage2D_Original ( target, level,
                              internalformat,
                                width, height,
                                  border,
                                    format, type, 
                                      data );

    if ( data != nullptr ) {
      GLuint cacheTexId;
      glGenTextures (1, &cacheTexId);

      PPrinny_TEX_AvoidRedundantCPUtoGPU ( target,
                                             internalformat,
                                               width,
                                                 height,
                                                   checksum,
                                                     cacheTexId, (void *)nullptr );

      glBindTexture (GL_TEXTURE_2D, texId);
    }
  }

  PPrinny_TEX_TestOpaque ( texId,
                           width,          height,
                           internalformat, format,
                           data );


  // Mipmap generation
  if (config.textures.force_mipmaps) {
    if ( data != nullptr && format != GL_COLOR_INDEX ) {
      PPrinny_TEX_GenerateMipmap (width, height);
      ////////dll_log.Log (L"[GL Texture] Generated mipmaps (on data upload) for Uncompressed TexID=%lu", texId);
    }

    if (format == GL_COLOR_INDEX) {
      // DO NOT mipmap paletted textures
      glTexParameteri_Original (GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
      glTexParameteri_Original (GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL,  0);
    }
  } else {
    glTexParameteri_Original (GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
    glTexParameteri_Original (GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL,  0);
  }


  // The game just re-allocated the depth texture, we need to attach the new texture for
  //   FBO completeness...
  if (texId == render_targets.depth) {
    GLuint draw_buffer;

    glGetIntegerv (GL_FRAMEBUFFER_BINDING, (GLint *)&draw_buffer);

    // Create the MSAA Resolve FBO if necessary
    if (config.render.msaa_samples > 1 && msaa.resolve_fbo == 0)
      glGenFramebuffers (1, &msaa.resolve_fbo);

    if (config.render.msaa_samples > 1)
      glBindFramebuffer_Original    ( GL_FRAMEBUFFER, msaa.resolve_fbo );
    else
      glBindFramebuffer_Original    ( GL_FRAMEBUFFER, 2                );

    glFramebufferTexture2D_Original ( GL_FRAMEBUFFER,
                                        GL_DEPTH_ATTACHMENT,
                                          GL_TEXTURE_2D,
                                            texId,
                                              0 );

    glBindFramebuffer_Original      ( GL_FRAMEBUFFER, draw_buffer      );
  }
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
    msaa.deleteRenderbuffer (renderbuffers [i]);
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

  if (width == game_width && height == game_height) {
    width = fb_width; height = fb_height;

    msaa.createRenderbuffer (internalformat, width, height);
  }

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
  if (pp::RenderFix::tracer.log) {
    dll_log.Log (L"[GL CallLog] -- glBlitFramebuffer (%li, %li, %li, %li, %li,"
                                                    L"%li, %li, %li, %li, %X)",
      srcX0, srcY0,
      srcX1, srcY1,
      dstX0, dstY0,
      dstX1, dstY1,
      mask, filter);
  }

#if 0
  if (srcX0 == 0    && srcY0 ==   0 &&
      srcX1 == 1280 && srcY1 == 720) {
    srcX1 = config.render.scene_res_x;
    srcY1 = config.render.scene_res_y;
  }
#endif

  return glBlitFramebuffer_Original (
    srcX0, srcY0,
    srcX1, srcY1,
    dstX0, dstY0,
    dstX1, dstY1,
    mask, filter
  );
}

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

  // Post-processing (which is actually pre-processing at this point)
  if (active_fbo == 2) {
    postprocessing = true;

BEGIN_TASK_TIMING
    if (GLPerf::timers [GLPerf::TASK_BEGIN_POSTPROC]->isReady ())
      GLPerf::timers [GLPerf::TASK_BEGIN_POSTPROC]->requestTimestamp ();
END_TASK_TIMING

    width  = fb_width;
    height = fb_height;

// For Depth of Field to Work
#if 1
    if (msaa.use)
      msaa.resolveTexture (render_targets.depth);
#endif

    // We don't need to make a copy, we attached this texture to the framebuffer
    //   already
    return;
  }

  // Shadow pass
  if (active_fbo == 1) {
#if 0
    glCopyImageSubData ( 1,                     GL_RENDERBUFFER, 0, x, y, 0,
                         render_targets.shadow, GL_TEXTURE_2D,   0, x, y, 0,
                         width, height, 1 );
    return;
#endif
  }

    //int internal_fmt;
    //glGetTexLevelParameteriv (target, level, GL_TEXTURE_INTERNAL_FORMAT, &internal_fmt);
    //dll_log.Log (L"[ GL Trace ] Destination internal format: 0x%X", internal_fmt);
    glCopyTexSubImage2D_Original ( target, level,
                                     xoffset, yoffset,
                                       x, y,
                                         width, height );
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
  if (pp::RenderFix::tracer.log)
    dll_log.Log ( L"[ GL Trace ] -- glViewport (%lu, %lu, %lu, %lu)",
                    x, y,
                      width, height );

  if (width == game_width && height == game_height) {
    width  = fb_width;
    height = fb_height;

    drawing_main_scene = true;
  } else {
    drawing_main_scene = false;
  }

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
glAttachShader_Detour ( GLuint program,
                        GLuint shader )
{
// Only Program #33 writes to normal / position
#if 0
  if (shader == 16)
    dll_log.Log ( L"[ GL MISC. ] Program %lu writes to gl_FragData [1] and gl_FragData [2]",
                    program );
#endif

  glAttachShader_Original (program, shader);
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

#if 0
  if (checksum == 0x12C7A66D) {
    dll_log.Log ( L"[ GL MISC. ] Shader %lu writes to gl_FragData [1] and gl_FragData [2]",
                    shader );
  }
#endif

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
      if (szSrc != nullptr) {
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
  }

  glShaderSource_Original (shader, count, (const GLubyte **)&szShaderSrc, length);

  free (szShaderSrc);
}

PROC
WINAPI
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

  if (! init_self){
    init_self = true;

    if (pp::GLPerf::Init ()) {
      GLPerf::timers [GLPerf::TASK_BEGIN_FRAME]    =
                  new GLPerf::TimerQuery (L"Frame Start");
      GLPerf::timers [GLPerf::TASK_BEGIN_SCENE]    =
                  new GLPerf::TimerQuery (L"Scene");
      GLPerf::timers [GLPerf::TASK_MSAA0]          =
                  new GLPerf::TimerQuery (L"MSAA Resolve0");
      GLPerf::timers [GLPerf::TASK_MSAA1]          =
                  new GLPerf::TimerQuery (L"MSAA Resolve1");
      GLPerf::timers [GLPerf::TASK_MSAA2]          =
                  new GLPerf::TimerQuery (L"MSAA Resolve2");
      GLPerf::timers [GLPerf::TASK_MSAA3]          =
                  new GLPerf::TimerQuery (L"MSAA Resolve3");
      GLPerf::timers [GLPerf::TASK_BEGIN_POSTPROC] =
                  new GLPerf::TimerQuery (L"Post-Processing Start");
      GLPerf::timers [GLPerf::TASK_END_POSTPROC]   =
                  new GLPerf::TimerQuery (L"Post-Processing End");
      GLPerf::timers [GLPerf::TASK_BEGIN_UI]       =
                  new GLPerf::TimerQuery (L"UI Start");
      GLPerf::timers [GLPerf::TASK_END_UI]         =
                  new GLPerf::TimerQuery (L"UI End");
      GLPerf::timers [GLPerf::TASK_END_FRAME]      =
                  new GLPerf::TimerQuery (L"Frame End");

      GLPerf::HAS_timer_query = true;
    }

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

    glDrawBuffers_Original =
      (glDrawBuffers_pfn)
        wglGetProcAddress_Original ("glDrawBuffers");

    glColorMaski =
      (glColorMaski_pfn)
        wglGetProcAddress_Original ("glColorMaski");

    glCheckFramebufferStatus =
      (glCheckFramebufferStatus_pfn)
        wglGetProcAddress_Original ("glCheckFramebufferStatus");

    glGenerateMipmap =
      (glGenerateMipmap_pfn)
        wglGetProcAddress_Original ("glGenerateMipmap");

    glCopyImageSubData = 
      (glCopyImageSubData_pfn)
        wglGetProcAddress_Original ("glCopyImageSubData");

    glTexStorage2D =
      (glTexStorage2D_pfn)
        wglGetProcAddress_Original ("glTexStorage2D");

    glCompressedTexSubImage2D =
      (glCompressedTexSubImage2D_pfn)
        wglGetProcAddress_Original ("glCompressedTexSubImage2D");

    glGenBuffers =
      (glGenBuffers_pfn)
        wglGetProcAddress_Original ("glGenBuffers");

    glBindBuffer =
      (glBindBuffer_pfn)
        wglGetProcAddress_Original ("glBindBuffer");

    glBufferData =
      (glBufferData_pfn)
        wglGetProcAddress_Original ("glBufferData");

    glGetBufferSubData =
      (glGetBufferSubData_pfn)
        wglGetProcAddress_Original ("glGetBufferSubData");


    glFenceSync =
      (glFenceSync_pfn)
        wglGetProcAddress ("glFenceSync");

    glClientWaitSync =
      (glClientWaitSync_pfn)
        wglGetProcAddress ("glClientWaitSync");

    glDeleteSync =
      (glDeleteSync_pfn)
        wglGetProcAddress ("glDeleteSync");
  }

  PROC ret = wglGetProcAddress_Original (szFuncName);

  bool detoured = false;

  if (! strcmp (szFuncName, "glUseProgram") && (ret != nullptr)) {
    glUseProgram_Original = (glUseProgram_pfn)ret;
    ret                   = (PROC)glUseProgram_Detour;
    detoured              = true;
  }

  else if (! strcmp (szFuncName, "glAttachShader") && (ret != nullptr)) {
    glAttachShader_Original = (glAttachShader_pfn)ret;
    ret                     = (PROC)glAttachShader_Detour;
    detoured                = true;
  }

  else if (! strcmp (szFuncName, "glShaderSource") && (ret != nullptr)) {
    glShaderSource_Original = (glShaderSource_pfn)ret;
    ret                     = (PROC)glShaderSource_Detour;
    detoured                = true;
  }

  else if (! strcmp (szFuncName, "glGetShaderiv") && (ret != nullptr)) {
    glGetShaderiv_Original = (glGetShaderiv_pfn)ret;
    ret                    = (PROC)glGetShaderiv_Detour;
    detoured               = true;
  }

  else if (! strcmp (szFuncName, "glVertexAttrib4f") && (ret != nullptr)) {
    glVertexAttrib4f_Original = (glVertexAttrib4f_pfn)ret;
    ret                       = (PROC)glVertexAttrib4f_Detour;
    detoured                  = true;
  }

  else if (! strcmp (szFuncName, "glVertexAttrib4fv") && (ret != nullptr)) {
    glVertexAttrib4fv_Original = (glVertexAttrib4fv_pfn)ret;
    ret                        = (PROC)glVertexAttrib4fv_Detour;
    detoured                   = true;
  }

  else if (! strcmp (szFuncName, "glActiveTexture") && (ret != nullptr)) {
    glActiveTexture_Original = (glActiveTexture_pfn)ret;
    ret                      = (PROC)glActiveTexture_Detour;
    detoured                 = true;
  }

  else if (! strcmp (szFuncName, "glCompressedTexImage2D") && (ret != nullptr)) {
    glCompressedTexImage2D_Original = (glCompressedTexImage2D_pfn)ret;
    ret                             = (PROC)glCompressedTexImage2D_Detour;
    detoured                        = true;
  }

#if 0
  else if (! strcmp (szFuncName, "glUniformMatrix3fv") && (ret != nullptr)) {
    glUniformMatrix3fv_Original = (glUniformMatrix3fv_pfn)ret;
    ret                         = (PROC)glUniformMatrix3fv_Detour;
    detoured                    = true;
  }

  else if (! strcmp (szFuncName, "glUniformMatrix4fv") && (ret != nullptr)) {
    glUniformMatrix4fv_Original = (glUniformMatrix4fv_pfn)ret;
    ret                         = (PROC)glUniformMatrix4fv_Detour;
    detoured                    = true;
  }
#endif

  else if (! strcmp (szFuncName, "glDrawBuffers") && (ret != nullptr)) {
    glDrawBuffers_Original = (glDrawBuffers_pfn)ret;
    ret                    = (PROC)glDrawBuffers_Detour;
    detoured               = true;
  }

  else if (! strcmp (szFuncName, "glBindFramebuffer") && (ret != nullptr)) {
    glBindFramebuffer_Original = (glBindFramebuffer_pfn)ret;
    ret                        = (PROC)glBindFramebuffer_Detour;
    detoured                   = true;
  }

  else if (! strcmp (szFuncName, "glFramebufferTexture2D") && (ret != nullptr)) {
    glFramebufferTexture2D_Original = (glFramebufferTexture2D_pfn)ret;
    ret                             = (PROC)glFramebufferTexture2D_Detour;
    detoured                        = true;
  }

  else if (! strcmp (szFuncName, "glFramebufferRenderbuffer") && (ret != nullptr)) {
    glFramebufferRenderbuffer_Original = (glFramebufferRenderbuffer_pfn)ret;
    ret                                = (PROC)glFramebufferRenderbuffer_Detour;
    detoured                           = true;
  }

  else if (! strcmp (szFuncName, "glBlitFramebuffer") && (ret != nullptr)) {
    glBlitFramebuffer_Original = (glBlitFramebuffer_pfn)ret;
    ret                        = (PROC)glBlitFramebuffer_Detour;
    detoured                   = true;
  }

  else if (! strcmp (szFuncName, "glDeleteRenderbuffers") && (ret != nullptr)) {
    glDeleteRenderbuffers_Original = (glDeleteRenderbuffers_pfn)ret;
    ret                            = (PROC)glDeleteRenderbuffers_Detour;
    detoured                       = true;
  }

  else if (! strcmp (szFuncName, "glRenderbufferStorage") && (ret != nullptr)) {
    glRenderbufferStorage_Original = (glRenderbufferStorage_pfn)ret;
    ret                            = (PROC)glRenderbufferStorage_Detour;
    detoured                       = true;
  }

  else if (! strcmp (szFuncName, "wglSwapIntervalEXT") && (ret != nullptr)) {
    wglSwapIntervalEXT_Original = (wglSwapIntervalEXT_pfn)ret;
    ret                         = (PROC)wglSwapIntervalEXT_Detour;
    detoured                    = true;
  }

  else if (! strcmp (szFuncName, "glDebugMessageCallbackARB") && ret != nullptr) {
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

wglGetProcAddress_pfn wglGetProcAddress_Log = wglGetProcAddress_Detour;

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


GLvoid
WINAPI
glClear_Detour (GLbitfield mask)
{
  //if (active_fbo == 1)
  mask &= ~(GL_STENCIL_BUFFER_BIT);

  glClear_Original (mask);
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

#ifdef RESPONSIBLE_CTX_MGMT
__declspec (thread) HDC   current_dc;
__declspec (thread) HGLRC current_glrc;
#else
HDC   current_dc;
HGLRC current_glrc;
#endif

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

#ifdef RESPONSIBLE_CTX_MGMT
  if (current_dc != hDC/* || current_glrc != hGLRC*/) {
#else
  if (current_dc != hDC || current_glrc != hGLRC) {
#endif
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
                              "glPixelMapfv",
                              glPixelMapfv_Detour,
                   (LPVOID *)&glPixelMapfv_Original );

      PPrinny_CreateDLLHook ( config.system.injector.c_str (),
                              "glPixelTransferi",
                              glPixelTransferi_Detour,
                   (LPVOID *)&glPixelTransferi_Original );

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
                              "glClear",
                              glClear_Detour,
                   (LPVOID *)&glClear_Original );

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

      PPrinny_CreateDLLHook ( config.system.injector.c_str (),
                              "glDepthMask",
                               glDepthMask_Detour,
                    (LPVOID *)&glDepthMask_Original );

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

  pCommandProc->AddVariable ("Render.AggressiveOpt",    new eTB_VarStub <bool> (&aggressive_optimization));
  pCommandProc->AddVariable ("Render.TaskTiming",       new eTB_VarStub <bool> (&GLPerf::time_tasks));
  pCommandProc->AddVariable ("Render.FastTexUploads",   new eTB_VarStub <bool> (&config.textures.caching));
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

// NOT IMPLEMENTED IN THIS GAME (Yet?)
#if 0
  pCommandProc->AddVariable ( "Render.AllowBG",
                                new eTB_VarStub <bool> (
                                  &config.render.allow_background
                                )
                            );
#endif

  high_precision_ssao =
    new eTB_VarStub <bool> (&config.render.high_precision_ssao, this);

  pCommandProc->AddVariable ("Render.HighPrecisionSSAO", high_precision_ssao);
}

bool
pp::RenderFix::CommandProcessor::OnVarChange (eTB_Variable* var, void* val)
{
  if (var == high_precision_ssao) {
    if (val != nullptr) {
      bool PP_ChangePositionFormat (GLenum format);

      if (PP_ChangePositionFormat (*(bool *)val ? GL_RGB32F : GL_RGB16F)) {
        config.render.high_precision_ssao = *(bool *)val;
      }
    } else {
      return config.render.high_precision_ssao;
    }
  }

  return true;
}

pp::RenderFix::CommandProcessor*
           pp::RenderFix::CommandProcessor::pCommProc = nullptr;

eTB_VarStub <bool>*
           pp::RenderFix::high_precision_ssao         = nullptr;

HWND     pp::RenderFix::hWndDevice       = NULL;

bool     pp::RenderFix::fullscreen       = false;
uint32_t pp::RenderFix::width            = 0UL;
uint32_t pp::RenderFix::height           = 0UL;

HMODULE  pp::RenderFix::user32_dll       = 0;







bool
MSAABackend::createTexture ( GLuint  texId,
                             GLenum  internalformat,
                             GLsizei width,
                             GLsizei height )
{
  if (config.render.msaa_samples < 2)
    return false;

  GLint original_rbo;
  glGetIntegerv (GL_RENDERBUFFER_BINDING, &original_rbo);

  std::unordered_map <GLuint, msaa_ref_s*>::iterator it =
    backing_textures.find (texId);

  if (it != backing_textures.end ()) {
    // Renderbuffer "Orphaning" (reconfigure existing datastore)
    glBindRenderbuffer               (GL_RENDERBUFFER, it->second->rbo);
    glRenderbufferStorageMultisample (GL_RENDERBUFFER, config.render.msaa_samples,
                                        internalformat, width, height);
    glBindRenderbuffer               (GL_RENDERBUFFER, original_rbo);
  } else {
    GLuint rbo;
    glGenRenderbuffers               (1, &rbo);
    glBindRenderbuffer               (GL_RENDERBUFFER, rbo);
    glRenderbufferStorageMultisample (GL_RENDERBUFFER, config.render.msaa_samples, 
                                        internalformat, width, height);

    msaa_ref_s* pRef = new msaa_ref_s;
    pRef->rbo        = rbo;
    pRef->parent_fbo = -1; // Not attached to anything yet

    backing_textures.insert (std::pair <GLuint, msaa_ref_s*> (texId, pRef));

    glBindRenderbuffer (GL_RENDERBUFFER, original_rbo);
  }

  return true;
}

bool
MSAABackend::deleteTexture (GLuint texId)
{
  if (backing_textures.find (texId) != backing_textures.end ()) {
    dll_log.Log ( L"[   MSAA   ] Game deleted an MSAA backed Texture, "
                  L"cleaning up... (TexID=%lu, RBO=%lu)",
                    texId, backing_textures [texId]->rbo );

    glDeleteRenderbuffers_Original (1, &backing_textures [texId]->rbo);

    backing_textures.erase (texId);

    return true;
  }

  return false;
}

bool
MSAABackend::attachTexture ( GLenum target,    GLenum attachment,
                             GLuint texture,   GLenum textarget,
                             GLuint level )
{
  GLuint fbo;
  glGetIntegerv (GL_FRAMEBUFFER_BINDING, (GLint *)&fbo);

  std::unordered_map <GLuint, msaa_ref_s*>::iterator it =
    backing_textures.find (texture);

  if (fbo == 2) {// && it != backing_textures.end ()) {
    if (resolve_fbo == 0)
      glGenFramebuffers (1, &resolve_fbo);

    if (it == backing_textures.end ()) {
      dll_log.Log ( L"[   MSAA   ] No backing Texture exists for attachment "
                    L"(TexId=%lu, Attachment=0x%X, Target=0x%X)",
                      texture, attachment, target );
      return false;
    }

    it->second->parent_fbo = fbo;
    it->second->color_loc  = (attachment-GL_COLOR_ATTACHMENT0) < 16 ?
                             (attachment-GL_COLOR_ATTACHMENT0) : -1;
    it->second->dirty      = true;

    dll_log.Log ( L"[   MSAA   ] Attaching Texture (id=%5lu) to FBO "
                               L"(id=%lu) via %s =>"
                               L" Proxy RBO (id=%lu, %lu-sample)",
                    texture,
                      fbo,
                        PP_FBO_AttachPoint_ToStr (attachment).c_str (),
                          it->second->rbo,
                            config.render.msaa_samples );

    glFramebufferRenderbuffer_Original ( target,
                                           attachment,
                                             GL_RENDERBUFFER,
                                               it->second->rbo );

    glBindFramebuffer_Original (GL_FRAMEBUFFER, resolve_fbo);

    glFramebufferTexture2D_Original ( target,
                                        attachment,
                                          textarget,
                                            texture,
                                              level );

    glBindFramebuffer_Original (GL_FRAMEBUFFER, fbo);

    return true;
  }

  else if (it != backing_textures.end ()) {
    // Remove this RBO, we only want to multisample stuff attached to FBO #2
    glDeleteRenderbuffers_Original (1, &it->second->rbo);
    delete it->second;
    backing_textures.erase         (it);
  }

  return false;
}

bool
MSAABackend::createRenderbuffer ( GLenum  internalformat,
                                  GLsizei width, GLsizei height )
{
  if (config.render.msaa_samples < 2)
    return false;

  GLuint rboId;
  glGetIntegerv (GL_RENDERBUFFER_BINDING, (GLint *)&rboId);

  std::unordered_map <GLuint, msaa_ref_s*>::iterator it =
    backing_rbos.find (rboId);

  if (it != backing_rbos.end ()) {
    // Renderbuffer "Orphaning" (reconfigure existing datastore)
    glBindRenderbuffer               (GL_RENDERBUFFER, it->second->rbo);
    glRenderbufferStorageMultisample (GL_RENDERBUFFER, config.render.msaa_samples,
                                        internalformat, width, height);
    glBindRenderbuffer               (GL_RENDERBUFFER, rboId);
  } else {
    GLuint rbo;
    glGenRenderbuffers               (1, &rbo);
    glBindRenderbuffer               (GL_RENDERBUFFER, rbo);
    glRenderbufferStorageMultisample (GL_RENDERBUFFER, config.render.msaa_samples,
                                        internalformat, width, height);

    msaa_ref_s* pRef = new msaa_ref_s;
    pRef->rbo        = rbo;
    pRef->parent_fbo = -1;  // Not attached to anything yet

    backing_rbos.insert (std::pair <GLuint, msaa_ref_s*> (rboId, pRef));

    glBindRenderbuffer (GL_RENDERBUFFER, rboId);
  }

  return true;
}

bool
MSAABackend::deleteRenderbuffer (GLuint rboId)
{
  if (config.render.msaa_samples < 2)
    return false;

  if (backing_rbos.find (rboId) != backing_rbos.end ()) {
    dll_log.Log ( L"[   MSAA   ] Game deleted an MSAA backed RBuffer, "
                               L"cleaning up... (RBO_ss=%lu, RBO_ms=%lu)",
                    rboId,
                      backing_rbos [rboId]->rbo );

    glDeleteRenderbuffers_Original (
                1,
                  &backing_rbos [rboId]->rbo
    );

    backing_rbos.erase (rboId);

    return true;
  }

  return false;
}

bool
MSAABackend::attachRenderbuffer (GLenum target, GLenum attachment, GLuint renderbuffer)
{
  GLuint fbo;
  glGetIntegerv (GL_FRAMEBUFFER_BINDING, (GLint *)&fbo);

  std::unordered_map <GLuint, msaa_ref_s*>::iterator it =
    backing_rbos.find (renderbuffer);


  if (fbo == 2) {//it != backing_rbos.end ()) {
    if (resolve_fbo == 0)
      glGenFramebuffers (1, &resolve_fbo);

    if (it == backing_rbos.end ()) {
      dll_log.Log ( L"[   MSAA   ] No backing RBuffer exists for attachment "
                    L"(RboId=%5lu, Attachment=0x%X, Target=0x%X)",
                      renderbuffer, attachment, target );
      return false;
    }

    it->second->parent_fbo = fbo;
    it->second->color_loc  = (attachment-GL_COLOR_ATTACHMENT0) < 16 ?
                              (attachment-GL_COLOR_ATTACHMENT0) : -1;
    it->second->dirty      = true;

    dll_log.Log ( L"[   MSAA   ] Attaching RBuffer (id=%5lu) to FBO "
                               L"(id=%lu) via %s",
                    it->second->rbo,
                      fbo,
                        PP_FBO_AttachPoint_ToStr (attachment).c_str () );

    glFramebufferRenderbuffer_Original ( target,
                                           attachment,
                                             GL_RENDERBUFFER,
                                               it->second->rbo );

    glBindFramebuffer_Original (GL_DRAW_FRAMEBUFFER, resolve_fbo);

    // Only attach an RBO if it's a color buffer, we will use a depth texture
    //   otherwise to save a copy operation
    if (it->second->color_loc != -1) {
      glFramebufferRenderbuffer_Original ( target,
                                             attachment,
                                               GL_RENDERBUFFER,
                                                 renderbuffer );
    } else {
      glFramebufferTexture2D_Original ( target,
                                          attachment,
                                            GL_TEXTURE_2D,
                                              render_targets.depth,
                                                0 );
    }

    glBindFramebuffer_Original (GL_DRAW_FRAMEBUFFER, fbo);

    return true;
  }

  else if (it != backing_rbos.end ()) {
    // Remove this RBO, we only want to multisample stuff attached to FBO #2
    glDeleteRenderbuffers_Original (1, &it->second->rbo);
    delete it->second;
    backing_rbos.erase             (it);
  }

  return false;
}

bool
PP_ChangePositionFormat (GLenum format)
{
  if (render_targets.position != GL_NONE) {
    GLuint texId = current_textures [active_texture].tex2d.name;

    glBindTexture (GL_TEXTURE_2D, render_targets.position);

    GLint width, height;
    glGetTexLevelParameteriv (GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH,  &width);
    glGetTexLevelParameteriv (GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &height);

    if (config.render.msaa_samples > 1)
      msaa.createTexture (render_targets.position, format, width, height);

    // This will orphan the old data store after any queued frames finish
    glTexImage2D_Original ( GL_TEXTURE_2D,
                              0,
                                format,
                                  width, height,
                                    0,
                                      GL_RGB, GL_FLOAT,
                                        nullptr );

    glBindTexture (GL_TEXTURE_2D, texId);

    return true;
  }

  return false;
}