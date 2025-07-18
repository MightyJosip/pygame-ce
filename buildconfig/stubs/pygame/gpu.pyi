from typing import Any
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

POSITION_VERTEX: int
POSITION_COLOR_VERTEX: int
POSITION_TEXTURE_VERTEX: int

def init() -> None: ...
def claim_window(window: Window) -> None: ...
def submit() -> None: ...
def quit() -> None: ...


class Shader:
    def __init__(self, file: str, stage: int, samplers: int = 0, uniform_buffers: int = 0, storage_buffers: int = 0, storage_textures: int = 0): ...


class RenderPass:
    def __init__(self, clear_color: ColorLike, load_op: int, store_op: int): ...
    def begin(self, window: Window) -> None: ...
    def end(self) -> None: ...
    def draw_primitives(self, vertices: int, instances: int) -> None: ...
    def set_viewport(self, x: float, y: float, w: float, h: float, min_depth: float = 0, max_depth: float = 0) -> None: ...
    def set_scissor(self, rect: RectLike) -> None: ...


class Pipeline:
    def __init__(self, window: Window, vertex_shader: Shader, fragment_shader: Shader, primitive_type: int, fill_mode: int = GPU_FILLMODE_FILL, vertex_input_state: int = -1, cull_mode: int = GPU_CULLMODE_NONE, front_face: int = GPU_FRONTFACE_COUNTER_CLOCKWISE): ...
    def bind(self, render_pass: RenderPass) -> None: ...


class Buffer:
    def __init__(self, usage: int, buffer_type: int, size: int): ...
    def upload(self, data: list[Any]) -> None: ...
    def bind(self, render_pass: RenderPass) -> None: ...