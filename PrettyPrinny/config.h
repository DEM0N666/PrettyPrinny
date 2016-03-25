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
#ifndef __PPRINNY__CONFIG_H__
#define __PPRINNY__CONFIG_H__

#include <Windows.h>
#include <string>

extern std::wstring PPRINNY_VER_STR;

struct pp_config_s
{
  struct {  
    bool     allow_background    = false;//true;
    int      msaa_samples        = 0;
    int      msaa_quality        = 0;
    int      refresh_rate        = 0;

    int      scene_res_x         = 1280;
    int      scene_res_y         = 720;

    int      swap_interval       = 1;

    bool     fringe_removal      = true;
    bool     conservative_msaa   = true;

    bool     high_precision_ssao = false;
  } render;

  struct {
    bool     match_desktop       = true; // Implied by options below
    int      width               = 0;    //   (all 0 = match)
    int      height              = 0;
    int      refresh             = 0;
    int      monitor             = 0;
  } display;

  struct {
    bool     bypass_intel_gl     = true;
    bool     allow_gl_cpu_sync   = false;
    bool     optimize_ctx_mgmt   = true;
    bool     debug_mode          = false;
    bool     support_old_drivers = false;
    bool     patch_damage_bug    = true;
  } compatibility;

  struct {
    bool     disable_bg_msaa     = true; // NV compatibility hack
    bool     borderless          = true;
    float    foreground_fps      = 30.0f; // 0.0 = Unlimited
    float    background_fps      = 30.0f;
    bool     center              = true;
    int      x_offset            = 0;
    int      y_offset            = 0;
  } window;

  struct {
    struct {
      float r = 0.50f;
      float g = 0.50f;
      float b = 0.90f;
    } blue_text;

    struct {
      float r = 0.90f;
      float g = 0.10f;
      float b = 0.10f;
    } red_text;
  } colors;

  struct {
    float    tolerance           = 0.333333f; // 33%
  } stutter;

  struct {
    //int      max_anisotropy   = 4;
    bool     dump             = false;
    bool     log              = false;
    bool     force_mipmaps    = false;
    int      pixelate         = 0;
    bool     caching          = false;
  } textures;

  struct {
    bool dump       = false;
  } shaders;

  struct {
    int  num_frames = 0;
    bool shaders    = false;
    bool ui         = false; // General-purpose UI stuff
  } trace;

  struct {
    bool wrap_xinput     = false; // PENDING REMOVAL (obsolete w/ 3/16 patch)
    bool block_left_alt  = false;
    bool block_left_ctrl = false;
    bool block_windows   = false;
    bool block_all_keys  = false;

    bool cursor_mgmt     = true;
    int  cursor_timeout  = 1500;
    bool activate_on_kbd = true;
    bool alias_wasd      = true;
    int  gamepad_slot    = 0;
  } input;

  struct {
    std::wstring
            version            = PPRINNY_VER_STR;
    std::wstring
            injector           = L"OpenGL32.dll";
  } system;
};

extern pp_config_s config;

bool PPrinny_LoadConfig (std::wstring name         = L"PrettyPrinny");
void PPrinny_SaveConfig (std::wstring name         = L"PrettyPrinny",
                         bool         close_config = false);

#endif /* __PPRINNY__CONFIG_H__ */