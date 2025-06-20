/*
  pygame-ce - Python Game Library
  Copyright (C) 2008 Marcus von Appen

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Library General Public
  License as published by the Free Software Foundation; either
  version 2 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Library General Public License for more details.

  You should have received a copy of the GNU Library General Public
  License along with this library; if not, write to the Free
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

*/

/* The follow bug was reported for the pygame.math module:
 *
 * Adjust gcc 4.4 optimization for floating point on x86-32 PCs running Linux.
 * This addresses bug 67:
 * https://github.com/pygame-community/pygame-ce/issues/67
 * With this option, floats have consistent precision regardless of optimize
 * level.
 *
 * Apparently, the same problem plagues pygame.color, as it failed the
 * test_hsva__all_elements_within_limits and
 * test_hsva__sanity_testing_converted_should_not_raise test cases due
 * to slight, on the order of 10e-14, discrepancies in calculated double
 * values.
 */
#if defined(__GNUC__) && defined(__linux__) && defined(__i386__) && \
    __SIZEOF_POINTER__ == 4 && __GNUC__ == 4 && __GNUC_MINOR__ >= 4
#pragma GCC optimize("float-store")
#endif

#define PYGAMEAPI_COLOR_INTERNAL

#include "doc/color_doc.h"

#include "pygame.h"

#include "pgcompat.h"

#include <ctype.h>

static inline double
pg_round(double d)
{
#if (!defined(__STDC_VERSION__) || __STDC_VERSION__ < 199901L) && \
    !defined(round)
    return (((d < 0) ? (ceil((d)-0.5)) : (floor((d) + 0.5))));
#else
    return round(d);
#endif
}

typedef enum { TRISTATE_SUCCESS, TRISTATE_FAIL, TRISTATE_ERROR } tristate;

static PyObject *_COLORDICT = NULL;

static int
_get_double(PyObject *, double *);
static int
_get_color_int_component(PyObject *, Uint8 *);
static int
_hextoint(char *, Uint8 *);
static tristate
_hexcolor(PyObject *, Uint8 *);
static int
_coerce_obj(PyObject *, Uint8 *);

static pgColorObject *
_color_new_internal(PyTypeObject *, const Uint8 *);
static pgColorObject *
_color_new_internal_length(PyTypeObject *, const Uint8 *, Uint8);

static PyObject *
_color_new(PyTypeObject *, PyObject *, PyObject *);
static int
_color_init(pgColorObject *, PyObject *, PyObject *);
static void
_color_dealloc(pgColorObject *);
static PyObject *
_color_repr(pgColorObject *);
static PyObject *
_color_iter(pgColorObject *);
static PyObject *
_color_normalize(pgColorObject *, PyObject *);
static PyObject *
_color_correct_gamma(pgColorObject *, PyObject *);
static PyObject *
_color_set_length(pgColorObject *, PyObject *);
static PyObject *
_color_lerp(pgColorObject *, PyObject *, PyObject *);
static PyObject *
_color_grayscale(pgColorObject *, PyObject *);
static PyObject *
_premul_alpha(pgColorObject *, PyObject *);
static PyObject *
_color_update(pgColorObject *self, PyObject *const *args, Py_ssize_t nargs);

