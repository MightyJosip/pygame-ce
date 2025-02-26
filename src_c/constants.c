/*
    pygame-ce - Python Game Library
    Copyright (C) 2000-2001  Pete Shinners

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public
    License along with this library; if not, write to the Free
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

    Pete Shinners
    pete@shinners.org
*/
#define NO_PYGAME_C_API
#include "pygame.h"

#include "pgcompat.h"

#include "scrap.h"

/* macros used to create each constant */
#define ADD_ERROR(x)                                      \
    {                                                     \
        Py_DECREF(module);                                \
        return NULL;                                      \
    }                                                     \
    else                                                  \
    {                                                     \
        PyList_Append(all_list, PyUnicode_FromString(x)); \
    }
#define STRINGIZE(x) #x
#define DEC_CONSTS_(x, y)                           \
    if (PyModule_AddIntConstant(module, x, (int)y)) \
    ADD_ERROR(x)
#define DEC_CONSTS(x, y) DEC_CONSTS_(#x, y)
#define DEC_CONST(x) DEC_CONSTS_(#x, SDL_##x)
#define DEC_CONSTKS(x, y) DEC_CONSTS_(STRINGIZE(K_##x), SDLK_##y)
#define DEC_CONSTK(x) DEC_CONSTS_(STRINGIZE(K_##x), SDLK_##x)
#define DEC_CONSTSCS(x, y) DEC_CONSTS_(STRINGIZE(KSCAN_##x), SDL_SCANCODE_##y)
#define DEC_CONSTSC(x) DEC_CONSTS_(STRINGIZE(KSCAN_##x), SDL_SCANCODE_##x)
#define DEC_CONSTKS_AND_SCS(x, y)           \
    DEC_CONSTS_(STRINGIZE(K_##x), SDLK_##y) \
    DEC_CONSTS_(STRINGIZE(KSCAN_##x), SDL_SCANCODE_##y)
#define DEC_CONSTK_AND_SC(x)                \
    DEC_CONSTS_(STRINGIZE(K_##x), SDLK_##x) \
    DEC_CONSTS_(STRINGIZE(KSCAN_##x), SDL_SCANCODE_##x)
#define DEC_CONSTN(x) DEC_CONSTS_(#x, x)
#define DEC_CONSTSF(x) DEC_CONSTS_(#x, PGS_##x)

#define ADD_STRING_CONST(x)                        \
    if (PyModule_AddStringConstant(module, #x, x)) \
    ADD_ERROR(#x)
#define ADD_NAMED_STRING_CONST(x, y)               \
    if (PyModule_AddStringConstant(module, #x, y)) \
    ADD_ERROR(#x)

static PyMethodDef _constant_methods[] = {{NULL}};

/*DOC*/ static char _constants_doc[] =
    /*DOC*/ "Constants defined by SDL and needed in Pygame.\n";

MODINIT_DEFINE(constants)
{
    PyObject *module, *all_list;

    static struct PyModuleDef _module = {PyModuleDef_HEAD_INIT,
                                         "constants",
                                         _constants_doc,
                                         -1,
                                         _constant_methods,
                                         NULL,
                                         NULL,
                                         NULL,
                                         NULL};

    module = PyModule_Create(&_module);
    if (module == NULL) {
        return NULL;
    }

    // Attempt to create __all__ variable for constants module
    all_list = PyList_New(0);
    if (!all_list) {
        Py_DECREF(module);
        return NULL;
    }

    DEC_CONST(LIL_ENDIAN);
    DEC_CONST(BIG_ENDIAN);

    DEC_CONSTSF(SWSURFACE);
    DEC_CONSTSF(HWSURFACE);
    DEC_CONSTSF(RESIZABLE);
    DEC_CONSTSF(ASYNCBLIT);
    DEC_CONSTSF(OPENGL);
    DEC_CONSTSF(OPENGLBLIT);
    DEC_CONSTSF(ANYFORMAT);
    DEC_CONSTSF(HWPALETTE);
    DEC_CONSTSF(DOUBLEBUF);
    DEC_CONSTSF(FULLSCREEN);
    DEC_CONSTSF(HWACCEL);
    DEC_CONSTSF(SRCCOLORKEY);
    DEC_CONSTSF(RLEACCELOK);
    DEC_CONSTSF(RLEACCEL);
    DEC_CONSTSF(SRCALPHA);
    DEC_CONSTSF(PREALLOC);
    DEC_CONSTSF(NOFRAME);
    DEC_CONSTSF(SHOWN);
    DEC_CONSTSF(HIDDEN);

    DEC_CONSTSF(SCROLL_ERASE);
    DEC_CONSTSF(SCROLL_REPEAT);

    DEC_CONSTSF(SCALED);

    DEC_CONST(GL_RED_SIZE);
    DEC_CONST(GL_GREEN_SIZE);
    DEC_CONST(GL_BLUE_SIZE);
    DEC_CONST(GL_ALPHA_SIZE);
    DEC_CONST(GL_BUFFER_SIZE);
    DEC_CONST(GL_DOUBLEBUFFER);
    DEC_CONST(GL_DEPTH_SIZE);
    DEC_CONST(GL_STENCIL_SIZE);
    DEC_CONST(GL_ACCUM_RED_SIZE);
    DEC_CONST(GL_ACCUM_GREEN_SIZE);
    DEC_CONST(GL_ACCUM_BLUE_SIZE);
    DEC_CONST(GL_ACCUM_ALPHA_SIZE);
    DEC_CONST(GL_ACCELERATED_VISUAL);
    DEC_CONST(GL_CONTEXT_MAJOR_VERSION);
    DEC_CONST(GL_CONTEXT_MINOR_VERSION);
    DEC_CONST(GL_SHARE_WITH_CURRENT_CONTEXT);

    DEC_CONST(GL_CONTEXT_FLAGS);
    /* context flag values: */
    DEC_CONST(GL_CONTEXT_DEBUG_FLAG);
    DEC_CONST(GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
    DEC_CONST(GL_CONTEXT_ROBUST_ACCESS_FLAG);
    DEC_CONST(GL_CONTEXT_RESET_ISOLATION_FLAG);

    DEC_CONST(GL_CONTEXT_PROFILE_MASK);
    /* values for GL_CONTEXT_PROFILE_MASK: */
    DEC_CONST(GL_CONTEXT_PROFILE_CORE);
    DEC_CONST(GL_CONTEXT_PROFILE_COMPATIBILITY);
    DEC_CONST(GL_CONTEXT_PROFILE_ES);
    DEC_CONST(GL_FRAMEBUFFER_SRGB_CAPABLE);
    DEC_CONST(GL_CONTEXT_RELEASE_BEHAVIOR);
    /* values for GL_CONTEXT_RELEASE_BEHAVIOR: */
    DEC_CONST(GL_CONTEXT_RELEASE_BEHAVIOR_NONE);
    DEC_CONST(GL_CONTEXT_RELEASE_BEHAVIOR_FLUSH);

    DEC_CONST(BLENDMODE_NONE);
    DEC_CONST(BLENDMODE_BLEND);
    DEC_CONST(BLENDMODE_ADD);
    DEC_CONST(BLENDMODE_MOD);
    DEC_CONST(BLENDMODE_MUL);
    DEC_CONST(BLENDFACTOR_ZERO);
    DEC_CONST(BLENDFACTOR_ONE);
    DEC_CONST(BLENDFACTOR_SRC_COLOR);
    DEC_CONST(BLENDFACTOR_ONE_MINUS_SRC_COLOR);
    DEC_CONST(BLENDFACTOR_SRC_ALPHA);
    DEC_CONST(BLENDFACTOR_ONE_MINUS_SRC_ALPHA);
    DEC_CONST(BLENDFACTOR_DST_COLOR);
    DEC_CONST(BLENDFACTOR_ONE_MINUS_DST_COLOR);
    DEC_CONST(BLENDFACTOR_DST_ALPHA);
    DEC_CONST(BLENDFACTOR_ONE_MINUS_DST_ALPHA);
    DEC_CONST(BLENDOPERATION_ADD);
    DEC_CONST(BLENDOPERATION_SUBTRACT);
    DEC_CONST(BLENDOPERATION_REV_SUBTRACT);
    DEC_CONST(BLENDOPERATION_MINIMUM);
    DEC_CONST(BLENDOPERATION_MAXIMUM);

    DEC_CONST(GL_STEREO);
    DEC_CONST(GL_MULTISAMPLEBUFFERS);
    DEC_CONST(GL_MULTISAMPLESAMPLES);
    DEC_CONST(GL_SWAP_CONTROL);

    DEC_CONSTN(TIMER_RESOLUTION);

    DEC_CONSTN(AUDIO_U8);
    DEC_CONSTN(AUDIO_S8);
    DEC_CONSTS(AUDIO_U16LSB, PG_AUDIO_U16LSB);
    DEC_CONSTN(AUDIO_S16LSB);
    DEC_CONSTS(AUDIO_U16MSB, PG_AUDIO_U16MSB);
    DEC_CONSTN(AUDIO_S16MSB);
    DEC_CONSTS(AUDIO_U16, PG_AUDIO_U16);
    DEC_CONSTN(AUDIO_S16);
    DEC_CONSTS(AUDIO_U16SYS, PG_AUDIO_U16SYS);
    DEC_CONSTN(AUDIO_S16SYS);

#define SCRAP_TEXT PYGAME_SCRAP_TEXT
#define SCRAP_BMP PYGAME_SCRAP_BMP
#define SCRAP_PPM PYGAME_SCRAP_PPM
#define SCRAP_PBM PYGAME_SCRAP_PBM

    ADD_STRING_CONST(SCRAP_TEXT);
    ADD_STRING_CONST(SCRAP_BMP);
    ADD_STRING_CONST(SCRAP_PPM);
    ADD_STRING_CONST(SCRAP_PBM);
    DEC_CONSTS(SCRAP_CLIPBOARD, 0);
    DEC_CONSTS(SCRAP_SELECTION, 1);

/* BLEND_ADD is an alias for BLEND_RGB_ADD
 */
#define PYGAME_BLEND_RGB_ADD 0x1
#define PYGAME_BLEND_RGB_SUB 0x2
#define PYGAME_BLEND_RGB_MULT 0x3
#define PYGAME_BLEND_RGB_MIN 0x4
#define PYGAME_BLEND_RGB_MAX 0x5

#define PYGAME_BLEND_ADD PYGAME_BLEND_RGB_ADD
#define PYGAME_BLEND_SUB PYGAME_BLEND_RGB_SUB
#define PYGAME_BLEND_MULT PYGAME_BLEND_RGB_MULT
#define PYGAME_BLEND_MIN PYGAME_BLEND_RGB_MIN
#define PYGAME_BLEND_MAX PYGAME_BLEND_RGB_MAX

#define PYGAME_BLEND_RGBA_ADD 0x6
#define PYGAME_BLEND_RGBA_SUB 0x7
#define PYGAME_BLEND_RGBA_MULT 0x8
#define PYGAME_BLEND_RGBA_MIN 0x9
#define PYGAME_BLEND_RGBA_MAX 0x10

#define PYGAME_BLEND_PREMULTIPLIED 0x11
#define PYGAME_BLEND_ALPHA_SDL2 0x12

    DEC_CONSTS(BLEND_ADD, PYGAME_BLEND_ADD);
    DEC_CONSTS(BLEND_SUB, PYGAME_BLEND_SUB);
    DEC_CONSTS(BLEND_MULT, PYGAME_BLEND_MULT);
    DEC_CONSTS(BLEND_MIN, PYGAME_BLEND_MIN);
    DEC_CONSTS(BLEND_MAX, PYGAME_BLEND_MAX);

    DEC_CONSTS(BLEND_RGB_ADD, PYGAME_BLEND_RGB_ADD);
    DEC_CONSTS(BLEND_RGB_SUB, PYGAME_BLEND_RGB_SUB);
    DEC_CONSTS(BLEND_RGB_MULT, PYGAME_BLEND_RGB_MULT);
    DEC_CONSTS(BLEND_RGB_MIN, PYGAME_BLEND_RGB_MIN);
    DEC_CONSTS(BLEND_RGB_MAX, PYGAME_BLEND_RGB_MAX);

    DEC_CONSTS(BLEND_RGBA_ADD, PYGAME_BLEND_RGBA_ADD);
    DEC_CONSTS(BLEND_RGBA_SUB, PYGAME_BLEND_RGBA_SUB);
    DEC_CONSTS(BLEND_RGBA_MULT, PYGAME_BLEND_RGBA_MULT);
    DEC_CONSTS(BLEND_RGBA_MIN, PYGAME_BLEND_RGBA_MIN);
    DEC_CONSTS(BLEND_RGBA_MAX, PYGAME_BLEND_RGBA_MAX);
    DEC_CONSTS(BLEND_PREMULTIPLIED, PYGAME_BLEND_PREMULTIPLIED);
    DEC_CONSTS(BLEND_ALPHA_SDL2, PYGAME_BLEND_ALPHA_SDL2);

    /* Event types
     */
    DEC_CONST(NOEVENT);
    DEC_CONST(ACTIVEEVENT);
    DEC_CONST(KEYDOWN);
    DEC_CONST(KEYUP);
    DEC_CONST(MOUSEMOTION);
    DEC_CONST(MOUSEBUTTONDOWN);
    DEC_CONST(MOUSEBUTTONUP);
    DEC_CONST(JOYAXISMOTION);
    DEC_CONSTS(JOYBALLMOTION, PG_JOYBALLMOTION);
    DEC_CONST(JOYHATMOTION);
    DEC_CONST(JOYBUTTONDOWN);
    DEC_CONST(JOYBUTTONUP);
    DEC_CONST(VIDEORESIZE);
    DEC_CONST(VIDEOEXPOSE);
    DEC_CONST(QUIT);
    DEC_CONST(SYSWMEVENT);
    DEC_CONSTS(MIDIIN, PGE_MIDIIN);
    DEC_CONSTS(MIDIOUT, PGE_MIDIOUT);
    DEC_CONSTS(USEREVENT, PGE_USEREVENT);
    DEC_CONSTS(NUMEVENTS, PG_NUMEVENTS);

    DEC_CONST(APP_TERMINATING);
    DEC_CONST(APP_LOWMEMORY);
    DEC_CONST(APP_WILLENTERBACKGROUND);
    DEC_CONST(APP_DIDENTERBACKGROUND);
    DEC_CONST(APP_WILLENTERFOREGROUND);
    DEC_CONST(APP_DIDENTERFOREGROUND);
    DEC_CONST(CLIPBOARDUPDATE);
    DEC_CONST(KEYMAPCHANGED);

    DEC_CONST(RENDER_TARGETS_RESET);
    DEC_CONST(RENDER_DEVICE_RESET);

    DEC_CONST(HAT_CENTERED);
    DEC_CONST(HAT_UP);
    DEC_CONST(HAT_RIGHTUP);
    DEC_CONST(HAT_RIGHT);
    DEC_CONST(HAT_RIGHTDOWN);
    DEC_CONST(HAT_DOWN);
    DEC_CONST(HAT_LEFTDOWN);
    DEC_CONST(HAT_LEFT);
    DEC_CONST(HAT_LEFTUP);

    DEC_CONST(BUTTON_LEFT);
    DEC_CONST(BUTTON_MIDDLE);
    DEC_CONST(BUTTON_RIGHT);

    DEC_CONST(FINGERMOTION);
    DEC_CONST(FINGERDOWN);
    DEC_CONST(FINGERUP);
    DEC_CONSTS(MULTIGESTURE, PG_MULTIGESTURE);
    DEC_CONST(AUDIODEVICEADDED);
    DEC_CONST(AUDIODEVICEREMOVED);
    DEC_CONST(MOUSEWHEEL);
    DEC_CONST(TEXTINPUT);
    DEC_CONST(TEXTEDITING);

    DEC_CONSTS(WINDOWSHOWN, PGE_WINDOWSHOWN);
    DEC_CONSTS(WINDOWHIDDEN, PGE_WINDOWHIDDEN);
    DEC_CONSTS(WINDOWEXPOSED, PGE_WINDOWEXPOSED);
    DEC_CONSTS(WINDOWMOVED, PGE_WINDOWMOVED);
    DEC_CONSTS(WINDOWRESIZED, PGE_WINDOWRESIZED);
    DEC_CONSTS(WINDOWSIZECHANGED, PGE_WINDOWSIZECHANGED);
    DEC_CONSTS(WINDOWMINIMIZED, PGE_WINDOWMINIMIZED);
    DEC_CONSTS(WINDOWMAXIMIZED, PGE_WINDOWMAXIMIZED);
    DEC_CONSTS(WINDOWRESTORED, PGE_WINDOWRESTORED);
    DEC_CONSTS(WINDOWENTER, PGE_WINDOWENTER);
    DEC_CONSTS(WINDOWLEAVE, PGE_WINDOWLEAVE);
    DEC_CONSTS(WINDOWFOCUSGAINED, PGE_WINDOWFOCUSGAINED);
    DEC_CONSTS(WINDOWFOCUSLOST, PGE_WINDOWFOCUSLOST);
    DEC_CONSTS(WINDOWCLOSE, PGE_WINDOWCLOSE);
    DEC_CONSTS(WINDOWTAKEFOCUS, PGE_WINDOWTAKEFOCUS);
    DEC_CONSTS(WINDOWHITTEST, PGE_WINDOWHITTEST);
    DEC_CONSTS(WINDOWICCPROFCHANGED, PGE_WINDOWICCPROFCHANGED);
    DEC_CONSTS(WINDOWDISPLAYCHANGED, PGE_WINDOWDISPLAYCHANGED);

    DEC_CONST(CONTROLLERAXISMOTION);
    DEC_CONST(CONTROLLERBUTTONDOWN);
    DEC_CONST(CONTROLLERBUTTONUP);
    DEC_CONST(CONTROLLERDEVICEADDED);
    DEC_CONST(CONTROLLERDEVICEREMOVED);
    DEC_CONST(CONTROLLERDEVICEREMAPPED);
    DEC_CONST(CONTROLLERTOUCHPADDOWN);
    DEC_CONST(CONTROLLERTOUCHPADMOTION);
    DEC_CONST(CONTROLLERTOUCHPADUP);
    DEC_CONST(CONTROLLERSENSORUPDATE);
    DEC_CONST(LOCALECHANGED);

    DEC_CONST(JOYDEVICEADDED);
    DEC_CONST(JOYDEVICEREMOVED);

    DEC_CONSTS(BUTTON_X1, PGM_BUTTON_X1);
    DEC_CONSTS(BUTTON_X2, PGM_BUTTON_X2);
    // Still to be decided
    DEC_CONSTS(BUTTON_WHEELUP, PGM_BUTTON_WHEELUP);
    DEC_CONSTS(BUTTON_WHEELDOWN, PGM_BUTTON_WHEELDOWN);
    DEC_CONSTS(AUDIO_ALLOW_FREQUENCY_CHANGE, PG_AUDIO_ALLOW_FREQUENCY_CHANGE);
    DEC_CONSTS(AUDIO_ALLOW_FORMAT_CHANGE, PG_AUDIO_ALLOW_FORMAT_CHANGE);
    DEC_CONSTS(AUDIO_ALLOW_CHANNELS_CHANGE, PG_AUDIO_ALLOW_CHANNELS_CHANGE);
    DEC_CONSTS(AUDIO_ALLOW_ANY_CHANGE, PG_AUDIO_ALLOW_ANY_CHANGE);
    DEC_CONST(DROPFILE);
    DEC_CONST(DROPTEXT);
    DEC_CONST(DROPBEGIN);
    DEC_CONST(DROPCOMPLETE);
    DEC_CONST(CONTROLLER_AXIS_INVALID);
    DEC_CONST(CONTROLLER_AXIS_LEFTX);
    DEC_CONST(CONTROLLER_AXIS_LEFTY);
    DEC_CONST(CONTROLLER_AXIS_RIGHTX);
    DEC_CONST(CONTROLLER_AXIS_RIGHTY);
    DEC_CONST(CONTROLLER_AXIS_TRIGGERLEFT);
    DEC_CONST(CONTROLLER_AXIS_TRIGGERRIGHT);
    DEC_CONST(CONTROLLER_AXIS_MAX);
    DEC_CONST(CONTROLLER_BUTTON_INVALID);
    DEC_CONST(CONTROLLER_BUTTON_A);
    DEC_CONST(CONTROLLER_BUTTON_B);
    DEC_CONST(CONTROLLER_BUTTON_X);
    DEC_CONST(CONTROLLER_BUTTON_Y);
    DEC_CONST(CONTROLLER_BUTTON_BACK);
    DEC_CONST(CONTROLLER_BUTTON_GUIDE);
    DEC_CONST(CONTROLLER_BUTTON_START);
    DEC_CONST(CONTROLLER_BUTTON_LEFTSTICK);
    DEC_CONST(CONTROLLER_BUTTON_RIGHTSTICK);
    DEC_CONST(CONTROLLER_BUTTON_LEFTSHOULDER);
    DEC_CONST(CONTROLLER_BUTTON_RIGHTSHOULDER);
    DEC_CONST(CONTROLLER_BUTTON_DPAD_UP);
    DEC_CONST(CONTROLLER_BUTTON_DPAD_DOWN);
    DEC_CONST(CONTROLLER_BUTTON_DPAD_LEFT);
    DEC_CONST(CONTROLLER_BUTTON_DPAD_RIGHT);
    DEC_CONST(CONTROLLER_BUTTON_MAX);

    /* Keyboard key codes: Pygame K_ constants. Scan codes: KSCAN_ constants.
     */
    DEC_CONSTK_AND_SC(AC_BACK);
    DEC_CONSTK_AND_SC(UNKNOWN);
    DEC_CONSTK_AND_SC(BACKSPACE);
    DEC_CONSTK_AND_SC(TAB);
    DEC_CONSTK_AND_SC(CLEAR);
    DEC_CONSTK_AND_SC(RETURN);
    DEC_CONSTK_AND_SC(PAUSE);
    DEC_CONSTK_AND_SC(ESCAPE);
    DEC_CONSTK_AND_SC(SPACE);
    DEC_CONSTK(QUOTE);
    DEC_CONSTSC(APOSTROPHE);
    DEC_CONSTK_AND_SC(COMMA);
    DEC_CONSTK_AND_SC(MINUS);
    DEC_CONSTK_AND_SC(PERIOD);
    DEC_CONSTK_AND_SC(SLASH);
    DEC_CONSTK_AND_SC(0);
    DEC_CONSTK_AND_SC(1);
    DEC_CONSTK_AND_SC(2);
    DEC_CONSTK_AND_SC(3);
    DEC_CONSTK_AND_SC(4);
    DEC_CONSTK_AND_SC(5);
    DEC_CONSTK_AND_SC(6);
    DEC_CONSTK_AND_SC(7);
    DEC_CONSTK_AND_SC(8);
    DEC_CONSTK_AND_SC(9);
    DEC_CONSTK_AND_SC(SEMICOLON);
    DEC_CONSTK_AND_SC(EQUALS);
    DEC_CONSTK_AND_SC(LEFTBRACKET);
    DEC_CONSTK_AND_SC(BACKSLASH);
    DEC_CONSTK_AND_SC(RIGHTBRACKET);
    DEC_CONSTK(BACKQUOTE);
    DEC_CONSTSC(GRAVE);
    DEC_CONSTK(a);
    DEC_CONSTSC(A);
    DEC_CONSTK(b);
    DEC_CONSTSC(B);
    DEC_CONSTK(c);
    DEC_CONSTSC(C);
    DEC_CONSTK(d);
    DEC_CONSTSC(D);
    DEC_CONSTK(e);
    DEC_CONSTSC(E);
    DEC_CONSTK(f);
    DEC_CONSTSC(F);
    DEC_CONSTK(g);
    DEC_CONSTSC(G);
    DEC_CONSTK(h);
    DEC_CONSTSC(H);
    DEC_CONSTK(i);
    DEC_CONSTSC(I);
    DEC_CONSTK(j);
    DEC_CONSTSC(J);
    DEC_CONSTK(k);
    DEC_CONSTSC(K);
    DEC_CONSTK(l);
    DEC_CONSTSC(L);
    DEC_CONSTK(m);
    DEC_CONSTSC(M);
    DEC_CONSTK(n);
    DEC_CONSTSC(N);
    DEC_CONSTK(o);
    DEC_CONSTSC(O);
    DEC_CONSTK(p);
    DEC_CONSTSC(P);
    DEC_CONSTK(q);
    DEC_CONSTSC(Q);
    DEC_CONSTK(r);
    DEC_CONSTSC(R);
    DEC_CONSTK(s);
    DEC_CONSTSC(S);
    DEC_CONSTK(t);
    DEC_CONSTSC(T);
    DEC_CONSTK(u);
    DEC_CONSTSC(U);
    DEC_CONSTK(v);
    DEC_CONSTSC(V);
    DEC_CONSTK(w);
    DEC_CONSTSC(W);
    DEC_CONSTK(x);
    DEC_CONSTSC(X);
    DEC_CONSTK(y);
    DEC_CONSTSC(Y);
    DEC_CONSTK(z);
    DEC_CONSTSC(Z);
    DEC_CONSTK_AND_SC(DELETE);

    DEC_CONSTK_AND_SC(KP_0);
    DEC_CONSTK_AND_SC(KP_1);
    DEC_CONSTK_AND_SC(KP_2);
    DEC_CONSTK_AND_SC(KP_3);
    DEC_CONSTK_AND_SC(KP_4);
    DEC_CONSTK_AND_SC(KP_5);
    DEC_CONSTK_AND_SC(KP_6);
    DEC_CONSTK_AND_SC(KP_7);
    DEC_CONSTK_AND_SC(KP_8);
    DEC_CONSTK_AND_SC(KP_9);
    DEC_CONSTKS_AND_SCS(KP0, KP_0);
    DEC_CONSTKS_AND_SCS(KP1, KP_1);
    DEC_CONSTKS_AND_SCS(KP2, KP_2);
    DEC_CONSTKS_AND_SCS(KP3, KP_3);
    DEC_CONSTKS_AND_SCS(KP4, KP_4);
    DEC_CONSTKS_AND_SCS(KP5, KP_5);
    DEC_CONSTKS_AND_SCS(KP6, KP_6);
    DEC_CONSTKS_AND_SCS(KP7, KP_7);
    DEC_CONSTKS_AND_SCS(KP8, KP_8);
    DEC_CONSTKS_AND_SCS(KP9, KP_9);
    DEC_CONSTK_AND_SC(KP_PERIOD);
    DEC_CONSTK_AND_SC(KP_DIVIDE);
    DEC_CONSTK_AND_SC(KP_MULTIPLY);
    DEC_CONSTK_AND_SC(KP_MINUS);
    DEC_CONSTK_AND_SC(KP_PLUS);
    DEC_CONSTK_AND_SC(KP_ENTER);
    DEC_CONSTK_AND_SC(KP_EQUALS);
    DEC_CONSTK_AND_SC(UP);
    DEC_CONSTK_AND_SC(DOWN);
    DEC_CONSTK_AND_SC(RIGHT);
    DEC_CONSTK_AND_SC(LEFT);
    DEC_CONSTK_AND_SC(INSERT);
    DEC_CONSTK_AND_SC(HOME);
    DEC_CONSTK_AND_SC(END);
    DEC_CONSTK_AND_SC(PAGEUP);
    DEC_CONSTK_AND_SC(PAGEDOWN);
    DEC_CONSTK_AND_SC(F1);
    DEC_CONSTK_AND_SC(F2);
    DEC_CONSTK_AND_SC(F3);
    DEC_CONSTK_AND_SC(F4);
    DEC_CONSTK_AND_SC(F5);
    DEC_CONSTK_AND_SC(F6);
    DEC_CONSTK_AND_SC(F7);
    DEC_CONSTK_AND_SC(F8);
    DEC_CONSTK_AND_SC(F9);
    DEC_CONSTK_AND_SC(F10);
    DEC_CONSTK_AND_SC(F11);
    DEC_CONSTK_AND_SC(F12);
    DEC_CONSTK_AND_SC(F13);
    DEC_CONSTK_AND_SC(F14);
    DEC_CONSTK_AND_SC(F15);

    DEC_CONSTK_AND_SC(NUMLOCKCLEAR)
    DEC_CONSTKS_AND_SCS(NUMLOCK, NUMLOCKCLEAR);
    DEC_CONSTK_AND_SC(CAPSLOCK);
    DEC_CONSTK_AND_SC(SCROLLLOCK);
    DEC_CONSTKS_AND_SCS(SCROLLOCK, SCROLLLOCK);
    DEC_CONSTK_AND_SC(RSHIFT);
    DEC_CONSTK_AND_SC(LSHIFT);
    DEC_CONSTK_AND_SC(RCTRL);
    DEC_CONSTK_AND_SC(LCTRL);
    DEC_CONSTK_AND_SC(RALT);
    DEC_CONSTK_AND_SC(LALT);
    DEC_CONSTK_AND_SC(RGUI);
    DEC_CONSTKS_AND_SCS(RMETA, RGUI);
    DEC_CONSTK_AND_SC(LGUI);
    DEC_CONSTKS_AND_SCS(LMETA, LGUI);
    DEC_CONSTKS_AND_SCS(LSUPER, LGUI);
    DEC_CONSTKS_AND_SCS(RSUPER, RGUI);
    DEC_CONSTK_AND_SC(MODE);

    DEC_CONSTK_AND_SC(HELP);
    DEC_CONSTK_AND_SC(PRINTSCREEN);
    DEC_CONSTKS_AND_SCS(PRINT, PRINTSCREEN);
    DEC_CONSTK_AND_SC(SYSREQ);
    DEC_CONSTKS_AND_SCS(BREAK, PAUSE);
    DEC_CONSTK_AND_SC(MENU);
    DEC_CONSTK_AND_SC(POWER);
    DEC_CONSTK_AND_SC(CURRENCYUNIT);
    DEC_CONSTK_AND_SC(CURRENCYSUBUNIT);
    DEC_CONSTKS_AND_SCS(EURO, CURRENCYUNIT);

    DEC_CONSTSC(INTERNATIONAL1);
    DEC_CONSTSC(INTERNATIONAL2);
    DEC_CONSTSC(INTERNATIONAL3);
    DEC_CONSTSC(INTERNATIONAL4);
    DEC_CONSTSC(INTERNATIONAL5);
    DEC_CONSTSC(INTERNATIONAL6);
    DEC_CONSTSC(INTERNATIONAL7);
    DEC_CONSTSC(INTERNATIONAL8);
    DEC_CONSTSC(INTERNATIONAL9);
    DEC_CONSTSC(LANG1);
    DEC_CONSTSC(LANG2);
    DEC_CONSTSC(LANG3);
    DEC_CONSTSC(LANG4);
    DEC_CONSTSC(LANG5);
    DEC_CONSTSC(LANG6);
    DEC_CONSTSC(LANG7);
    DEC_CONSTSC(LANG8);
    DEC_CONSTSC(LANG9);
    DEC_CONSTSC(NONUSBACKSLASH);
    DEC_CONSTSC(NONUSHASH);

    DEC_CONSTK(EXCLAIM);
    DEC_CONSTK(QUOTEDBL);
    DEC_CONSTK(HASH);
    DEC_CONSTK(DOLLAR);
    DEC_CONSTK(AMPERSAND);
    DEC_CONSTK(PERCENT);
    DEC_CONSTK(LEFTPAREN);
    DEC_CONSTK(RIGHTPAREN);
    DEC_CONSTK(ASTERISK);
    DEC_CONSTK(PLUS);
    DEC_CONSTK(COLON);
    DEC_CONSTK(LESS);
    DEC_CONSTK(GREATER);
    DEC_CONSTK(QUESTION);
    DEC_CONSTK(AT);
    DEC_CONSTK(CARET);
    DEC_CONSTK(UNDERSCORE);

    /* Keyboard key modifiers: Pygame KMOD_ constants.
     */
    DEC_CONSTN(KMOD_NONE);
    DEC_CONSTN(KMOD_LSHIFT);
    DEC_CONSTN(KMOD_RSHIFT);
    DEC_CONSTN(KMOD_LCTRL);
    DEC_CONSTN(KMOD_RCTRL);
    DEC_CONSTN(KMOD_LALT);
    DEC_CONSTN(KMOD_RALT);
    DEC_CONSTN(KMOD_LGUI);
    DEC_CONSTS(KMOD_LMETA, KMOD_LGUI);
    DEC_CONSTN(KMOD_RGUI);
    DEC_CONSTS(KMOD_RMETA, KMOD_RGUI);
    DEC_CONSTN(KMOD_NUM);
    DEC_CONSTN(KMOD_CAPS);
    DEC_CONSTN(KMOD_MODE);

    DEC_CONSTN(KMOD_CTRL);
    DEC_CONSTN(KMOD_SHIFT);
    DEC_CONSTN(KMOD_ALT);
    DEC_CONSTN(KMOD_GUI);
    DEC_CONSTS(KMOD_META, KMOD_GUI);

    DEC_CONST(APPMOUSEFOCUS);
    DEC_CONST(APPINPUTFOCUS);
    DEC_CONST(APPACTIVE);

    /* cursor constants */
    DEC_CONST(SYSTEM_CURSOR_ARROW);
    DEC_CONST(SYSTEM_CURSOR_IBEAM);
    DEC_CONST(SYSTEM_CURSOR_WAIT);
    DEC_CONST(SYSTEM_CURSOR_CROSSHAIR);
    DEC_CONST(SYSTEM_CURSOR_WAITARROW);
    DEC_CONST(SYSTEM_CURSOR_SIZENWSE);
    DEC_CONST(SYSTEM_CURSOR_SIZENESW);
    DEC_CONST(SYSTEM_CURSOR_SIZEWE);
    DEC_CONST(SYSTEM_CURSOR_SIZENS);
    DEC_CONST(SYSTEM_CURSOR_SIZEALL);
    DEC_CONST(SYSTEM_CURSOR_NO);
    DEC_CONST(SYSTEM_CURSOR_HAND);

#define PYGAME_USEREVENT_DROPFILE 0x1000
    DEC_CONSTS(USEREVENT_DROPFILE, PYGAME_USEREVENT_DROPFILE);

    /* constants for font direction */
    DEC_CONSTS(DIRECTION_LTR, 0);
    DEC_CONSTS(DIRECTION_RTL, 1);
    DEC_CONSTS(DIRECTION_TTB, 2);
    DEC_CONSTS(DIRECTION_BTT, 3);

    /* Font alignment constants */
    DEC_CONSTS(FONT_LEFT, 0);
    DEC_CONSTS(FONT_CENTER, 1);
    DEC_CONSTS(FONT_RIGHT, 2);

    // https://github.com/pygame-community/pygame-ce/issues/1845
    DEC_CONSTS(IS_CE, 1);

    ADD_NAMED_STRING_CONST(NULL_VIDEODRIVER, "dummy");

    DEC_CONSTS(WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED);
    DEC_CONSTS(WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);

#if SDL_VERSION_ATLEAST(2, 0, 16)
    DEC_CONSTS(FLASH_CANCEL, SDL_FLASH_CANCEL);
    DEC_CONSTS(FLASH_BRIEFLY, SDL_FLASH_BRIEFLY);
    DEC_CONSTS(FLASH_UNTIL_FOCUSED, SDL_FLASH_UNTIL_FOCUSED);
#else
    DEC_CONSTS(FLASH_CANCEL, -1);
    DEC_CONSTS(FLASH_BRIEFLY, -1);
    DEC_CONSTS(FLASH_UNTIL_FOCUSED, -1);
#endif

    if (PyModule_AddObject(module, "__all__", all_list)) {
        Py_DECREF(all_list);
        Py_DECREF(module);
        return NULL;
    }

    return module;
}
