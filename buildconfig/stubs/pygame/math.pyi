import sys
from typing import (
    Any,
    Generic,
    Iterator,
    List,
    Literal,
    Sequence,
    Tuple,
    TypeVar,
    Union,
    final,
    overload,
)

if sys.version_info >= (3, 9):
    from collections.abc import Collection
else:
    from typing import Collection

_T = TypeVar('_T')
def clamp(value: _T, min: _T, max: _T, /) -> _T: ...


_VectorTypeVar = TypeVar("_VectorTypeVar", Vector2, Vector3)

# VectorElementwiseProxy is a generic, it can be an elementwiseproxy object for
# both Vector2 and Vector3 objects
@final
class VectorElementwiseProxy(Generic[_VectorTypeVar]):
    def __add__(
        self,
        other: Union[float, _VectorTypeVar, VectorElementwiseProxy[_VectorTypeVar]],
    ) -> _VectorTypeVar: ...
    def __radd__(
        self,
        other: Union[float, _VectorTypeVar, VectorElementwiseProxy[_VectorTypeVar]],
    ) -> _VectorTypeVar: ...
    def __sub__(
        self,
        other: Union[float, _VectorTypeVar, VectorElementwiseProxy[_VectorTypeVar]],
    ) -> _VectorTypeVar: ...
    def __rsub__(
        self,
        other: Union[float, _VectorTypeVar, VectorElementwiseProxy[_VectorTypeVar]],
    ) -> _VectorTypeVar: ...
    def __mul__(
        self,
        other: Union[float, _VectorTypeVar, VectorElementwiseProxy[_VectorTypeVar]],
    ) -> _VectorTypeVar: ...
    def __rmul__(
        self,
        other: Union[float, _VectorTypeVar, VectorElementwiseProxy[_VectorTypeVar]],
    ) -> _VectorTypeVar: ...
    def __truediv__(
        self,
        other: Union[float, _VectorTypeVar, VectorElementwiseProxy[_VectorTypeVar]],
    ) -> _VectorTypeVar: ...
    def __rtruediv__(
        self,
        other: Union[float, _VectorTypeVar, VectorElementwiseProxy[_VectorTypeVar]],
    ) -> _VectorTypeVar: ...
    def __floordiv__(
        self,
        other: Union[float, _VectorTypeVar, VectorElementwiseProxy[_VectorTypeVar]],
    ) -> _VectorTypeVar: ...
    def __rfloordiv__(
        self,
        other: Union[float, _VectorTypeVar, VectorElementwiseProxy[_VectorTypeVar]],
    ) -> _VectorTypeVar: ...
    def __mod__(
        self,
        other: Union[float, _VectorTypeVar, VectorElementwiseProxy[_VectorTypeVar]],
    ) -> _VectorTypeVar: ...
    def __rmod__(
        self,
        other: Union[float, _VectorTypeVar, VectorElementwiseProxy[_VectorTypeVar]],
    ) -> _VectorTypeVar: ...
    def __pow__(
        self,
        power: Union[float, _VectorTypeVar, VectorElementwiseProxy[_VectorTypeVar]],
        mod: None = None,
    ) -> _VectorTypeVar: ...
    def __rpow__(
        self,
        power: Union[float, _VectorTypeVar, VectorElementwiseProxy[_VectorTypeVar]],
        mod: None = None,
    ) -> _VectorTypeVar: ...
    def __eq__(self, other: Any) -> bool: ...
    def __ne__(self, other: Any) -> bool: ...
    def __gt__(
        self,
        other: Union[float, _VectorTypeVar, VectorElementwiseProxy[_VectorTypeVar]],
    ) -> bool: ...
    def __lt__(
        self,
        other: Union[float, _VectorTypeVar, VectorElementwiseProxy[_VectorTypeVar]],
    ) -> bool: ...
    def __ge__(
        self,
        other: Union[float, _VectorTypeVar, VectorElementwiseProxy[_VectorTypeVar]],
    ) -> bool: ...
    def __le__(
        self,
        other: Union[float, _VectorTypeVar, VectorElementwiseProxy[_VectorTypeVar]],
    ) -> bool: ...
    def __abs__(self) -> _VectorTypeVar: ...
    def __neg__(self) -> _VectorTypeVar: ...
    def __pos__(self) -> _VectorTypeVar: ...
    def __bool__(self) -> bool: ...

