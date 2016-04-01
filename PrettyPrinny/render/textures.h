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
#ifndef __PPRINNY__TEXTURES_H__
#define __PPRINNY__TEXTURES_H__

#include <Windows.h>
#include <GL/gl.h>

#include <memory>
#include <unordered_set>

#include "../render.h"

GLvoid
PPrinny_DumpCompressedTexLevel ( uint32_t crc32,
                                 GLint    level,
                                 GLenum   format,
                                 GLsizei  width,
                                 GLsizei  height,

                                 GLsizei  imageSize,
                           const GLvoid*  data );


GLvoid
PPrinny_DumpUncompressedTexLevel ( uint32_t crc32,
                                   GLint    level,
                                   GLenum   format,
                                   GLsizei  width,
                                   GLsizei  height,

                                   GLsizei  imageSize,
                             const GLvoid*  data );


GLvoid
PPrinny_DumpTextureAndPalette ( uint32_t crc32_base,
                                uint32_t crc32_palette,
                                GLfloat* palette,
                                GLsizei  width,
                                GLsizei  height,
                          const GLvoid*  paletted_data );

std::unique_ptr <uint8_t*>
PPrinny_TEX_ConvertPalettedToBGRA ( GLsizei  width,
                                    GLsizei  height,
                                    GLfloat* palette,
                              const GLvoid*  data );

// Technically, we could have 80, but this game only uses two stages...
struct pp_texture_state_s {
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
    GLenum    min_filter;
    GLenum    mag_filter;
  } tex2d;

  struct tex3d_s {
    GLuint    name;
    GLboolean enable;
  } tex3d;
} extern current_textures [32];

extern GLuint                      active_texture;
extern std::unordered_set <GLuint> thirdparty_texids;

extern glTexParameteri_pfn glTexParameteri_Original;

extern uint32_t
crc32 (uint32_t crc, const void *buf, size_t size);

#endif /* __PPRINNY__TEXTURES_H__ */
