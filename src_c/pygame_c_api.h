#ifndef PYGAME_C_API_H
#define PYGAME_C_API_H

#include "pygame.h"

// Simplified visibility control - just use static linking approach
#if defined(_WIN32)
  #define API_EXPORT __declspec(dllexport)
#else
  #define API_EXPORT __attribute__((visibility("default")))
#endif

// Only export when building the module that contains the implementation
#ifdef PYGAMEAPI_RWOBJECT_INTERNAL
  #define RWOPS_API API_EXPORT
#else
  #define RWOPS_API extern
#endif

RWOPS_API SDL_RWops * pgRWops_FromObject(PyObject *obj, char **extptr);

RWOPS_API int pgRWops_IsFileObject(SDL_RWops *rw);

RWOPS_API PyObject * pg_EncodeFilePath(PyObject *obj, PyObject *eclass);

RWOPS_API PyObject * pg_EncodeString(PyObject *obj, const char *encoding, const char *errors, PyObject *eclass);

RWOPS_API SDL_RWops * pgRWops_FromFileObject(PyObject *obj);

#endif  /* !PYGAME_C_API_H */