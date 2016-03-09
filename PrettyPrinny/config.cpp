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
#include "config.h"
#include "parameter.h"
#include "ini.h"
#include "log.h"

static
  pp::INI::File* 
             dll_ini         = nullptr;
std::wstring PPRINNY_VER_STR = L"0.1.0";
pp_config_s config;

struct {
  pp::ParameterInt*     msaa_samples;
  pp::ParameterInt*     msaa_quality;
  pp::ParameterInt*     refresh_rate;
  pp::ParameterInt*     scene_res_x;
  pp::ParameterInt*     scene_res_y;
  pp::ParameterInt*     swap_interval;
  pp::ParameterBool*    fringe_removal;
} render;

struct {
  pp::ParameterBool*    bypass_intel_gl;
  pp::ParameterBool*    support_old_drivers;
  pp::ParameterBool*    debug_mode;
} compatibility;

struct {
  pp::ParameterBool*    borderless;
  pp::ParameterFloat*   foreground_fps;
  pp::ParameterFloat*   background_fps;
  pp::ParameterBool*    background_msaa;
  pp::ParameterBool*    center;
  pp::ParameterInt*     x_offset;
  pp::ParameterInt*     y_offset;
} window;

struct {
  pp::ParameterFloat*   tolerance;
} stutter;

struct {
  //pp::ParameterInt*     max_anisotropy;
} textures;

struct {
  pp::ParameterBool*    block_left_alt;
  pp::ParameterBool*    block_left_ctrl;
  pp::ParameterBool*    block_windows;
  pp::ParameterBool*    block_all_keys;

  pp::ParameterBool*    wrap_xinput;
} input;

struct {
  pp::ParameterStringW* injector;
  pp::ParameterStringW* version;
} sys;


pp::ParameterFactory g_ParameterFactory;


