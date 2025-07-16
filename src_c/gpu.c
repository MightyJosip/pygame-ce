#define PYGAMEAPI_GPU_INTERNAL

#include "pygame.h"

#include "pgcompat.h"

static SDL_GPUDevice *device;

static PyTypeObject pgShader_Type;

static PyTypeObject pgRenderPass_Type;

#define pgShader_Check(x) \
    (PyObject_IsInstance((x), (PyObject *)&pgShader_Type))

#define pgRenderPass_Check(x) \
    (PyObject_IsInstance((x), (PyObject *)&pgRenderPass_Type))

#define DEC_CONSTS_(x, y)                           \
    if (PyModule_AddIntConstant(module, x, (int)y)) \
    {                                               \
        Py_DECREF(module);                          \
        return NULL;                                \
    }
#define DEC_CONST(x) DEC_CONSTS_(#x, SDL_##x)

/* Shader implementation */
static int
shader_init(pgShaderObject *self, PyObject *args, PyObject *kwargs)
{
    PyObject* file;
    SDL_RWops *rw = NULL;
    int stage, samplers = 0, uniform_buffers = 0, storage_buffers = 0, storage_textures = 0;
    char *keywords[] = {"file", "stage", "samplers", "uniform_buffers", "storage_buffers", "storage_textures", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "Oi|iiii", keywords,
                                     &file, &stage, &samplers,
                                     &uniform_buffers, &storage_buffers, &storage_textures)) {
        return -1;
    }
    if (device == NULL) {
        RAISERETURN(pgExc_SDLError, "gpu module hasn't been initialized!", -1)
    }
    rw = pgRWops_FromObject(file, NULL);
    if (rw == NULL) {
        RAISERETURN(pgExc_SDLError, "Unable to read file", -1);
    }
    size_t code_size = SDL_GetIOSize(rw);
    Uint8 *code = (Uint8 *)malloc(code_size);
    SDL_ReadIO(rw, code, code_size);

    SDL_GPUShaderFormat backendFormats = SDL_GetGPUShaderFormats(device);
    SDL_GPUShaderFormat format = SDL_GPU_SHADERFORMAT_INVALID;
    const char *entrypoint;
    if (backendFormats & SDL_GPU_SHADERFORMAT_SPIRV) {
        format = SDL_GPU_SHADERFORMAT_SPIRV;
        entrypoint = "main";
    } else if (backendFormats & SDL_GPU_SHADERFORMAT_MSL) {
        format = SDL_GPU_SHADERFORMAT_MSL;
        entrypoint = "main0";
    } else if (backendFormats & SDL_GPU_SHADERFORMAT_DXIL) {
		format = SDL_GPU_SHADERFORMAT_DXIL;
		entrypoint = "main";
	} else {
        RAISERETURN(pgExc_SDLError, "Unrecognized backend shader format!", -1);
	}
	SDL_GPUShaderCreateInfo shaderInfo = {
		.code = code,
		.code_size = code_size,
		.entrypoint = entrypoint,
		.format = format,
		.stage = stage,
		.num_samplers = samplers,
		.num_uniform_buffers = uniform_buffers,
		.num_storage_buffers = storage_buffers,
		.num_storage_textures = storage_textures
	};
    SDL_GPUShader* shader = SDL_CreateGPUShader(device, &shaderInfo);
    if (shader == NULL) {
        PyErr_SetString(pgExc_SDLError, SDL_GetError());
        return -1;
    }
    self->shader = shader;
    free(code);
    return 0;
}

static void
shader_dealloc(pgShaderObject *self, PyObject *_null)
{
    if (device != NULL && self->shader) {
        SDL_ReleaseGPUShader(device, self->shader);
    }
    Py_TYPE(self)->tp_free(self);
}

/* RenderPass implementation */
static int
render_pass_init(pgRenderPassObject *self, PyObject *args, PyObject *kwargs)
{
    PyObject *colorobj = NULL;
    SDL_GPULoadOp load_op = 0;
    SDL_GPUStoreOp store_op = 0;
    Uint8 rgba[4];
    SDL_FColor floatrgba = { 0 };
    char *keywords[] = {"clear_color", "load_op", "store_op", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|Oii", keywords,
                                     &colorobj, &load_op, &store_op)) {
        return -1;
    }
    if (colorobj != NULL) {
        if (!pg_RGBAFromObjEx(colorobj, rgba, PG_COLOR_HANDLE_ALL)) {
            return -1;
        }
        floatrgba.r = (float)rgba[0] / 255;
        floatrgba.g = (float)rgba[1] / 255;
        floatrgba.b = (float)rgba[2] / 255;
        floatrgba.a = (float)rgba[3] / 255;
    }
    self->color_info = (SDL_GPUColorTargetInfo){ 0 };
    self->color_info.clear_color = floatrgba;
    self->color_info.load_op = load_op;
    self->color_info.store_op = store_op;
    return 0;
}

