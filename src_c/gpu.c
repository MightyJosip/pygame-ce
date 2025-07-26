#define PYGAMEAPI_GPU_INTERNAL

#include "pygame.h"

#include "pgcompat.h"

/* Context */
static SDL_GPUDevice *device;

static SDL_GPUCommandBuffer *cmdbuf = NULL;

static int active_render_passes = 0;

/* Types */
static PyTypeObject pgShader_Type;

static PyTypeObject pgRenderPass_Type;

static PyTypeObject pgPipeline_Type;

static PyTypeObject pgBuffer_Type;

#define pgShader_Check(x) \
    (PyObject_IsInstance((x), (PyObject *)&pgShader_Type))

#define pgRenderPass_Check(x) \
    (PyObject_IsInstance((x), (PyObject *)&pgRenderPass_Type))

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

static void
acquire_command_buffer()
{
    if (cmdbuf == NULL) {
        cmdbuf = SDL_AcquireGPUCommandBuffer(device);
        if (cmdbuf == NULL) {
            RAISE(pgExc_SDLError, SDL_GetError());
        }
    }
}

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
static PyObject *
render_pass_begin(pgRenderPassObject *self, PyObject *args, PyObject *kwargs)
{
    pgWindowObject *window;
    SDL_GPUTexture* swapchainTexture;
    char *keywords[] = {"window", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O!", keywords,
                                     &pgWindow_Type, &window)) {
        return NULL;
    }
    if (active_render_passes) {
        return RAISE(pgExc_SDLError, "You must end old render pass before starting new one");
    }
    acquire_command_buffer();
    if (!SDL_WaitAndAcquireGPUSwapchainTexture(cmdbuf, window->_win, &swapchainTexture, NULL, NULL)) {
        return RAISE(pgExc_SDLError, SDL_GetError());
    }
    if (swapchainTexture != NULL) {
        self->color_info.texture = swapchainTexture;
        if (self->render_pass == NULL) {
            self->render_pass = SDL_BeginGPURenderPass(cmdbuf, &self->color_info, 1, NULL);
            active_render_passes++;
        }
    }
    Py_RETURN_NONE;
}

static PyObject *
render_pass_end(pgRenderPassObject *self, PyObject *_null)
{
    if (self->render_pass != NULL) {
        SDL_EndGPURenderPass(self->render_pass);
        active_render_passes--;
        self->render_pass = NULL;
    }
    Py_RETURN_NONE;
}

static PyObject *
render_pass_draw_primitives(pgRenderPassObject *self, PyObject *args, PyObject *kwargs)
{
    int vertices, instances, vertex_offset = 0, indexed = 0, index_offset;
    char *keywords[] = {"vertices", "instances", "vertex_offset", "indexed", "index_offset", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "ii|ipi", keywords,
                                     &vertices, &instances, &vertex_offset, &indexed, &index_offset)) {
        return NULL;
    }
    if (indexed) {
        SDL_DrawGPUIndexedPrimitives(self->render_pass, vertices, instances, index_offset, vertex_offset, 0);
    }
    else {
        SDL_DrawGPUPrimitives(self->render_pass, vertices, instances, vertex_offset, 0);
    }
    Py_RETURN_NONE;
}

static PyObject *
render_pass_set_viewport(pgRenderPassObject *self, PyObject *args, PyObject *kwargs)
{
    float x, y, w, h, min_depth = 0, max_depth = 0;
    char *keywords[] = {"x", "y", "w", "h", "min_depth", "max_depth", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "ffff|ff", keywords,
                                     &x, &y, &w, &h, &min_depth, &max_depth)) {
        return NULL;
    }
    SDL_SetGPUViewport(self->render_pass, (SDL_GPUViewport[]){x, y, w, h, min_depth, max_depth});
    Py_RETURN_NONE;
}

