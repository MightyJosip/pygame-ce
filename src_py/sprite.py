import types
from typing import Optional
from warnings import warn

import pygame
from pygame.mask import from_surface
from pygame.rect import Rect
from pygame.time import get_ticks


class Sprite:
    def __init__(self, *groups):
        self.__g = {}  # The groups the sprite is in
        self.__image: Optional[pygame.surface.Surface] = None
        self.__rect: Optional[pygame.rect.Rect] = None
        if groups:
            self.add(*groups)

    @property
    def image(self):
        return self.__image

    @image.setter
    def image(self, value: Optional[pygame.surface.Surface]):
        self.__image = value

    @property
    def rect(self):
        return self.__rect

    @rect.setter
    def rect(self, value: Optional[pygame.rect.Rect]):
        self.__rect = value

    def add(self, *groups):
        has = self.__g.__contains__
        for group in groups:
            if hasattr(group, "_spritegroup"):
                if not has(group):
                    group.add_internal(self)
                    self.add_internal(group)
            else:
                self.add(*group)

    def remove(self, *groups):
        has = self.__g.__contains__
        for group in groups:
            if hasattr(group, "_spritegroup"):
                if has(group):
                    group.remove_internal(self)
                    self.remove_internal(group)
            else:
                self.remove(*group)

    def add_internal(self, group):
        self.__g[group] = 0

    def remove_internal(self, group):
        del self.__g[group]

    def update(self, *args, **kwargs):
        pass

    def kill(self):
        for group in self.__g:
            group.remove_internal(self)
        self.__g.clear()

    def groups(self):
        return list(self.__g)

    def alive(self):
        return bool(self.__g)

    def __repr__(self):
        return f"<{self.__class__.__name__} Sprite(in {len(self.__g)} groups)>"

    @property
    def layer(self):
        return getattr(self, "_layer")

    @layer.setter
    def layer(self, value):
        if not self.alive():
            setattr(self, "_layer", value)
        else:
            raise AttributeError(
                "Can't set layer directly after "
                "adding to group. Use "
                "group.change_layer(sprite, new_layer) "
                "instead."
            )


class DirtySprite(Sprite):
    def __init__(self, *groups):
        self.dirty = 1

        # referred to as special_flags in the documentation of Surface.blit
        self.blendmode = 0
        self._visible = 1

        # Default 0 unless initialized differently.
        self._layer = getattr(self, "_layer", 0)
        self.source_rect = None
        Sprite.__init__(self, *groups)

    def _set_visible(self, val):
        self._visible = val
        if self.dirty < 2:
            self.dirty = 1

    def _get_visible(self):
        return self._visible

    @property
    def visible(self):
        return self._get_visible()

    @visible.setter
    def visible(self, value):
        self._set_visible(value)

    @property
    def layer(self):
        return self._layer

    @layer.setter
    def layer(self, value):
        if not self.alive():
            self._layer = value
        else:
            raise AttributeError(
                "Can't set layer directly after "
                "adding to group. Use "
                "group.change_layer(sprite, new_layer) "
                "instead."
            )

    def __repr__(self):
        return (
            f"<{self.__class__.__name__} DirtySprite(in {len(self.groups())} groups)>"
        )


