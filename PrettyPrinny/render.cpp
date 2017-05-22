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
#include "render/textures.h"

#include <cstdint>
#include <memory>

//#define GL_PALETTES

#define RESPONSIBLE_CTX_MGMT
#ifdef RESPONSIBLE_CTX_MGMT
HDC   game_dc   = 0;
HGLRC game_glrc = 0;

__declspec (thread) HDC   current_dc;
__declspec (thread) HGLRC current_glrc;
#else
HDC   current_dc;
HGLRC current_glrc;
#endif


//
// Optimizations that may turn out to break stuff
//
int  program33_draws    = 0;
bool drawing_main_scene = false;

using namespace pp::RenderFix;

tracer_s         pp::RenderFix::tracer;
pp_draw_states_s pp::RenderFix::draw_state;
pp_framebuffer_s pp::RenderFix::framebuffer;

struct known_glsl_programs_s {
  struct {
    std::unordered_set <GLuint> programs;
    std::unordered_set <GLuint> shaders;
  } ssao;

  struct {
    std::unordered_set <GLuint> programs;
    std::unordered_set <GLuint> shaders;
  } shadow;
} static known_glsl;

#define PP_DEPTH_FORMAT       GL_DEPTH_COMPONENT/*24*/

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
glPixelTransferi_pfn                 glPixelTransferi_Original          = nullptr;
glPixelMapfv_pfn                     glPixelMapfv_Original              = nullptr;
glTexStorage2D_pfn                   glTexStorage2D                     = nullptr;
glGenerateMipmap_pfn                 glGenerateMipmap                   = nullptr;


// Fast-path to avoid redundant CPU->GPU data transfers (there are A LOT of them)
glCopyImageSubData_pfn               glCopyImageSubData                 = nullptr;


glReadPixels_pfn                     glReadPixels_Original              = nullptr;

glGetFloatv_pfn                      glGetFloatv_Original               = nullptr;
glGetIntegerv_pfn                    glGetIntegerv_Original             = nullptr;

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

glGenBuffers_pfn                     glGenBuffers                       = nullptr;
glBindBuffer_pfn                     glBindBuffer                       = nullptr;
glDeleteBuffers_pfn                  glDeleteBuffers                    = nullptr;
glBufferData_pfn                     glBufferData                       = nullptr;
glGetBufferSubData_pfn               glGetBufferSubData                 = nullptr;

glFenceSync_pfn                      glFenceSync                        = nullptr;
glClientWaitSync_pfn                 glClientWaitSync                   = nullptr;
glDeleteSync_pfn                     glDeleteSync                       = nullptr;

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

SK_BeginBufferSwap_pfn               SK_BeginBufferSwap                = nullptr;
SK_EndBufferSwap_pfn                 SK_EndBufferSwap                  = nullptr;

glClear_pfn                          glClear_Original                   = nullptr;

typedef GLvoid (WINAPI *glMemoryBarrier_pfn)(
  GLbitfield barriers
);

glMemoryBarrier_pfn                  glMemoryBarrier                    = nullptr;

#define GL_TEXTURE_UPDATE_BARRIER_BIT 0x0100
#define GL_FRAMEBUFFER_BARRIER_BIT    0x0400


#include <map>
#include <unordered_set>

bool deferred_msaa = false;

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