static PyObject *
render_pass_set_scissor(pgRenderPassObject *self, PyObject *args, PyObject *kwargs)
{
    PyObject *rectobj;
    SDL_Rect *rect = NULL, temp;
    char *keywords[] = {"rect", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O", keywords,
                                     &rectobj)) {
        return NULL;
    }
    rect = pgRect_FromObject(rectobj, &temp);
    if (!rect) {
        return RAISE(PyExc_TypeError, "rect argument is invalid");
    }
    SDL_SetGPUScissor(self->render_pass, rect);
    Py_RETURN_NONE;
}

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
    Py_TYPE(self)->tp_free(self);
}

/* Pipeline implementation */
static void
pipeline_fill_vertex_input_state(pgPipelineObject *self, BufferType vertex_input_state)
{
    SDL_GPUVertexBufferDescription* buffer_description;
    SDL_GPUVertexAttribute* vertex_attribute;
    if (vertex_input_state == POSITION_COLOR_VERTEX) {
        buffer_description = (SDL_GPUVertexBufferDescription*)malloc(sizeof(SDL_GPUVertexBufferDescription));
        buffer_description->slot = 0;
        buffer_description->input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;
        buffer_description->instance_step_rate = 0;
        buffer_description->pitch = sizeof(PositionColorVertex);

        vertex_attribute = (SDL_GPUVertexAttribute*)malloc(2 * sizeof(SDL_GPUVertexAttribute));
        vertex_attribute[0].buffer_slot = 0;
        vertex_attribute[0].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3;
        vertex_attribute[0].location = 0;
        vertex_attribute[0].offset = 0;
        vertex_attribute[1].buffer_slot = 0;
        vertex_attribute[1].format = SDL_GPU_VERTEXELEMENTFORMAT_UBYTE4_NORM;
        vertex_attribute[1].location = 1;
        vertex_attribute[1].offset = sizeof(float) * 3;

        self->pipeline_info.vertex_input_state = (SDL_GPUVertexInputState){
            .num_vertex_buffers = 1,
            .vertex_buffer_descriptions = buffer_description,
            .num_vertex_attributes = 2,
            .vertex_attributes = vertex_attribute
        };
    }
    free(buffer_description);
    free(vertex_attribute);
}

static PyObject *
pipeline_bind(pgPipelineObject *self, PyObject *args, PyObject *kwargs)
{
    pgRenderPassObject *render_pass;
    char *keywords[] = {"render_pass", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O!", keywords,
                                     &pgRenderPass_Type, &render_pass)) {
        return NULL;
    }
    SDL_BindGPUGraphicsPipeline(render_pass->render_pass, self->pipeline);
    Py_RETURN_NONE;
}

static int
pipeline_init(pgPipelineObject *self, PyObject *args, PyObject *kwargs)
{
    pgWindowObject *window;
    pgShaderObject *vertex_shader, *fragment_shader;
    SDL_GPUPrimitiveType primitive_type;
    SDL_GPUFillMode fill_mode = SDL_GPU_FILLMODE_FILL;
    BufferType vertex_input_state = -1;
    SDL_GPUCullMode cull_mode = SDL_GPU_CULLMODE_NONE;
    SDL_GPUFrontFace front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE;
    char *keywords[] = {"window", "vertex_shader", "fragment_shader", "primitive_type", "fill_mode", "vertex_input_state", "cull_mode", "front_face", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O!O!O!i|iiii", keywords,
                                     &pgWindow_Type, &window, &pgShader_Type, &vertex_shader, &pgShader_Type, &fragment_shader, &primitive_type, &fill_mode, &vertex_input_state, &cull_mode, &front_face)) {
        return -1;
    }
    self->pipeline_info.target_info = (SDL_GPUGraphicsPipelineTargetInfo){
        .num_color_targets = 1,
        .color_target_descriptions = (SDL_GPUColorTargetDescription[]){{
            .format = SDL_GetGPUSwapchainTextureFormat(device, window->_win)
        }},
    };
    self->pipeline_info.vertex_shader = vertex_shader->shader;
    self->pipeline_info.fragment_shader = fragment_shader->shader;
    self->pipeline_info.primitive_type = primitive_type;
    self->pipeline_info.rasterizer_state.fill_mode = fill_mode;
    self->pipeline_info.rasterizer_state.cull_mode = cull_mode;
    self->pipeline_info.rasterizer_state.front_face = front_face;
    if (vertex_input_state > -1) {
        pipeline_fill_vertex_input_state(self, vertex_input_state);
    }
    self->pipeline = SDL_CreateGPUGraphicsPipeline(device, &self->pipeline_info);
    if (self->pipeline == NULL) {
        RAISERETURN(pgExc_SDLError, SDL_GetError(), -1);
    }
    return 0;
}