/* Generic functions */
static PyObject *
_color_from_space(char *space, PyObject *args);
#define COLOR_FROM_SPACE(space)                                              \
    static PyObject *_color_from_##space(PyTypeObject *self, PyObject *args) \
    {                                                                        \
        return _color_from_space(#space, args);                              \
    }
COLOR_FROM_SPACE(hsva);
COLOR_FROM_SPACE(hsla);
COLOR_FROM_SPACE(cmy);
COLOR_FROM_SPACE(i1i2i3);
COLOR_FROM_SPACE(normalized);
COLOR_FROM_SPACE(hex);
#undef COLOR_FROM_SPACE

/* Getters/setters */
static PyObject *
_color_get_r(pgColorObject *, void *);
static int
_color_set_r(pgColorObject *, PyObject *, void *);
static PyObject *
_color_get_g(pgColorObject *, void *);
static int
_color_set_g(pgColorObject *, PyObject *, void *);
static PyObject *
_color_get_b(pgColorObject *, void *);
static int
_color_set_b(pgColorObject *, PyObject *, void *);
static PyObject *
_color_get_a(pgColorObject *, void *);
static int
_color_set_a(pgColorObject *, PyObject *, void *);
static PyObject *
_color_get_hsva(pgColorObject *, void *);
static int
_color_set_hsva(pgColorObject *, PyObject *, void *);
static PyObject *
_color_get_hsla(pgColorObject *, void *);
static int
_color_set_hsla(pgColorObject *, PyObject *, void *);
static PyObject *
_color_get_i1i2i3(pgColorObject *, void *);
static int
_color_set_i1i2i3(pgColorObject *, PyObject *, void *);
static PyObject *
_color_get_cmy(pgColorObject *, void *);
static int
_color_set_cmy(pgColorObject *, PyObject *, void *);
static PyObject *
_color_get_normalized(pgColorObject *, void *);
static int
_color_set_normalized(pgColorObject *, PyObject *, void *);
static PyObject *
_color_get_hex(pgColorObject *, void *);
static int
_color_set_hex(pgColorObject *, PyObject *, void *);
static PyObject *
_color_get_arraystruct(pgColorObject *, void *);

/* Number protocol methods */
static PyObject *
_color_add(PyObject *, PyObject *);
static PyObject *
_color_sub(PyObject *, PyObject *);
static PyObject *
_color_mul(PyObject *, PyObject *);
static PyObject *
_color_div(PyObject *, PyObject *);
static PyObject *
_color_mod(PyObject *, PyObject *);
static PyObject *
_color_inv(pgColorObject *);
static PyObject *
_color_int(pgColorObject *);
static PyObject *
_color_float(pgColorObject *);

/* Sequence protocol methods */
static Py_ssize_t
_color_length(pgColorObject *);
static PyObject *
_color_item(pgColorObject *, Py_ssize_t);
static int
_color_ass_item(pgColorObject *, Py_ssize_t, PyObject *);
static PyObject *
_color_slice(register pgColorObject *, register Py_ssize_t,
             register Py_ssize_t);
static int
_color_contains(pgColorObject *, PyObject *);

/* Mapping protocol methods. */
static PyObject *
_color_subscript(pgColorObject *, PyObject *);
static int
_color_set_slice(pgColorObject *, PyObject *, PyObject *);

/* Comparison */
static PyObject *
_color_richcompare(PyObject *, PyObject *, int);

/* New buffer protocol methods. */
static int
_color_getbuffer(pgColorObject *, Py_buffer *, int);

/* Fallback getattr and setattr for swizzling RGBA */
static PyObject *
_color_getAttr_swizzle(pgColorObject *self, PyObject *attr_name);
static int
_color_setAttr_swizzle(pgColorObject *self, PyObject *attr_name,
                       PyObject *val);

/* C API interfaces */
static PyObject *
pgColor_New(Uint8 rgba[]);
static PyObject *
pgColor_NewLength(Uint8 rgba[], Uint8 length);
static int
pg_RGBAFromObjEx(PyObject *color, Uint8 rgba[],
                 pgColorHandleFlags handle_flags);
static int
pg_MappedColorFromObj(PyObject *val, SDL_Surface *surf, Uint32 *color,
                      pgColorHandleFlags handle_flags);

/**
 * Methods, which are bound to the pgColorObject type.
 */
static PyMethodDef _color_methods[] = {
    {"from_hsva", (PyCFunction)_color_from_hsva, METH_CLASS | METH_VARARGS,
     DOC_COLOR_FROMHSVA},
    {"from_hsla", (PyCFunction)_color_from_hsla, METH_CLASS | METH_VARARGS,
     DOC_COLOR_FROMHSLA},
    {"from_cmy", (PyCFunction)_color_from_cmy, METH_CLASS | METH_VARARGS,
     DOC_COLOR_FROMCMY},
    {"from_i1i2i3", (PyCFunction)_color_from_i1i2i3, METH_CLASS | METH_VARARGS,
     DOC_COLOR_FROMI1I2I3},
    {"from_normalized", (PyCFunction)_color_from_normalized,
     METH_CLASS | METH_VARARGS, DOC_COLOR_FROMNORMALIZED},
    {"from_hex", (PyCFunction)_color_from_hex, METH_CLASS | METH_VARARGS,
     DOC_COLOR_FROMHEX},
    {"normalize", (PyCFunction)_color_normalize, METH_NOARGS,
     DOC_COLOR_NORMALIZE},
    {"correct_gamma", (PyCFunction)_color_correct_gamma, METH_VARARGS,
     DOC_COLOR_CORRECTGAMMA},
    {"set_length", (PyCFunction)_color_set_length, METH_VARARGS,
     DOC_COLOR_SETLENGTH},
    {"lerp", (PyCFunction)_color_lerp, METH_VARARGS | METH_KEYWORDS,
     DOC_COLOR_LERP},
    {"grayscale", (PyCFunction)_color_grayscale, METH_NOARGS,
     DOC_COLOR_GRAYSCALE},
    {"premul_alpha", (PyCFunction)_premul_alpha, METH_NOARGS,
     DOC_COLOR_PREMULALPHA},
    {"update", (PyCFunction)_color_update, METH_FASTCALL, DOC_COLOR_UPDATE},
    {NULL, NULL, 0, NULL}};

/**
 * Getters and setters for the pgColorObject.
 */
static PyGetSetDef _color_getsets[] = {
    {"r", (getter)_color_get_r, (setter)_color_set_r, DOC_COLOR_R, NULL},
    {"g", (getter)_color_get_g, (setter)_color_set_g, DOC_COLOR_G, NULL},
    {"b", (getter)_color_get_b, (setter)_color_set_b, DOC_COLOR_B, NULL},
    {"a", (getter)_color_get_a, (setter)_color_set_a, DOC_COLOR_A, NULL},
    {"hsva", (getter)_color_get_hsva, (setter)_color_set_hsva, DOC_COLOR_HSVA,
     NULL},
    {"hsla", (getter)_color_get_hsla, (setter)_color_set_hsla, DOC_COLOR_HSLA,
     NULL},
    {"i1i2i3", (getter)_color_get_i1i2i3, (setter)_color_set_i1i2i3,
     DOC_COLOR_I1I2I3, NULL},
    {"cmy", (getter)_color_get_cmy, (setter)_color_set_cmy, DOC_COLOR_CMY,
     NULL},
    {"normalized", (getter)_color_get_normalized,
     (setter)_color_set_normalized, DOC_COLOR_NORMALIZED, NULL},
    {"hex", (getter)_color_get_hex, (setter)_color_set_hex, DOC_COLOR_HEX,
     NULL},
    {"__array_struct__", (getter)_color_get_arraystruct, NULL,
     "array structure interface, read only", NULL},
    {NULL, NULL, NULL, NULL, NULL}};

static PyNumberMethods _color_as_number = {
    .nb_add = (binaryfunc)_color_add,
    .nb_subtract = (binaryfunc)_color_sub,
    .nb_multiply = (binaryfunc)_color_mul,
    .nb_remainder = (binaryfunc)_color_mod,
    .nb_invert = (unaryfunc)_color_inv,
    .nb_int = (unaryfunc)_color_int,
    .nb_float = (unaryfunc)_color_float,
    .nb_floor_divide = (binaryfunc)_color_div,
    .nb_index = (unaryfunc)_color_int,
};

/**
 * Sequence interface support for pgColorObject.
 */
/* sq_slice and sq_ass_slice are no longer used in this struct */
static PySequenceMethods _color_as_sequence = {
    .sq_length = (lenfunc)_color_length,
    .sq_item = (ssizeargfunc)_color_item,
    .sq_ass_item = (ssizeobjargproc)_color_ass_item,
    .sq_contains = (objobjproc)_color_contains,
};

static PyMappingMethods _color_as_mapping = {
    .mp_length = (lenfunc)_color_length,
    .mp_subscript = (binaryfunc)_color_subscript,
    .mp_ass_subscript = (objobjargproc)_color_set_slice,
};

static PyBufferProcs _color_as_buffer = {(getbufferproc)_color_getbuffer,
                                         NULL};

#define DEFERRED_ADDRESS(ADDR) 0

static PyTypeObject pgColor_Type = {
    PyVarObject_HEAD_INIT(NULL, 0).tp_name = "pygame.color.Color",
    .tp_basicsize = sizeof(pgColorObject),
    .tp_dealloc = (destructor)_color_dealloc,
    .tp_repr = (reprfunc)_color_repr,
    .tp_as_number = &_color_as_number,
    .tp_as_sequence = &_color_as_sequence,
    .tp_as_mapping = &_color_as_mapping,
    .tp_as_buffer = &_color_as_buffer,
    .tp_getattro = (getattrofunc)_color_getAttr_swizzle,
    .tp_setattro = (setattrofunc)_color_setAttr_swizzle,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_doc = DOC_COLOR,
    .tp_richcompare = _color_richcompare,
    .tp_iter = (getiterfunc)_color_iter,
    .tp_methods = _color_methods,
    .tp_getset = _color_getsets,
    .tp_init = (initproc)_color_init,
    .tp_new = _color_new,
};

/* Checks if object is an instance of Color (or a subtype of Color). If you
 * need exact checks, use pgColor_CheckExact */
#define pgColor_Check(o) PyObject_IsInstance((o), (PyObject *)&pgColor_Type)

static int
_get_double(PyObject *obj, double *val)
{
    PyObject *floatobj;
    if (!(floatobj = PyNumber_Float(obj))) {
        return 0;
    }
    *val = PyFloat_AsDouble(floatobj);
    Py_DECREF(floatobj);
    return 1;
}

static int
_get_color_int_component(PyObject *val, Uint8 *color)
{
    if (PyLong_Check(val)) {
        unsigned long longval = PyLong_AsUnsignedLong(val);
        if (PyErr_Occurred() || (longval > 255)) {
            PyErr_SetString(
                PyExc_ValueError,
                "invalid color component (must be in range [0, 255])");
            return 0;
        }
        *color = (Uint8)longval;
        return 1;
    }

    PyErr_Format(PyExc_TypeError, "color component must be a number, not '%s'",
                 Py_TYPE(val)->tp_name);
    return 0;
}

static int
_hextoint(char *hex, Uint8 *val)
{
    /* 'hex' is a two digit hexadecimal number, no spaces, no signs.
     * This algorithm is brute force, but it is character system agnostic.
     * It is definitely not a general purpose solution.
     */
    Uint8 temp = 0;

    switch (toupper(hex[0])) {
        case '0':
            break;
        case '1':
            temp += 0x10;
            break;
        case '2':
            temp += 0x20;
            break;
        case '3':
            temp += 0x30;
            break;
        case '4':
            temp += 0x40;
            break;
        case '5':
            temp += 0x50;
            break;
        case '6':
            temp += 0x60;
            break;
        case '7':
            temp += 0x70;
            break;
        case '8':
            temp += 0x80;
            break;
        case '9':
            temp += 0x90;
            break;
        case 'A':
            temp += 0xA0;
            break;
        case 'B':
            temp += 0xB0;
            break;
        case 'C':
            temp += 0xC0;
            break;
        case 'D':
            temp += 0xD0;
            break;
        case 'E':
            temp += 0xE0;
            break;
        case 'F':
            temp += 0xF0;
            break;
        default:
            return 0;
    }

    switch (toupper(hex[1])) {
        case '0':
            break;
        case '1':
            temp += 0x01;
            break;
        case '2':
            temp += 0x02;
            break;
        case '3':
            temp += 0x03;
            break;
        case '4':
            temp += 0x04;
            break;
        case '5':
            temp += 0x05;
            break;
        case '6':
            temp += 0x06;
            break;
        case '7':
            temp += 0x07;
            break;
        case '8':
            temp += 0x08;
            break;
        case '9':
            temp += 0x09;
            break;
        case 'A':
            temp += 0x0A;
            break;
        case 'B':
            temp += 0x0B;
            break;
        case 'C':
            temp += 0x0C;
            break;
        case 'D':
            temp += 0x0D;
            break;
        case 'E':
            temp += 0x0E;
            break;
        case 'F':
            temp += 0x0F;
            break;
        default:
            return 0;
    }

    *val = temp;
    return 1;
}

static tristate
_hexcolor(PyObject *color, Uint8 rgba[])
{
    Py_ssize_t len;
    char *name = (char *)PyUnicode_AsUTF8AndSize(color, &len);
    if (name == NULL) {
        return TRISTATE_ERROR;
    }

    /* hex colors can be
     * #RRGGBB
     * #RRGGBBAA
     * 0xRRGGBB
     * 0xRRGGBBAA
     */
    if (len < 7) {
        return TRISTATE_FAIL;
    }

    if (name[0] == '#') {
        if (len != 7 && len != 9) {
            return TRISTATE_FAIL;
        }
        if (!_hextoint(name + 1, &rgba[0])) {
            return TRISTATE_FAIL;
        }
        if (!_hextoint(name + 3, &rgba[1])) {
            return TRISTATE_FAIL;
        }
        if (!_hextoint(name + 5, &rgba[2])) {
            return TRISTATE_FAIL;
        }
        rgba[3] = 255;
        if (len == 9 && !_hextoint(name + 7, &rgba[3])) {
            return TRISTATE_FAIL;
        }
        return TRISTATE_SUCCESS;
    }
    else if (name[0] == '0' && name[1] == 'x') {
        if (len != 8 && len != 10) {
            return TRISTATE_FAIL;
        }
        if (!_hextoint(name + 2, &rgba[0])) {
            return TRISTATE_FAIL;
        }
        if (!_hextoint(name + 4, &rgba[1])) {
            return TRISTATE_FAIL;
        }
        if (!_hextoint(name + 6, &rgba[2])) {
            return TRISTATE_FAIL;
        }
        rgba[3] = 255;
        if (len == 10 && !_hextoint(name + 8, &rgba[3])) {
            return TRISTATE_FAIL;
        }
        return TRISTATE_SUCCESS;
    }
    return TRISTATE_FAIL;
}

static int
_coerce_obj(PyObject *obj, Uint8 rgba[])
{
    int ret = pg_RGBAFromObjEx(obj, rgba, PG_COLOR_HANDLE_RESTRICT_SEQ);
    PyErr_Clear(); /* Comparisons should never have to error */
    return ret;
}

static pgColorObject *
_color_new_internal(PyTypeObject *type, const Uint8 rgba[])
{
    /* default length of 4 - r,g,b,a. */
    return _color_new_internal_length(type, rgba, 4);
}

static pgColorObject *
_color_new_internal_length(PyTypeObject *type, const Uint8 rgba[],
                           Uint8 length)
{
    pgColorObject *color = (pgColorObject *)type->tp_alloc(type, 0);
    if (!color) {
        return NULL;
    }

    memcpy(color->data, rgba, 4);
    color->len = length;

    return color;
}

/**
 * Creates a new pgColorObject.
 */
static PyObject *
_color_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    static const Uint8 DEFAULT_RGBA[4] = {0, 0, 0, 255};

    return (PyObject *)_color_new_internal_length(type, DEFAULT_RGBA, 4);
}

