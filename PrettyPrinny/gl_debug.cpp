#include <Windows.h>
#include <gl/GL.h>

#include "log.h"

typedef GLbyte GLchar;
typedef GLvoid (APIENTRY *GLDEBUGPROC)(GLenum source,GLenum type,GLuint id,GLenum severity,GLsizei length,const GLchar *message,GLvoid *userParam);

#define GL_DEBUG_OUTPUT_SYNCHRONOUS                             0x8242
#define GL_DEBUG_SOURCE_API                                     0x8246
#define GL_DEBUG_TYPE_ERROR                                     0x824C
#define GL_DEBUG_SEVERITY_HIGH                                  0x9146

const char*
PP_GL_DEBUG_SOURCE_STR (GLenum source)
{
  static const char* sources [] = {
    "API",   "Window System", "Shader Compiler", "Third Party", "Application",
    "Other", "Unknown"
  };

  int str_idx =
    min ( source - GL_DEBUG_SOURCE_API,
            sizeof (sources) / sizeof (const char *) );

  return sources [str_idx];
}

const char*
PP_GL_DEBUG_TYPE_STR (GLenum type)
{
  static const char* types [] = {
    "Error",       "Deprecated Behavior", "Undefined Behavior", "Portability",
    "Performance", "Other",               "Unknown"
  };

  int str_idx =
    min ( type - GL_DEBUG_TYPE_ERROR,
            sizeof (types) / sizeof (const char *) );

  return types [str_idx];
}

const char*
PP_GL_DEBUG_SEVERITY_STR (GLenum severity)
{
  static const char* severities [] = {
    "High", "Medium", "Low", "Unknown"
  };

  int str_idx =
    min ( severity - GL_DEBUG_SEVERITY_HIGH,
            sizeof (severities) / sizeof (const char *) );

  return severities [str_idx];
}

GLvoid
WINAPI
PP_GL_ERROR_CALLBACK (GLenum        source,
                      GLenum        type,
                      GLuint        id,
                      GLenum        severity,
                      GLsizei       length,
                      const GLchar* message,
                      GLvoid*       userParam)
{
  dll_log.LogEx (false, L"\n");

  dll_log.Log (L"[ GL Debug ] OpenGL Error:");
  dll_log.Log (L"[ GL Debug ] =============");

  dll_log.Log (L"[ GL Debug ]  Object ID: %d", id);

  dll_log.Log (L"[ GL Debug ]  Severity:  %hs",
                 PP_GL_DEBUG_SEVERITY_STR (severity));

  dll_log.Log (L"[ GL Debug ]  Type:      %hs",
                 PP_GL_DEBUG_TYPE_STR     (type));

  dll_log.Log (L"[ GL Debug ]  Source:    %hs",
                 PP_GL_DEBUG_SOURCE_STR   (source));

  dll_log.Log (L"[ GL Debug ]  Message:   %hs\n",
                 message);
}

typedef PROC (APIENTRY *wglGetProcAddress_pfn)(LPCSTR);
extern wglGetProcAddress_pfn wglGetProcAddress_Original;


typedef GLvoid (WINAPI *glDebugMessageControl_pfn)(
        GLenum    source,
        GLenum    type,
        GLenum    severity,
        GLsizei   count,
  const GLuint   *ids,
        GLboolean enabled
);

typedef GLvoid (WINAPI *glDebugMessageCallback_pfn)(
  GLDEBUGPROC  callback,
  GLvoid      *userParam
);

glDebugMessageControl_pfn  glDebugMessageControl  = nullptr;
glDebugMessageCallback_pfn glDebugMessageCallback = nullptr;

void
PP_Init_glDebug (void)
{
  glDebugMessageControl  = 
    (glDebugMessageControl_pfn)
      wglGetProcAddress_Original ("glDebugMessageControl");

  glDebugMessageCallback =
    (glDebugMessageCallback_pfn)
      wglGetProcAddress_Original ("glDebugMessageCallback");

  // SUPER VERBOSE DEBUGGING!
  if (glDebugMessageControl != nullptr) {
    glEnable               (GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageControl  (GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
    glDebugMessageCallback ((GLDEBUGPROC)PP_GL_ERROR_CALLBACK, nullptr);
  }
}