#ifndef PYGAME_C_API_H
#define PYGAME_C_API_H

#include "pygame.h"

// Simplified visibility control - just use static linking approach
#if defined(_WIN32)
#define API_EXPORT __declspec(dllexport)
#else
#define API_EXPORT __attribute__((visibility("default")))
#endif

/* RWOBJECT */
// Only export when building the module that contains the implementation
#ifdef PYGAMEAPI_RWOBJECT_INTERNAL
#define RWOBJECT_API API_EXPORT
#else
#define RWOBJECT_API extern
#endif

RWOBJECT_API SDL_RWops *
pgRWops_FromObject(PyObject *obj, char **extptr);

RWOBJECT_API int
pgRWops_IsFileObject(SDL_RWops *rw);

RWOBJECT_API PyObject *
pg_EncodeFilePath(PyObject *obj, PyObject *eclass);

RWOBJECT_API PyObject *
pg_EncodeString(PyObject *obj, const char *encoding, const char *errors,
                PyObject *eclass);

RWOBJECT_API SDL_RWops *
pgRWops_FromFileObject(PyObject *obj);

/* SURFLOCK */
// Only export when building the module that contains the implementation
#ifdef PYGAMEAPI_SURFLOCK_INTERNAL
#define SURFLOCK_API API_EXPORT
#else
#define SURFLOCK_API extern
#endif

SURFLOCK_API void
pgSurface_Prep(pgSurfaceObject *surfobj);

SURFLOCK_API void
pgSurface_Unprep(pgSurfaceObject *surfobj);

SURFLOCK_API int
pgSurface_Lock(pgSurfaceObject *surfobj);

SURFLOCK_API int
pgSurface_Unlock(pgSurfaceObject *surfobj);

SURFLOCK_API int
pgSurface_LockBy(pgSurfaceObject *surfobj, PyObject *lockobj);

SURFLOCK_API int
pgSurface_UnlockBy(pgSurfaceObject *surfobj, PyObject *lockobj);

/* JOYSTICK */
// Only export when building the module that contains the implementation
#ifdef PYGAMEAPI_JOYSTICK_INTERNAL
#define JOYSTICK_API API_EXPORT
#else
#define JOYSTICK_API extern
#endif

JOYSTICK_API PyTypeObject pgJoystick_Type;

JOYSTICK_API PyObject *
pgJoystick_New(int id);

JOYSTICK_API int
pgJoystick_GetDeviceIndexByInstanceID(int instance_id);

#endif /* !PYGAME_C_API_H */