static int
_parse_color_from_text(PyObject *str_obj, Uint8 *rgba)
{
    /* Named color */
    PyObject *color = NULL;
    PyObject *name1 = NULL, *name2 = NULL;

    /* We assume the caller handled this check for us. */
    assert(PyUnicode_Check(str_obj));

    color = PyDict_GetItem(_COLORDICT,
                           str_obj);  // optimize for correct color names
    if (!color) {
        switch (_hexcolor(str_obj, rgba)) {
            case TRISTATE_FAIL:
                /* Do re-handling of colordict path below */
                break;
            case TRISTATE_ERROR:
                /* Some python error raised, so forward it */
                return -1;
            default:
                /* rgba is set, we are done here */
                return 0;
        }
        name1 = PyObject_CallMethod(str_obj, "replace", "(ss)", " ", "");
        if (!name1) {
            return -1;
        }
        name2 = PyObject_CallMethod(name1, "lower", NULL);
        Py_DECREF(name1);
        if (!name2) {
            return -1;
        }
        color = PyDict_GetItem(_COLORDICT, name2);
        Py_DECREF(name2);
        if (!color) {
            PyErr_SetString(PyExc_ValueError, "invalid color name");
            return -1;
        }
    }

    if (!pg_RGBAFromObjEx(color, rgba, PG_COLOR_HANDLE_RESTRICT_SEQ)) {
        PyErr_Format(PyExc_RuntimeError,
                     "internal pygame error - colordict is supposed to "
                     "only have tuple values, but there is an object of "
                     "type '%s' here - Report this to the pygame devs",
                     Py_TYPE(color)->tp_name);
        return -1;
    }
    return 0;
}

static int
_color_init(pgColorObject *self, PyObject *args, PyObject *kwds)
{
    self->len = 4;
    if (PyTuple_GET_SIZE(args) == 1) {
        args = PyTuple_GET_ITEM(args, 0);
    }

    return pg_RGBAFromObjEx(args, self->data, PG_COLOR_HANDLE_ALL) - 1;
}

/**
 * Deallocates the pgColorObject.
 */
static void
_color_dealloc(pgColorObject *color)
{
    Py_TYPE(color)->tp_free((PyObject *)color);
}

/**
 * repr(color)
 */
static PyObject *
_color_repr(pgColorObject *color)
{
    return PyUnicode_FromFormat("Color(%d, %d, %d, %d)", color->data[0],
                                color->data[1], color->data[2],
                                color->data[3]);
}

static PyObject *
_color_iter(pgColorObject *self)
{
    Uint8 i;
    PyObject *iter, *tup = PyTuple_New(self->len);
    if (!tup) {
        return NULL;
    }
    for (i = 0; i < self->len; i++) {
        PyObject *val = PyLong_FromLong(self->data[i]);
        if (!val) {
            Py_DECREF(tup);
            return NULL;
        }

        PyTuple_SET_ITEM(tup, i, val);
    }
    iter = PyTuple_Type.tp_iter(tup);
    Py_DECREF(tup);
    return iter;
}

static PyObject *
_color_from_space(char *space, PyObject *args)
{
    pgColorObject *color =
        (pgColorObject *)pgColor_New((Uint8[]){0, 0, 0, 255});
    int set_success = 1;

    if (color == NULL) {
        return NULL;
    }

    if (PyTuple_GET_SIZE(args) == 1) {
        args = PyTuple_GET_ITEM(args, 0);
    }

    if (strcmp(space, "hsva") == 0) {
        set_success = _color_set_hsva(color, args, NULL);
    }
    else if (strcmp(space, "hsla") == 0) {
        set_success = _color_set_hsla(color, args, NULL);
    }
    else if (strcmp(space, "cmy") == 0) {
        set_success = _color_set_cmy(color, args, NULL);
    }
    else if (strcmp(space, "i1i2i3") == 0) {
        set_success = _color_set_i1i2i3(color, args, NULL);
    }
    else if (strcmp(space, "normalized") == 0) {
        set_success = _color_set_normalized(color, args, NULL);
    }
    else if (strcmp(space, "hex") == 0) {
        set_success = _color_set_hex(color, args, NULL);
    }

    if (set_success != 0) {
        return NULL;
    }

    return (PyObject *)color;
}

/**
 * color.normalize()
 */
static PyObject *
_color_normalize(pgColorObject *color, PyObject *_null)
{
    double rgba[4];
    rgba[0] = color->data[0] / 255.0;
    rgba[1] = color->data[1] / 255.0;
    rgba[2] = color->data[2] / 255.0;
    rgba[3] = color->data[3] / 255.0;
    return Py_BuildValue("(ffff)", rgba[0], rgba[1], rgba[2], rgba[3]);
}

/**
 * color.correct_gamma(x)
 */
static PyObject *
_color_correct_gamma(pgColorObject *color, PyObject *args)
{
    double frgba[4];
    Uint8 rgba[4];
    double _gamma;

    if (!PyArg_ParseTuple(args, "d", &_gamma)) {
        return NULL;
    }

    frgba[0] = pow(color->data[0] / 255.0, _gamma);
    frgba[1] = pow(color->data[1] / 255.0, _gamma);
    frgba[2] = pow(color->data[2] / 255.0, _gamma);
    frgba[3] = pow(color->data[3] / 255.0, _gamma);

    /* visual studio doesn't have a round func, so doing it with +.5 and
     * truncaction */
    rgba[0] = (frgba[0] > 1.0)
                  ? 255
                  : ((frgba[0] < 0.0) ? 0 : (Uint8)(frgba[0] * 255 + .5));
    rgba[1] = (frgba[1] > 1.0)
                  ? 255
                  : ((frgba[1] < 0.0) ? 0 : (Uint8)(frgba[1] * 255 + .5));
    rgba[2] = (frgba[2] > 1.0)
                  ? 255
                  : ((frgba[2] < 0.0) ? 0 : (Uint8)(frgba[2] * 255 + .5));
    rgba[3] = (frgba[3] > 1.0)
                  ? 255
                  : ((frgba[3] < 0.0) ? 0 : (Uint8)(frgba[3] * 255 + .5));
    return (PyObject *)_color_new_internal(Py_TYPE(color), rgba);
}

/**
 * color.grayscale()
 */
static PyObject *
_color_grayscale(pgColorObject *self, PyObject *_null)
{
    // RGBA to GRAY formula used by OpenCV
    Uint8 grayscale_pixel =
        (Uint8)(0.299 * self->data[0] + 0.587 * self->data[1] +
                0.114 * self->data[2]);

    Uint8 new_rgba[4];
    new_rgba[0] = grayscale_pixel;
    new_rgba[1] = grayscale_pixel;
    new_rgba[2] = grayscale_pixel;
    new_rgba[3] = self->data[3];
    return (PyObject *)_color_new_internal(Py_TYPE(self), new_rgba);
}

/**
 * color.lerp(other, x)
 */
static PyObject *
_color_lerp(pgColorObject *self, PyObject *args, PyObject *kw)
{
    Uint8 rgba[4];
    Uint8 new_rgba[4];
    PyObject *colobj;
    double amt;
    static char *keywords[] = {"color", "amount", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kw, "Od", keywords, &colobj,
                                     &amt)) {
        return NULL;
    }

    if (!pg_RGBAFromObjEx(colobj, rgba, PG_COLOR_HANDLE_ALL)) {
        /* Exception already set for us */
        return NULL;
    }

    // TOLERANCE to account for double precison floating point inaccuracy at
    // the very limits, like if you're LERP'ing by 0.01. When you hit the end,
    // something stupid like this might happen
    /*  >>> value = 0
        >>> offset = 0.01
        >>> while value < 1:
        ...     value += offset
        ...
        >>> print(value)
        1.0000000000000007
    */
    static const double TOLERANCE = 1e-6;
    if ((amt < -TOLERANCE) || (amt > (1.0 + TOLERANCE))) {
        return RAISE(PyExc_ValueError, "Argument 2 must be in range [0, 1]");
    }

    for (int i = 0; i < 4; i++) {
        new_rgba[i] =
            (Uint8)pg_round(self->data[i] * (1 - amt) + rgba[i] * amt);
    }

    return (PyObject *)_color_new_internal(Py_TYPE(self), new_rgba);
}

/**
 * color.premul_alpha()
 */
static PyObject *
_premul_alpha(pgColorObject *color, PyObject *_null)
{
    Uint8 new_rgba[4];
    new_rgba[0] = (Uint8)(((color->data[0] + 1) * color->data[3]) >> 8);
    new_rgba[1] = (Uint8)(((color->data[1] + 1) * color->data[3]) >> 8);
    new_rgba[2] = (Uint8)(((color->data[2] + 1) * color->data[3]) >> 8);
    new_rgba[3] = color->data[3];

    return (PyObject *)_color_new_internal(Py_TYPE(color), new_rgba);
}

static PyObject *
_color_update(pgColorObject *self, PyObject *const *args, Py_ssize_t nargs)
{
    Uint8 *rgba = self->data;
    if (nargs == 1) {
        if (!pg_RGBAFromObjEx(args[0], rgba, PG_COLOR_HANDLE_ALL)) {
            return NULL;
        }
    }
    else if (nargs == 3 || nargs == 4) {
        Py_ssize_t i;
        for (i = 0; i < nargs; i++) {
            if (!_get_color_int_component(args[i], &rgba[i])) {
                return NULL;
            }
        }
        /* Update len only if alpha component was passed (because the previous
         * implementation of this function behaved so) */
        if (nargs == 4) {
            self->len = 4;
        }
    }
    else {
        return RAISE(PyExc_TypeError,
                     "update can take only 1, 3 or 4 arguments");
    }
    Py_RETURN_NONE;
}

/**
 * color.r
 */
static PyObject *
_color_get_r(pgColorObject *color, void *closure)
{
    return PyLong_FromLong(color->data[0]);
}

/**
 * color.r = x
 */