class AbstractGroup:
    __class_getitem__ = classmethod(types.GenericAlias)

    _spritegroup = True

    def __init__(self):
        self.spritedict = {}
        self.lostsprites = []

    def sprites(self):
        return list(self.spritedict)

    def add_internal(
        self,
        sprite,
        layer=None,  # noqa pylint: disable=unused-argument; supporting legacy derived classes that override in non-pythonic way
    ):
        self.spritedict[sprite] = None

    def remove_internal(self, sprite):
        if lost_rect := self.spritedict[sprite]:
            self.lostsprites.append(lost_rect)
        del self.spritedict[sprite]

    def has_internal(self, sprite):
        return sprite in self.spritedict

    def copy(self):
        return self.__class__(  # noqa pylint: disable=too-many-function-args
            self.sprites()  # Needed because copy() won't work on AbstractGroup
        )

    def __iter__(self):
        return iter(self.sprites())

    def __contains__(self, sprite):
        return self.has(sprite)

    def add(self, *sprites):
        for sprite in sprites:
            if isinstance(sprite, Sprite):
                if not self.has_internal(sprite):
                    self.add_internal(sprite)
                    sprite.add_internal(self)
            else:
                try:
                    self.add(*sprite)
                except (TypeError, AttributeError):
                    if hasattr(sprite, "_spritegroup"):
                        for spr in sprite.sprites():
                            if not self.has_internal(spr):
                                self.add_internal(spr)
                                spr.add_internal(self)
                    elif not self.has_internal(sprite):
                        self.add_internal(sprite)
                        sprite.add_internal(self)

    def remove(self, *sprites):
        for sprite in sprites:
            if isinstance(sprite, Sprite):
                if self.has_internal(sprite):
                    self.remove_internal(sprite)
                    sprite.remove_internal(self)
            else:
                try:
                    self.remove(*sprite)
                except (TypeError, AttributeError):
                    if hasattr(sprite, "_spritegroup"):
                        for spr in sprite.sprites():
                            if self.has_internal(spr):
                                self.remove_internal(spr)
                                spr.remove_internal(self)
                    elif self.has_internal(sprite):
                        self.remove_internal(sprite)
                        sprite.remove_internal(self)

    def has(self, *sprites):
        if not sprites:
            return False

        for sprite in sprites:
            if isinstance(sprite, Sprite):
                if not self.has_internal(sprite):
                    return False
            else:
                try:
                    if not self.has(*sprite):
                        return False
                except (TypeError, AttributeError):
                    if hasattr(sprite, "_spritegroup"):
                        for spr in sprite.sprites():
                            if not self.has_internal(spr):
                                return False
                    else:
                        if not self.has_internal(sprite):
                            return False

        return True

    def update(self, *args, **kwargs):
        for sprite in self.sprites():
            sprite.update(*args, **kwargs)

    def draw(self, surface, bgd=None, special_flags=0):
        sprites = self.sprites()
        if hasattr(surface, "blits"):
            self.spritedict.update(
                zip(
                    sprites,
                    surface.blits(
                        (spr.image, spr.rect, None, special_flags) for spr in sprites
                    ),
                )
            )
        else:
            for spr in sprites:
                self.spritedict[spr] = surface.blit(
                    spr.image, spr.rect, None, special_flags
                )
        self.lostsprites = []
        dirty = self.lostsprites

        return dirty

    def clear(self, surface, bgd):
        if callable(bgd):
            for lost_clear_rect in self.lostsprites:
                bgd(surface, lost_clear_rect)
            for clear_rect in self.spritedict.values():
                if clear_rect:
                    bgd(surface, clear_rect)
        else:
            surface_blit = surface.blit
            for lost_clear_rect in self.lostsprites:
                surface_blit(bgd, lost_clear_rect, lost_clear_rect)
            for clear_rect in self.spritedict.values():
                if clear_rect:
                    surface_blit(bgd, clear_rect, clear_rect)

    def empty(self):
        for sprite in self.sprites():
            self.remove_internal(sprite)
            sprite.remove_internal(self)

    def __bool__(self):
        return bool(self.sprites())

    def __len__(self):
        return len(self.sprites())

    def __repr__(self):
        return f"<{self.__class__.__name__}({len(self)} sprites)>"


class Group(AbstractGroup):
    def __init__(self, *sprites):
        AbstractGroup.__init__(self)
        self.add(*sprites)


class RenderPlain(Group):
    def __init__(self, *sprites):
        super().__init__(*sprites)
        warn(
            "This class is deprecated and will be removed in a future version.",
            DeprecationWarning,
            stacklevel=2,
        )


class RenderClear(Group):
    def __init__(self, *sprites):
        super().__init__(*sprites)
        warn(
            "This class is deprecated and will be removed in a future version.",
            DeprecationWarning,
            stacklevel=2,
        )


class RenderUpdates(Group):
    def draw(self, surface, bgd=None, special_flags=0):
        surface_blit = surface.blit
        dirty = self.lostsprites
        self.lostsprites = []
        dirty_append = dirty.append
        for sprite in self.sprites():
            old_rect = self.spritedict[sprite]
            new_rect = surface_blit(sprite.image, sprite.rect, None, special_flags)
            if old_rect:
                if new_rect.colliderect(old_rect):
                    dirty_append(new_rect.union(old_rect))
                else:
                    dirty_append(new_rect)
                    dirty_append(old_rect)
            else:
                dirty_append(new_rect)
            self.spritedict[sprite] = new_rect
        return dirty


class OrderedUpdates(RenderUpdates):
    def __init__(self, *sprites):
        super().__init__(*sprites)
        warn(
            "OrderedUpdates is now just an alias to RenderUpdates, order of "
            "sprites is now maintained in all sprite Group classes. This "
            "class is deprecated and will be removed in a future version.",
            DeprecationWarning,
            stacklevel=2,
        )


