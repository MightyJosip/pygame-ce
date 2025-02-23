#define PYGAMEAPI_GPU_INTERNAL

#include "pygame.h"

#include "pgcompat.h"

static PyTypeObject pgGPUDevice_Type;

static PyTypeObject pgShader_Type;

static PyTypeObject pgPipeline_Type;

#define pgGPUDevice_Check(x) \
    (PyObject_IsInstance((x), (PyObject *)&pgGPUDevice_Type))

#define pgShader_Check(x) \
    (PyObject_IsInstance((x), (PyObject *)&pgShader_Type))

#define pgPipeline_Check(x) \
    (PyObject_IsInstance((x), (PyObject *)&pgPipeline_Type))

/* GPUDevice implementation */
static int
gpu_device_init(pgGPUDeviceObject *self, PyObject *args, PyObject *kwargs)
{
    pgWindowObject *window;
    char *keywords[] = {"window", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O!", keywords,
                                     &pgWindow_Type, &window)) {
        return -1;
    }
    self->device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV | SDL_GPU_SHADERFORMAT_DXIL | SDL_GPU_SHADERFORMAT_MSL, false, NULL);
    if (self->device == NULL) {
        PyErr_SetString(pgExc_SDLError, "GPUCreateDevice failed");
        return -1;
    }
    self->window = window;
    if (!SDL_ClaimWindowForGPUDevice(self->device, self->window->_win)) {
        PyErr_SetString(pgExc_SDLError, "GPUClaimWindow failed");
        return -1;
    }
    return 0;
}

static void
gpu_device_dealloc(pgGPUDeviceObject *self, PyObject *_null)
{
    SDL_ReleaseWindowFromGPUDevice(self->device, self->window->_win);
    SDL_DestroyGPUDevice(self->device);
    Py_TYPE(self)->tp_free(self);
}

/* Shader implementation */
static int
shader_init(pgShaderObject *self, PyObject *args, PyObject *kwargs)
{
    pgGPUDeviceObject *gpu_device;
    PyObject* bytes_object;
    const char *stage_str;
    int samplers = 0, uniform_buffers = 0, storage_buffers = 0, storage_textures = 0;
    char *keywords[] = {"device", "code", "stage", "samplers", "uniform_buffers", "storage_buffers", "storage_textures", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O!Ss|iiii", keywords,
                                     &pgGPUDevice_Type, &gpu_device, &bytes_object, &stage_str, &samplers,
                                     &uniform_buffers, &storage_buffers, &storage_textures)) {
        return -1;
    }
    void *code = (void*)PyBytes_AsString(bytes_object);
    SDL_GPUShaderStage stage;
    if (!strcmp(stage_str, "vert")) {
        stage = SDL_GPU_SHADERSTAGE_VERTEX;
    }
    else if (!strcmp(stage_str, "frag")) {
        stage = SDL_GPU_SHADERSTAGE_FRAGMENT;
    }
    else {
        PyErr_SetString(pgExc_SDLError, "Invalid shader stage");
        return -1;
    }
    self->device = gpu_device;
    SDL_GPUShaderFormat backendFormats = SDL_GetGPUShaderFormats(self->device->device);
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
        PyErr_SetString(pgExc_SDLError, "Unrecognized backend shader format!");
		return -1;
	}
    Py_ssize_t length = PyBytes_Size(bytes_object);
	SDL_GPUShaderCreateInfo shaderInfo = {
		.code = code,
		.code_size = length,
		.entrypoint = entrypoint,
		.format = format,
		.stage = stage,
		.num_samplers = samplers,
		.num_uniform_buffers = uniform_buffers,
		.num_storage_buffers = storage_buffers,
		.num_storage_textures = storage_textures
	};
    SDL_GPUShader* shader = SDL_CreateGPUShader(self->device->device, &shaderInfo);
    if (shader == NULL) {
        PyErr_SetString(pgExc_SDLError, "Failed to create shader!");
        return -1;
    }
    self->shader = shader;
    return 0;
}

