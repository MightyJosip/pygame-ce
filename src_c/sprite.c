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
    char has_layer;
    int layer;
} pgSpriteObject;

typedef struct {
    PyObject_HEAD PyObject* spritedict;
    PyObject* lostsprites;
} pgAbstractGroupObject;

#define CONNECT_SPRITE_AND_GROUP_SEQUENCE(name, function)                                                                               \
    static void                                                                                                                         \
    name##_sprite_and_group_sequence(pgSpriteObject* sprite, PyObject *sequence, int *result)                                           \
    {                                                                                                                                   \
        PyObject *group = NULL;                                                                                                         \
        Py_ssize_t i, length;                                                                                                           \
        length = PySequence_Size(sequence);                                                                                             \
        if (length) {                                                                                                                   \
            for (i = 0; i < length; i++) {                                                                                              \
                if (*result != -1) {                                                                                                    \
                    group = PySequence_GetItem(sequence, i);                                                                            \
                    if (pgAbstractGroup_Check(group)) {                                                                                 \
                        function;                                                                                                       \
                    }                                                                                                                   \
                    else {                                                                                                              \
                        if (PySequence_Check(group) && !PyUnicode_Check(group) && !PyBytes_Check(group) && !PyByteArray_Check(group)) { \
                            name##_sprite_and_group_sequence(sprite, group, result);                                                    \
                        }                                                                                                               \
                        else {                                                                                                          \
                            PyErr_SetString(PyExc_TypeError, "each argument must be group or sequence");                                \
                            Py_XDECREF(group);                                                                                          \
                            *result = -1;                                                                                               \
                        }                                                                                                               \
                    }                                                                                                                   \
                    Py_XDECREF(group);                                                                                                  \
                }                                                                                                                       \
            }                                                                                                                           \
        }                                                                                                                               \
    }

static inline const char *
object_get_name(PyObject *self)
{
    return PyUnicode_AsUTF8(PyObject_GetAttrString((PyObject *)Py_TYPE(self), "__name__"));
}

static inline void
add_sprite_to_group(pgAbstractGroupObject* group, pgSpriteObject* sprite)
{
    PyDict_SetItem(group->spritedict, (PyObject *)sprite, Py_None);
}

static inline void
add_group_to_sprite(pgSpriteObject* sprite, pgAbstractGroupObject* group)
{
    PyDict_SetItem(sprite->groups, (PyObject *)group, PyLong_FromLong(0));
}

static inline void
remove_sprite_from_group(pgAbstractGroupObject* group, pgSpriteObject* sprite)
{
    PyDict_DelItem(group->spritedict, (PyObject *)sprite);
}

static inline void
remove_group_from_sprite(pgSpriteObject* sprite, pgAbstractGroupObject* group)
{
    PyDict_DelItem(sprite->groups, (PyObject *)group);
}

static inline Py_ssize_t
sprite_num_groups(pgSpriteObject *self)
{
    return PyDict_Size(self->groups);
}

static inline void
add_sprite_and_group(pgSpriteObject* sprite, pgAbstractGroupObject* group)
{
    if (!PyDict_Contains(sprite->groups, (PyObject *)group)) {
        add_sprite_to_group(group, sprite);
        add_group_to_sprite(sprite, group);
    }
}

static inline void
remove_sprite_and_group(pgSpriteObject* sprite, pgAbstractGroupObject* group)
{
    if (PyDict_Contains(sprite->groups, (PyObject *)group)) {
        remove_sprite_from_group(group, sprite);
        remove_group_from_sprite(sprite, group);
    }
}

CONNECT_SPRITE_AND_GROUP_SEQUENCE(add, add_sprite_and_group(sprite, (pgAbstractGroupObject *)group))

CONNECT_SPRITE_AND_GROUP_SEQUENCE(remove, remove_sprite_and_group(sprite, (pgAbstractGroupObject *)group))

static PyObject *
sprite_add(pgSpriteObject *self, PyObject *args)
{
    int result = 0;
    add_sprite_and_group_sequence(self, args, &result);
    if (result == -1) {
        return NULL;
    }
    Py_RETURN_NONE;
}

static PyObject *
sprite_remove(pgSpriteObject *self, PyObject *args)
{
    int result = 0;
    remove_sprite_and_group_sequence(self, args, &result);
    if (result == -1) {
        return NULL;
    }
    Py_RETURN_NONE;
}

static PyObject *
sprite_update(pgSpriteObject *self, PyObject *args, PyObject *kwargs)
{
    Py_RETURN_NONE;
}

static PyObject *
sprite_kill(pgSpriteObject *self, PyObject *_null)
{
    PyObject *key, *value;
    Py_ssize_t pos = 0;
    while (PyDict_Next(self->groups, &pos, &key, &value)) {
        remove_sprite_from_group((pgAbstractGroupObject *) key, self);
    }
    PyDict_Clear(self->groups);
    Py_RETURN_NONE;
}

static PyObject *
sprite_groups(pgSpriteObject *self, PyObject *_null)
{
    return PyDict_Keys(self->groups);
}

static PyObject *
sprite_alive(pgSpriteObject *self, PyObject *_null)
{
    return PyBool_FromLong(PyObject_IsTrue(self->groups));
}

static PyObject *
sprite_get_image(pgSpriteObject *self, void *closure)
{
    if (self->image == NULL) {
        Py_RETURN_NONE;
    }
    Py_XINCREF(self->image);
    return (PyObject *)self->image;
}