class LayeredUpdates(AbstractGroup):
    _init_rect = Rect(0, 0, 0, 0)

    def __init__(self, *sprites, **kwargs):
        self._spritelayers = {}
        self._spritelist = []
        AbstractGroup.__init__(self)
        self._default_layer = kwargs.get("default_layer", 0)

        self.add(*sprites, **kwargs)

    def add_internal(self, sprite, layer=None):
        self.spritedict[sprite] = self._init_rect

        if layer is None:
            try:
                layer = sprite.layer
            except AttributeError:
                layer = self._default_layer
                setattr(sprite, "_layer", layer)
        elif hasattr(sprite, "_layer"):
            setattr(sprite, "_layer", layer)

        sprites = self._spritelist
        sprites_layers = self._spritelayers
        sprites_layers[sprite] = layer

        leng = len(sprites)
        low = mid = 0
        high = leng - 1
        while low <= high:
            mid = low + (high - low) // 2
            if sprites_layers[sprites[mid]] <= layer:
                low = mid + 1
            else:
                high = mid - 1
        while mid < leng and sprites_layers[sprites[mid]] <= layer:
            mid += 1
        sprites.insert(mid, sprite)

    def add(self, *sprites, **kwargs):
        if not sprites:
            return
        layer = kwargs["layer"] if "layer" in kwargs else None
        for sprite in sprites:
            if isinstance(sprite, Sprite):
                if not self.has_internal(sprite):
                    self.add_internal(sprite, layer)
                    sprite.add_internal(self)
            else:
                try:
                    self.add(*sprite, **kwargs)
                except (TypeError, AttributeError):
                    if hasattr(sprite, "_spritegroup"):
                        for spr in sprite.sprites():
                            if not self.has_internal(spr):
                                self.add_internal(spr, layer)
                                spr.add_internal(self)
                    elif not self.has_internal(sprite):
                        self.add_internal(sprite, layer)
                        sprite.add_internal(self)

    def remove_internal(self, sprite):
        self._spritelist.remove(sprite)
        old_rect = self.spritedict[sprite]
        if old_rect is not self._init_rect:
            self.lostsprites.append(old_rect)
        if hasattr(sprite, "rect"):
            self.lostsprites.append(sprite.rect)

        del self.spritedict[sprite]
        del self._spritelayers[sprite]

    def sprites(self):
        return self._spritelist.copy()

    def draw(self, surface, bgd=None, special_flags=0):
        spritedict = self.spritedict
        surface_blit = surface.blit
        dirty = self.lostsprites
        self.lostsprites = []
        dirty_append = dirty.append
        init_rect = self._init_rect
        for spr in self.sprites():
            rec = spritedict[spr]
            newrect = surface_blit(spr.image, spr.rect, None, special_flags)
            if rec is init_rect:
                dirty_append(newrect)
            else:
                if newrect.colliderect(rec):
                    dirty_append(newrect.union(rec))
                else:
                    dirty_append(newrect)
                    dirty_append(rec)
            spritedict[spr] = newrect
        return dirty

    def get_sprites_at(self, pos):
        _sprites = self._spritelist
        rect = Rect(pos, (1, 1))
        colliding_idx = rect.collidelistall(_sprites)
        return [_sprites[i] for i in colliding_idx]

    def get_sprite(self, idx):
        return self._spritelist[idx]

    def remove_sprites_of_layer(self, layer_nr):
        sprites = self.get_sprites_from_layer(layer_nr)
        self.remove(*sprites)
        return sprites

    # layer methods
    def layers(self):
        return sorted(set(self._spritelayers.values()))

    def change_layer(self, sprite, new_layer):
        sprites = self._spritelist
        sprites_layers = self._spritelayers

        sprites.remove(sprite)
        sprites_layers.pop(sprite)

        leng = len(sprites)
        low = mid = 0
        high = leng - 1
        while low <= high:
            mid = low + (high - low) // 2
            if sprites_layers[sprites[mid]] <= new_layer:
                low = mid + 1
            else:
                high = mid - 1
        while mid < leng and sprites_layers[sprites[mid]] <= new_layer:
            mid += 1
        sprites.insert(mid, sprite)
        if hasattr(sprite, "_layer"):
            setattr(sprite, "_layer", new_layer)

        sprites_layers[sprite] = new_layer

    def get_layer_of_sprite(self, sprite):
        return self._spritelayers.get(sprite, self._default_layer)

    def get_top_layer(self):
        return self._spritelayers[self._spritelist[-1]]

    def get_bottom_layer(self):
        return self._spritelayers[self._spritelist[0]]

    def move_to_front(self, sprite):
        self.change_layer(sprite, self.get_top_layer())

    def move_to_back(self, sprite):
        self.change_layer(sprite, self.get_bottom_layer() - 1)

    def get_top_sprite(self):
        return self._spritelist[-1]

    def get_sprites_from_layer(self, layer):
        sprites = []
        sprites_append = sprites.append
        sprite_layers = self._spritelayers
        for spr in self._spritelist:
            if sprite_layers[spr] == layer:
                sprites_append(spr)
            elif sprite_layers[spr] > layer:
                break
        return sprites

    def switch_layer(self, layer1_nr, layer2_nr):
        sprites1 = self.remove_sprites_of_layer(layer1_nr)
        for spr in self.get_sprites_from_layer(layer2_nr):
            self.change_layer(spr, layer1_nr)
        self.add(layer=layer2_nr, *sprites1)


