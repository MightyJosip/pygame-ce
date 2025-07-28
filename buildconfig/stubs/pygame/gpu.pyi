from typing import Any, Iterable
from ctypes import Structure
from pygame.surface import Surface
from pygame.window import Window
from pygame.typing import ColorLike, RectLike

GPU_SHADERSTAGE_VERTEX: int
GPU_SHADERSTAGE_FRAGMENT: int
GPU_LOADOP_LOAD: int
GPU_LOADOP_CLEAR: int
GPU_LOADOP_DONT_CARE: int
GPU_STOREOP_STORE: int
GPU_STOREOP_DONT_CARE: int
GPU_STOREOP_RESOLVE: int
GPU_STOREOP_RESOLVE_AND_STORE: int
GPU_PRIMITIVETYPE_TRIANGLELIST: int
GPU_PRIMITIVETYPE_TRIANGLESTRIP: int
GPU_PRIMITIVETYPE_LINELIST: int
GPU_PRIMITIVETYPE_LINESTRIP: int
GPU_PRIMITIVETYPE_POINTLIST: int
GPU_FILLMODE_FILL: int
GPU_FILLMODE_LINE: int
GPU_BUFFERUSAGE_VERTEX: int
GPU_BUFFERUSAGE_INDEX: int
GPU_BUFFERUSAGE_INDIRECT: int
GPU_BUFFERUSAGE_GRAPHICS_STORAGE_READ: int
GPU_BUFFERUSAGE_COMPUTE_STORAGE_READ: int
GPU_BUFFERUSAGE_COMPUTE_STORAGE_WRITE: int
GPU_CULLMODE_NONE: int
GPU_CULLMODE_FRONT: int
GPU_CULLMODE_BACK: int
GPU_FRONTFACE_COUNTER_CLOCKWISE: int
GPU_FRONTFACE_CLOCKWISE: int
GPU_FILTER_NEAREST: int
GPU_FILTER_LINEAR: int
GPU_SAMPLERMIPMAPMODE_NEAREST: int
GPU_SAMPLERMIPMAPMODE_LINEAR: int
GPU_SAMPLERADDRESSMODE_REPEAT: int
GPU_SAMPLERADDRESSMODE_MIRRORED_REPEAT: int
GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE: int
GPU_TEXTURETYPE_2D: int
GPU_TEXTURETYPE_2D_ARRAY: int
GPU_TEXTURETYPE_3D: int
GPU_TEXTURETYPE_CUBE: int
GPU_TEXTURETYPE_CUBE_ARRAY: int
GPU_TEXTUREUSAGE_SAMPLER: int
GPU_TEXTUREUSAGE_COLOR_TARGET: int
GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET: int
GPU_TEXTUREUSAGE_GRAPHICS_STORAGE_READ: int
GPU_TEXTUREUSAGE_COMPUTE_STORAGE_READ: int
GPU_TEXTUREUSAGE_COMPUTE_STORAGE_WRITE: int
GPU_TEXTUREUSAGE_COMPUTE_STORAGE_SIMULTANEOUS_READ_WRITE: int
GPU_BLENDFACTOR_ZERO: int
GPU_BLENDFACTOR_ONE: int
GPU_BLENDFACTOR_SRC_COLOR: int
GPU_BLENDFACTOR_ONE_MINUS_SRC_COLOR: int
GPU_BLENDFACTOR_DST_COLOR: int
GPU_BLENDFACTOR_ONE_MINUS_DST_COLOR: int
GPU_BLENDFACTOR_SRC_ALPHA: int
GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA: int
GPU_BLENDFACTOR_DST_ALPHA: int
GPU_BLENDFACTOR_ONE_MINUS_DST_ALPHA: int
GPU_BLENDFACTOR_CONSTANT_COLOR: int
GPU_BLENDFACTOR_ONE_MINUS_CONSTANT_COLOR: int
GPU_BLENDFACTOR_SRC_ALPHA_SATURATE: int

POSITION_VERTEX: int
POSITION_COLOR_VERTEX: int
POSITION_TEXTURE_VERTEX: int
PUSH_VERTEX: int
PUSH_FRAGMENT: int
PUSH_UNIFORM: int

def init() -> None: ...
def claim_window(window: Window) -> None: ...
def push_data(structure: Structure, data_type: int) -> None: ...
def submit() -> None: ...
def quit() -> None: ...


class Shader:
    def __init__(self, file: str, stage: int, samplers: int = 0, uniform_buffers: int = 0, storage_buffers: int = 0, storage_textures: int = 0): ...


class RenderPass:
    def __init__(self, clear_color: ColorLike, load_op: int, store_op: int): ...
    def begin(self, window: Window) -> None: ...
    def end(self) -> None: ...
    def draw_primitives(self, vertices: int, instances: int, vertex_offset: int = 0, indexed: bool = False, index_offset: int = 0) -> None: ...
    def set_viewport(self, x: float, y: float, w: float, h: float, min_depth: float = 0, max_depth: float = 0) -> None: ...
    def set_scissor(self, rect: RectLike) -> None: ...


class Pipeline:
    def __init__(self, window: Window, vertex_shader: Shader, fragment_shader: Shader, primitive_type: int, fill_mode: int = GPU_FILLMODE_FILL, vertex_input_state: int = -1, cull_mode: int = GPU_CULLMODE_NONE, front_face: int = GPU_FRONTFACE_COUNTER_CLOCKWISE): ...
    def bind(self, render_pass: RenderPass) -> None: ...


class Buffer:
    def __init__(self, usage: int, size: int, buffer_type: int = -1): ...
    def upload(self, data: list[Any]) -> None: ...
    def bind(self, render_pass: RenderPass) -> None: ...


class Texture:
    def __init__(self, size: Iterable[int], texture_type: int, usage: int): ...
    def upload(self, surface: Surface) -> None: ...


class Sampler:
    def __init__(self, filter: int, mipmap_mode: int, address_mode: int, anisotropy: float = 0): ...
    def bind(self, render_pass: RenderPass, texture: Texture) -> None: ...