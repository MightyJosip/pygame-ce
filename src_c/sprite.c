#define PYGAMEAPI_SPRITE_INTERNAL

#include "pygame.h"

#include "pgcompat.h"

static PyTypeObject pgSprite_Type;

static PyTypeObject pgAbstractGroup_Type;

#define pgSprite_Check(x) (PyObject_IsInstance(x, (PyObject *)&pgSprite_Type))

#define pgAbstractGroup_Check(x) (PyObject_IsInstance(x, (PyObject *)&pgAbstractGroup_Type))

typedef struct {
    PyObject_HEAD PyObject* groups;
    pgSurfaceObject* image;
    pgRectObject* rect;
} pgSpriteObject;

typedef struct {
    PyObject_HEAD PyObject* spritedict;
    PyObject* lostsprites;
} pgAbstractGroupObject;

static inline void
connect_sprite_and_group(pgSpriteObject* sprite, pgAbstractGroupObject* group)
{
    printf("Adding sprite to the group");
}

static void
sprite_inner_add()
{
    
}

static PyObject *
sprite_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    pgSpriteObject *self = (pgSpriteObject *)type->tp_alloc(type, 0);
    if (self) {
        PyObject *dict = PyDict_New();
        if (!dict) {
            RAISE(PyExc_TypeError, "Python internal error");
        }
        self->groups = dict;
        Py_XINCREF(self->groups);
        self->image = NULL;
        self->rect = NULL;
    }
    return (PyObject *)self;
}

static int
sprite_init(pgSpriteObject *self, PyObject *args, PyObject *kwargs)
{
    PyObject *group = NULL;
    Py_ssize_t i, length;
    length = PyTuple_Size(args);
    if (length) {
        for (i = 0; i < length; i++) {
            group = PySequence_GetItem(args, i);
            if (!pgAbstractGroup_Check(group)) {
                if (!PySequence_Check(group)) {
                    Py_DECREF(group);
                    PyErr_SetString(PyExc_TypeError, "argument must be group or sequence");
                    return -1;
                }
            }
            connect_sprite_and_group(self, (pgAbstractGroupObject *)group);
            Py_DECREF(group);
        }
    }
    return 0;
}

static void
sprite_dealloc(pgSpriteObject *self)
{
    Py_XDECREF(self->groups);
    Py_XDECREF(self->image);
    Py_XDECREF(self->rect);
    Py_TYPE(self)->tp_free((PyObject *)self);
}

static PyObject *
abstract_group_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    pgAbstractGroupObject *self = (pgAbstractGroupObject *)type->tp_alloc(type, 0);
    if (self) {
        PyObject *dict = PyDict_New();
        if (!dict) {
            RAISE(PyExc_TypeError, "Python internal error");
        }
        self->spritedict = dict;
        Py_XINCREF(self->spritedict);
        PyObject *list = PyList_New(0);
        if (!list) {
            RAISE(PyExc_TypeError, "Python internal error");
        }
        self->lostsprites = list;
        Py_XINCREF(self->lostsprites);
    }
    return (PyObject *)self;
}

static void
abstract_group_dealloc(pgAbstractGroupObject *self)
{
    Py_XDECREF(self->spritedict);
    Py_XDECREF(self->lostsprites);
    Py_TYPE(self)->tp_free((PyObject *)self);
}

static PyMethodDef sprite_methods[] = {{NULL, NULL, 0, NULL}};

static PyGetSetDef sprite_getset[] = {{NULL, 0, NULL, NULL, NULL}};

static PyMethodDef abstract_group_methods[] = {{NULL, NULL, 0, NULL}};

static PyGetSetDef abstract_group_getset[] = {{NULL, 0, NULL, NULL, NULL}};

static PyTypeObject pgSprite_Type = {
    PyVarObject_HEAD_INIT(NULL, 0).tp_name = "pygame._sprite.Sprite",
    .tp_basicsize = sizeof(pgSpriteObject),
    .tp_dealloc = (destructor)sprite_dealloc,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_doc = "",
    .tp_methods = sprite_methods,
    .tp_init = (initproc)sprite_init,
    .tp_new = (newfunc)sprite_new,
    .tp_getset = sprite_getset
};

static PyTypeObject pgAbstractGroup_Type = {
    PyVarObject_HEAD_INIT(NULL, 0).tp_name = "pygame._sprite.AbstractGroup",
    .tp_basicsize = sizeof(pgAbstractGroupObject),
    .tp_dealloc = (destructor)abstract_group_dealloc,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_doc = "",
    .tp_methods = abstract_group_methods,
    .tp_new = (newfunc)abstract_group_new,
    .tp_getset = abstract_group_getset
};

static PyMethodDef _sprite_methods[] = {{NULL, NULL, 0, NULL}};

MODINIT_DEFINE(_sprite)
{
    PyObject *module;

    static struct PyModuleDef _module = {PyModuleDef_HEAD_INIT,
                                         "_sprite",
                                         "docs_needed",
                                         -1,
                                         _sprite_methods,
                                         NULL,
                                         NULL,
                                         NULL,
                                         NULL};
    
    /* imported needed apis; Do this first so if there is an error
       the module is not loaded.
    */
    import_pygame_base();
    if (PyErr_Occurred()) {
        return NULL;
    }

    import_pygame_surface();
    if (PyErr_Occurred()) {
        return NULL;
    }

    import_pygame_rect();
    if (PyErr_Occurred()) {
        return NULL;
    }

    /* create the module */
    module = PyModule_Create(&_module);
    if (module == 0) {
        return NULL;
    }

    if (!PyModule_AddType(module, &pgSprite_Type) < 0) {
        Py_DECREF(module);
        return NULL;
    }

    if (!PyModule_AddType(module, &pgAbstractGroup_Type) < 0) {
        Py_DECREF(module);
        return NULL;
    }

    return module;
}