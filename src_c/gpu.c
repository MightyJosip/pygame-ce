#define PYGAMEAPI_GPU_INTERNAL

#include "pygame.h"

#include "pgcompat.h"

static SDL_GPUDevice *device;

#define DRAW_CODE(...) \
    cmdbuf = SDL_AcquireGPUCommandBuffer(device);                                                \
    if (cmdbuf == NULL) {                                                                        \
        return RAISE(pgExc_SDLError, SDL_GetError());                                            \
    }                                                                                            \
    if (!SDL_WaitAndAcquireGPUSwapchainTexture(cmdbuf, window->_win, &swapchainTexture, NULL, NULL)) { \
        return RAISE(pgExc_SDLError, SDL_GetError());                                            \
    }                                                                                            \
    if (swapchainTexture != NULL) {                                                              \
        __VA_ARGS__                                                                              \
    }                                                                                            \
    SDL_SubmitGPUCommandBuffer(cmdbuf);

/* GPU Functions */
static PyObject *
init(PyObject *self, PyObject *args, PyObject *kwargs)
{
    if (!SDL_WasInit(SDL_INIT_VIDEO)) {
        SDL_InitSubSystem(SDL_INIT_VIDEO);
    }
    device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV | SDL_GPU_SHADERFORMAT_DXIL | SDL_GPU_SHADERFORMAT_MSL, false, NULL);
    if (device == NULL) {
        return RAISE(pgExc_SDLError, SDL_GetError());
    }
    Py_RETURN_NONE;
}

static PyObject *
claim_window(PyObject *self, PyObject *args, PyObject *kwargs)
{
    pgWindowObject *window;
    char *keywords[] = {"window", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O!", keywords,
                                     &pgWindow_Type, &window)) {
        return NULL;
    }
    if (!SDL_ClaimWindowForGPUDevice(device, window->_win)) {
        return RAISE(pgExc_SDLError, SDL_GetError());
    }
    Py_RETURN_NONE;
}

static PyObject *
fill(PyObject *self, PyObject *args, PyObject *kwargs)
{
    PyObject *colorobj;
    pgWindowObject *window;
    SDL_GPUCommandBuffer* cmdbuf;
    SDL_GPUTexture* swapchainTexture;
    Uint8 rgba[4];
    char *keywords[] = {"window", "color", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O!O", keywords,
                                     &pgWindow_Type, &window, &colorobj)) {
        return NULL;
    }
    if (!pg_RGBAFromObjEx(colorobj, rgba, PG_COLOR_HANDLE_ALL)) {
        return NULL;
    }
    cmdbuf = SDL_AcquireGPUCommandBuffer(device);
    if (cmdbuf == NULL) {
        return RAISE(pgExc_SDLError, SDL_GetError());
    }
    if (!SDL_WaitAndAcquireGPUSwapchainTexture(cmdbuf, window->_win, &swapchainTexture, NULL, NULL)) {
        return RAISE(pgExc_SDLError, SDL_GetError());
    }
    if (swapchainTexture != NULL) {
		SDL_GPUColorTargetInfo colorTargetInfo = { 0 };
		colorTargetInfo.texture = swapchainTexture;
		colorTargetInfo.clear_color = (SDL_FColor){ (float)rgba[0] / 255, (float)rgba[1] / 255, (float)rgba[2] / 255, (float)rgba[3] / 255 };
		colorTargetInfo.load_op = SDL_GPU_LOADOP_CLEAR;
		colorTargetInfo.store_op = SDL_GPU_STOREOP_STORE;
		SDL_GPURenderPass* renderPass = SDL_BeginGPURenderPass(cmdbuf, &colorTargetInfo, 1, NULL);
		SDL_EndGPURenderPass(renderPass);
    }
    SDL_SubmitGPUCommandBuffer(cmdbuf);
    Py_RETURN_NONE;
}

static PyObject *
quit(PyObject *self, PyObject *args, PyObject *kwargs)
{
    if (device != NULL) {
        SDL_DestroyGPUDevice(device);
        device = NULL;
    }
    Py_RETURN_NONE;
}

static PyMethodDef gpu_methods[] = {
    {"init", (PyCFunction)init, METH_NOARGS, NULL},
    {"claim_window", (PyCFunction)claim_window, METH_VARARGS | METH_KEYWORDS, NULL},
    {"fill", (PyCFunction)fill, METH_VARARGS | METH_KEYWORDS, NULL},
    {"quit", (PyCFunction)quit, METH_NOARGS, NULL},
    {NULL, NULL, 0, NULL}
};

MODINIT_DEFINE(gpu)
{
    PyObject *module, *apiobj;
    static void *c_api[PYGAMEAPI_GPU_NUMSLOTS];

    static struct PyModuleDef _module = {PyModuleDef_HEAD_INIT,
                                         "gpu",
                                         "docs_needed",
                                         -1,
                                         gpu_methods,
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
    import_pygame_window();
    if (PyErr_Occurred()) {
        return NULL;
    }
    import_pygame_rwobject();
    if (PyErr_Occurred()) {
        return NULL;
    }
    import_pygame_color();
    if (PyErr_Occurred()) {
        return NULL;
    }

    /* create the module */
    module = PyModule_Create(&_module);
    if (module == 0) {
        return NULL;
    }

    apiobj = encapsulate_api(c_api, "_gpu");
    if (PyModule_AddObject(module, PYGAMEAPI_LOCAL_ENTRY, apiobj)) {
        Py_XDECREF(apiobj);
        Py_DECREF(module);
        return NULL;
    }

    return module;
}