class LayeredDirty(LayeredUpdates):
    def __init__(self, *sprites, **kwargs):
        LayeredUpdates.__init__(self, *sprites, **kwargs)
        self._clip = None

        self._use_update = False

        self._time_threshold = 1000.0 / 80.0  # 1000.0 / fps

        self._bgd = None
        for key, val in kwargs.items():
            if key in ["_use_update", "_time_threshold", "_default_layer"] and hasattr(
                self, key
            ):
                setattr(self, key, val)

    def add_internal(self, sprite, layer=None):
        if not hasattr(sprite, "dirty"):
            raise AttributeError()
        if not hasattr(sprite, "visible"):
            raise AttributeError()
        if not hasattr(sprite, "blendmode"):
            raise AttributeError()

        if not isinstance(sprite, DirtySprite):
            raise TypeError()

        if sprite.dirty == 0:
            sprite.dirty = 1

        LayeredUpdates.add_internal(self, sprite, layer)

    def draw(self, surface, bgd=None, special_flags=None):
        orig_clip = surface.get_clip()
        latest_clip = self._clip
        if latest_clip is None:
            latest_clip = orig_clip

        local_sprites = self._spritelist
        local_old_rect = self.spritedict
        local_update = self.lostsprites
        rect_type = Rect

        surf_blit_func = surface.blit
        if bgd is not None:
            self._bgd = bgd
        local_bgd = self._bgd

        surface.set_clip(latest_clip)
        start_time = get_ticks()
        if self._use_update:
            self._find_dirty_area(
                latest_clip,
                local_old_rect,
                rect_type,
                local_sprites,
                local_update,
                local_update.append,
                self._init_rect,
            )
            if local_bgd is not None:
                flags = 0 if special_flags is None else special_flags
                for rec in local_update:
                    surf_blit_func(local_bgd, rec, rec, flags)
            self._draw_dirty_internal(
                local_old_rect,
                rect_type,
                local_sprites,
                surf_blit_func,
                local_update,
                special_flags,
            )
            local_ret = list(local_update)
        else:
            if local_bgd is not None:
                flags = 0 if special_flags is None else special_flags
                surf_blit_func(local_bgd, (0, 0), None, flags)
            for spr in local_sprites:
                if spr.visible:
                    flags = spr.blendmode if special_flags is None else special_flags
                    local_old_rect[spr] = surf_blit_func(
                        spr.image, spr.rect, spr.source_rect, flags
                    )
            local_ret = [rect_type(latest_clip)]
        end_time = get_ticks()
        if end_time - start_time > self._time_threshold:
            self._use_update = False
        else:
            self._use_update = True
        local_update[:] = []
        surface.set_clip(orig_clip)
        return local_ret

    @staticmethod
    def _draw_dirty_internal(
        _old_rect, _rect, _sprites, _surf_blit, _update, _special_flags
    ):
        for spr in _sprites:
            flags = spr.blendmode if _special_flags is None else _special_flags
            if spr.dirty < 1 and spr.visible:
                if spr.source_rect is not None:
                    _spr_rect = _rect(spr.rect.topleft, spr.source_rect.size)
                    rect_offset_x = spr.source_rect[0] - _spr_rect[0]
                    rect_offset_y = spr.source_rect[1] - _spr_rect[1]
                else:
                    _spr_rect = spr.rect
                    rect_offset_x = -_spr_rect[0]
                    rect_offset_y = -_spr_rect[1]

                _spr_rect_clip = _spr_rect.clip

                for idx in _spr_rect.collidelistall(_update):
                    clip = _spr_rect_clip(_update[idx])
                    _surf_blit(
                        spr.image,
                        clip,
                        (
                            clip[0] + rect_offset_x,
                            clip[1] + rect_offset_y,
                            clip[2],
                            clip[3],
                        ),
                        flags,
                    )
            else:
                if spr.visible:
                    _old_rect[spr] = _surf_blit(
                        spr.image, spr.rect, spr.source_rect, flags
                    )
                if spr.dirty == 1:
                    spr.dirty = 0

    @staticmethod
    def _find_dirty_area(
        _clip, _old_rect, _rect, _sprites, _update, _update_append, init_rect
    ):
        for spr in _sprites:
            if spr.dirty > 0:
                if spr.source_rect:
                    _union_rect = _rect(spr.rect.topleft, spr.source_rect.size)
                else:
                    _union_rect = _rect(spr.rect)

                _union_rect_collidelist = _union_rect.collidelist
                _union_rect_union_ip = _union_rect.union_ip
                i = _union_rect_collidelist(_update)
                while i > -1:
                    _union_rect_union_ip(_update[i])
                    del _update[i]
                    i = _union_rect_collidelist(_update)
                _update_append(_union_rect.clip(_clip))

                if _old_rect[spr] is not init_rect:
                    _union_rect = _rect(_old_rect[spr])
                    _union_rect_collidelist = _union_rect.collidelist
                    _union_rect_union_ip = _union_rect.union_ip
                    i = _union_rect_collidelist(_update)
                    while i > -1:
                        _union_rect_union_ip(_update[i])
                        del _update[i]
                        i = _union_rect_collidelist(_update)
                    _update_append(_union_rect.clip(_clip))

    def clear(self, surface, bgd):
        self._bgd = bgd

    def repaint_rect(self, screen_rect):
        if self._clip:
            self.lostsprites.append(screen_rect.clip(self._clip))
        else:
            self.lostsprites.append(Rect(screen_rect))

    def set_clip(self, screen_rect=None):
        if screen_rect is None:
            self._clip = pygame.display.get_surface().get_rect()
        else:
            self._clip = screen_rect
        self._use_update = False

    def get_clip(self):
        return self._clip

    def change_layer(self, sprite, new_layer):
        LayeredUpdates.change_layer(self, sprite, new_layer)
        if sprite.dirty == 0:
            sprite.dirty = 1

    def set_timing_treshold(self, time_ms):
        warn(
            "This function will be removed, use set_timing_threshold function instead",
            DeprecationWarning,
        )
        self.set_timing_threshold(time_ms)

    def set_timing_threshold(self, time_ms):
        if isinstance(time_ms, (int, float)):
            self._time_threshold = time_ms
        else:
            raise TypeError(
                f"Expected numeric value, got {time_ms.__class__.__name__} instead"
            )