static void
pipeline_dealloc(pgPipelineObject *self, PyObject *_null)
{
    if (device != NULL && self->pipeline) {
        SDL_ReleaseGPUGraphicsPipeline(device, self->pipeline);
        self->pipeline = NULL;
    }
    Py_TYPE(self)->tp_free(self);
}

/* Buffer implementation */
static int
buffer_get_element_size(SDL_GPUBufferUsageFlags usage, BufferType buffer_type) {
    if (usage == SDL_GPU_BUFFERUSAGE_VERTEX) {
        switch (buffer_type) {
            case POSITION_VERTEX:
                return sizeof(float) * 3;
            case POSITION_COLOR_VERTEX:
                return sizeof(float) * 3 + 4 * sizeof(Uint8);
            case POSITION_TEXTURE_VERTEX:
                return sizeof(float) * 5;
            default:
                return 0;
        }
    }
    else if (usage == SDL_GPU_BUFFERUSAGE_INDEX) {
        return sizeof(Uint16);
    }
    return 0;
}

static inline SDL_GPUTransferBuffer*
buffer_upload_position_color_vertex(pgBufferObject *self, PyObject* data, int size)
{
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
    return transfer_buffer;
}

static inline SDL_GPUTransferBuffer*
buffer_upload_index(pgBufferObject *self, PyObject* data, int size)
{
    SDL_GPUTransferBuffer* transfer_buffer = SDL_CreateGPUTransferBuffer(device, &(SDL_GPUTransferBufferCreateInfo) {
        .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
        .size = size
    });
    Uint16* transfer_data = SDL_MapGPUTransferBuffer(device, transfer_buffer, false);
    for (int i = 0; i < self->no_of_elements; i++) {
        transfer_data[i] = (Uint16)PyLong_AsInt(PySequence_GetItem(data, i));
    }
    SDL_UnmapGPUTransferBuffer(device, transfer_buffer);
    return transfer_buffer;
}

static PyObject *
buffer_upload(pgBufferObject *self, PyObject *args, PyObject *kwargs)
{
    PyObject* data;
    int size = buffer_get_element_size(self->usage, self->buffer_type) * self->no_of_elements;
    char *keywords[] = {"data", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O", keywords, &data)) {
        return NULL;
    }
    SDL_GPUTransferBuffer* transfer_buffer;
    if (self->usage == SDL_GPU_BUFFERUSAGE_VERTEX) {
        switch (self->buffer_type) {
            case POSITION_COLOR_VERTEX:
                transfer_buffer = buffer_upload_position_color_vertex(self, data, size);
                break;
            default:
                transfer_buffer = buffer_upload_position_color_vertex(self, data, size);
        }
    }
    else if (self->usage == SDL_GPU_BUFFERUSAGE_INDEX) {
        transfer_buffer = buffer_upload_index(self, data, size);
    }
    SDL_GPUCommandBuffer* upload_cmd_buf = SDL_AcquireGPUCommandBuffer(device);
    SDL_GPUCopyPass* copy_pass = SDL_BeginGPUCopyPass(upload_cmd_buf);
    SDL_UploadToGPUBuffer(
		copy_pass,
		&(SDL_GPUTransferBufferLocation) {
			.transfer_buffer = transfer_buffer,
			.offset = 0
		},
		&(SDL_GPUBufferRegion) {
			.buffer = self->buffer,
			.offset = 0,
			.size = size
		},
		false
	);
    SDL_ReleaseGPUTransferBuffer(device, transfer_buffer);
    SDL_EndGPUCopyPass(copy_pass);
	SDL_SubmitGPUCommandBuffer(upload_cmd_buf);
    Py_RETURN_NONE;
}