static int
_color_set_r(pgColorObject *color, PyObject *value, void *closure)
{
    DEL_ATTR_NOT_SUPPORTED_CHECK("r", value);
    return _get_color_int_component(value, &(color->data[0])) - 1;
}
/**
 * color.g
 */
static PyObject *
_color_get_g(pgColorObject *color, void *closure)
{
    return PyLong_FromLong(color->data[1]);
}

/**
 * color.g = x
 */
static int
_color_set_g(pgColorObject *color, PyObject *value, void *closure)
{
    DEL_ATTR_NOT_SUPPORTED_CHECK("g", value);
    return _get_color_int_component(value, &(color->data[1])) - 1;
}

/**
 * color.b
 */
static PyObject *
_color_get_b(pgColorObject *color, void *closure)
{
    return PyLong_FromLong(color->data[2]);
}

/**
 * color.b = x
 */
static int
_color_set_b(pgColorObject *color, PyObject *value, void *closure)
{
    DEL_ATTR_NOT_SUPPORTED_CHECK("b", value);
    return _get_color_int_component(value, &(color->data[2])) - 1;
}

/**
 * color.a
 */
static PyObject *
_color_get_a(pgColorObject *color, void *closure)
{
    return PyLong_FromLong(color->data[3]);
}

/**
 * color.a = x
 */
static int
_color_set_a(pgColorObject *color, PyObject *value, void *closure)
{
    DEL_ATTR_NOT_SUPPORTED_CHECK("a", value);
    return _get_color_int_component(value, &(color->data[3])) - 1;
}

/**
 * color.hsva
 */
static PyObject *
_color_get_hsva(pgColorObject *color, void *closure)
{
    double hsv[3] = {0, 0, 0};
    double frgb[4];
    double minv, maxv, diff;

    /* Normalize */
    frgb[0] = color->data[0] / 255.0;
    frgb[1] = color->data[1] / 255.0;
    frgb[2] = color->data[2] / 255.0;
    frgb[3] = color->data[3] / 255.0;

    maxv = MAX(MAX(frgb[0], frgb[1]), frgb[2]);
    minv = MIN(MIN(frgb[0], frgb[1]), frgb[2]);
    diff = maxv - minv;

    /* Calculate V */
    hsv[2] = 100. * maxv;

    if (maxv == minv) {
        hsv[0] = 0;
        hsv[1] = 0;
        return Py_BuildValue("(ffff)", hsv[0], hsv[1], hsv[2], frgb[3] * 100);
    }
    /* Calculate S */
    hsv[1] = 100. * (maxv - minv) / maxv;

    /* Clamp S, needed on some but not all FPUs */
    if (hsv[1] < 0) {
        hsv[1] = 0.f;
    }
    else if (hsv[1] > 100) {
        hsv[1] = 100.f;
    }

    /* Calculate H */
    if (maxv == frgb[0]) {
        hsv[0] = fmod((60 * ((frgb[1] - frgb[2]) / diff)), 360.f);
    }
    else if (maxv == frgb[1]) {
        hsv[0] = (60 * ((frgb[2] - frgb[0]) / diff)) + 120.f;
    }
    else {
        hsv[0] = (60 * ((frgb[0] - frgb[1]) / diff)) + 240.f;
    }

    if (hsv[0] < 0) {
        hsv[0] += 360.f;
    }

    /* H,S,V,A */
    return Py_BuildValue("(ffff)", hsv[0], hsv[1], hsv[2], frgb[3] * 100);
}

static int
_color_set_hsva(pgColorObject *color, PyObject *value, void *closure)
{
    PyObject *item;
    double hsva[4] = {0, 0, 0, 0};
    double f, p, q, t, v, s;
    int hi;

    DEL_ATTR_NOT_SUPPORTED_CHECK("hsva", value);

    if (!PySequence_Check(value) || PySequence_Size(value) < 3 ||
        PySequence_Size(value) > 4) {
        PyErr_SetString(PyExc_ValueError, "invalid HSVA value");
        return -1;
    }

    /* H */
    item = PySequence_GetItem(value, 0);
    if (!item || !_get_double(item, &(hsva[0])) || hsva[0] < 0 ||
        hsva[0] > 360) {
        Py_XDECREF(item);
        PyErr_SetString(PyExc_ValueError, "invalid HSVA value");
        return -1;
    }
    Py_DECREF(item);

    /* S */
    item = PySequence_GetItem(value, 1);
    if (!item || !_get_double(item, &(hsva[1])) || hsva[1] < 0 ||
        hsva[1] > 100) {
        Py_XDECREF(item);
        PyErr_SetString(PyExc_ValueError, "invalid HSVA value");
        return -1;
    }
    Py_DECREF(item);

    /* V */
    item = PySequence_GetItem(value, 2);
    if (!item || !_get_double(item, &(hsva[2])) || hsva[2] < 0 ||
        hsva[2] > 100) {
        Py_XDECREF(item);
        PyErr_SetString(PyExc_ValueError, "invalid HSVA value");
        return -1;
    }
    Py_DECREF(item);

    /* A */
    if (PySequence_Size(value) > 3) {
        item = PySequence_GetItem(value, 3);
        if (!item || !_get_double(item, &(hsva[3])) || hsva[3] < 0 ||
            hsva[3] > 100) {
            Py_XDECREF(item);
            PyErr_SetString(PyExc_ValueError, "invalid HSVA value");
            return -1;
        }
        Py_DECREF(item);
    }

    color->data[3] = (Uint8)((hsva[3] / 100.0f) * 255);

    s = hsva[1] / 100.f;
    v = hsva[2] / 100.f;

    hi = (int)floor(hsva[0] / 60.f);
    f = (hsva[0] / 60.f) - hi;
    p = v * (1 - s);
    q = v * (1 - s * f);
    t = v * (1 - s * (1 - f));

    switch (hi) {
        case 1:
            color->data[0] = (Uint8)(q * 255);
            color->data[1] = (Uint8)(v * 255);
            color->data[2] = (Uint8)(p * 255);
            break;
        case 2:
            color->data[0] = (Uint8)(p * 255);
            color->data[1] = (Uint8)(v * 255);
            color->data[2] = (Uint8)(t * 255);
            break;
        case 3:
            color->data[0] = (Uint8)(p * 255);
            color->data[1] = (Uint8)(q * 255);
            color->data[2] = (Uint8)(v * 255);
            break;
        case 4:
            color->data[0] = (Uint8)(t * 255);
            color->data[1] = (Uint8)(p * 255);
            color->data[2] = (Uint8)(v * 255);
            break;
        case 5:
            color->data[0] = (Uint8)(v * 255);
            color->data[1] = (Uint8)(p * 255);
            color->data[2] = (Uint8)(q * 255);
            break;
        default:
            /* 0 or 6, which are equivalent. */
            assert(hi == 0 || hi == 6);
            color->data[0] = (Uint8)(v * 255);
            color->data[1] = (Uint8)(t * 255);
            color->data[2] = (Uint8)(p * 255);
    }

    return 0;
}

/**
 * color.hsla
 */
static PyObject *
_color_get_hsla(pgColorObject *color, void *closure)
{
    double hsl[3] = {0, 0, 0};
    double frgb[4];
    double minv, maxv, diff;

    /* Normalize */
    frgb[0] = color->data[0] / 255.0;
    frgb[1] = color->data[1] / 255.0;
    frgb[2] = color->data[2] / 255.0;
    frgb[3] = color->data[3] / 255.0;

    maxv = MAX(MAX(frgb[0], frgb[1]), frgb[2]);
    minv = MIN(MIN(frgb[0], frgb[1]), frgb[2]);

    diff = maxv - minv;

    /* Calculate L */
    hsl[2] = 50.f * (maxv + minv); /* 1/2 (max + min) */

    if (maxv == minv) {
        hsl[1] = 0;
        hsl[0] = 0;
        return Py_BuildValue("(ffff)", hsl[0], hsl[1], hsl[2], frgb[3] * 100);
    }

    /* Calculate S */
    if (hsl[2] <= 50) {
        hsl[1] = diff / (maxv + minv);
    }
    else {
        hsl[1] = diff / (2 - maxv - minv);
    }
    hsl[1] *= 100.f;

    /* Calculate H */
    if (maxv == frgb[0]) {
        hsl[0] = fmod((60 * ((frgb[1] - frgb[2]) / diff)), 360.f);
    }
    else if (maxv == frgb[1]) {
        hsl[0] = (60 * ((frgb[2] - frgb[0]) / diff)) + 120.f;
    }
    else {
        hsl[0] = (60 * ((frgb[0] - frgb[1]) / diff)) + 240.f;
    }
    if (hsl[0] < 0) {
        hsl[0] += 360.f;
    }

    /* H,S,L,A */
    return Py_BuildValue("(ffff)", hsl[0], hsl[1], hsl[2], frgb[3] * 100);
}

/**
 * color.hsla = x
 */