bool
PPrinny_LoadConfig (std::wstring name) {
  // Load INI File
  std::wstring full_name = name + L".ini";  
  dll_ini = new pp::INI::File ((wchar_t *)full_name.c_str ());

  bool empty = dll_ini->get_sections ().empty ();

  //
  // Create Parameters
  //
//  render.allow_background =
//    static_cast <pp::ParameterBool *>
//      (g_ParameterFactory.create_parameter <bool> (
//        L"Allow background rendering")
//      );
//  render.allow_background->register_to_ini (
//    dll_ini,
//      L"PrettyPrinny.Render",
//        L"AllowBackground" );

  render.msaa_samples =
    static_cast <pp::ParameterInt *>
      (g_ParameterFactory.create_parameter <int> (
        L"MSAA Sample Count")
      );
  render.msaa_samples->register_to_ini (
    dll_ini,
      L"PrettyPrinny.Render",
        L"MSAA_Samples" );

  render.msaa_quality =
    static_cast <pp::ParameterInt *>
      (g_ParameterFactory.create_parameter <int> (
        L"MSAA Quality")
      );
  render.msaa_quality->register_to_ini (
    dll_ini,
      L"PrettyPrinny.Render",
        L"MSAA_Quality" );

  render.refresh_rate =
    static_cast <pp::ParameterInt *>
      (g_ParameterFactory.create_parameter <int> (
        L"Refresh Rate Override; 0=Unchanged")
      );
  render.refresh_rate->register_to_ini (
    dll_ini,
      L"PrettyPrinny.Render",
        L"RefreshRate" );

  render.scene_res_x =
    static_cast <pp::ParameterInt *>
      (g_ParameterFactory.create_parameter <int> (
        L"Scene Resolution (X)")
      );
  render.scene_res_x->register_to_ini (
    dll_ini,
      L"PrettyPrinny.Render",
        L"SceneResX" );

  render.scene_res_y =
    static_cast <pp::ParameterInt *>
      (g_ParameterFactory.create_parameter <int> (
        L"Scene Resolution (Y)")
      );
  render.scene_res_y->register_to_ini (
    dll_ini,
      L"PrettyPrinny.Render",
        L"SceneResY" );

  render.swap_interval =
    static_cast <pp::ParameterInt *>
      (g_ParameterFactory.create_parameter <int> (
        L"Swap Interval (VSYNC)")
      );
  render.swap_interval->register_to_ini (
    dll_ini,
      L"PrettyPrinny.Render",
        L"SwapInterval" );

  render.fringe_removal = 
    static_cast <pp::ParameterBool *>
      (g_ParameterFactory.create_parameter <bool> (
        L"Remove Alpha Fringing (depth shenanigans)")
      );
  render.fringe_removal->register_to_ini (
    dll_ini,
      L"PrettyPrinny.Render",
        L"RemoveFringing" );

  //render.adaptive =
    //static_cast <pp::ParameterBool *>
      //(g_ParameterFactory.create_parameter <bool> (
        //L"Adaptive VSYNC")
      //);
  //render.adaptive->register_to_ini (
    //dll_ini,
      //L"PrettyPrinny.Render",
        //L"Adaptive" );



  compatibility.bypass_intel_gl =
    static_cast <pp::ParameterBool *>
      (g_ParameterFactory.create_parameter <bool> (
        L"Bypass Intel ICD")
      );
  compatibility.bypass_intel_gl->register_to_ini (
    dll_ini,
      L"PrettyPrinny.Compatibility",
        L"BypassIntelGL" );

  compatibility.support_old_drivers =
    static_cast <pp::ParameterBool *>
      (g_ParameterFactory.create_parameter <bool> (
        L"Disable Modern Features")
      );
  compatibility.support_old_drivers->register_to_ini (
    dll_ini,
      L"PrettyPrinny.Compatibility",
        L"ForceGL3_1Mode" );

  compatibility.debug_mode =
    static_cast <pp::ParameterBool *>
      (g_ParameterFactory.create_parameter <bool> (
        L"Disable Modern Features")
      );
  compatibility.debug_mode->register_to_ini (
    dll_ini,
      L"PrettyPrinny.Compatibility",
        L"Debug" );



  window.borderless =
    static_cast <pp::ParameterBool *>
      (g_ParameterFactory.create_parameter <bool> (
        L"Borderless mode")
      );
  window.borderless->register_to_ini (
    dll_ini,
      L"PrettyPrinny.Window",
        L"Borderless" );

  window.foreground_fps =
    static_cast <pp::ParameterFloat *>
      (g_ParameterFactory.create_parameter <float> (
        L"Foreground Framerate Limit")
      );
  window.foreground_fps->register_to_ini (
    dll_ini,
      L"PrettyPrinny.Window",
        L"ForegroundFPS" );

  window.background_fps =
    static_cast <pp::ParameterFloat *>
      (g_ParameterFactory.create_parameter <float> (
        L"Background Framerate Limit")
      );
  window.background_fps->register_to_ini (
    dll_ini,
      L"PrettyPrinny.Window",
        L"BackgroundFPS" );

  window.background_msaa =
    static_cast <pp::ParameterBool *>
      (g_ParameterFactory.create_parameter <bool> (
        L"MSAA Disable in BG")
      );
  window.background_msaa->register_to_ini (
    dll_ini,
      L"PrettyPrinny.Window",
        L"BackgroundMSAA" );

  window.center =
    static_cast <pp::ParameterBool *>
      (g_ParameterFactory.create_parameter <bool> (
        L"Center the window")
      );
  window.center->register_to_ini (
    dll_ini,
      L"PrettyPrinny.Window",
        L"Center" );

  window.x_offset =
    static_cast <pp::ParameterInt *>
      (g_ParameterFactory.create_parameter <int> (
        L"X Offset Coordinate")
      );
  window.x_offset->register_to_ini (
    dll_ini,
      L"PrettyPrinny.Window",
        L"XOffset" );

  window.y_offset =
    static_cast <pp::ParameterInt *>
      (g_ParameterFactory.create_parameter <int> (
        L"Y Offset Coordinate")
      );
  window.y_offset->register_to_ini (
    dll_ini,
      L"PrettyPrinny.Window",
        L"YOffset" );


  stutter.tolerance =
    static_cast <pp::ParameterFloat *>
      (g_ParameterFactory.create_parameter <float> (
        L"Framerate Limiter Variance")
      );
  stutter.tolerance->register_to_ini (
    dll_ini,
      L"PrettyPrinny.Stutter",
        L"Tolerance" );



  input.block_left_alt =
    static_cast <pp::ParameterBool *>
      (g_ParameterFactory.create_parameter <bool> (
        L"Block Left Alt Key")
      );
  input.block_left_alt->register_to_ini (
    dll_ini,
      L"PrettyPrinny.Input",
        L"BlockLeftAlt" );

  input.block_left_ctrl =
    static_cast <pp::ParameterBool *>
      (g_ParameterFactory.create_parameter <bool> (
        L"Block Left Ctrl Key")
      );
  input.block_left_ctrl->register_to_ini (
    dll_ini,
      L"PrettyPrinny.Input",
        L"BlockLeftCtrl" );

  input.block_windows =
    static_cast <pp::ParameterBool *>
      (g_ParameterFactory.create_parameter <bool> (
        L"Block Windows Key")
      );
  input.block_windows->register_to_ini (
    dll_ini,
      L"PrettyPrinny.Input",
        L"BlockWindows" );

  input.block_all_keys =
    static_cast <pp::ParameterBool *>
      (g_ParameterFactory.create_parameter <bool> (
        L"Block All Keys")
      );
  input.block_all_keys->register_to_ini (
    dll_ini,
      L"PrettyPrinny.Input",
        L"BlockAllKeys" );

  input.wrap_xinput = 
    static_cast <pp::ParameterBool *>
      (g_ParameterFactory.create_parameter <bool> (
        L"Wrap XInput around MMsystem")
      );
  input.wrap_xinput->register_to_ini (
    dll_ini,
      L"PrettyPrinny.Input",
        L"WrapXInput" );


  sys.version =
    static_cast <pp::ParameterStringW *>
      (g_ParameterFactory.create_parameter <std::wstring> (
        L"Software Version")
      );
  sys.version->register_to_ini (
    dll_ini,
      L"PrettyPrinny.System",
        L"Version" );

  sys.injector =
    static_cast <pp::ParameterStringW *>
      (g_ParameterFactory.create_parameter <std::wstring> (
        L"Injector DLL")
      );
  sys.injector->register_to_ini (
    dll_ini,
      L"PrettyPrinny.System",
        L"Injector" );


  //
  // Load Parameters
  //
  /////if (render.allow_background->load ())
    ////config.render.allow_background = render.allow_background->get_value ();

  if (render.msaa_samples->load ())
    config.render.msaa_samples = render.msaa_samples->get_value ();

  if (render.msaa_quality->load ())
    config.render.msaa_quality = render.msaa_quality->get_value ();

  if (render.refresh_rate->load ())
    config.render.refresh_rate = render.refresh_rate->get_value ();

  if (render.scene_res_x->load ())
    config.render.scene_res_x = render.scene_res_x->get_value ();

  if (render.scene_res_y->load ())
    config.render.scene_res_y = render.scene_res_y->get_value ();

  if (render.swap_interval->load ())
    config.render.swap_interval = render.swap_interval->get_value ();

  if (render.fringe_removal->load ())
    config.render.fringe_removal = render.fringe_removal->get_value ();

  //if (render.adaptive->load ())
    //config.render.adaptive = render.adaptive->get_value ();


  if (compatibility.bypass_intel_gl->load ())
    config.compatibility.bypass_intel_gl = compatibility.bypass_intel_gl->get_value ();

  if (compatibility.support_old_drivers->load ())
    config.compatibility.support_old_drivers = compatibility.support_old_drivers->get_value ();

  if (compatibility.debug_mode->load ())
    config.compatibility.debug_mode = compatibility.debug_mode->get_value ();



  if (window.borderless->load ())
    config.window.borderless = window.borderless->get_value ();

  if (window.foreground_fps->load ())
    config.window.foreground_fps = window.foreground_fps->get_value ();

  if (window.background_fps->load ())
    config.window.background_fps = window.background_fps->get_value ();

  if (window.background_msaa->load ())
    config.window.disable_bg_msaa = (! window.background_msaa->get_value ());

  if (window.center->load ())
    config.window.center = window.center->get_value ();

  if (window.x_offset->load ())
    config.window.x_offset = window.x_offset->get_value ();

  if (window.y_offset->load ())
    config.window.y_offset = window.y_offset->get_value ();


  if (stutter.tolerance->load ())
    config.stutter.tolerance = stutter.tolerance->get_value ();


//  if (textures.log->load ())
//    config.textures.log = textures.log->get_value ();

#if 0
  if (textures.cleanup->load ())
    config.textures.cleanup = textures.cleanup->get_value ();
#endif

  //if (input.block_left_alt->load ())
    //config.input.block_left_alt = input.block_left_alt->get_value ();

  //if (input.block_left_ctrl->load ())
    //config.input.block_left_ctrl = input.block_left_ctrl->get_value ();

  //if (input.block_windows->load ())
    //config.input.block_windows = input.block_windows->get_value ();

  //if (input.block_all_keys->load ())
    //config.input.block_all_keys = input.block_all_keys->get_value ();

  if (input.wrap_xinput->load ())
    config.input.wrap_xinput = input.wrap_xinput->get_value ();

  if (sys.version->load ())
    config.system.version = sys.version->get_value ();

  if (sys.injector->load ())
    config.system.injector = sys.injector->get_value ();

  if (empty)
    return false;

  return true;
}

