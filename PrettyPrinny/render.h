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

    extern HWND               hWndDevice;

    extern uint32_t           width;
    extern uint32_t           height;
    extern bool               fullscreen;

    extern HMODULE            user32_dll;
  }
}

typedef void (STDMETHODCALLTYPE *BMF_BeginBufferSwap_pfn)(void);

typedef HRESULT (STDMETHODCALLTYPE *BMF_EndBufferSwap_pfn)
(
  HRESULT   hr,
  IUnknown *device
);

#endif /* __PPRINNY__RENDER_H__ */