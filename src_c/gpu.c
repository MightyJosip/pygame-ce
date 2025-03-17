#define PYGAMEAPI_GPU_INTERNAL

#include "pygame.h"

#include "pgcompat.h"

static SDL_GPUDevice *device;

static PyTypeObject pgShader_Type;

static PyTypeObject pgPipeline_Type;

static PyTypeObject pgBuffer_Type;

#define pgShader_Check(x) \
    (PyObject_IsInstance((x), (PyObject *)&pgShader_Type))

#define pgPipeline_Check(x) \
    (PyObject_IsInstance((x), (PyObject *)&pgPipeline_Type))

#define pgBuffer_Check(x) \
    (PyObject_IsInstance((x), (PyObject *)&pgBuffer_Type))

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
        PyErr_SetString(pgExc_SDLError, "Unable to read file");
        return -1;
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
        PyErr_SetString(pgExc_SDLError, "Unrecognized backend shader format!");
		return -1;
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
        PyErr_SetString(pgExc_SDLError, "Failed to create shader!");
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

/* Buffer implementation */

static int
buffer_get_element_size(VertexBufferType buffer_type) {
    switch (buffer_type) {
        case POSITION_VERTEX:
            return sizeof(float) * 3;
        case POSITION_COLOR_VERTEX:
            return sizeof(float) * 3 + sizeof(Uint32);
        case POSITION_TEXTURE_VERTEX:
            return sizeof(float) * 5;
        default:
            return 0;
    }
}

typedef struct PositionColorVertex
{
    float x, y, z;
    Uint8 r, g, b, a;
} PositionColorVertex;

static PyObject *
buffer_upload(pgBufferObject *self, PyObject *args, PyObject *kwargs) {
    PyObject* data;
    int size = buffer_get_element_size(self->type) * self->no_of_elements;
    char *keywords[] = {"data", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O", keywords, &data)) {
        return NULL;
    }
    SDL_GPUTransferBuffer* transfer_buffer = SDL_CreateGPUTransferBuffer(device, &(SDL_GPUTransferBufferCreateInfo) {
        .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
        .size = size
    });
    PositionColorVertex* transfer_data = SDL_MapGPUTransferBuffer(device, transfer_buffer, false);
    for (int i = 0; i < self->no_of_elements; i++) {
        PyObject* inner_data_obj = PySequence_GetItem(data, i);
        transfer_data[i].x = (float)PyFloat_AsDouble(PySequence_GetItem(inner_data_obj, 0));
        transfer_data[i].y = (float)PyFloat_AsDouble(PySequence_GetItem(inner_data_obj, 1));
        transfer_data[i].z = (float)PyFloat_AsDouble(PySequence_GetItem(inner_data_obj, 2));
        transfer_data[i].r = (Uint8)PyLong_AsInt(PySequence_GetItem(inner_data_obj, 3));
        transfer_data[i].g = (Uint8)PyLong_AsInt(PySequence_GetItem(inner_data_obj, 4));
        transfer_data[i].b = (Uint8)PyLong_AsInt(PySequence_GetItem(inner_data_obj, 5));
        transfer_data[i].a = (Uint8)PyLong_AsInt(PySequence_GetItem(inner_data_obj, 6));
    }
    SDL_UnmapGPUTransferBuffer(device, transfer_buffer);
    SDL_GPUCommandBuffer* uploadCmdBuf = SDL_AcquireGPUCommandBuffer(device);
    SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(uploadCmdBuf);
    SDL_UploadToGPUBuffer(copyPass, &(SDL_GPUTransferBufferLocation) {
        .transfer_buffer = transfer_buffer,
        .offset = 0
    }, &(SDL_GPUBufferRegion) {
        .buffer = self->buffer,
        .offset = 0,
        .size = size
    }, false);
    SDL_EndGPUCopyPass(copyPass);
    SDL_SubmitGPUCommandBuffer(uploadCmdBuf);
    SDL_ReleaseGPUTransferBuffer(device, transfer_buffer);
    Py_RETURN_NONE;
}

