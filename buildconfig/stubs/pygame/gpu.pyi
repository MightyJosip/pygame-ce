from pygame.window import Window
from pygame.typing import ColorLike

GPU_SHADERSTAGE_VERTEX: int
GPU_SHADERSTAGE_FRAGMENT: int
GPU_LOADOP_LOAD: int
GPU_LOADOP_CLEAR: int
GPU_LOADOP_DONT_CARE: int
GPU_STOREOP_STORE: int
GPU_STOREOP_DONT_CARE: int
GPU_STOREOP_RESOLVE: int
GPU_STOREOP_RESOLVE_AND_STORE: int

def init() -> None: ...
def claim_window(window: Window) -> None: ...
def submit(window: Window, render_pass: RenderPass) -> None: ...
def quit() -> None: ...


class Shader:
    def __init__(self, file: str, stage: int, samplers: int = 0, uniform_buffers: int = 0, storage_buffers: int = 0, storage_textures: int = 0): ...


class RenderPass:
    def __init__(self, clear_color: ColorLike, load_op: int, store_op: int): ...