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

#include <Windows.h>
#include <mmsystem.h>
#include <cstdint>

#include <unordered_map>
#include <unordered_set>

#include "textures.h"

#include "../render.h"
#include "../config.h"

struct DDS_PIXELFORMAT {
  DWORD dwSize;
  DWORD dwFlags;
  DWORD dwFourCC;
  DWORD dwRGBBitCount;
  DWORD dwRBitMask;
  DWORD dwGBitMask;
  DWORD dwBBitMask;
  DWORD dwABitMask;
};

typedef struct {
  DWORD           dwSize;
  DWORD           dwFlags;
  DWORD           dwHeight;
  DWORD           dwWidth;
  DWORD           dwPitchOrLinearSize;
  DWORD           dwDepth;
  DWORD           dwMipMapCount;
  DWORD           dwReserved1[11];
  DDS_PIXELFORMAT ddspf;
  DWORD           dwCaps;
  DWORD           dwCaps2;
  DWORD           dwCaps3;
  DWORD           dwCaps4;
  DWORD           dwReserved2;
} DDS_HEADER;


#define DDSD_CAPS        0x00000001
#define DDSD_HEIGHT      0x00000002
#define DDSD_WIDTH       0x00000004
#define DDSD_PITCH       0x00000008
#define DDSD_PIXELFORMAT 0x00001000
#define DDSD_MIPMAPCOUNT 0x00020000
#define DDSD_LINEARSIZE  0x00080000
#define DDSD_DEPTH       0x00800000

#define DDPF_ALPHAPIXELS            0x00000001
#define DDPF_ALPHA                  0x00000002
#define DDPF_FOURCC                 0x00000004
#define DDPF_RGB                    0x00000040
#define DDPF_LUMINANCE              0x00020000

#define DDSCAPS_TEXTURE             0x00001000

#include <gl/GL.h>

#define GL_COMPRESSED_RGB_S3TC_DXT1_EXT    0x83F0
#define GL_COMPRESSED_RGBA_S3TC_DXT1_EXT   0x83F1
#define GL_COMPRESSED_RGBA_S3TC_DXT3_EXT   0x83F2
#define GL_COMPRESSED_RGBA_S3TC_DXT5_EXT   0x83F3

static uint32_t crc32_tab[] = { 
   0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419, 0x706af48f, 
   0xe963a535, 0x9e6495a3, 0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988, 
   0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91, 0x1db71064, 0x6ab020f2, 
   0xf3b97148, 0x84be41de, 0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7, 
   0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec, 0x14015c4f, 0x63066cd9, 
   0xfa0f3d63, 0x8d080df5, 0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172, 
   0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b, 0x35b5a8fa, 0x42b2986c, 
   0xdbbbc9d6, 0xacbcf940, 0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59, 
   0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423, 
   0xcfba9599, 0xb8bda50f, 0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924, 
   0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d, 0x76dc4190, 0x01db7106, 
   0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433, 
   0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818, 0x7f6a0dbb, 0x086d3d2d, 
   0x91646c97, 0xe6635c01, 0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e, 
   0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457, 0x65b0d9c6, 0x12b7e950, 
   0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65, 
   0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2, 0x4adfa541, 0x3dd895d7, 
   0xa4d1c46d, 0xd3d6f4fb, 0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0, 
   0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9, 0x5005713c, 0x270241aa, 
   0xbe0b1010, 0xc90c2086, 0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f, 
   0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17, 0x2eb40d81, 
   0xb7bd5c3b, 0xc0ba6cad, 0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a, 
   0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683, 0xe3630b12, 0x94643b84, 
   0x0d6d6a3e, 0x7a6a5aa8, 0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1, 
   0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb, 
   0x196c3671, 0x6e6b06e7, 0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc, 
   0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5, 0xd6d6a3e8, 0xa1d1937e, 
   0x38d8c2c4, 0x4fdff252, 0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b, 
   0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55, 
   0x316e8eef, 0x4669be79, 0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236, 
   0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f, 0xc5ba3bbe, 0xb2bd0b28, 
   0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d, 
   0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a, 0x9c0906a9, 0xeb0e363f, 
   0x72076785, 0x05005713, 0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38, 
   0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21, 0x86d3d2d4, 0xf1d4e242, 
   0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777, 
   0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c, 0x8f659eff, 0xf862ae69, 
   0x616bffd3, 0x166ccf45, 0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2, 
   0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db, 0xaed16a4a, 0xd9d65adc, 
   0x40df0b66, 0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9, 
   0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605, 0xcdd70693, 
   0x54de5729, 0x23d967bf, 0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94, 
   0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d 
 };