static PyObject *
buffer_bind(pgBufferObject *self, PyObject *args, PyObject *kwargs)
{
    pgRenderPassObject *render_pass;
    char *keywords[] = {"render_pass", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O!", keywords,
                                     &pgRenderPass_Type, &render_pass)) {
        return NULL;
    }
    if (self->usage == SDL_GPU_BUFFERUSAGE_VERTEX) {
        SDL_BindGPUVertexBuffers(render_pass->render_pass, 0,
            &(SDL_GPUBufferBinding){
                .buffer = self->buffer,
                .offset = 0
            },
            1
        );
    }
    else if (self->usage == SDL_GPU_BUFFERUSAGE_INDEX) {
        SDL_BindGPUIndexBuffer(render_pass->render_pass, 
            &(SDL_GPUBufferBinding){
                .buffer = self->buffer,
                .offset = 0
            },
            SDL_GPU_INDEXELEMENTSIZE_16BIT);
    }
    Py_RETURN_NONE;
}

static int
buffer_init(pgBufferObject *self, PyObject *args, PyObject *kwargs)
{
    SDL_GPUBuffer *buffer = NULL;
    SDL_GPUBufferUsageFlags usage;
    BufferType buffer_type = -1;
    Uint32 size;
    int element_size;
    char *keywords[] = {"usage", "size", "buffer_type", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "ii|i", keywords,
                                     &usage, &size, &buffer_type)) {
        return -1;
    }
    element_size = buffer_get_element_size(usage, buffer_type);
    if (!element_size) {
        RAISERETURN(pgExc_SDLError, "Unknown buffer type!", -1)
    }
    buffer = SDL_CreateGPUBuffer(device, &(SDL_GPUBufferCreateInfo) {
        .usage = usage,
        .size = element_size * size
    });
    if (buffer == NULL) {
        RAISERETURN(pgExc_SDLError, SDL_GetError(), -1);
    }
    self->buffer = buffer;
    self->buffer_type = buffer_type;
    self->no_of_elements = size;
    self->usage = usage;
    return 0;
}