std::wstring
PP_GL_InternalFormat_ToStr (GLenum format)
{
  switch (format)
  {
    case GL_DEPTH_COMPONENT:
      return L"Depth     (Unsized)";

    case GL_RGB:
      return L"RGB       (Unsized UNORM)";

    case GL_RGBA:
      return L"RGBA      (Unsized UNORM)";

    case GL_RGBA8:
      return L"RGBA      (8-bit UNORM)";

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

// -------------------- Texture Management ---------------------//

GLfloat  pixel_maps [10][256];
uint32_t pixel_map_crc32 = 0x0UL;

pp_texture_state_s
  current_textures [32] =
  { { 0, 0 },
    { 0, 0 },
    { 0, 0 },
    { 0, 0 } };

GLuint                      active_texture = GL_NONE;
std::unordered_set <GLuint> thirdparty_texids;


#include <memory>

std::unique_ptr <uint8_t*>
PPrinny_TEX_ConvertPalettedToBGRA ( GLsizei  width,
                                    GLsizei  height,
                                    GLfloat* palette,
                              const GLvoid*  data )
{
  uint8_t* out_img =
    new uint8_t [width * height * 4];

  for (int i = 0, j = 0 ; j < width * height ; j++, i += 4) {
    uint8_t entry = ((uint8_t *)data) [j];

#if 0 // Range restriction is unnecessary
    out_img [ i + 2 ] = max (0, min (255, 255 * palette [000 + entry]));
    out_img [ i + 1 ] = max (0, min (255, 255 * palette [256 + entry]));
    out_img [ i     ] = max (0, min (255, 255 * palette [512 + entry]));
    out_img [ i + 3 ] = max (0, min (255, 255 * palette [768 + entry]));
#else
    const bool premultiplied = false;

    float alpha = premultiplied ? palette [768 + entry] : 1.0f;

    out_img [ i + 2 ] = 255 * palette [000 + entry] * alpha;
    out_img [ i + 1 ] = 255 * palette [256 + entry] * alpha;
    out_img [ i     ] = 255 * palette [512 + entry] * alpha;
    out_img [ i + 3 ] = 255 * palette [768 + entry];

#endif
  }

  return std::make_unique <uint8_t*> (out_img);
}

struct texture_remapping_s {
  GLuint getRemapped (GLuint texId) {
    if (thirdparty_texids.count (texId))
      return texId;

    std::unordered_map <GLuint, GLuint>::iterator it =
      remaps.find (texId);

    if (remaps.find (texId) != remaps.end ())
      return remaps [texId];

    return texId;
  }

  GLuint findPermanentTexByChecksum (uint32_t checksum) {
    if (checksums.find (checksum) != checksums.end ()) {
      return checksums [checksum];
    }

    else
      return (1 << 31);
  }

  uint32_t getChecksumForPermanentTex (GLuint texId) {
    if (checksums_rev.find (texId) != checksums_rev.end ()) {
      return checksums_rev [texId];
    }

    else
      return 0x00UL;
  }

  void addChecksum (GLuint texId, uint32_t checksum) {
    checksums.insert     (std::make_pair (checksum, texId));
    checksums_rev.insert (std::make_pair (texId, checksum));
  }

  void addRemapping (GLuint texId_GAME, GLuint texId_GL) {
    if ( remaps.find (texId_GAME) != remaps.end () ) {
      if (remaps [texId_GAME] != texId_GL) {
        remaps.erase  (texId_GAME);
        remaps.insert (std::make_pair (texId_GAME,texId_GL));
      }

      return;
    }

    remaps.insert (std::make_pair (texId_GAME,texId_GL));
  }

  void removeRemapping (GLuint texId_GAME, GLuint texID_GL) {
    remaps.erase (texId_GAME);
  }

  std::unordered_map <GLuint,   GLuint> remaps;

  std::unordered_map <uint32_t, GLuint> checksums;
  std::unordered_map <GLuint, uint32_t> checksums_rev;
} texture_remap;

void
PPrinny_TEX_DisableMipmapping (void)
{
  glTexParameteri_Original (GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
  glTexParameteri_Original (GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL,  0);
}

void
PPrinny_TEX_GenerateMipmap (GLsizei width, GLsizei height)
{
#if 1
  //glTexParameteri ( GL_TEXTURE_2D,
                      //GL_TEXTURE_MIN_LOD,
                        //0 );
  glTexParameteri ( GL_TEXTURE_2D,
                      GL_TEXTURE_MAX_LEVEL,
                        max (0, (int)log2 (max (width,height) - 1)));

  glGenerateMipmap ( GL_TEXTURE_2D );

  if (config.textures.pixelate > 2) {
    int lod_bias = config.textures.pixelate - 1;

    glTexParameterf ( GL_TEXTURE_2D,
                        GL_TEXTURE_LOD_BIAS,
                          (float)lod_bias );
  }

  if ( /*active_texture == 1 ||*/
         thirdparty_texids.count (
           current_textures [active_texture].tex2d.name ) ) {
    PPrinny_TEX_DisableMipmapping ();
  }
#else
  PPrinny_TEX_DisableMipmapping ();
#endif
}

std::unordered_set <GLuint> known_textures;

void
PPrinny_TEX_CopyTexState ( GLuint srcTex,
                           GLuint dstTex )
{
  GLint num_levels;
  GLint states [4];

  glBindTexture_Original (GL_TEXTURE_2D, dstTex);//srcTex);

  for (int i = 0; i < 4; i++)
    glGetTexParameteriv (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER+i, &states [i]);

  glGetTexParameteriv (GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, &num_levels);

  glBindTexture_Original (GL_TEXTURE_2D, dstTex);

  for (int i = 0; i < 4; i++)
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER+i, states [i]);

  if (num_levels != 1000)
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, num_levels);
  else
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
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
  if (target != GL_TEXTURE_2D)
    dll_log.Log (L"Uh oh! (texid=%lu, target=%x)", target, texId);

  if (! config.textures.caching)
    return false;

  //
  // We only want the textures created by the game
  //
  if (texId > 67551 || texId == 0) {
    dll_log.Log (L"[GL SpeedUp] Will not cache texture %lu...", texId);
    return false;
  }

  known_textures.insert (texId);

  if (texture_remap.getRemapped (texId) != texId) {
    if ( texture_remap.getChecksumForPermanentTex (
           texture_remap.getRemapped ( texId )
         ) == checksum && checksum != 0x00UL ) {
      //dll_log.Log ( L"[GL SpeedUp] 100%% Redundant Texture Upload Eliminated"
                                //L" (texId=%lu)",
                      //texId );
      return true;
    }
    else {
      if ( texture_remap.findPermanentTexByChecksum (checksum) != (1 << 31) ) {
        GLuint tex = texture_remap.findPermanentTexByChecksum (checksum);
        glBindTexture_Original (GL_TEXTURE_2D, tex);
        texture_remap.addRemapping (texId, tex);

        //dll_log.Log ( L"[GL SpeedUp] Cached Texture Upload Eliminated"
                                //L" (texId=%lu)",
                      //texId );
        return true;
      }
    }
  } else { /*dll_log.Log (L"[GL SpeedUp] Texture (%lu) remaps to itself?! <crc32: %x> format: %x, (%lux%lux), data: %ph",
                        texId, checksum, internalFormat, width, height, data_addr); */}

  return false;
}

void
PPrinny_TEX_UploadTextureImage2D ( GLenum  target,
                                   GLint   level,
                                   GLenum  internalFormat,
                                   GLsizei width,
                                   GLsizei height,
                                   GLint   border,
                                   void*   data      = 0,
                                   GLenum  format    = GL_BGRA,
                                   GLenum  type      = GL_UNSIGNED_INT_8_8_8_8_REV,
                                   GLsizei imageSize = 0 )
{
  // I am almost positive that no code should intentionally be
  //   attempting to do this.
  if (target == GL_TEXTURE_2D && current_textures [active_texture].tex2d.name == 0) {
    dll_log.Log ( L"[GL Texture] Ignoring request to modify GL's default texture <"
                  L"glCompressedTexImage2D (...)>" );
    return;
  }

  uint32_t checksum = 0x00UL;

  if (imageSize != 0 && data != nullptr)
    checksum = crc32 (0, data, imageSize);

  GLuint texId = current_textures [active_texture].tex2d.name;

  //dll_log.Log ( L"texId: %lu, crc32: %x, Level: %li, target: %x, border: %lu, data: %ph",
                  //texId, checksum, level, target, border, data );

#define GL_COMPRESSED_RGB_S3TC_DXT1_EXT                   0x83F0
#define GL_COMPRESSED_RGBA_S3TC_DXT1_EXT                  0x83F1
#define GL_COMPRESSED_RGBA_S3TC_DXT3_EXT                  0x83F2
#define GL_COMPRESSED_RGBA_S3TC_DXT5_EXT                  0x83F3

  //
  // COMPRESSED TEXTURES
  //
  if ( internalFormat >= GL_COMPRESSED_RGB_S3TC_DXT1_EXT &&
       internalFormat <= GL_COMPRESSED_RGBA_S3TC_DXT5_EXT ) {
    if ( config.textures.caching &&
         PPrinny_TEX_AvoidRedundantCPUtoGPU ( target,
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

    GLuint cacheTexId;

    if (config.textures.caching) {
      glGenTextures            (1, &cacheTexId);
      PPrinny_TEX_CopyTexState (texId, cacheTexId);
      //glBindTexture_Original (GL_TEXTURE_2D, cacheTexId);
    }

    PPrinny_TEX_DisableMipmapping ();

    glCompressedTexImage2D_Original (
      target, level,
        internalFormat,
          width, height,
            border,
              imageSize, data );

    if (config.textures.caching) {
      texture_remap.addChecksum  (cacheTexId, checksum);
      texture_remap.addRemapping (texId, cacheTexId);
    }

#if 0
    dll_log.Log ( L"[GL Texture] Loaded Compressed Texture: Level=%li, (%lix%li) {%5.2f KiB}",
                    level, width, height, (float)imageSize / 1024.0f );
#endif

  }


  //
  // UNCOMPRESSED TEXTURES
  //
  else {
    bool depaletted = false;

    //dll_log.Log ( L"[GL Texture] Target: %X, InternalFmt: %X, Format: %X, Type: %X, (%lux%lu @ %lu), data: %ph  [texId: %lu, unit: %lu]",
                   //target, internalformat, format, type, width, height, level, data, texId, active_texture );

    if (data != nullptr) {
      if (internalFormat == GL_RGBA) {
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
        if (checksum == 0x7BE1DE43 || checksum == 0x95DBEDA5) {
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

              glTexImage2D_Original ( target,
                                        level,
                                          internalFormat,
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
            PPrinny_DumpTextureAndPalette ( checksum, pixel_map_crc32,
                                  (GLfloat *)&pixel_maps [2],
                                               width, height,
                                                 data );
          }

#ifndef GL_PALETTES
          checksum = crc32 (pixel_map_crc32, data, imageSize);
        //^^^^^^^^
        //
        //   We checksummed the palette earlier, to avoid having to run through the process of
        //     converting from palette -> BGRA just to find out if the image is already in VRAM.
        //
          if ( config.textures.caching &&
               PPrinny_TEX_AvoidRedundantCPUtoGPU ( target,
                                                      GL_RGBA,
                                                        width,
                                                          height,
                                                            checksum,
                                                              texId, nullptr )) {
            if (pp::RenderFix::tracer.log)
              dll_log.Log (L"[GL SpeedUp] Avoided redundant paletted texture upload...");
              // Happy day! We just skipped a lot of framerate problems.
            return;
          }

          auto out_img =
            PPrinny_TEX_ConvertPalettedToBGRA ( width, height,
                                      (GLfloat *)&pixel_maps [2],
                                                   data );
          GLuint cacheTexId;

          if (config.textures.caching) {
            glGenTextures            (1, &cacheTexId);
            PPrinny_TEX_CopyTexState (texId, cacheTexId);
            //glBindTexture_Original (GL_TEXTURE_2D, cacheTexId);
          }

          PPrinny_TEX_DisableMipmapping ();

          glTexImage2D_Original ( target,
                                    level,
                                      GL_RGBA,
                                        width,
                                          height,
                                            border,
                                              GL_BGRA,
                                                GL_UNSIGNED_INT_8_8_8_8_REV,
                                                  *out_img );


          if (config.textures.caching) {
            texture_remap.addChecksum  (cacheTexId, checksum);
            texture_remap.addRemapping (texId, cacheTexId);
          }

          depaletted = true;
#endif
        } else {
          dll_log.Log ( L"[ Tex Dump ] Color Index Format: (%lux%lu), Data Type: 0x%X",
                          width, height, type );
        }
      }
    }

    // data == nullptr
    else {
      PPrinny_TEX_DisableMipmapping ();

      dll_log.Log ( L"[GL Texture] Id=%i, LOD: %i, (%ix%i)",
                      texId, level, width, height );

      dll_log.Log ( L"[GL Texture]  >> Render Target - "
                    L"Internal Format: %s",
                      PP_GL_InternalFormat_ToStr (internalFormat).c_str () );

      if (internalFormat == GL_RGBA16F) {
        internalFormat = config.render.high_precision_ssao ? GL_RGB32F : GL_RGB16F;
        format         = GL_RGB;

        glTexParameteri_Original ( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
        glTexParameteri_Original ( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
      } else if (internalFormat == GL_RGBA) {
        glTexParameteri_Original ( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
        glTexParameteri_Original ( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
      }

      // Avoid driver weirdness from using a non-color renderable format
      else if (internalFormat == GL_LUMINANCE8) {
        internalFormat = GL_R8;
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
      } else if (internalFormat == GL_DEPTH_COMPONENT) {
        internalFormat = PP_DEPTH_FORMAT;
        glTexParameteri_Original ( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
        glTexParameteri_Original ( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
      }

      if (texId >= render_targets.color && texId <= render_targets.position+1) {

        //pp::RenderFix::fullscreen = true;


        framebuffer.game_height = height;
        framebuffer.game_width  = width;

        if (framebuffer.game_height >= 720 && framebuffer.game_width >= 1280) {
          framebuffer.real_width  = config.render.scene_res_x;
          framebuffer.real_height = config.render.scene_res_y;
        } else {
          framebuffer.real_width  = framebuffer.game_width;
          framebuffer.real_height = framebuffer.game_height;
        }

        width  = framebuffer.real_width;
        height = framebuffer.real_height;

        if (texId == render_targets.normal)
          internalFormat = GL_RGB10_A2;

        if (config.render.msaa_samples > 1)
          msaa.createTexture (texId, internalFormat, width, height);
      }
    }

    if (! depaletted) {
      GLuint cacheTexId;

      if ( config.textures.caching      &&
           data != nullptr              &&
           format != GL_DEPTH_COMPONENT &&
           PPrinny_TEX_AvoidRedundantCPUtoGPU ( target,
                                                  internalFormat,
                                                    width,
                                                      height,
                                                        checksum,
                                                            texId, nullptr )) {
          //if (log_level > WARN)
          //dll_log.Log (L"[GL SpeedUp] Avoided redundant uncompressed texture upload...");
          // Happy day! We just skipped a lot of framerate problems.
        return;
      }

      if (data != nullptr && config.textures.caching && format != GL_DEPTH_COMPONENT) {
        glGenTextures            (1, &cacheTexId);
        PPrinny_TEX_CopyTexState (texId, cacheTexId);
        //glBindTexture_Original (GL_TEXTURE_2D, cacheTexId);
      }

      glTexImage2D_Original ( target, level,
                                internalFormat,
                                  width, height,
                                    border,
                                      format, type,
                                        data );

      if (data != nullptr && config.textures.caching && format != GL_DEPTH_COMPONENT) {
        texture_remap.addChecksum  (cacheTexId, checksum);
        texture_remap.addRemapping (texId, cacheTexId);
      }
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

  //
  // Mipmap Generation
  //
  if (config.textures.force_mipmaps) {
    if ( data != nullptr /*&& texId < 67551*/) {
      PPrinny_TEX_GenerateMipmap (width, height);
      ////////dll_log.Log (L"[GL Texture] Generated mipmaps (on data upload) for Uncompressed TexID=%lu", texId);
    }
  } else {
    //PPrinny_TEX_DisableMipmapping ();
  }
}

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

  return SK_BeginBufferSwap ();
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

  pp::window.hwnd = WindowFromDC (wglGetCurrentDC ());

  pp::RenderFix::draw_state.frames++;
  pp::RenderFix::draw_state.thread = GetCurrentThreadId ();

  glFlush_Original ();

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

  HRESULT ret = SK_EndBufferSwap (S_OK, nullptr);

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
  if (current_dc == 0 || current_glrc == 0)
    return;

  if (current_dc != game_dc || current_glrc != game_glrc) {
    return glUseProgram_Original (program);
  }

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

  if (config.compatibility.aggressive_opt) {
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
glGetIntegerv_Detour ( GLenum pname,
                       GLint *params )
{
  if (current_dc == 0 || current_glrc == 0)
    return;

  if (current_dc != game_dc || current_glrc != game_glrc) {
    return glGetIntegerv_Original (pname, params);
  }

  if (pname == GL_TEXTURE_BINDING_2D) {
    *params = current_textures [active_texture].tex2d.name;
    return;
  }

  glGetIntegerv_Original (pname, params);
}

__declspec (noinline)
GLvoid
WINAPI
glBlendFunc_Detour (GLenum srcfactor, GLenum dstfactor)
{
  if (current_dc == 0 || current_glrc == 0)
    return;

  if (current_dc != game_dc || current_glrc != game_glrc) {
    return glBlendFunc_Original (srcfactor, dstfactor);
  }

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
  if (current_dc == 0 || current_glrc == 0)
    return;

  if (current_dc != game_dc || current_glrc != game_glrc) {
    return glAlphaFunc_Original (alpha_test, ref_val);
  }

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
  if (current_dc == 0 || current_glrc == 0)
    return;

  if (current_dc != game_dc || current_glrc != game_glrc) {
    return glDepthMask_Original (flag);
  }

  real_depth_mask = flag;

  return glDepthMask_Original (flag);
}

__declspec (noinline)
GLvoid
WINAPI
glEnable_Detour (GLenum cap)
{
  if (current_dc == 0 || current_glrc == 0)
    return;

  if (current_dc != game_dc || current_glrc != game_glrc) {
    return glEnable_Original (cap);
  }

  // The game doesn't use fixed-function lighting, sheesh!
  //if (cap == GL_LIGHTING)
    //return;

  // This invalid state setup is embedded in some display lists
  if (cap == GL_TEXTURE) cap = GL_TEXTURE_2D;

  if (cap == GL_TEXTURE_2D) {
    texture_2d = true;

    current_textures [active_texture].tex2d.enable = true;
  }

  if (cap == GL_BLEND)      real_blend_state = true;

  if (cap == GL_ALPHA_TEST) real_alpha_state = true;
  if (cap == GL_DEPTH_TEST) real_depth_test  = true;

  return glEnable_Original (cap);
}

__declspec (noinline)
GLvoid
WINAPI
glDisable_Detour (GLenum cap)
{
  if (current_dc == 0 || current_glrc == 0)
    return;

  if (current_dc != game_dc || current_glrc != game_glrc) {
    return glDisable_Original (cap);
  }

  // The game doesn't use fixed-function lighting, sheesh!
  //if (cap == GL_LIGHTING)
    //return;

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
  if (current_dc == 0 || current_glrc == 0)
    return;

  if (current_dc != game_dc || current_glrc != game_glrc) {
    return glTexParameteri_Original (target, pname, param);
  }

  GLuint texId = 0;

  if (target == GL_TEXTURE_2D) {
    texId = current_textures [active_texture].tex2d.name;

    // I am almost positive that no code should intentionally be
    //   attempting to do this.
    if (current_textures [active_texture].tex2d.name == 0) {
      dll_log.Log ( L"[GL Texture] Ignoring request to modify GL's default texture <"
                    L"glTexParameteri (...)>" );
      return;
    }
  }

  //dll_log.Log (L"[ GL Trace ] pname: %x", pname);

  if (texId > 67551 && texId <= 67557) {
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
  }

  // Extra conditions are necessary to prevent Steam from breaking itself
  else if (! (texId > 67557 || target != GL_TEXTURE_2D || thirdparty_texids.count (texId))) {
    bool force_nearest = config.textures.pixelate > 0;

    GLint fmt;
    glGetTexLevelParameteriv (GL_TEXTURE_2D, 0, GL_TEXTURE_INTERNAL_FORMAT, &fmt);

    if (force_nearest) {
      // Don't force nearest-neighbor sampling on UI textures
      if (/*fmt != GL_RGBA &&*/ (pname == GL_TEXTURE_MAG_FILTER || pname == GL_TEXTURE_MIN_FILTER))
        param = GL_NEAREST;
    }

    if (config.textures.force_mipmaps && active_texture == 0)
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

  GLint filter;

  if (pname == GL_TEXTURE_MIN_FILTER && target == GL_TEXTURE_2D) {
    glGetTexParameteriv (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, &filter);
    current_textures [active_texture].tex2d.min_filter = filter;
  }

  if (pname == GL_TEXTURE_MAG_FILTER && target == GL_TEXTURE_2D) {
    glGetTexParameteriv (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, &filter);
    current_textures [active_texture].tex2d.mag_filter = filter;
  }

  glTexParameteri_Original (target, pname, param);
}

__declspec (noinline)
GLvoid
WINAPI
glPixelTransferi_Detour (
  GLenum pname,
  GLint  param )
{
  if (current_dc == 0 || current_glrc == 0)
    return;

  if (current_dc != game_dc || current_glrc != game_glrc) {
    return glPixelTransferi_Original (pname, param);
  }

//  dll_log.Log ( L"[ GL Trace ] PixelTransferi: %x, %lu",
//                  pname, param );

#ifdef GL_PALETTES
  glPixelTransferi_Original (pname, param);
#endif
}

__declspec (noinline)
GLvoid
WINAPI
glPixelMapfv_Detour (
        GLenum   map,
        GLsizei  mapsize,
  const GLfloat* values )
{
  if (current_dc == 0 || current_glrc == 0)
    return;

  if (current_dc != game_dc || current_glrc != game_glrc) {
    return glPixelMapfv_Original (map, mapsize, values);
  }

  //if ((! aggressive_optimization) || memcmp (pixel_maps [map - GL_PIXEL_MAP_I_TO_I], values, mapsize * sizeof (GLfloat))) {
    //dll_log.Log (L"Map Change");
    memcpy (pixel_maps [map - GL_PIXEL_MAP_I_TO_I], values, mapsize * sizeof (GLfloat));
    pixel_map_crc32 =
      crc32 (0, &pixel_maps [2], 4 * sizeof (GLfloat) * 256);
    //dll_log.Log (L"Pixel Map Size: %lu, idx: %lu", mapsize, map - GL_PIXEL_MAP_I_TO_I);
#ifdef GL_PALETTES
    glPixelMapfv_Original (map, mapsize, values);
#endif
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
  if (current_dc == 0 || current_glrc == 0)
    return;

  if (current_dc != game_dc || current_glrc != game_glrc) {
    return glCompressedTexImage2D_Original ( target, level, internalFormat,
                                             width, height, border, imageSize,
                                             data );
  }

  extern LPVOID __PP_end_img_addr;

  // I am almost positive that no code should intentionally be
  //   attempting to do this.
  if (target == GL_TEXTURE_2D && current_textures [active_texture].tex2d.name == 0) {
    dll_log.Log ( L"[GL Texture] Ignoring request to modify GL's default texture <"
                  L"glCompressedTexImage2D (...)>" );
    return;
  }

  //
  // Only cache stuff that came from the game!
  //
  if (target == GL_TEXTURE_2D) {
    PPrinny_TEX_UploadTextureImage2D (
      target,
        level,
          internalFormat,
            width,
              height,
                border,
                  (void *)data,
                    GL_NONE, GL_NONE,
                      imageSize );
  } else {
    dll_log.Log (L"[ GL Trace ] Target: %x", target);
    PPrinny_TEX_DisableMipmapping ();

    if (target == GL_TEXTURE_RECTANGLE)
      thirdparty_texids.insert (current_textures [active_texture].texRect.name);
    else
      thirdparty_texids.insert (current_textures [active_texture].tex2d.name);

    //dll_log.Log (L"[SteamOvrly] Assuming upload is from 3rd party software -- IntFmt: %x, Fmt: %x, Target: %x, data: %ph, texId: %lu",
      //internalformat, format, target, data, current_textures [active_texture].tex2d.name );
    glCompressedTexImage2D_Original (
      target,
        level,
          internalFormat,
            width,
              height,
                border,
                  imageSize,
                    data );
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
        case render_targets.depth:
BEGIN_TASK_TIMING
          if (GLPerf::timers [GLPerf::TASK_MSAA0]->isReady ())
            GLPerf::timers [GLPerf::TASK_MSAA0]->requestTimestamp ();
END_TASK_TIMING
          break;
        case render_targets.color:
BEGIN_TASK_TIMING
           if (GLPerf::timers [GLPerf::TASK_MSAA1]->isReady ())
            GLPerf::timers [GLPerf::TASK_MSAA1]->requestTimestamp ();
END_TASK_TIMING
          break;

        // Normals
        case render_targets.normal:
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
        case render_targets.position:
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
                                       framebuffer.real_width, framebuffer.real_height,
                                     0, 0,
                                       framebuffer.real_width, framebuffer.real_height,
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
  if (current_dc == 0 || current_glrc == 0)
    return;

  if (current_dc != game_dc || current_glrc != game_glrc) {
    return glBindFramebuffer_Original ( target, framebuffer );
  }

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

  // Avoid redundant state changes, drivers are not stupid and will not
  //   incur validation overhead if they detect this, but it's silly.
  //if (active_fbo != previous_fbo)
    glBindFramebuffer_Original (target, framebuffer);


  if (config.render.msaa_samples > 1) {
    if (msaa.use && framebuffer == 2 && framebuffer != msaa.resolve_fbo)
      glEnable  (GL_MULTISAMPLE);
    else
      glDisable (GL_MULTISAMPLE);
  }
}

__declspec (noinline)
GLvoid
WINAPI
glFramebufferRenderbuffer_Detour ( GLenum target,
                                   GLenum attachment,
                                   GLenum renderbuffertarget,
                                   GLuint renderbuffer )
{
  if (current_dc == 0 || current_glrc == 0)
    return;

  if (current_dc != game_dc || current_glrc != game_glrc) {
    return glFramebufferRenderbuffer_Original ( target, attachment, renderbuffertarget, renderbuffer );
  }

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
  if (current_dc == 0 || current_glrc == 0)
    return;

  if (current_dc != game_dc || current_glrc != game_glrc) {
    return glDrawBuffers_Original ( n, bufs );
  }


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
  if (current_dc == 0 || current_glrc == 0)
    return;

  if (current_dc != game_dc || current_glrc != game_glrc) {
    return glFramebufferTexture2D_Original ( target, attachment,
                                             textarget, texture, level );
  }

  if (config.render.msaa_samples > 1)
    if (msaa.attachTexture (target, attachment, texture, textarget, level))
      return;

  glFramebufferTexture2D_Original ( target,
                                      attachment,
                                        textarget,
                                          texture,
                                            level );
}

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
  if (current_dc == 0 || current_glrc == 0)
    return;

  if (current_dc != game_dc || current_glrc != game_glrc) {
    return glReadPixels_Original ( x, y, width, height, format, type, data );
  }


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
  if (x == framebuffer.game_width / 2 - 5 && y == framebuffer.game_height / 2 - 5) {
    x = framebuffer.real_width  / 2 - 5;
    y = framebuffer.real_height / 2 - 5;

    const int QUEUE_DEPTH = 4;

    static GLubyte cached_data [10 * 10 * 4] = { 0 };
    static GLuint  pbo_idx                   =   0;
    static GLuint  pbo [6]                   = { 0, 0, 0,
                                                 0, 0, 0 };

    if (pbo [pbo_idx] == 0) {
#define GL_STATIC_READ   0x88E5
      glGenBuffers (1, &pbo [pbo_idx]);

      GLuint original_pbo;
      glGetIntegerv (GL_PIXEL_PACK_BUFFER_BINDING, (GLint *)&original_pbo);

      glBindBuffer (GL_PIXEL_PACK_BUFFER, pbo [pbo_idx]);
      glBufferData (GL_PIXEL_PACK_BUFFER, 10 * 10 * 4, nullptr, GL_STREAM_READ);

      glBindBuffer (GL_PIXEL_PACK_BUFFER, original_pbo);
    }

    //
    // Even on systems that do not support sync objects, the way this is written
    //   it will introduce at least a 1 frame latency between issuing a read
    //     and actually trying to get the result, which helps performance.
    //
    //  On a system with sync object support, this is very efficient.
    //
           GLint  fence_idx  = pbo_idx;

    static GLsync fences [6] = { nullptr, nullptr, nullptr,
                                 nullptr, nullptr, nullptr };

    if (fences [fence_idx] == nullptr) {
// For Depth of Field  (color buffers should alreaby be resolved)
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

      GLuint original_pbo;
      glGetIntegerv         (GL_PIXEL_PACK_BUFFER_BINDING, (GLint *)&original_pbo);

      glBindBuffer          (GL_PIXEL_PACK_BUFFER, pbo [pbo_idx]);
      glReadPixels_Original (x, y, width, height, format, type, nullptr);

      if (glFenceSync != nullptr)
        fences [fence_idx] = glFenceSync (GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
      else
        fences [fence_idx] = (GLsync)0x01;

      //dll_log.Log ( L"[GL SyncOp] Inserted fence sync (%ph) into command stream "
                    //L"for glReadPixels (...)", fence );

      glBindBuffer          (GL_PIXEL_PACK_BUFFER, original_pbo);

      pbo_idx++;

      if (pbo_idx >= QUEUE_DEPTH)
        pbo_idx = 0;

    } else {

      int start_idx = pbo_idx;

      // Run through the entire queue, clearing all buffers with
      //   data in them (save for the one we just queued).
      for (int i = 0; i < QUEUE_DEPTH/2; i++) {
      fence_idx = start_idx + i;

      if (fence_idx >= QUEUE_DEPTH)
        fence_idx -= QUEUE_DEPTH;

      if (fences [fence_idx] == 0)
        continue;

      GLenum status = 
        glClientWaitSync != nullptr ?
          glClientWaitSync ( fences [fence_idx], GL_SYNC_FLUSH_COMMANDS_BIT, 0ULL ) :
            GL_CONDITION_SATISFIED;

      if (status == GL_CONDITION_SATISFIED || status == GL_ALREADY_SIGNALED) {
        if (wait_one_extra && (! waited))
          waited = true;
        else {
          waited = false;
          //dll_log.Log (L"[GL SyncOp] ReadPixels complete...");

          if (glDeleteSync != nullptr)
            glDeleteSync (fences [fence_idx]);

          fences [fence_idx] = nullptr;

          GLuint original_pbo;
          glGetIntegerv (GL_PIXEL_PACK_BUFFER_BINDING, (GLint *)&original_pbo);

          glBindBuffer       (GL_PIXEL_PACK_BUFFER, pbo [fence_idx]);
          glGetBufferSubData (GL_PIXEL_PACK_BUFFER, 0, 10 * 10 * 4, cached_data);
          //glDeleteBuffers    (1, &pbo [fence_idx]);
          //pbo [fence_idx] = 0;
          glBindBuffer       (GL_PIXEL_PACK_BUFFER, original_pbo);

          pbo_idx = fence_idx + 1;

          if (pbo_idx >= QUEUE_DEPTH)
            pbo_idx = 0;

          break;
        }
      } else {
        //dll_log.Log (L"[GL SyncOp] glClientWaitSync --> %x", status);
      }
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
  if (current_dc == 0 || current_glrc == 0)
    return;

  if (current_dc != game_dc || current_glrc != game_glrc) {
    return glDrawArrays_Original (mode, first, count);
  }

  bool fringed = false;

  if (known_glsl.ssao.programs.count (active_program)) {
    if (glMemoryBarrier != nullptr)
      glMemoryBarrier ( GL_TEXTURE_UPDATE_BARRIER_BIT );

    glPushAttrib (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDepthMask  (GL_FALSE);
    glDisable    (GL_DEPTH_TEST);
    glColorMaski (1, GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    glColorMaski (2, GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    glColorMaski (3, GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    glColorMaski (4, GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    glColorMaski (5, GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    glColorMaski (6, GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    glColorMaski (7, GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
  }

  else if (config.render.fringe_removal && active_fbo == 2) {
    bool world  = (mode == GL_TRIANGLES);// && count < 180);//      && count == 6);
    bool sprite = (mode == GL_TRIANGLE_STRIP && count == 4);

     if (drawing_main_scene && (world || sprite)) {
       if ( real_depth_mask &&
            real_depth_test &&
            current_textures [active_texture].tex2d.enable ) {
       GLint filter = current_textures [active_texture].tex2d.min_filter;

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
  if (fringed) {
    glAlphaFunc_Original (real_alpha_test, real_alpha_ref);// GL_GREATER, 0.01f);
    if (! real_alpha_state)
      glDisable_Original (GL_ALPHA_TEST);
  }

  if (known_glsl.ssao.programs.count (active_program)) {
    if (glMemoryBarrier != nullptr)
      glMemoryBarrier ( GL_FRAMEBUFFER_BARRIER_BIT );
    glPopAttrib ();
  }
}

__declspec (noinline)
GLvoid
WINAPI
glMatrixMode_Detour (GLenum mode)
{
  if (current_dc == 0 || current_glrc == 0)
    return;

  if (current_dc != game_dc || current_glrc != game_glrc) {
    return glMatrixMode_Original ( mode );
  }


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
  if (current_dc == 0 || current_glrc == 0)
    return;

  if (current_dc != game_dc || current_glrc != game_glrc) {
    return glGetFloatv_Original ( pname, pparams );
  }


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
  if (current_dc == 0 || current_glrc == 0)
    return;

  if (current_dc != game_dc || current_glrc != game_glrc) {
    return glDeleteTextures_Original (n, textures);
  }


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

  else {
    dll_log.Log (L"[ GL Trace ] glDeleteTextures (%lu, ...)", n);
    texture_remap.remaps.clear ();
  }

  for (int i = 0; i < n; i++) {
    for (int j = 0; j < 32; j++) {
      if (textures [i] == current_textures [j].tex2d.name) {
        current_textures [j].tex2d.name = 0;
      }
    }

    if (ui_textures.find (textures [i]) != ui_textures.end ()) {
      //dll_log.Log (L"Deleted UI texture");
      ui_textures.erase (textures [i]);
    }

    msaa.deleteTexture (textures [i]);


    texture_remap.remaps.erase (textures [i]);
    known_textures.erase       (textures [i]);
    thirdparty_texids.erase    (textures [i]);

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
  if (current_dc == 0 || current_glrc == 0)
    return;

  if (current_dc != game_dc || current_glrc != game_glrc) {
    return glTexSubImage2D_Original ( target, level, xoffset, yoffset,
                                      width, height, format, type, pixels );
  }


  dll_log.Log (L"[ Tex Dump ] SubImage2D - Level: %li, <%li,%li>", level, xoffset, yoffset);

  glTexSubImage2D_Original (
    target, level,
      xoffset, yoffset,
        width, height,
          format, type,
            pixels );
}

glActiveTexture_pfn glActiveTexture_Original = nullptr;

__declspec (noinline)
GLvoid
WINAPI
glActiveTexture_Detour ( GLenum texture )
{
  if (current_dc == 0 || current_glrc == 0)
    return;

  if (current_dc != game_dc || current_glrc != game_glrc) {
    return glActiveTexture_Original (texture);
  }

  active_texture = texture-GL_TEXTURE0;

  glActiveTexture_Original (texture);
}

__declspec (noinline)
GLvoid
WINAPI
glBindTexture_Detour ( GLenum target,
                       GLuint texture )
{
  if (current_dc == 0 || current_glrc == 0)
    return;

  if (current_dc != game_dc || current_glrc != game_glrc) {
    return glBindTexture_Original (target, texture);
  }

  // Binding texture 0 when disabling texturing is a bad practice, it seems
  //   to be corrupting other commands that assume the bound texture did not
  //     change.
  //
  //  Moreover, the default texture is immutable.
//  if (texture == 0 && (! current_textures [active_texture].tex2d.enable))
//    return;

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

  GLuint actual_tex = texture_remap.getRemapped (texture);

  glBindTexture_Original (target, actual_tex);//texture);

  if (target == GL_TEXTURE_2D) {
    GLint filter;

    glGetTexParameteriv (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, &filter);
    current_textures [active_texture].tex2d.min_filter = filter;

    glGetTexParameteriv (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, &filter);
    current_textures [active_texture].tex2d.mag_filter = filter;
  }
}

#pragma intrinsic (_ReturnAddress)

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
  if (current_dc == 0 || current_glrc == 0)
    return;

  if (current_dc != game_dc || current_glrc != game_glrc) {
    return glTexImage2D_Original ( target, level, internalformat, width, height,
                                   border, format, type, data );
  }

  extern LPVOID __PP_end_img_addr;

  // I am almost positive that no code should intentionally be
  //   attempting to do this.
  if (target == GL_TEXTURE_2D && current_textures [active_texture].tex2d.name == 0) {
    dll_log.Log ( L"[GL Texture] Ignoring request to modify GL's default texture <"
                  L"glTexImage2D (...)>" );
    return;
  }

  //
  // Only cache stuff that came from the game!
  //
  if (target == GL_TEXTURE_2D && internalformat != GL_RGBA8) {
    PPrinny_TEX_UploadTextureImage2D (
      target,
        level,
          internalformat,
            width,
              height,
                border,
                  (void *)data,
                    format, type );
  } else {
    PPrinny_TEX_DisableMipmapping ();

    if (target == GL_TEXTURE_RECTANGLE)
      thirdparty_texids.insert (current_textures [active_texture].texRect.name);
    else
      thirdparty_texids.insert (current_textures [active_texture].tex2d.name);

    //dll_log.Log (L"[SteamOvrly] Assuming upload is from 3rd party software -- IntFmt: %x, Fmt: %x, Target: %x, data: %ph, texId: %lu",
      //internalformat, format, target, data, current_textures [active_texture].tex2d.name );
    glTexImage2D_Original (
      target,
        level,
          internalformat,
            width,
              height,
                border,
                  format,
                    type,
                      data );
  }
}

__declspec (noinline)
GLvoid
WINAPI
glDeleteRenderbuffers_Detour (       GLsizei n,
                               const GLuint* renderbuffers )
{
  if (current_dc == 0 || current_glrc == 0)
    return;

  if (current_dc != game_dc || current_glrc != game_glrc) {
    return glDeleteRenderbuffers_Original (n, renderbuffers);
  }

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
  if (current_dc == 0 || current_glrc == 0)
    return;

  if (current_dc != game_dc || current_glrc != game_glrc) {
    return glRenderbufferStorage_Original ( target, internalformat,
                                            width, height );
  }

  dll_log.Log ( L"[GL Surface]  >> Render Buffer - Internal Format: %s",
                  PP_GL_InternalFormat_ToStr (internalformat).c_str () );

  if (internalformat == GL_DEPTH_COMPONENT)
    internalformat = PP_DEPTH_FORMAT;
  if (width == framebuffer.game_width && height == framebuffer.game_height) {
    width = framebuffer.real_width; height = framebuffer.real_height;

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
  if (current_dc == 0 || current_glrc == 0)
    return;

  if (current_dc != game_dc || current_glrc != game_glrc) {
    return glBlitFramebuffer_Original ( srcX0, srcY0,
                                        srcX1, srcY1,
                                        dstX0, dstY0,
                                        dstX1, dstY1,
                                        mask, filter );
  }

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
  if (current_dc == 0 || current_glrc == 0)
    return;

  if (current_dc != game_dc || current_glrc != game_glrc) {
    return glCopyTexSubImage2D_Original ( target, level, xoffset, yoffset,
                                          x, y, width, height );
  }

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

    width  = framebuffer.real_width;
    height = framebuffer.real_height;

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
  if (current_dc == 0 || current_glrc == 0)
    return;

  if (current_dc != game_dc || current_glrc != game_glrc) {
    return glScissor_Original ( x, y, width, height );
  }

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
  if (current_dc == 0 || current_glrc == 0)
    return;

  if (current_dc != game_dc || current_glrc != game_glrc) {
    return glViewport_Original ( x, y, width, height );
  }

  if (pp::RenderFix::tracer.log)
    dll_log.Log ( L"[ GL Trace ] -- glViewport (%lu, %lu, %lu, %lu)",
                    x, y,
                      width, height );

  if (width == framebuffer.game_width && height == framebuffer.game_height) {
    width  = framebuffer.real_width;
    height = framebuffer.real_height;

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
  if (current_dc == 0 || current_glrc == 0)
    return;

  if (current_dc != game_dc || current_glrc != game_glrc) {
    return glUniformMatrix4fv_Original (location, count, transpose, value);
  }

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
  if (current_dc == 0 || current_glrc == 0)
    return;

  if (current_dc != game_dc || current_glrc != game_glrc) {
    return glUniformMatrix3fv_Original (location, count, transpose, value);
  }

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
  if (current_dc == 0 || current_glrc == 0)
    return;

  if (current_dc != game_dc || current_glrc != game_glrc) {
    return glVertexAttrib4f_Original (index, v0, v1, v2, v3);
  }

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
  if (current_dc == 0 || current_glrc == 0)
    return;

  if (current_dc != game_dc || current_glrc != game_glrc) {
    return glVertexAttrib4fv_Original (index, v);
  }

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
  if (current_dc == 0 || current_glrc == 0)
    return;

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
  if (current_dc == 0 || current_glrc == 0)
    return;

  if (current_dc != game_dc || current_glrc != game_glrc) {
    return glAttachShader_Original (program, shader);
  }

  // Mark this program as including SSAO if one of its shaders
  //   does.
  if (known_glsl.ssao.shaders.count (shader)) {
    known_glsl.ssao.programs.insert (program);
  }

  if (known_glsl.shadow.shaders.count (shader)) {
    known_glsl.shadow.programs.insert (program);
  }


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
  if (current_dc == 0 || current_glrc == 0)
    return;

  if (current_dc != game_dc || current_glrc != game_glrc) {
    return glShaderSource_Original ( shader, count, string, length );
  }
  uint32_t checksum = crc32 (0x00, (const void *)*string, strlen ((const char *)*string));

#if 0
  if (checksum == 0x12C7A66D) {
    dll_log.Log ( L"[ GL MISC. ] Shader %lu writes to gl_FragData [1] and gl_FragData [2]",
                    shader );
  }
#endif

  if ( strstr ( (const char *)*string,
         "col *= ( 1.0 - ssaoTest( gl_TexCoord[0] ));") ) {
    known_glsl.ssao.shaders.insert (shader);
  }

  if ( strstr ( (const char *)*string,
         "float calcShadowCoef()") ) {
    known_glsl.shadow.shaders.insert (shader);
  }


  PPrinny_DumpShader (checksum, shader, (const char *)*string);

  char szShaderName [MAX_PATH];
  sprintf (szShaderName, "shaders/%X.glsl", checksum);

  GLint type;
  glGetShaderiv_Original (shader, GL_SHADER_TYPE, &type);

  char szFullShaderName [MAX_PATH];
  sprintf ( szFullShaderName, "shaders/%s_%X.glsl",
              type == GL_VERTEX_SHADER ? "Vertex" :
                                         "Fragment",
                checksum );

  char* szShaderFile = nullptr;

  if (GetFileAttributesA (szFullShaderName) != INVALID_FILE_ATTRIBUTES)
    szShaderFile = szFullShaderName;
  else if (GetFileAttributesA (szShaderName) != INVALID_FILE_ATTRIBUTES)
    szShaderFile = szShaderName;


  if (szShaderFile != nullptr) {
    FILE* fShader = fopen (szShaderFile, "rb");

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

        free (szSrc);

        return;
      }
    }
  }

  char* szShaderSrc = _strdup ((const char *)*string);

  //
  // This is more or less obsolete because all of the shaders that used this
  //   now use textureSize (..., 0) for proper texture coordinate math.
  //
  //  * Minimum GLSL version is now 1.30 (GL 3.0), but those are the advertised
  //      system requirements anyway, so I have not lost any sleep.
  //
  if (type == GL_FRAGMENT_SHADER) {
    char* szTexWidth  =  strstr (szShaderSrc, "const float texWidth = 720.0;  ");

    if (szTexWidth != nullptr) {
      char szNewWidth [33];
      sprintf (szNewWidth, "const float texWidth = %7.1f;", (float)config.render.scene_res_y);
      memcpy (szTexWidth, szNewWidth, 31);
    }
  }

  glShaderSource_Original (shader, count, (const GLubyte **)&szShaderSrc, length);

  free (szShaderSrc);
}

#include <set>

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

    glDeleteBuffers =
      (glDeleteBuffers_pfn)
        wglGetProcAddress_Original ("glDeleteBuffers");

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

    glMemoryBarrier =
      (glMemoryBarrier_pfn)
        wglGetProcAddress ("glMemoryBarrier");
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

  static std::set <std::string> funcs;

  if (! funcs.count (szFuncName))
  {
    dll_log.LogEx (true, L"[OpenGL Ext] %#32hs => %ph", szFuncName, ret);

    if (detoured)
      dll_log.LogEx (false, L"    {Detoured}\n");
    else
      dll_log.LogEx (false, L"\n");

    funcs.emplace (szFuncName);
  }

  return ret;
}

wglGetProcAddress_pfn wglGetProcAddress_Log = wglGetProcAddress_Detour;

__declspec (noinline)
GLvoid
WINAPI
glFinish_Detour (void)
{
  if (current_dc == 0 || current_glrc == 0)
    return;

  if (current_dc != game_dc || current_glrc != game_glrc) {
    return glFinish_Original ();
  }

  if (config.compatibility.allow_gl_cpu_sync)
    glFinish_Original ();
}

__declspec (noinline)
GLvoid
WINAPI
glFlush_Detour (void)
{
  if (current_dc == 0 || current_glrc == 0)
    return;

  if (current_dc != game_dc || current_glrc != game_glrc) {
    return glFlush_Original ();
  }

  if (config.compatibility.allow_gl_cpu_sync)
    glFlush_Original ();
}

__declspec (noinline)
GLvoid
WINAPI
glLoadMatrixf_Detour (const GLfloat* m)
{
  if (current_dc == 0 || current_glrc == 0)
    return;

  if (current_dc != game_dc || current_glrc != game_glrc) {
    return glLoadMatrixf_Original (m);
  }

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
  if (current_dc == 0 || current_glrc == 0)
    return;

  if (current_dc != game_dc || current_glrc != game_glrc) {
    return glOrtho_Original ( left,    right,
                              bottom,  top,
                              nearVal, farVal );
  }

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
  if (current_dc == 0 || current_glrc == 0)
    return;

  if (current_dc != game_dc || current_glrc != game_glrc) {
    return glScalef_Original ( x, y, z );
  }


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
  if (current_dc == 0 || current_glrc == 0)
    return;

  if (current_dc != game_dc || current_glrc != game_glrc) {
    return glClear_Original ( mask );
  }


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

  if (game_glrc == 0) {
    game_dc   = hDC;
    game_glrc = ret;
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
  //if (config.compatibility.optimize_ctx_mgmt)
    //return TRUE;

  return wglCopyContext_Original (hglrcSrc, hglrcDst, mask);
}

__declspec (noinline)
BOOL
WINAPI
wglMakeCurrent_Detour ( HDC   hDC,
                        HGLRC hGLRC )
{
//  if (! config.compatibility.optimize_ctx_mgmt) {
//    return wglMakeCurrent_Original (hDC, hGLRC);
//  }

  //dll_log.Log ( L"[ GL Trace ] wglMakeCurrent (%Xh, %Xh) <tid=%X>",
                  //hDC,
                    //hGLRC,
                      //GetCurrentThreadId () );

  if (game_glrc == 0 && hGLRC != 0) {
    game_dc   = hDC;
    game_glrc = hGLRC;
  }

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
                              "SK_BeginBufferSwap",
                               OGLEndFrame_Pre,
                     (LPVOID*)&SK_BeginBufferSwap );

      PPrinny_CreateDLLHook ( config.system.injector.c_str (),
                              "SK_EndBufferSwap",
                               OGLEndFrame_Post,
                     (LPVOID*)&SK_EndBufferSwap );


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
                              "glGetIntegerv",
                              glGetIntegerv_Detour,
                   (LPVOID *)&glGetIntegerv_Original );

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

HWND hWndDummy = 0;

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

  hWndDummy = 
    CreateWindowW ( wc.lpszClassName,
                      L"Dummy_Pretty_Prinny",
                        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                          0,0,
                            0,0,
                              0,0,
                                hInstance,0);

  DestroyWindow (hWndDummy);
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
      pp::window.WndProc_Original = nullptr;
      return DefWindowProc (hWnd, message, wParam, lParam);
  }

  pp::window.WndProc_Original = nullptr;
  return 0;
}



void
pp::RenderFix::Init (void)
{
  PP_InitGL ();

  CommandProcessor*     comm_proc    = CommandProcessor::getInstance ();
  SK_ICommandProcessor* pCommandProc = SK_GetCommandProcessor        ();

  // Don't directly modify this state, switching mid-frame would be disasterous...
  pCommandProc->AddVariable ("Render.MSAA",             PP_CreateVar (SK_IVariable::Boolean, (&msaa.use)));

  pCommandProc->AddVariable ("Render.FringeRemoval",    PP_CreateVar (SK_IVariable::Boolean, &config.render.fringe_removal));

  pCommandProc->AddVariable ("Trace.NumFrames",         PP_CreateVar (SK_IVariable::Int,     &tracer.count));
  pCommandProc->AddVariable ("Trace.Enable",            PP_CreateVar (SK_IVariable::Boolean, &tracer.log));

  pCommandProc->AddVariable ("Render.ConservativeMSAA", PP_CreateVar (SK_IVariable::Boolean, &config.render.conservative_msaa));
  pCommandProc->AddVariable ("Render.EarlyResolve",     PP_CreateVar (SK_IVariable::Boolean, &msaa.early_resolve));

  pCommandProc->AddVariable ("Render.AggressiveOpt",    PP_CreateVar (SK_IVariable::Boolean,&config.compatibility.aggressive_opt));
  pCommandProc->AddVariable ("Render.TaskTiming",       PP_CreateVar (SK_IVariable::Boolean,&GLPerf::time_tasks));
  pCommandProc->AddVariable ("Render.FastTexUploads",   PP_CreateVar (SK_IVariable::Boolean,&config.textures.caching));
}

void
pp::RenderFix::Shutdown (void)
{
}

float ar;

pp::RenderFix::CommandProcessor::CommandProcessor (void)
{
  SK_ICommandProcessor* pCommandProc =
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
    PP_CreateVar (SK_IVariable::Boolean, &config.render.high_precision_ssao, this);

  pCommandProc->AddVariable ("Render.HighPrecisionSSAO", high_precision_ssao);
}

bool
pp::RenderFix::CommandProcessor::OnVarChange (SK_IVariable* var, void* val)
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

SK_IVariable*
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

struct game_cfg_s {
  uint8_t unknown [27];
  uint8_t cursor_mode;  // 0=A, 1=B
  uint8_t other_mode;   // 0=A, 1=B
  uint8_t volume [3];   // 0=Mute, 5=Full
};