static int
_color_set_hsla(pgColorObject *color, PyObject *value, void *closure)
{
    PyObject *item;
    double hsla[4] = {0, 0, 0, 0};
    double ht, h, q, p = 0, s, l = 0;
    static double onethird = 1.0 / 3.0f;

    DEL_ATTR_NOT_SUPPORTED_CHECK("hsla", value);

    if (!PySequence_Check(value) || PySequence_Size(value) < 3 ||
        PySequence_Size(value) > 4) {
        PyErr_SetString(PyExc_ValueError, "invalid HSLA value");
        return -1;
    }

    /* H */
    item = PySequence_GetItem(value, 0);
    if (!item || !_get_double(item, &(hsla[0])) || hsla[0] < 0 ||
        hsla[0] > 360) {
        Py_XDECREF(item);
        PyErr_SetString(PyExc_ValueError, "invalid HSLA value");
        return -1;
    }
    Py_DECREF(item);

    /* S */
    item = PySequence_GetItem(value, 1);
    if (!item || !_get_double(item, &(hsla[1])) || hsla[1] < 0 ||
        hsla[1] > 100) {
        Py_XDECREF(item);
        PyErr_SetString(PyExc_ValueError, "invalid HSLA value");
        return -1;
    }
    Py_DECREF(item);

    /* L */
    item = PySequence_GetItem(value, 2);
    if (!item || !_get_double(item, &(hsla[2])) || hsla[2] < 0 ||
        hsla[2] > 100) {
        Py_XDECREF(item);
        PyErr_SetString(PyExc_ValueError, "invalid HSLA value");
        return -1;
    }
    Py_DECREF(item);

    /* A */
    if (PySequence_Size(value) > 3) {
        item = PySequence_GetItem(value, 3);
        if (!item || !_get_double(item, &(hsla[3])) || hsla[3] < 0 ||
            hsla[3] > 100) {
            Py_XDECREF(item);
            PyErr_SetString(PyExc_ValueError, "invalid HSLA value");
            return -1;
        }
        Py_DECREF(item);
    }

    color->data[3] = (Uint8)((hsla[3] / 100.f) * 255);

    s = hsla[1] / 100.f;
    l = hsla[2] / 100.f;

    if (s == 0) {
        color->data[0] = (Uint8)(l * 255);
        color->data[1] = (Uint8)(l * 255);
        color->data[2] = (Uint8)(l * 255);
        return 0;
    }

    if (l < 0.5f) {
        q = l * (1 + s);
    }
    else {
        q = l + s - (l * s);
    }
    p = 2 * l - q;

    ht = hsla[0] / 360.f;

    /* Calculate R */
    h = ht + onethird;
    if (h < 0) {
        h += 1;
    }
    else if (h > 1) {
        h -= 1;
    }

    if (h < 1. / 6.f) {
        color->data[0] = (Uint8)((p + ((q - p) * 6 * h)) * 255);
    }
    else if (h < 0.5f) {
        color->data[0] = (Uint8)(q * 255);
    }
    else if (h < 2. / 3.f) {
        color->data[0] = (Uint8)((p + ((q - p) * 6 * (2. / 3.f - h))) * 255);
    }
    else {
        color->data[0] = (Uint8)(p * 255);
    }

    /* Calculate G */
    h = ht;
    if (h < 0) {
        h += 1;
    }
    else if (h > 1) {
        h -= 1;
    }

    if (h < 1. / 6.f) {
        color->data[1] = (Uint8)((p + ((q - p) * 6 * h)) * 255);
    }
    else if (h < 0.5f) {
        color->data[1] = (Uint8)(q * 255);
    }
    else if (h < 2. / 3.f) {
        color->data[1] = (Uint8)((p + ((q - p) * 6 * (2. / 3.f - h))) * 255);
    }
    else {
        color->data[1] = (Uint8)(p * 255);
    }

    /* Calculate B */
    h = ht - onethird;
    if (h < 0) {
        h += 1;
    }
    else if (h > 1) {
        h -= 1;
    }

    if (h < 1. / 6.f) {
        color->data[2] = (Uint8)((p + ((q - p) * 6 * h)) * 255);
    }
    else if (h < 0.5f) {
        color->data[2] = (Uint8)(q * 255);
    }
    else if (h < 2. / 3.f) {
        color->data[2] = (Uint8)((p + ((q - p) * 6 * (2. / 3.f - h))) * 255);
    }
    else {
        color->data[2] = (Uint8)(p * 255);
    }

    return 0;
}

static PyObject *
_color_get_i1i2i3(pgColorObject *color, void *closure)
{
    double i1i2i3[3] = {0, 0, 0};
    double frgb[3];

    /* Normalize */
    frgb[0] = color->data[0] / 255.0;
    frgb[1] = color->data[1] / 255.0;
    frgb[2] = color->data[2] / 255.0;

    i1i2i3[0] = (frgb[0] + frgb[1] + frgb[2]) / 3.0f;
    i1i2i3[1] = (frgb[0] - frgb[2]) / 2.0f;
    i1i2i3[2] = (2 * frgb[1] - frgb[0] - frgb[2]) / 4.0f;

    return Py_BuildValue("(fff)", i1i2i3[0], i1i2i3[1], i1i2i3[2]);
}

static int
_color_set_i1i2i3(pgColorObject *color, PyObject *value, void *closure)
{
    PyObject *item;
    double i1i2i3[3] = {0, 0, 0};
    double ar, ag, ab;

    DEL_ATTR_NOT_SUPPORTED_CHECK("i1i2i3", value);

    if (!PySequence_Check(value) || PySequence_Size(value) != 3) {
        PyErr_SetString(PyExc_ValueError, "invalid I1I2I3 value");
        return -1;
    }

    /* I1 */
    item = PySequence_GetItem(value, 0);
    if (!item || !_get_double(item, &(i1i2i3[0])) || i1i2i3[0] < 0 ||
        i1i2i3[0] > 1) {
        Py_XDECREF(item);
        PyErr_SetString(PyExc_ValueError, "invalid I1I2I3 value");
        return -1;
    }
    Py_DECREF(item);

    /* I2 */
    item = PySequence_GetItem(value, 1);
    if (!item || !_get_double(item, &(i1i2i3[1])) || i1i2i3[1] < -0.5f ||
        i1i2i3[1] > 0.5f) {
        Py_XDECREF(item);
        PyErr_SetString(PyExc_ValueError, "invalid I1I2I3 value");
        return -1;
    }
    Py_DECREF(item);

    /* I3 */
    item = PySequence_GetItem(value, 2);
    if (!item || !_get_double(item, &(i1i2i3[2])) || i1i2i3[2] < -0.5f ||
        i1i2i3[2] > 0.5f) {
        Py_XDECREF(item);
        PyErr_SetString(PyExc_ValueError, "invalid I1I2I3 value");
        return -1;
    }
    Py_DECREF(item);

    ab = i1i2i3[0] - i1i2i3[1] - 2 * i1i2i3[2] / 3.f;
    ar = 2 * i1i2i3[1] + ab;
    ag = 3 * i1i2i3[0] - ar - ab;

    color->data[0] = (Uint8)(ar * 255);
    color->data[1] = (Uint8)(ag * 255);
    color->data[2] = (Uint8)(ab * 255);

    return 0;
}

static PyObject *
_color_get_cmy(pgColorObject *color, void *closure)
{
    double cmy[3] = {0, 0, 0};
    double frgb[3];

    /* Normalize */
    frgb[0] = color->data[0] / 255.0;
    frgb[1] = color->data[1] / 255.0;
    frgb[2] = color->data[2] / 255.0;

    cmy[0] = 1.0 - frgb[0];
    cmy[1] = 1.0 - frgb[1];
    cmy[2] = 1.0 - frgb[2];

    return Py_BuildValue("(fff)", cmy[0], cmy[1], cmy[2]);
}

static int
_color_set_cmy(pgColorObject *color, PyObject *value, void *closure)
{
    PyObject *item;
    double cmy[3] = {0, 0, 0};

    DEL_ATTR_NOT_SUPPORTED_CHECK("cmy", value);

    if (!PySequence_Check(value) || PySequence_Size(value) != 3) {
        PyErr_SetString(PyExc_ValueError, "invalid CMY value");
        return -1;
    }

    /* I1 */
    item = PySequence_GetItem(value, 0);
    if (!item || !_get_double(item, &(cmy[0])) || cmy[0] < 0 || cmy[0] > 1) {
        Py_XDECREF(item);
        PyErr_SetString(PyExc_ValueError, "invalid CMY value");
        return -1;
    }
    Py_DECREF(item);

    /* I2 */
    item = PySequence_GetItem(value, 1);
    if (!item || !_get_double(item, &(cmy[1])) || cmy[1] < 0 || cmy[1] > 1) {
        Py_XDECREF(item);
        PyErr_SetString(PyExc_ValueError, "invalid CMY value");
        return -1;
    }
    Py_DECREF(item);

    /* I2 */
    item = PySequence_GetItem(value, 2);
    if (!item || !_get_double(item, &(cmy[2])) || cmy[2] < 0 || cmy[2] > 1) {
        Py_XDECREF(item);
        PyErr_SetString(PyExc_ValueError, "invalid CMY value");
        return -1;
    }
    Py_DECREF(item);

    color->data[0] = (Uint8)((1.0 - cmy[0]) * 255);
    color->data[1] = (Uint8)((1.0 - cmy[1]) * 255);
    color->data[2] = (Uint8)((1.0 - cmy[2]) * 255);

    return 0;
}

static PyObject *
_color_get_normalized(pgColorObject *color, void *closure)
{
    double frgba[4];

    frgba[0] = color->data[0] / 255.0;
    frgba[1] = color->data[1] / 255.0;
    frgba[2] = color->data[2] / 255.0;
    frgba[3] = color->data[3] / 255.0;

    return Py_BuildValue("(ffff)", frgba[0], frgba[1], frgba[2], frgba[3]);
}

