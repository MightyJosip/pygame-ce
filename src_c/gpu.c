#define PYGAMEAPI_GPU_INTERNAL

#include "pygame.h"

#include "pgcompat.h"

static PyTypeObject pgGPUDevice_Type;

static PyMethodDef gpu_device_methods[] = {{NULL, NULL, 0, NULL}};

static PyGetSetDef gpu_device_getset[] = {{NULL, 0, NULL, NULL, NULL}};

static PyTypeObject pgGPUDevice_Type = {
    PyVarObject_HEAD_INIT(NULL, 0).tp_name = "pygame._gpu.GPUDevice",
    .tp_basicsize = sizeof(pgGPUDeviceObject),
    //.tp_dealloc = (destructor)gpu_device_dealloc,
    //.tp_doc = DOC_GPU_GPUDEVICE,
    .tp_methods = gpu_device_methods,
    //.tp_init = (initproc)gpu_device_init,
    .tp_new = PyType_GenericNew,
    .tp_getset = gpu_device_getset
};

static PyMethodDef _gpu_methods[] = {{NULL, NULL, 0, NULL}};

MODINIT_DEFINE(_gpu)
{
    PyObject *module, *apiobj;
    static void *c_api[PYGAMEAPI_GPU_NUMSLOTS];

    static struct PyModuleDef _module = {PyModuleDef_HEAD_INIT,
                                         "_gpu",
                                         "docs_needed",
                                         -1,
                                         _gpu_methods,
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
    /*import_pygame_window();
    if (PyErr_Occurred()) {
        return NULL;
    }*/

    if (PyType_Ready(&pgGPUDevice_Type) < 0) {
        return NULL;
    }

    /* create the module */
    module = PyModule_Create(&_module);
    if (module == 0) {
        return NULL;
    }

    Py_INCREF(&pgGPUDevice_Type);
    if (PyModule_AddObject(module, "GPUDevice", (PyObject *)&pgGPUDevice_Type)) {
        Py_DECREF(&pgGPUDevice_Type);
        Py_DECREF(module);
        return NULL;
    }

    c_api[0] = &pgGPUDevice_Type;
    apiobj = encapsulate_api(c_api, "_gpu");
    if (PyModule_AddObject(module, PYGAMEAPI_LOCAL_ENTRY, apiobj)) {
        Py_XDECREF(apiobj);
        Py_DECREF(module);
        return NULL;
    }

    return module;
}