@final
class VectorIterator:
    def __length_hint__(self) -> int: ...
    def __iter__(self) -> Iterator[float]: ...
    def __next__(self) -> float: ...

_SupportsVector2 = Union[Sequence[float], Vector2]
_SupportsVector3 = Union[Sequence[float], Vector3]

class Vector2(Collection[float]):
    epsilon: float
    x: float
    y: float
    xx: Vector2
    xy: Vector2
    yx: Vector2
    yy: Vector2
    __hash__: None  # type: ignore
    @overload
    def __init__(self, x: Union[str, float, _SupportsVector2] = 0) -> None: ...
    @overload
    def __init__(self, x: float, y: float) -> None: ...
    def __len__(self) -> int: ...
    @overload
    def __setitem__(self, key: int, value: float) -> None: ...
    @overload
    def __setitem__(
        self, key: slice, value: Union[Sequence[float], Vector2, Vector3]
    ) -> None: ...
    @overload
    def __getitem__(self, i: int) -> float: ...
    @overload
    def __getitem__(self, s: slice) -> List[float]: ...
    def __iter__(self) -> VectorIterator: ...
    def __add__(self, other: _SupportsVector2) -> Vector2: ...
    def __radd__(self, other: _SupportsVector2) -> Vector2: ...
    def __sub__(self, other: _SupportsVector2) -> Vector2: ...
    def __rsub__(self, other: _SupportsVector2) -> Vector2: ...
    @overload
    def __mul__(self, other: _SupportsVector2) -> float: ...
    @overload
    def __mul__(self, other: float) -> Vector2: ...
    def __rmul__(self, other: float) -> Vector2: ...
    def __truediv__(self, other: float) -> Vector2: ...
    def __rtruediv__(self, other: float) -> Vector2: ...
    def __floordiv__(self, other: float) -> Vector2: ...
    def __neg__(self) -> Vector2: ...
    def __pos__(self) -> Vector2: ...
    def __bool__(self) -> bool: ...
    def __iadd__(self, other: _SupportsVector2) -> Vector2: ...
    def __isub__(self, other: _SupportsVector2) -> Vector2: ...
    @overload
    def __imul__(self, other: _SupportsVector2) -> float: ...
    @overload
    def __imul__(self, other: float) -> Vector2: ...
    def __copy__(self) -> Vector2: ...
    copy = __copy__
    def __safe_for_unpickling__(self) -> Literal[True]: ...
    def __contains__(self, other: float) -> bool: ...  # type: ignore[override]
    def dot(self, other: _SupportsVector2) -> float: ...
    def cross(self, other: _SupportsVector2) -> float: ...
    def magnitude(self) -> float: ...
    def magnitude_squared(self) -> float: ...
    def length(self) -> float: ...
    def length_squared(self) -> float: ...
    def normalize(self) -> Vector2: ...
    def normalize_ip(self) -> None: ...
    def is_normalized(self) -> bool: ...
    def scale_to_length(self, value: float) -> None: ...
    def reflect(self, other: _SupportsVector2) -> Vector2: ...
    def reflect_ip(self, other: _SupportsVector2) -> None: ...
    def distance_to(self, other: _SupportsVector2) -> float: ...
    def distance_squared_to(self, other: _SupportsVector2) -> float: ...
    def lerp(self, other: Vector2, value: float) -> Vector2: ...
    def slerp(self, other: Vector2, value: float) -> Vector2: ...
    def elementwise(self) -> VectorElementwiseProxy[Vector2]: ...
    def rotate(self, angle: float) -> Vector2: ...
    def rotate_rad(self, angle: float) -> Vector2: ...
    def rotate_ip(self, angle: float) -> None: ...
    def rotate_rad_ip(self, angle: float) -> None: ...
    def rotate_ip_rad(self, angle: float) -> None: ...
    def angle_to(self, other: _SupportsVector2) -> float: ...
    def as_polar(self) -> Tuple[float, float]: ...
    def from_polar(
        self, polar_value: Union[Sequence[float], Tuple[float, float]]
    ) -> None: ...
    def move_towards(
        self, target: _SupportsVector2, max_distance: float
    ) -> Vector2: ...
    def move_towards_ip(
        self, target: _SupportsVector2, max_distance: float
    ) -> None: ...
    @overload
    def clamp_magnitude(self, max_length: float) -> Vector2: ...
    @overload
    def clamp_magnitude(self, min_length: float, max_length: float) -> Vector2: ...
    @overload
    def clamp_magnitude_ip(self, min_length: float) -> None: ...
    @overload
    def clamp_magnitude_ip(self, min_length: float, max_length: float) -> None: ...
    def project(self, other: _SupportsVector2) -> Vector2: ...
    @overload
    def update(self, x: Union[str, float, _SupportsVector2] = 0) -> None: ...
    @overload
    def update(self, x: float = 0, y: float = 0) -> None: ...