class GroupSingle(AbstractGroup):
    def __init__(self, sprite=None):
        AbstractGroup.__init__(self)
        self.__sprite = None
        if sprite is not None:
            self.add(sprite)

    def copy(self):
        return GroupSingle(self.__sprite)

    def sprites(self):
        if self.__sprite is not None:
            return [self.__sprite]
        return []

    def add_internal(self, sprite, layer=None):
        if self.__sprite is not None:
            self.__sprite.remove_internal(self)
            self.remove_internal(self.__sprite)
        self.__sprite = sprite

    def __bool__(self):
        return self.__sprite is not None

    def _get_sprite(self):
        return self.__sprite

    def _set_sprite(self, sprite):
        self.add_internal(sprite)
        sprite.add_internal(self)
        return sprite

    @property
    def sprite(self):
        return self._get_sprite()

    @sprite.setter
    def sprite(self, sprite_to_set):
        self._set_sprite(sprite_to_set)

    def remove_internal(self, sprite):
        if sprite is self.__sprite:
            self.__sprite = None
        if sprite in self.spritedict:
            AbstractGroup.remove_internal(self, sprite)

    def has_internal(self, sprite):
        return self.__sprite is sprite

    def __contains__(self, sprite):
        return self.__sprite is sprite


def collide_rect(left, right):
    return left.rect.colliderect(right.rect)


