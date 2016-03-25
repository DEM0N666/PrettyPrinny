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

#include "gl_perf.h"
#include "../render.h"

glGenQueries_pfn          glGenQueries          = nullptr;
glDeleteQueries_pfn       glDeleteQueries       = nullptr;

glBeginQuery_pfn          glBeginQuery          = nullptr;
glBeginQueryIndexed_pfn   glBeginQueryIndexed   = nullptr;
glEndQuery_pfn            glEndQuery            = nullptr;

glIsQuery_pfn             glIsQuery             = nullptr;
glQueryCounter_pfn        glQueryCounter        = nullptr;
glGetQueryObjectiv_pfn    glGetQueryObjectiv    = nullptr;
glGetQueryObjecti64v_pfn  glGetQueryObjecti64v  = nullptr;
glGetQueryObjectui64v_pfn glGetQueryObjectui64v = nullptr;

extern wglGetProcAddress_pfn wglGetProcAddress_Log;

bool
pp::GLPerf::Init (void)
{
  glGenQueries =
    (glGenQueries_pfn)
      wglGetProcAddress_Log ("glGenQueries");

  glDeleteQueries =
    (glDeleteQueries_pfn)
      wglGetProcAddress_Log ("glDeleteQueries");


  glBeginQuery =
    (glBeginQuery_pfn)
      wglGetProcAddress_Log ("glBeginQuery");

  glBeginQueryIndexed =
    (glBeginQueryIndexed_pfn)
      wglGetProcAddress_Log ("glBeginQueryIndexed");

  glEndQuery =
    (glEndQuery_pfn)
      wglGetProcAddress_Log ("glEndQuery");


  glIsQuery =
    (glIsQuery_pfn)
      wglGetProcAddress_Log ("glIsQuery");

  glQueryCounter =
    (glQueryCounter_pfn)
      wglGetProcAddress_Log ("glQueryCounter");


  glGetQueryObjectiv =
    (glGetQueryObjectiv_pfn)
      wglGetProcAddress_Log ("glGetQueryObjectiv");

  glGetQueryObjecti64v =
    (glGetQueryObjecti64v_pfn)
      wglGetProcAddress_Log ("glGetQueryObjecti64v");

  glGetQueryObjectui64v =
    (glGetQueryObjectui64v_pfn)
      wglGetProcAddress_Log ("glGetQueryObjectui64v");

  if ( glGenQueries && glDeleteQueries     &&
       glBeginQuery && glBeginQueryIndexed && glEndQuery &&
       glIsQuery    && glQueryCounter      &&

       glGetQueryObjectiv    && glGetQueryObjecti64v     &&
       glGetQueryObjectui64v )
    return true;

  // OpenGL ICD does not support all of this stuff, what a shame.
  return false;
}

bool
pp::GLPerf::Shutdown (void)
{
  return true;
}

using namespace pp::GLPerf;

TimerQuery::TimerQuery (const wchar_t* wszName)
{
  finished_  = GL_FALSE;
  ready_     = GL_TRUE;

  timestamp_ = 0ULL;

  name_      = wszName;

  glGenQueries (1, &query_);
}

TimerQuery::~TimerQuery (GLvoid)
{
  if (glIsQuery (query_)) {
    glDeleteQueries (1, &query_);
    query_ = GL_NONE;
  }

  finished_ = GL_TRUE;
  ready_    = GL_FALSE;
}

std::wstring
TimerQuery::getName (GLvoid)
{
  return name_;
}

GLvoid
TimerQuery::requestTimestamp (GLvoid)
{
  glQueryCounter (query_, GL_TIMESTAMP);

  //glBeginQuery (GL_TIME_ELAPSED, query_);
  ready_    = GL_FALSE;
  finished_ = GL_FALSE;
}

#if 0
GLvoid
TimerQuery::endQuery (GLvoid)
{
  glEndQuery (GL_TIME_ELAPSED);
  active_ = GL_FALSE;
}

GLboolean
TimerQuery::isActive (GLvoid)
{
  return active_;
}
#endif

GLboolean
TimerQuery::isReady (GLvoid)
{
  return ready_;
}

GLboolean
TimerQuery::isFinished (GLvoid)
{
  GLint finished;

  glGetQueryObjectiv (query_, GL_QUERY_RESULT_AVAILABLE, &finished);

  finished_ = (finished > 0);

  if (finished_)
    return GL_TRUE;

  return GL_FALSE;
}

GLvoid
TimerQuery::getResult (GLuint64* pResult)
{
  glGetQueryObjectui64v (query_, GL_QUERY_RESULT, pResult);
  ready_ = GL_TRUE;
}

GLboolean
TimerQuery::getResulIfFinished (GLuint64* pResult)
{
  if (isFinished ()) {
    getResult (pResult);
    return GL_TRUE;
  }

  return GL_FALSE;
}

bool pp::GLPerf::time_tasks      = false;
bool pp::GLPerf::HAS_timer_query = false;

TimerQuery* pp::GLPerf::timers [] = {
  nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
  nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
  nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
  nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
  nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
  nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
  nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
  nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
  nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr

};