static void
render_pass_dealloc(pgRenderPassObject *self, PyObject *_null)
{
    if (device != NULL && self->render_pass) {
        SDL_EndGPURenderPass(self->render_pass);
    }
    Py_TYPE(self)->tp_free(self);
}

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
submit(PyObject *self, PyObject *args, PyObject *kwargs)
{
    pgWindowObject *window;
    pgRenderPassObject *render_pass;
    SDL_GPUCommandBuffer* cmdbuf;
    SDL_GPUTexture* swapchainTexture;
    char *keywords[] = {"window", "render_pass", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O!O!", keywords,
                                     &pgWindow_Type, &window, &pgRenderPass_Type, &render_pass)) {
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
        render_pass->color_info.texture = swapchainTexture;
        render_pass->render_pass = SDL_BeginGPURenderPass(cmdbuf, &render_pass->color_info, 1, NULL);
        SDL_EndGPURenderPass(render_pass->render_pass);
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

static PyMethodDef shader_methods[] = {{NULL, NULL, 0, NULL}};

static PyGetSetDef shader_getset[] = {{NULL, 0, NULL, NULL, NULL}};

static PyMethodDef render_pass_methods[] = {{NULL, NULL, 0, NULL}};

static PyGetSetDef render_pass_getset[] = {{NULL, 0, NULL, NULL, NULL}};

static PyTypeObject pgShader_Type = {
    PyVarObject_HEAD_INIT(NULL, 0).tp_name = "pygame.gpu.Shader",
    .tp_basicsize = sizeof(pgShaderObject),
    .tp_dealloc = (destructor)shader_dealloc,
    //.tp_doc = DOC_GPU_SHADER,
    .tp_methods = shader_methods,
    .tp_init = (initproc)shader_init,
    .tp_new = PyType_GenericNew,
    .tp_getset = shader_getset
};

static PyTypeObject pgRenderPass_Type = {
    PyVarObject_HEAD_INIT(NULL, 0).tp_name = "pygame.gpu.RenderPass",
    .tp_basicsize = sizeof(pgRenderPassObject),
    .tp_dealloc = (destructor)render_pass_dealloc,
    //.tp_doc = DOC_GPU_RENDER_PASS,
    .tp_methods = render_pass_methods,
    .tp_init = (initproc)render_pass_init,
    .tp_new = PyType_GenericNew,
    .tp_getset = render_pass_getset
};

static PyMethodDef gpu_methods[] = {
    {"init", (PyCFunction)init, METH_NOARGS, NULL},
    {"claim_window", (PyCFunction)claim_window, METH_VARARGS | METH_KEYWORDS, NULL},
    {"submit", (PyCFunction)submit, METH_VARARGS | METH_KEYWORDS, NULL},
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

    if (PyModule_AddType(module, &pgShader_Type)) {
        Py_XDECREF(module);
        return NULL;
    }

    if (PyModule_AddType(module, &pgRenderPass_Type)) {
        Py_XDECREF(module);
        return NULL;
    }

    DEC_CONST(GPU_SHADERSTAGE_VERTEX);
    DEC_CONST(GPU_SHADERSTAGE_FRAGMENT);
    DEC_CONST(GPU_LOADOP_LOAD);
    DEC_CONST(GPU_LOADOP_CLEAR);
    DEC_CONST(GPU_LOADOP_DONT_CARE);
    DEC_CONST(GPU_STOREOP_STORE);
    DEC_CONST(GPU_STOREOP_DONT_CARE);
    DEC_CONST(GPU_STOREOP_RESOLVE);
    DEC_CONST(GPU_STOREOP_RESOLVE_AND_STORE);

    apiobj = encapsulate_api(c_api, "gpu");
    if (PyModule_AddObject(module, PYGAMEAPI_LOCAL_ENTRY, apiobj)) {
        Py_XDECREF(apiobj);
        Py_DECREF(module);
        return NULL;
    }

    return module;
}