uint32_t
crc32 (uint32_t crc, const void *buf, size_t size)
{
  const uint8_t *p;

  p = (uint8_t *)buf;
  crc = crc ^ ~0U;

  while (size--)
    crc = crc32_tab[(crc ^ *p++) & 0xFF] ^ (crc >> 8);

  return crc ^ ~0U;
}
#include <cstdio>

#include "../log.h"

// -------------------- Texture Dumping ---------------------//

void
PPrinny_DumpCompressedTexLevel ( uint32_t crc32,
                                 GLint    level,
                                 GLenum   format,
                                 GLsizei  width,
                                 GLsizei  height,

                                 GLsizei  imageSize,
                           const GLvoid*  data )
{
  if (GetFileAttributes (L"dump") == INVALID_FILE_ATTRIBUTES)
    CreateDirectoryW (L"dump", nullptr);

  if (GetFileAttributes (L"dump/textures") == INVALID_FILE_ATTRIBUTES)
    CreateDirectoryW (L"dump/textures", nullptr);

  wchar_t wszOutName [MAX_PATH];
  wsprintf ( wszOutName, L"dump\\textures\\Compressed_%lu_%X.dds",
               level, crc32 );

  // Early-Out if the texutre was already dumped.
  if (GetFileAttributes (wszOutName) != INVALID_FILE_ATTRIBUTES)
    return;

  DDS_PIXELFORMAT dds_fmt = { 0 };
  DDS_HEADER      dds_hdr = { 0 };

  dds_fmt.dwSize = sizeof DDS_PIXELFORMAT;

  bool dxt1 = false;

  switch (format) {
    case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
      //dll_log.Log (L"[ Tex Dump ] DXT1");

      dds_fmt.dwFourCC = MAKEFOURCC ('D', 'X', 'T', '1');
      dxt1             = true;
      break;

    case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
      //dll_log.Log (L"[ Tex Dump ] DXT1 (RGBA)");

      dds_fmt.dwFourCC    = MAKEFOURCC ('D', 'X', 'T', '1');
      dds_fmt.dwFlags    |= DDPF_ALPHAPIXELS;
      dds_fmt.dwABitMask  = 0xff000000;
      dxt1                = true;
      break;

    case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
      //dll_log.Log (L"[ Tex Dump ] DXT3");

      dds_fmt.dwFourCC    = MAKEFOURCC ('D', 'X', 'T', '3');
      dds_fmt.dwFlags    |= DDPF_ALPHAPIXELS;
      dds_fmt.dwABitMask  = 0xff000000;
      break;

    case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
      //dll_log.Log (L"[ Tex Dump ] DXT5");

      dds_fmt.dwFourCC    = MAKEFOURCC ('D', 'X', 'T', '5');
      dds_fmt.dwFlags    |= DDPF_ALPHAPIXELS;
      dds_fmt.dwABitMask  = 0xff000000;
      break;
  }

  dds_fmt.dwRGBBitCount; 
  dds_fmt.dwRBitMask = 0x00ff0000;
  dds_fmt.dwGBitMask = 0x0000ff00;
  dds_fmt.dwBBitMask = 0x000000ff;

  GLint block_size = dxt1 ? 8 : 16;

  dds_fmt.dwFlags |= DDPF_FOURCC;

  dds_hdr.dwSize              = sizeof DDS_HEADER;
  dds_hdr.dwFlags            |= DDSD_CAPS  | DDSD_PIXELFORMAT |
                                DDSD_WIDTH | DDSD_HEIGHT      |
                                DDSD_LINEARSIZE;
  dds_hdr.dwHeight            = height;
  dds_hdr.dwWidth             = width;
  dds_hdr.dwPitchOrLinearSize = imageSize;//max (1, ((width + 3) / 4)) * block_size;
  dds_hdr.dwCaps              = DDSCAPS_TEXTURE;

  memcpy (&dds_hdr.ddspf, &dds_fmt, sizeof DDS_PIXELFORMAT);

  FILE *fOut = _wfopen (wszOutName, L"wb+");

  if (fOut != nullptr) {
    fwrite ("DDS ", 4,                             1, fOut);
    fwrite ((void *)&dds_hdr,   sizeof DDS_HEADER, 1, fOut);
    fwrite (        data,       imageSize,         1, fOut);
    fclose (fOut);
  }
}