static void
buffer_dealloc(pgBufferObject *self, PyObject *_null) {
    if (device != NULL && self->buffer) {
        SDL_ReleaseGPUBuffer(device, self->buffer);
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
submit(PyObject *self, PyObject *_null)
{
    if (cmdbuf != NULL) {
        SDL_SubmitGPUCommandBuffer(cmdbuf);
        cmdbuf = NULL;
        Py_RETURN_NONE;
    }
    else {
        RAISERETURN(pgExc_SDLError, "command buffer is not acquired", NULL);
    }
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

static PyMethodDef render_pass_methods[] = {
    {"begin", (PyCFunction)render_pass_begin,
     METH_VARARGS | METH_KEYWORDS, NULL},
    {"draw_primitives", (PyCFunction)render_pass_draw_primitives,
     METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_viewport", (PyCFunction)render_pass_set_viewport,
     METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_scissor", (PyCFunction)render_pass_set_scissor,
     METH_VARARGS | METH_KEYWORDS, NULL},
    {"end", (PyCFunction)render_pass_end,
    METH_NOARGS, NULL},
    {NULL, NULL, 0, NULL}
};

static PyGetSetDef render_pass_getset[] = {{NULL, 0, NULL, NULL, NULL}};

static PyMethodDef pipeline_methods[] = {
    {"bind", (PyCFunction)pipeline_bind,
     METH_VARARGS | METH_KEYWORDS, NULL},
    {NULL, NULL, 0, NULL}
};

static PyGetSetDef pipeline_getset[] = {{NULL, 0, NULL, NULL, NULL}};

static PyMethodDef buffer_methods[] = {
    {"upload", (PyCFunction)buffer_upload,
     METH_VARARGS | METH_KEYWORDS, NULL},
    {"bind", (PyCFunction)buffer_bind,
     METH_VARARGS | METH_KEYWORDS, NULL},
    {NULL, NULL, 0, NULL}
};

static PyGetSetDef buffer_getset[] = {{NULL, 0, NULL, NULL, NULL}};

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

static PyTypeObject pgPipeline_Type = {
    PyVarObject_HEAD_INIT(NULL, 0).tp_name = "pygame.gpu.Pipeline",
    .tp_basicsize = sizeof(pgPipelineObject),
    .tp_dealloc = (destructor)pipeline_dealloc,
    //.tp_doc = DOC_GPU_PIPELINE,
    .tp_methods = pipeline_methods,
    .tp_init = (initproc)pipeline_init,
    .tp_new = PyType_GenericNew,
    .tp_getset = pipeline_getset
};

static PyTypeObject pgBuffer_Type = {
    PyVarObject_HEAD_INIT(NULL, 0).tp_name = "pygame.gpu.Buffer",
    .tp_basicsize = sizeof(pgBufferObject),
    .tp_dealloc = (destructor)buffer_dealloc,
    //.tp_doc = DOC_GPU_BUFFER,
    .tp_methods = buffer_methods,
    .tp_init = (initproc)buffer_init,
    .tp_new = PyType_GenericNew,
    .tp_getset = buffer_getset
};

static PyMethodDef gpu_methods[] = {
    {"init", (PyCFunction)init, METH_NOARGS, NULL},
    {"claim_window", (PyCFunction)claim_window, METH_VARARGS | METH_KEYWORDS, NULL},
    {"submit", (PyCFunction)submit, METH_NOARGS, NULL},
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
    import_pygame_rect();
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

    if (PyModule_AddType(module, &pgPipeline_Type)) {
        Py_XDECREF(module);
        return NULL;
    }

    if (PyModule_AddType(module, &pgBuffer_Type)) {
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
    DEC_CONST(GPU_PRIMITIVETYPE_TRIANGLELIST);
    DEC_CONST(GPU_PRIMITIVETYPE_TRIANGLESTRIP);
    DEC_CONST(GPU_PRIMITIVETYPE_LINELIST);
    DEC_CONST(GPU_PRIMITIVETYPE_LINESTRIP);
    DEC_CONST(GPU_PRIMITIVETYPE_POINTLIST);
    DEC_CONST(GPU_FILLMODE_FILL);
    DEC_CONST(GPU_FILLMODE_LINE);
    DEC_CONST(GPU_BUFFERUSAGE_VERTEX);
    DEC_CONST(GPU_BUFFERUSAGE_INDEX);
    DEC_CONST(GPU_BUFFERUSAGE_INDIRECT);
    DEC_CONST(GPU_BUFFERUSAGE_GRAPHICS_STORAGE_READ);
    DEC_CONST(GPU_BUFFERUSAGE_COMPUTE_STORAGE_READ);
    DEC_CONST(GPU_BUFFERUSAGE_COMPUTE_STORAGE_WRITE);
    DEC_CONST(GPU_CULLMODE_NONE);
    DEC_CONST(GPU_CULLMODE_FRONT);
    DEC_CONST(GPU_CULLMODE_BACK);
    DEC_CONST(GPU_FRONTFACE_COUNTER_CLOCKWISE);
    DEC_CONST(GPU_FRONTFACE_CLOCKWISE);
    DEC_CONSTS_("POSITION_VERTEX", POSITION_VERTEX);
    DEC_CONSTS_("POSITION_COLOR_VERTEX", POSITION_COLOR_VERTEX);
    DEC_CONSTS_("POSITION_TEXTURE_VERTEX", POSITION_TEXTURE_VERTEX);

    apiobj = encapsulate_api(c_api, "gpu");
    if (PyModule_AddObject(module, PYGAMEAPI_LOCAL_ENTRY, apiobj)) {
        Py_XDECREF(apiobj);
        Py_DECREF(module);
        return NULL;
    }

    return module;
}