static int
_color_set_normalized(pgColorObject *color, PyObject *value, void *closure)
{
    PyObject *item;
    double frgba[4] = {0.0, 0.0, 0.0, 1.0};

    DEL_ATTR_NOT_SUPPORTED_CHECK("normalized", value);

    if (!PySequence_Check(value) || PySequence_Size(value) < 3 ||
        PySequence_Size(value) > 4) {
        PyErr_SetString(PyExc_ValueError, "invalid normalized value");
        return -1;
    }

    item = PySequence_GetItem(value, 0);
    if (!item || !_get_double(item, &(frgba[0])) || frgba[0] < 0.0 ||
        frgba[0] > 1.0) {
        Py_XDECREF(item);
        PyErr_SetString(PyExc_ValueError, "invalid normalized value");
        return -1;
    }
    Py_DECREF(item);

    item = PySequence_GetItem(value, 1);
    if (!item || !_get_double(item, &(frgba[1])) || frgba[1] < 0.0 ||
        frgba[1] > 1.0) {
        Py_XDECREF(item);
        PyErr_SetString(PyExc_ValueError, "invalid normalized value");
        return -1;
    }
    Py_DECREF(item);

    item = PySequence_GetItem(value, 2);
    if (!item || !_get_double(item, &(frgba[2])) || frgba[2] < 0.0 ||
        frgba[2] > 1.0) {
        Py_XDECREF(item);
        PyErr_SetString(PyExc_ValueError, "invalid normalized value");
        return -1;
    }
    Py_DECREF(item);

    if (PySequence_Size(value) > 3) {
        item = PySequence_GetItem(value, 3);
        if (!item || !_get_double(item, &(frgba[3])) || frgba[3] < 0.0 ||
            frgba[3] > 1.0) {
            Py_XDECREF(item);
            PyErr_SetString(PyExc_ValueError, "invalid normalized value");
            return -1;
        }
        Py_DECREF(item);
    }

    color->data[0] = (Uint8)round(frgba[0] * 255.0);
    color->data[1] = (Uint8)round(frgba[1] * 255.0);
    color->data[2] = (Uint8)round(frgba[2] * 255.0);
    color->data[3] = (Uint8)round(frgba[3] * 255.0);

    return 0;
}

static PyObject *
_color_get_hex(pgColorObject *color, void *closure)
{
    return PyUnicode_FromFormat("#%02x%02x%02x%02x", color->data[0],
                                color->data[1], color->data[2],
                                color->data[3]);
}

static int
_color_set_hex(pgColorObject *color, PyObject *value, void *closure)
{
    DEL_ATTR_NOT_SUPPORTED_CHECK("hex", value);

    if (!PyUnicode_Check(value)) {
        PyErr_SetString(PyExc_TypeError, "hex color must be a string");
        return -1;
    }

    switch (_hexcolor(value, color->data)) {
        case TRISTATE_FAIL:
            PyErr_SetString(PyExc_ValueError, "invalid hex string");
            return -1;
        case TRISTATE_ERROR:
            return -1; /* forward python error */
        default:
            return 0;
    }
    return 0;
}

static PyObject *
_color_get_arraystruct(pgColorObject *color, void *closure)
{
    Py_buffer view;
    PyObject *capsule;

    if (_color_getbuffer(color, &view, PyBUF_FULL_RO)) {
        return 0;
    }
    capsule = pgBuffer_AsArrayStruct(&view);
    Py_DECREF(color);
    return capsule;
}

/* Number protocol methods */

/**
 * color1 + color2
 */
static PyObject *
_color_add(PyObject *obj1, PyObject *obj2)
{
    Uint8 rgba[4];
    pgColorObject *color1 = (pgColorObject *)obj1;
    pgColorObject *color2 = (pgColorObject *)obj2;
    if (!pgColor_Check(obj1) || !pgColor_Check(obj2)) {
        Py_INCREF(Py_NotImplemented);
        return Py_NotImplemented;
    }
    rgba[0] = MIN(color1->data[0] + color2->data[0], 255);
    rgba[1] = MIN(color1->data[1] + color2->data[1], 255);
    rgba[2] = MIN(color1->data[2] + color2->data[2], 255);
    rgba[3] = MIN(color1->data[3] + color2->data[3], 255);
    return (PyObject *)_color_new_internal(Py_TYPE(obj1), rgba);
}

/**
 * color1 - color2
 */
static PyObject *
_color_sub(PyObject *obj1, PyObject *obj2)
{
    Uint8 rgba[4];
    pgColorObject *color1 = (pgColorObject *)obj1;
    pgColorObject *color2 = (pgColorObject *)obj2;
    if (!pgColor_Check(obj1) || !pgColor_Check(obj2)) {
        Py_INCREF(Py_NotImplemented);
        return Py_NotImplemented;
    }
    rgba[0] = MAX(color1->data[0] - color2->data[0], 0);
    rgba[1] = MAX(color1->data[1] - color2->data[1], 0);
    rgba[2] = MAX(color1->data[2] - color2->data[2], 0);
    rgba[3] = MAX(color1->data[3] - color2->data[3], 0);
    return (PyObject *)_color_new_internal(Py_TYPE(obj1), rgba);
}

/**
 * color1 * color2
 */
static PyObject *
_color_mul(PyObject *obj1, PyObject *obj2)
{
    Uint8 rgba[4];
    pgColorObject *color1 = (pgColorObject *)obj1;
    pgColorObject *color2 = (pgColorObject *)obj2;
    if (!pgColor_Check(obj1) || !pgColor_Check(obj2)) {
        Py_INCREF(Py_NotImplemented);
        return Py_NotImplemented;
    }
    rgba[0] = MIN(color1->data[0] * color2->data[0], 255);
    rgba[1] = MIN(color1->data[1] * color2->data[1], 255);
    rgba[2] = MIN(color1->data[2] * color2->data[2], 255);
    rgba[3] = MIN(color1->data[3] * color2->data[3], 255);
    return (PyObject *)_color_new_internal(Py_TYPE(obj1), rgba);
}

/**
 * color1 / color2
 */
static PyObject *
_color_div(PyObject *obj1, PyObject *obj2)
{
    Uint8 rgba[4] = {0, 0, 0, 0};
    pgColorObject *color1 = (pgColorObject *)obj1;
    pgColorObject *color2 = (pgColorObject *)obj2;
    if (!pgColor_Check(obj1) || !pgColor_Check(obj2)) {
        Py_INCREF(Py_NotImplemented);
        return Py_NotImplemented;
    }
    if (color2->data[0] != 0) {
        rgba[0] = color1->data[0] / color2->data[0];
    }
    if (color2->data[1] != 0) {
        rgba[1] = color1->data[1] / color2->data[1];
    }
    if (color2->data[2]) {
        rgba[2] = color1->data[2] / color2->data[2];
    }
    if (color2->data[3]) {
        rgba[3] = color1->data[3] / color2->data[3];
    }
    return (PyObject *)_color_new_internal(Py_TYPE(obj1), rgba);
}

/**
 * color1 % color2
 */
static PyObject *
_color_mod(PyObject *obj1, PyObject *obj2)
{
    Uint8 rgba[4] = {0, 0, 0, 0};
    pgColorObject *color1 = (pgColorObject *)obj1;
    pgColorObject *color2 = (pgColorObject *)obj2;
    if (!pgColor_Check(obj1) || !pgColor_Check(obj2)) {
        Py_INCREF(Py_NotImplemented);
        return Py_NotImplemented;
    }
    if (color2->data[0] != 0) {
        rgba[0] = color1->data[0] % color2->data[0];
    }
    if (color2->data[1] != 0) {
        rgba[1] = color1->data[1] % color2->data[1];
    }
    if (color2->data[2]) {
        rgba[2] = color1->data[2] % color2->data[2];
    }
    if (color2->data[3]) {
        rgba[3] = color1->data[3] % color2->data[3];
    }
    return (PyObject *)_color_new_internal(Py_TYPE(obj1), rgba);
}

/**
 * ~color
 */
static PyObject *
_color_inv(pgColorObject *color)
{
    Uint8 rgba[4];
    rgba[0] = 255 - color->data[0];
    rgba[1] = 255 - color->data[1];
    rgba[2] = 255 - color->data[2];
    rgba[3] = 255 - color->data[3];
    return (PyObject *)_color_new_internal(Py_TYPE(color), rgba);
}

/**
 * int(color)
 */
static PyObject *
_color_int(pgColorObject *color)
{
    Uint32 tmp = ((Uint32)color->data[0] << 24) + (color->data[1] << 16) +
                 (color->data[2] << 8) + color->data[3];
    return PyLong_FromUnsignedLong(tmp);
}

/**
 * float(color)
 */
static PyObject *
_color_float(pgColorObject *color)
{
    Uint32 tmp = (((Uint32)color->data[0] << 24) + (color->data[1] << 16) +
                  (color->data[2] << 8) + color->data[3]);
    return PyFloat_FromDouble((double)tmp);
}

/* Sequence protocol methods */

/**
 * len(color)
 */
static Py_ssize_t
_color_length(pgColorObject *color)
{
    return color->len;
}

/**
 * color.set_length(3)
 */

static PyObject *
_color_set_length(pgColorObject *color, PyObject *args)
{
    int clength;

    if (PyErr_WarnEx(PyExc_DeprecationWarning,
                     "pygame.Color.set_length deprecated since 2.1.3",
                     1) == -1) {
        return NULL;
    }

    if (!PyArg_ParseTuple(args, "i", &clength)) {
        if (!PyErr_ExceptionMatches(PyExc_OverflowError)) {
            return NULL;
        }
        /* OverflowError also means the value is out-of-range */
        PyErr_Clear();
        clength = INT_MAX;
    }

    if (clength > 4 || clength < 1) {
        return RAISE(PyExc_ValueError, "Length needs to be 1,2,3, or 4.");
    }

    color->len = clength;

    Py_RETURN_NONE;
}

/**
 * color[x]
 */
static PyObject *
_color_item(pgColorObject *color, Py_ssize_t _index)
{
    if ((_index > (color->len - 1))) {
        return RAISE(PyExc_IndexError, "invalid index");
    }

    switch (_index) {
        case 0:
            return PyLong_FromLong(color->data[0]);
        case 1:
            return PyLong_FromLong(color->data[1]);
        case 2:
            return PyLong_FromLong(color->data[2]);
        case 3:
            return PyLong_FromLong(color->data[3]);
        default:
            return RAISE(PyExc_IndexError, "invalid index");
    }
}