class collide_rect_ratio:
    def __init__(self, ratio):
        self.ratio = ratio

    def __repr__(self):
        return "<{klass} @{id:x} {attrs}>".format(
            klass=self.__class__.__name__,
            id=id(self) & 0xFFFFFF,
            attrs=" ".join(f"{k}={v!r}" for k, v in self.__dict__.items()),
        )

    def __call__(self, left, right):
        ratio = self.ratio

        leftrect = left.rect
        width = leftrect.width
        height = leftrect.height
        leftrect = leftrect.inflate(width * ratio - width, height * ratio - height)

        rightrect = right.rect
        width = rightrect.width
        height = rightrect.height
        rightrect = rightrect.inflate(width * ratio - width, height * ratio - height)

        return leftrect.colliderect(rightrect)


def collide_circle(left, right):
    xdistance = left.rect.centerx - right.rect.centerx
    ydistance = left.rect.centery - right.rect.centery
    distancesquared = xdistance**2 + ydistance**2

    try:
        leftradius = left.radius
    except AttributeError:
        leftrect = left.rect
        leftradius = 0.5 * ((leftrect.width**2 + leftrect.height**2) ** 0.5)
        left.radius = leftradius

    try:
        rightradius = right.radius
    except AttributeError:
        rightrect = right.rect
        rightradius = 0.5 * ((rightrect.width**2 + rightrect.height**2) ** 0.5)
        right.radius = rightradius
    return distancesquared <= (leftradius + rightradius) ** 2


class collide_circle_ratio:
    def __init__(self, ratio):
        self.ratio = ratio

    def __repr__(self):
        return "<{klass} @{id:x} {attrs}>".format(
            klass=self.__class__.__name__,
            id=id(self) & 0xFFFFFF,
            attrs=" ".join(f"{k}={v!r}" for k, v in self.__dict__.items()),
        )

    def __call__(self, left, right):
        ratio = self.ratio
        xdistance = left.rect.centerx - right.rect.centerx
        ydistance = left.rect.centery - right.rect.centery
        distancesquared = xdistance**2 + ydistance**2

        try:
            leftradius = left.radius
        except AttributeError:
            leftrect = left.rect
            leftradius = 0.5 * ((leftrect.width**2 + leftrect.height**2) ** 0.5)
            left.radius = leftradius
        leftradius *= ratio

        try:
            rightradius = right.radius
        except AttributeError:
            rightrect = right.rect
            rightradius = 0.5 * ((rightrect.width**2 + rightrect.height**2) ** 0.5)
            right.radius = rightradius
        rightradius *= ratio

        return distancesquared <= (leftradius + rightradius) ** 2


def collide_mask(left, right):
    xoffset = right.rect[0] - left.rect[0]
    yoffset = right.rect[1] - left.rect[1]
    try:
        leftmask = left.mask
    except AttributeError:
        leftmask = from_surface(left.image)
    try:
        rightmask = right.mask
    except AttributeError:
        rightmask = from_surface(right.image)
    return leftmask.overlap(rightmask, (xoffset, yoffset))


def spritecollide(sprite, group, dokill, collided=None):
    default_sprite_collide_func = sprite.rect.colliderect

    if dokill:
        crashed = []
        append = crashed.append

        for group_sprite in group.sprites():
            if collided is not None:
                if collided(sprite, group_sprite):
                    group_sprite.kill()
                    append(group_sprite)
            else:
                if default_sprite_collide_func(group_sprite.rect):
                    group_sprite.kill()
                    append(group_sprite)

        return crashed

    if collided is not None:
        return [
            group_sprite for group_sprite in group if collided(sprite, group_sprite)
        ]

    return [
        group_sprite
        for group_sprite in group
        if default_sprite_collide_func(group_sprite.rect)
    ]


def groupcollide(groupa, groupb, dokilla, dokillb, collided=None):
    crashed = {}
    sprite_collide_func = spritecollide
    if dokilla:
        for group_a_sprite in groupa.sprites():
            collision = sprite_collide_func(group_a_sprite, groupb, dokillb, collided)
            if collision:
                crashed[group_a_sprite] = collision
                group_a_sprite.kill()
    else:
        for group_a_sprite in groupa:
            collision = sprite_collide_func(group_a_sprite, groupb, dokillb, collided)
            if collision:
                crashed[group_a_sprite] = collision
    return crashed


def spritecollideany(sprite, group, collided=None):
    default_sprite_collide_func = sprite.rect.colliderect

    if collided is not None:
        for group_sprite in group:
            if collided(sprite, group_sprite):
                return group_sprite
    else:
        for group_sprite in group:
            if default_sprite_collide_func(group_sprite.rect):
                return group_sprite
    return None