static int
buffer_init(pgBufferObject *self, PyObject *args, PyObject *kwargs) {
    SDL_GPUBuffer *buffer;
    int usage, buffer_type, no_of_elements;
    Uint32 element_size;
    char *keywords[] = {"usage", "buffer_type", "size", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "iii", keywords, &usage, &buffer_type, &no_of_elements)) {
        return -1;
    }
    if (device == NULL) {
        RAISERETURN(pgExc_SDLError, "gpu module hasn't been initialized!", -1)
    }
    element_size = buffer_get_element_size(buffer_type);
    if (!element_size) {
        RAISERETURN(pgExc_SDLError, "Unknown buffer type!", -1)
    }
    buffer = SDL_CreateGPUBuffer(device, &(SDL_GPUBufferCreateInfo) {
        .usage = usage,
        .size = element_size * no_of_elements
    });
    if (buffer == NULL) {
        PyErr_SetString(pgExc_SDLError, "Failed to create buffer!");
        return -1;
    }
    self->buffer = buffer;
    self->type = buffer_type;
    self->no_of_elements = no_of_elements;
    return 0;
}

static void
buffer_dealloc(pgBufferObject *self, PyObject *_null) {
    if (device != NULL && self->buffer) {
        SDL_ReleaseGPUBuffer(device, self->buffer);
    }
    Py_TYPE(self)->tp_free(self);
}

/* Pipeline implementation */
static int
pipeline_init(pgPipelineObject *self, PyObject *args, PyObject *kwargs)
{
    SDL_GPUGraphicsPipeline *pipeline;
    int primitive_type, fill_mode, num_vertex_buffer, buffer_type;
    pgShaderObject *vertex_shader, *fragment_shader;
    pgWindowObject *window;
    char *keywords[] = {"window", "primitive_type", "vertex_shader", "fragment_shader", "fill_mode", "num_vertex_buffers", "buffer_type", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O!iO!O!|iii", keywords, &pgWindow_Type, &window,
                                     &primitive_type, &pgShader_Type, &vertex_shader,
                                     &pgShader_Type, &fragment_shader, &fill_mode, &num_vertex_buffer, &buffer_type)) {
        return -1;
    }
    if (device == NULL) {
        RAISERETURN(pgExc_SDLError, "gpu module hasn't been initialized!", -1)
    }
    SDL_GPUGraphicsPipelineCreateInfo pipelineCreateInfo = {
        .target_info = {
            .num_color_targets = 1,
            .color_target_descriptions = &(SDL_GPUColorTargetDescription){
                .format = SDL_GetGPUSwapchainTextureFormat(device, window->_win)
            },
        },
        .vertex_input_state = (SDL_GPUVertexInputState){
            .num_vertex_buffers = num_vertex_buffer,
            .vertex_buffer_descriptions = &(SDL_GPUVertexBufferDescription){
                .slot = 0,
                .input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX,
                .instance_step_rate = 0,
                .pitch = buffer_get_element_size(buffer_type)
            },
            .num_vertex_attributes = 2,
            .vertex_attributes = (SDL_GPUVertexAttribute[]){{
                .buffer_slot = 0,
                .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
                .location = 0,
                .offset = 0
            }, {
                .buffer_slot = 0,
                .format = SDL_GPU_VERTEXELEMENTFORMAT_UBYTE4_NORM,
                .location = 1,
                .offset = sizeof(float) * 3
            }}
        },
        .primitive_type = primitive_type,
        .vertex_shader = vertex_shader->shader,
        .fragment_shader = fragment_shader->shader,
        .rasterizer_state = {
            .fill_mode = fill_mode
        }
    };
    pipeline = SDL_CreateGPUGraphicsPipeline(device, &pipelineCreateInfo);
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
    if (device != NULL && self->pipeline) {
        SDL_ReleaseGPUGraphicsPipeline(device, self->pipeline);
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
        return RAISE(pgExc_SDLError, "GPUCreateDevice failed");
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
        return RAISE(pgExc_SDLError, "Unable to claim Window for GPU");
    }
    Py_RETURN_NONE;
}

static PyObject *
draw(PyObject *self, PyObject *args, PyObject *kwargs)
{
    SDL_GPUCommandBuffer* command_buffer;
    SDL_GPUTexture* swapchain_texture;
    pgWindowObject *window;
    pgPipelineObject *pipeline;
    pgBufferObject *buffer;
    char *keywords[] = {"window", "pipeline", "buffer", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O!O!|O!", keywords,
                                     &pgWindow_Type, &window, &pgPipeline_Type, &pipeline, &pgBuffer_Type, &buffer)) {
        return NULL;
    }
    command_buffer = SDL_AcquireGPUCommandBuffer(device);
    if (SDL_WaitAndAcquireGPUSwapchainTexture(command_buffer, window->_win, &swapchain_texture, NULL, NULL)) {
        SDL_GPUColorTargetInfo colorTargetInfo = { 0 };
        colorTargetInfo.texture = swapchain_texture;
        colorTargetInfo.clear_color = (SDL_FColor){ 0.0f, 0.0f, 0.0f, 1.0f };
        colorTargetInfo.load_op = SDL_GPU_LOADOP_CLEAR;
        colorTargetInfo.store_op = SDL_GPU_STOREOP_STORE;
        SDL_GPURenderPass* render_pass = SDL_BeginGPURenderPass(command_buffer, &colorTargetInfo, 1, NULL);
        SDL_BindGPUGraphicsPipeline(render_pass, pipeline->pipeline);
        if (buffer) {
            SDL_BindGPUVertexBuffers(render_pass, 0, &(SDL_GPUBufferBinding){ .buffer = buffer->buffer, .offset = 0 }, 1);
        }
        SDL_DrawGPUPrimitives(render_pass, 3, 1, 0, 0);
        SDL_EndGPURenderPass(render_pass);
    }
    SDL_SubmitGPUCommandBuffer(command_buffer);
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