void
PPrinny_SaveConfig (std::wstring name, bool close_config) {
  //render.allow_background->set_value  (config.render.allow_background);
  //render.allow_background->store      ();

#if 0
  render.msaa_samples->set_value      (config.render.msaa_samples);
  render.msaa_samples->store          ();

  render.msaa_quality->set_value      (config.render.msaa_quality);
  render.msaa_quality->store          ();

  render.refresh_rate->set_value      (config.render.refresh_rate);
  render.refresh_rate->store          ();
#endif

  render.scene_res_x->set_value        (config.render.scene_res_x);
  render.scene_res_x->store            ();

  render.scene_res_y->set_value        (config.render.scene_res_y);
  render.scene_res_y->store            ();

  render.swap_interval->set_value      (config.render.swap_interval);
  render.swap_interval->store          ();

  render.fringe_removal->set_value     (config.render.fringe_removal);
  render.fringe_removal->store         ();

  //render.adaptive->set_value          (config.render.adaptive);
  //render.adaptive->store              ();


  compatibility.bypass_intel_gl->set_value
                                       (config.compatibility.bypass_intel_gl);
  compatibility.bypass_intel_gl->store ();



#if 0
  window.borderless->set_value        (config.window.borderless);
  window.borderless->store            ();
#endif

#if 0
  window.foreground_fps->set_value    (config.window.foreground_fps);
  window.foreground_fps->store        ();

  window.background_fps->set_value    (config.window.background_fps);
  window.background_fps->store        ();
#endif

#if 0
  window.center->set_value            (config.window.center);
  window.center->store                ();

  window.x_offset->set_value          (config.window.x_offset);
  window.x_offset->store              ();

  window.y_offset->set_value          (config.window.y_offset);
  window.y_offset->store              ();
#endif

  stutter.tolerance->set_value        (config.stutter.tolerance);
  stutter.tolerance->store            ();



  //
  // This gets set dynamically depending on certain settings, don't
  //   save this...
  //
  //textures.max_anisotropy->set_value (config.textures.max_anisotropy);
  //textures.max_anisotropy->store     ();

//  textures.dump->set_value           (config.textures.dump);
//  textures.dump->store               ();

//  textures.log->set_value            (config.textures.log);
//  textures.log->store                ();


  //input.block_left_alt->set_value  (config.input.block_left_alt);
  //input.block_left_alt->store      ();

  //input.block_left_ctrl->set_value (config.input.block_left_ctrl);
  //input.block_left_ctrl->store     ();

  //input.block_windows->set_value   (config.input.block_windows);
  //input.block_windows->store       ();

  //input.block_all_keys->set_value  (config.input.block_all_keys);
  //input.block_all_keys->store      ();

  input.wrap_xinput->set_value      (config.input.wrap_xinput);
  input.wrap_xinput->store          ();

  sys.version->set_value  (PPRINNY_VER_STR);
  sys.version->store      ();

  sys.injector->set_value (config.system.injector);
  sys.injector->store     ();

  dll_ini->write (name + L".ini");

  if (close_config) {
    if (dll_ini != nullptr) {
      delete dll_ini;
      dll_ini = nullptr;
    }
  }
}