class Vector3(Collection[float]):
    epsilon: float
    x: float
    y: float
    z: float
    xx: Vector2
    xy: Vector2
    xz: Vector2
    yx: Vector2
    yy: Vector2
    yz: Vector2
    zx: Vector2
    zy: Vector2
    zz: Vector2
    xxx: Vector3
    xxy: Vector3
    xxz: Vector3
    xyx: Vector3
    xyy: Vector3
    xyz: Vector3
    xzx: Vector3
    xzy: Vector3
    xzz: Vector3
    yxx: Vector3
    yxy: Vector3
    yxz: Vector3
    yyx: Vector3
    yyy: Vector3
    yyz: Vector3
    yzx: Vector3
    yzy: Vector3
    yzz: Vector3
    zxx: Vector3
    zxy: Vector3
    zxz: Vector3
    zyx: Vector3
    zyy: Vector3
    zyz: Vector3
    zzx: Vector3
    zzy: Vector3
    zzz: Vector3
    __hash__: None  # type: ignore
    @overload
    def __init__(
        self,
        xyz: Union[str, float, _SupportsVector3] = 0,
    ) -> None: ...
    @overload
    def __init__(self, x: float, y: float, z: float) -> None: ...
    def __len__(self) -> int: ...
    @overload
    def __setitem__(self, key: int, value: float) -> None: ...
    @overload
    def __setitem__(
        self, key: slice, value: Union[Sequence[float], Vector2, Vector3]
    ) -> None: ...
    @overload
    def __getitem__(self, i: int) -> float: ...
    @overload
    def __getitem__(self, s: slice) -> List[float]: ...
    def __iter__(self) -> VectorIterator: ...
    def __add__(self, other: _SupportsVector3) -> Vector3: ...
    def __radd__(self, other: _SupportsVector3) -> Vector3: ...
    def __sub__(self, other: _SupportsVector3) -> Vector3: ...
    def __rsub__(self, other: _SupportsVector3) -> Vector3: ...
    @overload
    def __mul__(self, other: _SupportsVector3) -> float: ...
    @overload
    def __mul__(self, other: float) -> Vector3: ...
    def __rmul__(self, other: float) -> Vector3: ...
    def __truediv__(self, other: float) -> Vector3: ...
    def __rtruediv__(self, other: float) -> Vector3: ...
    def __floordiv__(self, other: float) -> Vector3: ...
    def __neg__(self) -> Vector3: ...
    def __pos__(self) -> Vector3: ...
    def __bool__(self) -> bool: ...
    def __iadd__(self, other: _SupportsVector3) -> Vector3: ...
    def __isub__(self, other: _SupportsVector3) -> Vector3: ...
    @overload
    def __imul__(self, other: _SupportsVector3) -> float: ...
    @overload
    def __imul__(self, other: float) -> Vector3: ...
    def __copy__(self) -> Vector3: ...
    copy = __copy__
    def __safe_for_unpickling__(self) -> Literal[True]: ...
    def __contains__(self, other: float) -> bool: ...  # type: ignore[override]
    def dot(self, other: _SupportsVector3) -> float: ...
    def cross(self, other: _SupportsVector3) -> Vector3: ...
    def magnitude(self) -> float: ...
    def magnitude_squared(self) -> float: ...
    def length(self) -> float: ...
    def length_squared(self) -> float: ...
    def normalize(self) -> Vector3: ...
    def normalize_ip(self) -> None: ...
    def is_normalized(self) -> bool: ...
    def scale_to_length(self, value: float) -> None: ...
    def reflect(self, other: Vector3) -> Vector3: ...
    def reflect_ip(self, other: _SupportsVector3) -> None: ...
    def distance_to(self, other: _SupportsVector3) -> float: ...
    def distance_squared_to(self, other: _SupportsVector3) -> float: ...
    def lerp(self, other: _SupportsVector3, value: float) -> Vector3: ...
    def slerp(self, other: _SupportsVector3, value: float) -> Vector3: ...
    def elementwise(self) -> VectorElementwiseProxy[Vector3]: ...
    def rotate(self, angle: float, axis: _SupportsVector3) -> Vector3: ...
    def rotate_rad(self, angle: float, axis: _SupportsVector3) -> Vector3: ...
    def rotate_ip(self, angle: float, axis: _SupportsVector3) -> None: ...
    def rotate_rad_ip(self, angle: float, axis: _SupportsVector3) -> None: ...
    def rotate_ip_rad(self, angle: float, axis: _SupportsVector3) -> None: ...
    def rotate_x(self, angle: float) -> Vector3: ...
    def rotate_x_rad(self, angle: float) -> Vector3: ...
    def rotate_x_ip(self, angle: float) -> None: ...
    def rotate_x_rad_ip(self, angle: float) -> None: ...
    def rotate_x_ip_rad(self, angle: float) -> None: ...
    def rotate_y(self, angle: float) -> Vector3: ...
    def rotate_y_rad(self, angle: float) -> Vector3: ...
    def rotate_y_ip(self, angle: float) -> None: ...
    def rotate_y_rad_ip(self, angle: float) -> None: ...
    def rotate_y_ip_rad(self, angle: float) -> None: ...
    def rotate_z(self, angle: float) -> Vector3: ...
    def rotate_z_rad(self, angle: float) -> Vector3: ...
    def rotate_z_ip(self, angle: float) -> None: ...
    def rotate_z_rad_ip(self, angle: float) -> None: ...
    def rotate_z_ip_rad(self, angle: float) -> None: ...
    def angle_to(self, other: _SupportsVector3) -> float: ...
    def as_spherical(self) -> Tuple[float, float, float]: ...
    def from_spherical(self, spherical: Tuple[float, float, float]) -> None: ...
    def move_towards(
        self, target: _SupportsVector3, max_distance: float
    ) -> Vector3: ...
    def move_towards_ip(
        self, target: _SupportsVector3, max_distance: float
    ) -> None: ...
    def project(self, other: _SupportsVector3) -> Vector3: ...
    @overload
    def clamp_magnitude(self, max_length: float) -> Vector3: ...
    @overload
    def clamp_magnitude(self, min_length: float, max_length: float) -> Vector3: ...
    @overload
    def clamp_magnitude_ip(self, min_length: float) -> None: ...
    @overload
    def clamp_magnitude_ip(self, min_length: float, max_length: float) -> None: ...
    @overload
    def update(self, xyz: Union[str, float, _SupportsVector3] = 0) -> None: ...
    @overload
    def update(self, x: int, y: int, z: int) -> None: ...

# typehints for deprecated functions, to be removed in a future version
def enable_swizzling() -> None: ...
def disable_swizzling() -> None: ...