static PyMethodDef pipeline_methods[] = {{NULL, NULL, 0, NULL}};

static PyGetSetDef pipeline_getset[] = {{NULL, 0, NULL, NULL, NULL}};

static PyMethodDef buffer_methods[] = {
    {"upload", (PyCFunction)buffer_upload,
        METH_VARARGS | METH_KEYWORDS, NULL},
    {NULL, NULL, 0, NULL}
};

static PyGetSetDef buffer_getset[] = {{NULL, 0, NULL, NULL, NULL}};

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

static PyTypeObject pgBuffer_Type = {
    PyVarObject_HEAD_INIT(NULL, 0).tp_name = "pygame._gpu.Buffer",
    .tp_basicsize = sizeof(pgBufferObject),
    .tp_dealloc = (destructor)buffer_dealloc,
    //.tp_doc = DOC_GPU_BUFFER,
    .tp_methods = buffer_methods,
    .tp_init = (initproc)buffer_init,
    .tp_new = PyType_GenericNew,
    .tp_getset = buffer_getset
};

static PyMethodDef _gpu_methods[] = {
    {"init", (PyCFunction)init, METH_NOARGS, NULL},
    {"claim_window", (PyCFunction)claim_window, METH_VARARGS | METH_KEYWORDS, NULL},
    {"draw", (PyCFunction)draw, METH_VARARGS | METH_KEYWORDS, NULL},
    {"quit", (PyCFunction)quit, METH_NOARGS, NULL},
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
    import_pygame_color();
    if (PyErr_Occurred()) {
        return NULL;
    }

    if (PyType_Ready(&pgShader_Type) < 0) {
        return NULL;
    }

    if (PyType_Ready(&pgBuffer_Type) < 0) {
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

    Py_INCREF(&pgShader_Type);
    if (PyModule_AddObject(module, "Shader", (PyObject *)&pgShader_Type)) {
        Py_DECREF(&pgShader_Type);
        Py_DECREF(module);
        return NULL;
    }

    Py_INCREF(&pgBuffer_Type);
    if (PyModule_AddObject(module, "Buffer", (PyObject *)&pgBuffer_Type)) {
        Py_DECREF(&pgBuffer_Type);
        Py_DECREF(module);
        return NULL;
    }

    Py_INCREF(&pgPipeline_Type);
    if (PyModule_AddObject(module, "Pipeline", (PyObject *)&pgPipeline_Type)) {
        Py_DECREF(&pgPipeline_Type);
        Py_DECREF(module);
        return NULL;
    }

    DEC_CONST(GPU_PRIMITIVETYPE_TRIANGLELIST);
    DEC_CONST(GPU_SHADERSTAGE_VERTEX);
    DEC_CONST(GPU_SHADERSTAGE_FRAGMENT);
    DEC_CONST(GPU_FILLMODE_FILL);
    DEC_CONST(GPU_FILLMODE_LINE);
    DEC_CONST(GPU_BUFFERUSAGE_VERTEX);
    DEC_CONSTS_("POSITION_VERTEX", POSITION_VERTEX);
    DEC_CONSTS_("POSITION_COLOR_VERTEX", POSITION_COLOR_VERTEX);
    DEC_CONSTS_("POSITION_TEXTURE_VERTEX", POSITION_TEXTURE_VERTEX);

    c_api[0] = &pgShader_Type;
    c_api[1] = &pgBuffer_Type;
    c_api[2] = &pgPipeline_Type;
    apiobj = encapsulate_api(c_api, "_gpu");
    if (PyModule_AddObject(module, PYGAMEAPI_LOCAL_ENTRY, apiobj)) {
        Py_XDECREF(apiobj);
        Py_DECREF(module);
        return NULL;
    }

    return module;
}