static int
sprite_set_image(pgSpriteObject *self, PyObject *arg, void *closure)
{
    if (pgSurface_Check(arg)) {
        self->image = (pgSurfaceObject *)arg;
        Py_XINCREF(self->image);
        return 0;
    }
    else if (Py_IsNone(arg)) {
        if (self->image != NULL) {
            Py_XDECREF(self->image);
            self->image = NULL;
        }
        return 0;
    }
    else {
        RAISERETURN(PyExc_TypeError, "image must be Surface object or None",
                    -1);
    }
}

static PyObject *
sprite_get_rect(pgSpriteObject *self, void *closure)
{
    if (self->rect == NULL) {
        Py_RETURN_NONE;
    }
    Py_XINCREF(self->rect);
    return (PyObject *)self->rect;
}

static int
sprite_set_rect(pgSpriteObject *self, PyObject *arg, void *closure)
{
    if (pgRect_Check(arg)) {
        self->rect = (pgRectObject *)arg;
        Py_XINCREF(self->rect);
        return 0;
    }
    else if (Py_IsNone(arg)) {
        if (self->rect != NULL) {
            Py_XDECREF(self->rect);
            self->rect = NULL;
        }
        return 0;
    }
    else {
        RAISERETURN(PyExc_TypeError, "rect must be Rect object or None",
                    -1);
    }
}

static PyObject *
sprite_get_layer(pgSpriteObject *self, void *closure)
{
    if (!self->has_layer) {
        RAISERETURN(PyExc_AttributeError, "Sprite doesn't have layer attribute set", NULL);
    }
    return PyLong_FromLong(self->layer);
}

static int
sprite_set_layer(pgSpriteObject *self, PyObject *arg, void *closure)
{
    if (!PyObject_IsTrue(self->groups)) {
        if (!pg_IntFromObj(arg, &self->layer)) {
            RAISERETURN(PyExc_TypeError, "layer argument must be a number", -1);
        }
        self->has_layer = 1;
        return 0;
    }
    else {
        RAISERETURN(PyExc_AttributeError, "Can't set layer directly after adding to group. Use group.change_layer(sprite, new_layer) instead.", -1);
    }
}

static PyObject *
sprite_repr(pgSpriteObject *self)
{
    return PyUnicode_FromFormat("<%s Sprite(in %lld groups)>",
                                object_get_name((PyObject *)self),
                                sprite_num_groups(self));
}

static PyObject *
sprite_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
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
        self->has_layer = 0;
    }
    return (PyObject *)self;
}

static int
sprite_init(pgSpriteObject *self, PyObject *args, PyObject *kwargs)
{
    int result = 0;
    add_sprite_and_group_sequence(self, args, &result);
    return result;
}

static void
sprite_dealloc(pgSpriteObject *self)
{
    Py_XDECREF(self->groups);
    Py_XDECREF(self->image);
    Py_XDECREF(self->rect);
    Py_TYPE(self)->tp_free((PyObject *)self);
}

static inline Py_ssize_t
abstract_group_num_sprites(pgAbstractGroupObject *self)
{
    return PyDict_Size(self->spritedict);
}

static PyObject *
abstract_group_repr(pgAbstractGroupObject *self)
{
    return PyUnicode_FromFormat("<AbstractGroup(%lld sprites)>", abstract_group_num_sprites(self));
}

static PyObject *
abstract_group_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
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

static int
abstract_group_init(pgAbstractGroupObject *self, PyObject *args, PyObject *kwargs)
{
    static char *keywords[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "", keywords)) {
        return -1;
    }
    return 0;
}

static void
abstract_group_dealloc(pgAbstractGroupObject *self)
{
    Py_XDECREF(self->spritedict);
    Py_XDECREF(self->lostsprites);
    Py_TYPE(self)->tp_free((PyObject *)self);
}

static PyMethodDef sprite_methods[] = {
    {"add", (PyCFunction)sprite_add, METH_VARARGS, ""},
    {"remove", (PyCFunction)sprite_remove, METH_VARARGS, ""},
    {"update", (PyCFunction)sprite_update, METH_VARARGS | METH_KEYWORDS, ""},
    {"kill", (PyCFunction)sprite_kill, METH_NOARGS, ""},
    {"groups", (PyCFunction)sprite_groups, METH_NOARGS, ""},
    {"alive", (PyCFunction)sprite_alive, METH_NOARGS, ""},
    {NULL, NULL, 0, NULL}
};

static PyGetSetDef sprite_getset[] = {
    {"image", (getter)sprite_get_image, (setter)sprite_set_image, "", NULL},
    {"rect", (getter)sprite_get_rect, (setter)sprite_set_rect, "", NULL},
    {"layer", (getter)sprite_get_layer, (setter)sprite_set_layer, "", NULL},
    {NULL, 0, NULL, NULL, NULL}
};

static PyMethodDef abstract_group_methods[] = {{NULL, NULL, 0, NULL}};

static PyGetSetDef abstract_group_getset[] = {{NULL, 0, NULL, NULL, NULL}};

static PyTypeObject pgSprite_Type = {
    PyVarObject_HEAD_INIT(NULL, 0).tp_name = "pygame._sprite.Sprite",
    .tp_basicsize = sizeof(pgSpriteObject),
    .tp_dealloc = (destructor)sprite_dealloc,
    .tp_repr = (reprfunc)sprite_repr,
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
    .tp_repr = (reprfunc)abstract_group_repr,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_doc = "",
    .tp_methods = abstract_group_methods,
    .tp_init = (initproc)abstract_group_init,
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