static void
shader_dealloc(pgShaderObject *self, PyObject *_null)
{
    SDL_ReleaseGPUShader(self->device->device, self->shader);
    Py_TYPE(self)->tp_free(self);
}

/* Pipeline implementation */
static int
pipeline_init(pgPipelineObject *self, PyObject *args, PyObject *kwargs)
{
    pgGPUDeviceObject *gpu_device;
    SDL_GPUGraphicsPipeline *pipeline;
    pgShaderObject *vertex_shader, *fragment_shader;
    char *keywords[] = {"device", "vertex_shader", "fragment_shader", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O!O!O!", keywords,
                                     &pgGPUDevice_Type, &gpu_device, &pgShader_Type, &vertex_shader, &pgShader_Type, &fragment_shader)) {
        return -1;
    }
    self->device = gpu_device;
	SDL_GPUGraphicsPipelineCreateInfo pipelineCreateInfo = {
		.target_info = {
			.num_color_targets = 1,
			.color_target_descriptions = (SDL_GPUColorTargetDescription[]){{
				.format = SDL_GetGPUSwapchainTextureFormat(gpu_device->device, gpu_device->window->_win)
			}},
		},
		.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
		.vertex_shader = vertex_shader->shader,
		.fragment_shader = fragment_shader->shader,
	};
    pipelineCreateInfo.rasterizer_state.fill_mode = SDL_GPU_FILLMODE_FILL;
    pipeline = SDL_CreateGPUGraphicsPipeline(gpu_device->device, &pipelineCreateInfo);
    if (pipeline == NULL) {
        PyErr_SetString(pgExc_SDLError, "Failed to create pipeline!");
        return -1;
    }
    self->pipeline = pipeline;
    return 0;
}

static void
pipeline_dealloc(pgPipelineObject *self, PyObject *_null)
{
    SDL_ReleaseGPUGraphicsPipeline(self->device->device, self->pipeline);
    Py_TYPE(self)->tp_free(self);
}

/* GPU Functions */
static PyObject *
draw(PyObject *self, PyObject *args, PyObject *kwargs)
{
    pgGPUDeviceObject *gpu_device;
    pgPipelineObject *pipeline;
    printf("HERE 1\n");
    char *keywords[] = {"device", "pipeline", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O!O!", keywords,
                                     &pgGPUDevice_Type, &gpu_device, &pgPipeline_Type, &pipeline)) {
        return NULL;
    }
    printf("HERE 2\n");
    SDL_GPUCommandBuffer* cmdbuf = SDL_AcquireGPUCommandBuffer(gpu_device->device);
    if (cmdbuf == NULL) {
        return RAISE(pgExc_SDLError, "AcquireGPUCommandBuffer failed");
    }
    SDL_GPUTexture* swapchainTexture;
    printf("HERE 3\n");
    if (!SDL_WaitAndAcquireGPUSwapchainTexture(cmdbuf, gpu_device->window->_win, &swapchainTexture, NULL, NULL)) {
        return RAISE(pgExc_SDLError, "SDL_WaitAndAcquireGPUSwapchainTexture failed");
    }
    printf("HERE 4\n");
    if (swapchainTexture != NULL) {
        printf("HERE 5\n");
        SDL_GPUColorTargetInfo colorTargetInfo = { 0 };
        colorTargetInfo.texture = swapchainTexture;
        colorTargetInfo.clear_color = (SDL_FColor){ 0.0f, 0.0f, 0.0f, 1.0f };
        colorTargetInfo.load_op = SDL_GPU_LOADOP_CLEAR;
        colorTargetInfo.store_op = SDL_GPU_STOREOP_STORE;

        printf("HERE 6\n");
        SDL_GPURenderPass* renderPass = SDL_BeginGPURenderPass(cmdbuf, &colorTargetInfo, 1, NULL);
        SDL_BindGPUGraphicsPipeline(renderPass, pipeline->pipeline);
        SDL_DrawGPUPrimitives(renderPass, 3, 1, 0, 0);
        SDL_EndGPURenderPass(renderPass);
    }
    printf("HERE 7\n");
    SDL_SubmitGPUCommandBuffer(cmdbuf);
    printf("HERE 8\n");
    Py_RETURN_NONE;
}