void
PPrinny_WriteUncompressedDDS ( GLenum  format,
                               GLsizei  width,
                               GLsizei  height,
                               GLsizei  imageSize,
                         const GLvoid*  data,
                         const wchar_t* wszOutFile )
{
  DDS_PIXELFORMAT dds_fmt = { 0 };
  DDS_HEADER      dds_hdr = { 0 };

  dds_fmt.dwSize = sizeof DDS_PIXELFORMAT;

  switch (format) {
    case GL_BGRA:
      //dll_log.Log (L"[ Tex Dump ] RGBA");

      dds_fmt.dwRGBBitCount = 32;
      dds_fmt.dwABitMask    = 0xff000000;
      dds_fmt.dwRBitMask    = 0x00ff0000;
      dds_fmt.dwGBitMask    = 0x0000ff00;
      dds_fmt.dwBBitMask    = 0x000000ff;

      dds_fmt.dwFlags      |= DDPF_RGB | DDPF_ALPHAPIXELS;

      dds_hdr.dwPitchOrLinearSize = 4 * width;
      break;

    case GL_COLOR_INDEX:
      dds_fmt.dwRGBBitCount = 8;

      dds_fmt.dwRBitMask    = 0xFF;
      dds_fmt.dwGBitMask    = 0x00;
      dds_fmt.dwBBitMask    = 0x00;
      dds_fmt.dwABitMask    = 0x00;
      dds_fmt.dwFlags      |= DDPF_LUMINANCE;

      dds_hdr.dwPitchOrLinearSize = (width * 8 + 7) / 8;
      break;

    default:
      dll_log.Log (L"[ Tex Dump ] Unexpected Format: %X", format);
      return;
  }

  dds_hdr.dwSize              = sizeof DDS_HEADER;
  dds_hdr.dwFlags            |= DDSD_CAPS  | DDSD_PIXELFORMAT |
                                DDSD_WIDTH | DDSD_HEIGHT      |
                                DDSD_PITCH;
  dds_hdr.dwHeight            = height;
  dds_hdr.dwWidth             = width;
  dds_hdr.dwCaps              = DDSCAPS_TEXTURE;

  memcpy (&dds_hdr.ddspf, &dds_fmt, sizeof DDS_PIXELFORMAT);

  FILE *fOut = _wfopen (wszOutFile, L"wb+");

  if (fOut != nullptr) {
    fwrite ("DDS ", 4,                             1, fOut);
    fwrite ((void *)&dds_hdr,   sizeof DDS_HEADER, 1, fOut);
    fwrite (        data,       imageSize,         1, fOut);
    fclose (fOut);
  }
}