static PyObject *
_color_subscript(pgColorObject *self, PyObject *item)
{
    if (PyIndex_Check(item)) {
        Py_ssize_t i;
        i = PyNumber_AsSsize_t(item, PyExc_IndexError);

        if (i == -1 && PyErr_Occurred()) {
            return NULL;
        }
        /*
        if (i < 0)
            i += PyList_GET_SIZE(self);
        */
        return _color_item(self, i);
    }
    if (PySlice_Check(item)) {
        int len = 4;
        Py_ssize_t start, stop, step, slicelength;

        if (PySlice_GetIndicesEx(item, len, &start, &stop, &step,
                                 &slicelength) < 0) {
            return NULL;
        }

        if (slicelength <= 0) {
            return PyTuple_New(0);
        }
        else if (step == 1) {
            return _color_slice(self, start, stop);
        }
        else {
            return RAISE(PyExc_TypeError, "slice steps not supported");
        }
    }
    else {
        PyErr_Format(PyExc_TypeError,
                     "Color indices must be integers, not %.200s",
                     Py_TYPE(item)->tp_name);
        return NULL;
    }
}

/**
 * color[x] = y
 */
static int
_color_ass_item(pgColorObject *color, Py_ssize_t _index, PyObject *value)
{
    switch (_index) {
        case 0:
            return _color_set_r(color, value, NULL);
        case 1:
            return _color_set_g(color, value, NULL);
        case 2:
            return _color_set_b(color, value, NULL);
        case 3:
            return _color_set_a(color, value, NULL);
        default:
            PyErr_SetString(PyExc_IndexError, "invalid index");
            break;
    }
    return -1;
}

static PyObject *
_color_slice(register pgColorObject *a, register Py_ssize_t ilow,
             register Py_ssize_t ihigh)
{
    Py_ssize_t len;
    Py_ssize_t c1 = 0;
    Py_ssize_t c2 = 0;
    Py_ssize_t c3 = 0;
    Py_ssize_t c4 = 0;

    /* printf("ilow :%d:, ihigh:%d:\n", ilow, ihigh); */

    if (ilow < 0) {
        ilow = 0;
    }
    if (ihigh > 3) {
        ihigh = 4;
    }
    if (ihigh < ilow) {
        ihigh = ilow;
    }

    len = ihigh - ilow;
    /* printf("2 ilow :%d:, ihigh:%d: len:%d:\n", ilow, ihigh, len); */

    if (ilow == 0) {
        c1 = a->data[0];
        c2 = a->data[1];
        c3 = a->data[2];
        c4 = a->data[3];
    }
    else if (ilow == 1) {
        c1 = a->data[1];
        c2 = a->data[2];
        c3 = a->data[3];
    }
    else if (ilow == 2) {
        c1 = a->data[2];
        c2 = a->data[3];
    }
    else if (ilow == 3) {
        c1 = a->data[3];
    }

    /* return a tuple depending on which elements are wanted.  */
    if (len == 4) {
        return Py_BuildValue("(iiii)", c1, c2, c3, c4);
    }
    else if (len == 3) {
        return Py_BuildValue("(iii)", c1, c2, c3);
    }
    else if (len == 2) {
        return pg_tuple_couple_from_values_int((int)c1, (int)c2);
    }
    else if (len == 1) {
        return Py_BuildValue("(i)", c1);
    }
    else {
        return Py_BuildValue("()");
    }
}

static int
_color_contains(pgColorObject *self, PyObject *arg)
{
    if (!PyLong_Check(arg)) {
        PyErr_SetString(PyExc_TypeError,
                        "'in <pygame.Color>' requires integer object");
        return -1;
    }

    long comp = PyLong_AsLong(arg);
    if (comp == -1 && PyErr_Occurred()) {
        return -1;
    }

    int i;
    for (i = 0; i < self->len; i++) {
        if (self->data[i] == comp) {
            return 1;
        }
    }

    return 0;
}

static int
_color_set_slice(pgColorObject *color, PyObject *idx, PyObject *val)
{
    if (val == NULL) {
        PyErr_SetString(PyExc_TypeError,
                        "Color object doesn't support item deletion");
        return -1;
    }
    if (PyLong_Check(idx)) {
        return _color_ass_item(color, PyLong_AsLong(idx), val);
    }
    else if (PySlice_Check(idx)) {
        Py_ssize_t start, stop, step, slicelength;
        PyObject *fastitems;
        int c;
        Py_ssize_t i, cur;

        if (PySlice_GetIndicesEx(idx, color->len, &start, &stop, &step,
                                 &slicelength) < 0) {
            return -1;
        }
        if ((step < 0 && start < stop) || (step > 0 && start > stop)) {
            stop = start;
        }

        if (!(fastitems = PySequence_Fast(val, "expected sequence"))) {
            return -1;
        }
        if (PySequence_Fast_GET_SIZE(fastitems) != slicelength) {
            PyErr_Format(PyExc_ValueError,
                         "attempting to assign sequence of length %zd "
                         "to slice of length %zd",
                         PySequence_Fast_GET_SIZE(fastitems), slicelength);
            Py_DECREF(fastitems);
            return -1;
        }

        for (cur = start, i = 0; i < slicelength; cur += step, i++) {
            PyObject *obj = PySequence_Fast_GET_ITEM(fastitems, i);
            if (PyLong_Check(obj)) {
                c = PyLong_AsLong(obj);
            }
            else {
                PyErr_SetString(PyExc_TypeError,
                                "color components must be integers");
                Py_DECREF(fastitems);
                return -1;
            }
            if (c < 0 || c > 255) {
                PyErr_SetString(PyExc_ValueError,
                                "color component must be 0-255");
                Py_DECREF(fastitems);
                return -1;
            }
            color->data[cur] = (Uint8)c;
        }

        Py_DECREF(fastitems);
        return 0;
    }
    PyErr_SetString(PyExc_IndexError, "Index must be an integer or slice");
    return -1;
}

/*
 * colorA == colorB
 * colorA != colorB
 */
static PyObject *
_color_richcompare(PyObject *o1, PyObject *o2, int opid)
{
    typedef union {
        Uint32 pixel;
        Uint8 bytes[4];
    } _rgba_t;
    _rgba_t rgba1, rgba2;

    switch (_coerce_obj(o1, rgba1.bytes)) {
        case -1:
            return 0;
        case 0:
            goto Unimplemented;
        default:
            break;
    }
    switch (_coerce_obj(o2, rgba2.bytes)) {
        case -1:
            return 0;
        case 0:
            goto Unimplemented;
        default:
            break;
    }

    switch (opid) {
        case Py_EQ:
            return PyBool_FromLong(rgba1.pixel == rgba2.pixel);
        case Py_NE:
            return PyBool_FromLong(rgba1.pixel != rgba2.pixel);
        default:
            break;
    }

Unimplemented:
    Py_INCREF(Py_NotImplemented);
    return Py_NotImplemented;
}

static int
_color_getbuffer(pgColorObject *color, Py_buffer *view, int flags)
{
    static char format[] = "B";

    if (PyBUF_HAS_FLAG(flags, PyBUF_WRITABLE)) {
        PyErr_SetString(pgExc_BufferError, "color buffer is read-only");
        return -1;
    }
    view->buf = color->data;
    view->ndim = 1;
    view->itemsize = 1;
    view->len = color->len;
    view->readonly = 1;
    if (PyBUF_HAS_FLAG(flags, PyBUF_ND)) {
        view->ndim = 1;
        view->shape = &view->len;
    }
    else {
        view->ndim = 0;
        view->shape = 0;
    }
    if (PyBUF_HAS_FLAG(flags, PyBUF_FORMAT)) {
        view->format = format;
    }
    else {
        view->format = 0;
    }
    if (PyBUF_HAS_FLAG(flags, PyBUF_STRIDES)) {
        view->strides = &view->itemsize;
    }
    else {
        view->strides = 0;
    }
    view->suboffsets = 0;
    Py_INCREF(color);
    view->obj = (PyObject *)color;
    return 0;
}

static PyObject *
_color_getAttr_swizzle(pgColorObject *self, PyObject *attr_name)
{
    const char *attr = NULL;
    PyObject *attr_unicode = NULL;
    Py_ssize_t i, len = PySequence_Length(attr_name);
    PyObject *res = NULL;
    Uint8 value;

    if (len == 1) {
        return PyObject_GenericGetAttr((PyObject *)self, attr_name);
    }

    attr_unicode = PyUnicode_FromObject(attr_name);
    if (attr_unicode == NULL) {
        goto swizzle_failed;
    }
    attr = PyUnicode_AsUTF8AndSize(attr_unicode, &len);
    if (attr == NULL) {
        goto swizzle_error;
    }

    /* If we are not a swizzle, go straight to GenericGetAttr. */
    if ((attr[0] != 'r') && (attr[0] != 'g') && (attr[0] != 'b') &&
        (attr[0] != 'a')) {
        goto swizzle_failed;
    }

    if (len == 4) {
        static Uint8 rgba[4];
        res = (PyObject *)pgColor_New(rgba);
    }
    else {
        /* More than 3, we return a tuple. */
        res = (PyObject *)PyTuple_New(len);
    }

    if (res == NULL) {
        goto swizzle_error;
    }

    for (i = 0; i < len; i++) {
        switch (attr[i]) {
            case 'r':
                value = self->data[0];
                break;
            case 'g':
                value = self->data[1];
                break;
            case 'b':
                value = self->data[2];
                break;
            case 'a':
                value = self->data[3];
                break;

            case '0':
                value = 0;
                break;
            case '1':
                value = 255;
                break;

            default:
                goto swizzle_failed;
        }

        if (len == 4) {
            ((pgColorObject *)res)->data[i] = value;
        }
        else {
            if (PyTuple_SetItem(res, i, PyLong_FromLong(value)) != 0) {
                goto swizzle_error;
            }
        }
    }

    /* Swizzling succeeded! */
    Py_XDECREF(attr_unicode);
    return res;

    /* Swizzling failed! Fallback to PyObject_GenericGetAttr */
swizzle_failed:
    Py_XDECREF(res);
    Py_XDECREF(attr_unicode);
    return PyObject_GenericGetAttr((PyObject *)self, attr_name);

    /* Something else happened while trying to swizzle. Return NULL */
swizzle_error:
    Py_XDECREF(res);
    Py_XDECREF(attr_unicode);
    return NULL;
}