static PyMethodDef gpu_device_methods[] = {{NULL, NULL, 0, NULL}};

static PyGetSetDef gpu_device_getset[] = {{NULL, 0, NULL, NULL, NULL}};

static PyMethodDef shader_methods[] = {{NULL, NULL, 0, NULL}};

static PyGetSetDef shader_getset[] = {{NULL, 0, NULL, NULL, NULL}};

static PyMethodDef pipeline_methods[] = {{NULL, NULL, 0, NULL}};

static PyGetSetDef pipeline_getset[] = {{NULL, 0, NULL, NULL, NULL}};

static PyTypeObject pgGPUDevice_Type = {
    PyVarObject_HEAD_INIT(NULL, 0).tp_name = "pygame._gpu.GPUDevice",
    .tp_basicsize = sizeof(pgGPUDeviceObject),
    .tp_dealloc = (destructor)gpu_device_dealloc,
    //.tp_doc = DOC_GPU_GPUDEVICE,
    .tp_methods = gpu_device_methods,
    .tp_init = (initproc)gpu_device_init,
    .tp_new = PyType_GenericNew,
    .tp_getset = gpu_device_getset
};

static PyTypeObject pgShader_Type = {
    PyVarObject_HEAD_INIT(NULL, 0).tp_name = "pygame._gpu.Shader",
    .tp_basicsize = sizeof(pgShaderObject),
    .tp_dealloc = (destructor)shader_dealloc,
    //.tp_doc = DOC_GPU_SHADER,
    .tp_methods = shader_methods,
    .tp_init = (initproc)shader_init,
    .tp_new = PyType_GenericNew,
    .tp_getset = shader_getset
};

static PyTypeObject pgPipeline_Type = {
    PyVarObject_HEAD_INIT(NULL, 0).tp_name = "pygame._gpu.Pipeline",
    .tp_basicsize = sizeof(pgPipelineObject),
    .tp_dealloc = (destructor)pipeline_dealloc,
    //.tp_doc = DOC_GPU_PIPELINE,
    .tp_methods = pipeline_methods,
    .tp_init = (initproc)pipeline_init,
    .tp_new = PyType_GenericNew,
    .tp_getset = pipeline_getset
};

static PyMethodDef _gpu_methods[] = {
    {"draw", (PyCFunction)draw, METH_VARARGS | METH_KEYWORDS,
        NULL},
    {NULL, NULL, 0, NULL}
};

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
    import_pygame_window();
    if (PyErr_Occurred()) {
        return NULL;
    }
    import_pygame_rwobject();
    if (PyErr_Occurred()) {
        return NULL;
    }

    if (PyType_Ready(&pgGPUDevice_Type) < 0) {
        return NULL;
    }

    if (PyType_Ready(&pgShader_Type) < 0) {
        return NULL;
    }

    if (PyType_Ready(&pgPipeline_Type) < 0) {
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

    Py_INCREF(&pgShader_Type);
    if (PyModule_AddObject(module, "Shader", (PyObject *)&pgShader_Type)) {
        Py_DECREF(&pgShader_Type);
        Py_DECREF(module);
        return NULL;
    }

    Py_INCREF(&pgPipeline_Type);
    if (PyModule_AddObject(module, "Pipeline", (PyObject *)&pgPipeline_Type)) {
        Py_DECREF(&pgPipeline_Type);
        Py_DECREF(module);
        return NULL;
    }

    c_api[0] = &pgGPUDevice_Type;
    c_api[1] = &pgShader_Type;
    c_api[2] = &pgPipeline_Type;
    apiobj = encapsulate_api(c_api, "_gpu");
    if (PyModule_AddObject(module, PYGAMEAPI_LOCAL_ENTRY, apiobj)) {
        Py_XDECREF(apiobj);
        Py_DECREF(module);
        return NULL;
    }

    return module;
}