void
PPrinny_DumpTextureAndPalette ( uint32_t crc32_base,
                                uint32_t crc32_palette,
                                GLfloat* palette,
                                GLsizei  width,
                                GLsizei  height,
                          const GLvoid*  paletted_data )
{
  if (GetFileAttributes (L"dump") == INVALID_FILE_ATTRIBUTES)
    CreateDirectoryW (L"dump", nullptr);

  if (GetFileAttributes (L"dump/textures") == INVALID_FILE_ATTRIBUTES)
    CreateDirectoryW (L"dump/textures", nullptr);

  wchar_t wszOutPath [MAX_PATH] = { L'\0' };

  wsprintf ( wszOutPath, L"dump\\textures\\Paletted_%X",
                 crc32_base );

  if (GetFileAttributes (wszOutPath) == INVALID_FILE_ATTRIBUTES)
    CreateDirectoryW (wszOutPath, nullptr);

  wchar_t wszBaseFile [MAX_PATH] = { L'\0' };

  lstrcatW (wszBaseFile, wszOutPath);
  lstrcatW (wszBaseFile, L"\\base.dds");

  if (GetFileAttributes (wszBaseFile) == INVALID_FILE_ATTRIBUTES) {
    PPrinny_WriteUncompressedDDS ( GL_COLOR_INDEX,
                                     width, height,
                                       width * height, paletted_data,
                                         wszBaseFile );
  }

  wchar_t wszUnpalettedFile [MAX_PATH] = { L'\0' };
  wchar_t wszUnpalettedName [32]       = { L'\0' };

  wsprintf ( wszUnpalettedName, L"color_%x.dds",
               crc32_palette );

  lstrcatW (wszUnpalettedFile, wszOutPath);
  lstrcatW (wszUnpalettedFile, L"\\");
  lstrcatW (wszUnpalettedFile, wszUnpalettedName);

  if (GetFileAttributes (wszUnpalettedFile) == INVALID_FILE_ATTRIBUTES) {
    auto color_img =
      PPrinny_TEX_ConvertPalettedToBGRA ( width, height,
                                            palette,
                                              paletted_data );

    PPrinny_WriteUncompressedDDS ( GL_BGRA,
                                     width, height,
                                       4 * width * height,
                                         *color_img,wszUnpalettedFile);
  }

  wchar_t wszPaletteFile [MAX_PATH] = { L'\0' };
  wchar_t wszPaletteName [32]       = { L'\0' };

  wsprintf ( wszPaletteName, L"palette_%x.pal",
               crc32_palette );

  lstrcatW (wszPaletteFile, wszOutPath);
  lstrcatW (wszPaletteFile, L"\\");
  lstrcatW (wszPaletteFile, wszPaletteName);

  FILE* fPalette = _wfopen (wszPaletteFile, L"wb+");

  if (fPalette != nullptr) {
    fwrite (palette, sizeof (float) * 256, 4, fPalette);
    fclose (fPalette);
  }
}

void
PPrinny_DumpUncompressedTexLevel ( uint32_t crc32,
                                   GLint    level,
                                   GLenum   format,
                                   GLsizei  width,
                                   GLsizei  height,

                                   GLsizei  imageSize,
                             const GLvoid*  data )
{
  if (GetFileAttributes (L"dump") == INVALID_FILE_ATTRIBUTES)
    CreateDirectoryW (L"dump", nullptr);

  if (GetFileAttributes (L"dump/textures") == INVALID_FILE_ATTRIBUTES)
    CreateDirectoryW (L"dump/textures", nullptr);

  wchar_t wszOutName [MAX_PATH];

  if (format != GL_COLOR_INDEX)
    wsprintf ( wszOutName, L"dump\\textures\\Uncompressed_%lu_%X.dds",
                 level, crc32 );
  else {
    wsprintf ( wszOutName, L"dump\\textures\\Paletted_%X",
                 crc32 );
    if (GetFileAttributes (wszOutName) == INVALID_FILE_ATTRIBUTES)
      CreateDirectoryW (wszOutName, nullptr);

    lstrcatW (wszOutName, L"\\base.dds");
  }

  // Early-Out if the texutre was already dumped.
  if (GetFileAttributes (wszOutName) != INVALID_FILE_ATTRIBUTES)
    return;

  PPrinny_WriteUncompressedDDS ( format,
                                   width,       height,
                                     imageSize, data,
                                           wszOutName );
}