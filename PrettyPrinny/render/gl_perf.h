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
#include <gl/GL.h>
#include <cstdint>
#include <string>

#define GL_QUERY_RESULT           0x8866
#define GL_QUERY_RESULT_AVAILABLE 0x8867
#define GL_QUERY_RESULT_NO_WAIT   0x9194

#define GL_TIMESTAMP              0x8E28
#define GL_TIME_ELAPSED           0x88BF

typedef int64_t  GLint64;
typedef uint64_t GLuint64;

typedef GLvoid    (WINAPI *glGenQueries_pfn)   (GLsizei n​,       GLuint *ids​);
typedef GLvoid    (WINAPI *glDeleteQueries_pfn)(GLsizei n​, const GLuint *ids​);

typedef GLvoid    (WINAPI *glBeginQuery_pfn)       (GLenum target​, GLuint id​);
typedef GLvoid    (WINAPI *glBeginQueryIndexed_pfn)(GLenum target​, GLuint index​, GLuint id​);
typedef GLvoid    (WINAPI *glEndQuery_pfn)         (GLenum target​);

typedef GLboolean (WINAPI *glIsQuery_pfn)            (GLuint id​);
typedef GLvoid    (WINAPI *glQueryCounter_pfn)       (GLuint id, GLenum target);
typedef GLvoid    (WINAPI *glGetQueryObjectiv_pfn)   (GLuint id, GLenum pname, GLint    *params);
typedef GLvoid    (WINAPI *glGetQueryObjecti64v_pfn) (GLuint id, GLenum pname, GLint64  *params);
typedef GLvoid    (WINAPI *glGetQueryObjectui64v_pfn)(GLuint id, GLenum pname, GLuint64 *params);

extern glGenQueries_pfn          glGenQueries;
extern glDeleteQueries_pfn       glDeleteQueries;

extern glBeginQuery_pfn          glBeginQuery;
extern glBeginQueryIndexed_pfn   glBeginQueryIndexed;
extern glEndQuery_pfn            glEndQuery;

extern glIsQuery_pfn             glIsQuery;
extern glQueryCounter_pfn        glQueryCounter;
extern glGetQueryObjectiv_pfn    glGetQueryObjectiv;
extern glGetQueryObjecti64v_pfn  glGetQueryObjecti64v;
extern glGetQueryObjectui64v_pfn glGetQueryObjectui64v;

namespace pp
{
  namespace GLPerf
  {
    bool Init     (void);
    bool Shutdown (void);

    class TimerQuery {
    public:
      TimerQuery (const wchar_t* wszName);
     ~TimerQuery (GLvoid);

      std::
        wstring getName            (GLvoid);

      GLvoid    requestTimestamp   (GLvoid);
      //GLvoid    endQuery           (GLvoid);

      GLboolean isFinished         (GLvoid);
      GLboolean isReady            (GLvoid);

      // Will return immediately if not ready
      GLboolean getResulIfFinished (GLuint64* pResult);
      GLvoid    getResult          (GLuint64* pResult); // Will BLOCK

    protected:
      GLboolean    finished_; // Has GL given us the data yet?
      GLboolean    ready_;    // Has the old value has been retrieved yet?

      GLuint64     timestamp_;// Cached result

    private:
      GLuint       query_;    // GL Name of Query Object
      std::wstring name_;     // Human-readable name
    };

    extern bool time_tasks;
    extern bool HAS_timer_query;


#define BEGIN_TASK_TIMING if (pp::GLPerf::HAS_timer_query && pp::GLPerf::time_tasks) {
#define IF_NO_TASK_TIMING } else {
#define END_TASK_TIMING   }

    enum {
      TASK_BEGIN_FRAME    = 0,

      TASK_BEGIN_SCENE,
      TASK_MSAA0,
      TASK_MSAA1,
      TASK_MSAA2,
      TASK_MSAA3,
      TASK_BEGIN_POSTPROC,
      TASK_END_POSTPROC,
      TASK_BEGIN_UI,
      TASK_END_UI,

      TASK_END_FRAME,

      TASK_STAGES
    };

    extern TimerQuery* timers [];
  }
}