static int
_color_setAttr_swizzle(pgColorObject *self, PyObject *attr_name, PyObject *val)
{
    const char *attr = NULL;
    PyObject *attr_unicode = NULL;
    Py_ssize_t i, len = PySequence_Length(attr_name);
    Uint8 entry[4] = {0};
    int entry_was_set[4] = {0};

    if (len == 1) {
        return PyObject_GenericSetAttr((PyObject *)self, attr_name, val);
    }

    /* Handle string and unicode uniformly */
    attr_unicode = PyUnicode_FromObject(attr_name);
    if (attr_unicode == NULL) {
        return -1;
    }
    attr = PyUnicode_AsUTF8AndSize(attr_unicode, &len);

    if (attr == NULL) {
        goto swizzle_error;
    }

    /* Make sure that it's supposed to be a swizzle */
    for (i = 0; i < len; ++i) {
        switch (attr[i]) {
            case 'r':
            case 'g':
            case 'b':
            case 'a':
                break;

            default:
                goto swizzle_failed;
        }
    }

    for (i = 0; i < len; ++i) {
        int idx;
        PyObject *entry_obj;
        long entry_long;

        switch (attr[i]) {
            case 'r':
                idx = 0;
                break;
            case 'g':
                idx = 1;
                break;
            case 'b':
                idx = 2;
                break;
            case 'a':
                idx = 3;
                break;
            default:
                /* Swizzle failed */
                goto swizzle_failed;
        }

        if (idx >= 4) {
            /* Swizzle failed */
            goto swizzle_failed;
        }

        if (entry_was_set[idx]) {
            PyErr_SetString(PyExc_AttributeError,
                            "Attribute assignment conflicts with swizzling");
            goto swizzle_error;
        }

        entry_was_set[idx] = 1;
        entry_obj = PySequence_GetItem(val, i);
        if (entry_obj == NULL) {
            PyErr_SetString(
                PyExc_TypeError,
                "A sequence of the corresponding elements is expected");
            goto swizzle_error;
        }

        entry_long = PyLong_AsLong(entry_obj);
        Py_DECREF(entry_obj);
        if (PyErr_Occurred()) {
            goto swizzle_error;
        }

        if (entry_long >= 0 && entry_long <= 255) {
            entry[idx] = (Uint8)entry_long;
        }
        else {
            PyErr_SetString(
                PyExc_TypeError,
                "Color element is outside of the range from 0 to 255");
            goto swizzle_error;
        }
    }

    /* Swizzle successful */
    for (i = 0; i < 4; ++i) {
        if (entry_was_set[i]) {
            self->data[i] = entry[i];
        }
    }
    return 0;

    /* Swizzling failed! Fallback to PyObject_GenericSetAttr */
swizzle_failed:
    Py_DECREF(attr_unicode);
    return PyObject_GenericSetAttr((PyObject *)self, attr_name, val);

    /* Something else happened. Immediately return */
swizzle_error:
    Py_DECREF(attr_unicode);
    return -1;
}

/**** C API interfaces ****/
static PyObject *
pgColor_New(Uint8 rgba[])
{
    return (PyObject *)_color_new_internal(&pgColor_Type, rgba);
}

static PyObject *
pgColor_NewLength(Uint8 rgba[], Uint8 length)
{
    if (length < 1 || length > 4) {
        return PyErr_Format(PyExc_ValueError,
                            "Expected length within range [1,4]: got %d",
                            (int)length);
    }

    return (PyObject *)_color_new_internal_length(&pgColor_Type, rgba, length);
}

static int
_pg_pylong_to_uint32(PyObject *val, Uint32 *color, int handle_negative)
{
    int overflow;
    long longval = PyLong_AsLongAndOverflow(val, &overflow);
    if (overflow == 1) {
        /* Can happen when long is 32-bit (like on windows). We first try
         * to recover */
        unsigned long ulongval = PyLong_AsUnsignedLong(val);
        if (PyErr_Occurred()) {
            /* Overflowed again, this is an error in user input */
            goto error;
        }
        *color = (Uint32)ulongval;
        return 1;
    }

    if (overflow == -1) {
        /* Do not try to recover from a negative overflow */
        goto error;
    }

    if (longval == -1 && PyErr_Occurred()) {
        /* Some other internal error occurred */
        return 0;
    }

    /* negatives are handled for backcompat with old code where mapped ints
     * with most significant bit set could be incorrectly exposed as
     * negative
     */
    if (longval > 0xFFFFFFFF || (longval < 0 && !handle_negative)) {
        goto error;
    }

    *color = (Uint32)longval;
    return 1;

error:
    PyErr_SetString(
        PyExc_ValueError,
        "invalid color argument (integer out of acceptable range)");
    return 0;
}

static int
pg_RGBAFromObjEx(PyObject *obj, Uint8 *rgba, pgColorHandleFlags handle_flags)
{
    /* Also works as a fastpath */
    if (pgColor_Check(obj)) {
        memcpy(rgba, ((pgColorObject *)obj)->data, 4);
        return 1;
    }

    if ((handle_flags & PG_COLOR_HANDLE_INT) && PyLong_Check(obj)) {
        Uint32 color;
        /* in this block, _pg_pylong_to_uint32 sets python exception */
        if (_pg_pylong_to_uint32(obj, &color, 0)) {
            rgba[0] = (Uint8)(color >> 24);
            rgba[1] = (Uint8)(color >> 16);
            rgba[2] = (Uint8)(color >> 8);
            rgba[3] = (Uint8)color;
            return 1;
        }
        return 0;
    }

    if ((handle_flags & PG_COLOR_HANDLE_STR) && PyUnicode_Check(obj)) {
        /* python exception already set */
        return _parse_color_from_text(obj, rgba) + 1;
    }

    if ((handle_flags & PG_COLOR_HANDLE_RESTRICT_SEQ) && !PyTuple_Check(obj)) {
        /* ValueError (and not TypeError) for backcompat reasons */
        PyErr_SetString(
            PyExc_ValueError,
            "invalid color (here, generic sequences are "
            "restricted, but pygame.Color and RGB[A] tuples are allowed)");
        return 0;
    }

    if (pg_RGBAFromObj(obj, rgba)) {
        /* success */
        return 1;
    }

    /* Error */
    if (PySequence_Check(obj)) {
        /* It was a tuple-like; raise a ValueError */
        PyErr_SetString(
            PyExc_ValueError,
            "invalid color (color sequence must have size 3 or "
            "4, and each element must be an integer in the range [0, "
            "255])");
    }
    else {
        PyErr_Format(PyExc_TypeError,
                     "unable to interpret object of type '%128s' as a color",
                     Py_TYPE(obj)->tp_name);
    }
    return 0;
}

static int
pg_MappedColorFromObj(PyObject *val, SDL_Surface *surf, Uint32 *color,
                      pgColorHandleFlags handle_flags)
{
    Uint8 rgba[] = {0, 0, 0, 0};
    if (!val) {
        return 0;
    }

    if ((handle_flags & PG_COLOR_HANDLE_INT) && PyLong_Check(val)) {
        return _pg_pylong_to_uint32(val, color, 1);
    }

    /* int is already handled, unset it */
    handle_flags &= ~PG_COLOR_HANDLE_INT;
    if (pg_RGBAFromObjEx(val, rgba, handle_flags)) {
#if SDL_VERSION_ATLEAST(3, 0, 0)
        *color = SDL_MapSurfaceRGBA(surf, rgba[0], rgba[1], rgba[2], rgba[3]);
#else
        *color = SDL_MapRGBA(surf->format, rgba[0], rgba[1], rgba[2], rgba[3]);
#endif
        return 1;
    }
    return 0;
}

/*DOC*/ static char _color_doc[] =
    /*DOC*/ "color module for pygame";

MODINIT_DEFINE(color)
{
    PyObject *module = NULL, *colordict_module, *apiobj;
    static void *c_api[PYGAMEAPI_COLOR_NUMSLOTS];

    static struct PyModuleDef _module = {PyModuleDef_HEAD_INIT,
                                         "color",
                                         _color_doc,
                                         -1,
                                         NULL,
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

    colordict_module = PyImport_ImportModule("pygame.colordict");
    if (!colordict_module) {
        return NULL;
    }

    _COLORDICT = PyObject_GetAttrString(colordict_module, "THECOLORS");
    Py_DECREF(colordict_module);
    if (!_COLORDICT) {
        return NULL;
    }

    /* create the module */
    module = PyModule_Create(&_module);
    if (!module) {
        goto error;
    }

    if (PyModule_AddType(module, &pgColor_Type)) {
        goto error;
    }
    if (PyModule_AddObjectRef(module, "THECOLORS", _COLORDICT)) {
        goto error;
    }

    c_api[0] = &pgColor_Type;
    c_api[1] = pgColor_New;
    c_api[2] = pg_RGBAFromObjEx;
    c_api[3] = pgColor_NewLength;
    c_api[4] = pg_MappedColorFromObj;

    apiobj = encapsulate_api(c_api, "color");
    if (PyModule_AddObject(module, PYGAMEAPI_LOCAL_ENTRY, apiobj)) {
        Py_XDECREF(apiobj);
        goto error;
    }
    return module;

error:
    Py_XDECREF(module);
    Py_DECREF(_COLORDICT);
    return NULL;
}
