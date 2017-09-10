/*
 * data-types.h
 * Copyright (C) 2016 Kovid Goyal <kovid at kovidgoyal.net>
 *
 * Distributed under terms of the GPL3 license.
 */

#pragma once


#include <stdint.h>
#include <stdbool.h>
#include <poll.h>
#include <pthread.h>
#define PY_SSIZE_T_CLEAN
#include <Python.h>
#define UNUSED __attribute__ ((unused))
#define EXPORTED __attribute__ ((visibility ("default")))
#define LIKELY(x)    __builtin_expect (!!(x), 1)
#define UNLIKELY(x)  __builtin_expect (!!(x), 0)
#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) > (y)) ? (y) : (x))
#define xstr(s) str(s)
#define str(s) #s

typedef uint32_t char_type;
typedef uint32_t color_type;
typedef uint32_t combining_type;
typedef unsigned int index_type;
typedef uint16_t sprite_index;

#define ERROR_PREFIX "[PARSE ERROR]"
#define ANY_MODE 3
#define MOTION_MODE 2
#define BUTTON_MODE 1
#define NORMAL_PROTOCOL 0
#define UTF8_PROTOCOL 1
#define SGR_PROTOCOL 2
#define URXVT_PROTOCOL 3

#define BLANK_CHAR 0
#define CHAR_MASK 0xFFFFFF
#define ATTRS_SHIFT 24
#define ATTRS_MASK_WITHOUT_WIDTH 0xFC000000
#define WIDTH_MASK  3
#define DECORATION_SHIFT  2
#define DECORATION_MASK 3
#define BOLD_SHIFT 4
#define ITALIC_SHIFT 5
#define POSCHAR_MASK 0x30FFFFFF
#define REVERSE_SHIFT 6
#define STRIKE_SHIFT 7
#define COL_MASK 0xFFFFFFFF
#define CC_MASK 0xFFFF
#define CC_SHIFT 16
#define UTF8_ACCEPT 0
#define UTF8_REJECT 1
#define UNDERCURL_CODE 6
#define DECORATION_FG_CODE 58
#define CHAR_IS_BLANK(ch) ((ch & CHAR_MASK) == 32 || (ch & CHAR_MASK) == 0)

#define CURSOR_BLOCK 1
#define CURSOR_BEAM 2
#define CURSOR_UNDERLINE 3
#define FG 1
#define BG 2

#define CURSOR_TO_ATTRS(c, w) \
    ((w) | (((c->decoration & 3) << DECORATION_SHIFT) | ((c->bold & 1) << BOLD_SHIFT) | \
            ((c->italic & 1) << ITALIC_SHIFT) | ((c->reverse & 1) << REVERSE_SHIFT) | ((c->strikethrough & 1) << STRIKE_SHIFT))) << ATTRS_SHIFT

#define ATTRS_TO_CURSOR(a, c) \
    c->decoration = (a >> DECORATION_SHIFT) & 3; c->bold = (a >> BOLD_SHIFT) & 1; c->italic = (a >> ITALIC_SHIFT) & 1; \
    c->reverse = (a >> REVERSE_SHIFT) & 1; c->strikethrough = (a >> STRIKE_SHIFT) & 1;

#define COPY_CELL(src, s, dest, d) \
    (dest)->cells[d] = (src)->cells[s];

#define COPY_SELF_CELL(s, d) COPY_CELL(self, s, self, d)

#define METHOD(name, arg_type) {#name, (PyCFunction)name, arg_type, name##_doc},

#define BOOL_GETSET(type, x) \
    static PyObject* x##_get(type *self, void UNUSED *closure) { PyObject *ans = self->x ? Py_True : Py_False; Py_INCREF(ans); return ans; } \
    static int x##_set(type *self, PyObject *value, void UNUSED *closure) { if (value == NULL) { PyErr_SetString(PyExc_TypeError, "Cannot delete attribute"); return -1; } self->x = PyObject_IsTrue(value) ? true : false; return 0; }

#define GETSET(x) \
    {#x, (getter) x##_get, (setter) x##_set, #x, NULL},

#ifndef EXTRA_INIT
#define EXTRA_INIT
#endif
#define INIT_TYPE(type) \
    int init_##type(PyObject *module) {\
        if (PyType_Ready(&type##_Type) < 0) return 0; \
        if (PyModule_AddObject(module, #type, (PyObject *)&type##_Type) != 0) return 0; \
        Py_INCREF(&type##_Type); \
        EXTRA_INIT; \
        return 1; \
    }

#define RICHCMP(type) \
    static PyObject * richcmp(PyObject *obj1, PyObject *obj2, int op) { \
        PyObject *result = NULL; \
        int eq; \
        if (op != Py_EQ && op != Py_NE) { Py_RETURN_NOTIMPLEMENTED; } \
        if (!PyObject_TypeCheck(obj1, &type##_Type)) { Py_RETURN_FALSE; } \
        if (!PyObject_TypeCheck(obj2, &type##_Type)) { Py_RETURN_FALSE; } \
        eq = __eq__((type*)obj1, (type*)obj2); \
        if (op == Py_NE) result = eq ? Py_False : Py_True; \
        else result = eq ? Py_True : Py_False; \
        Py_INCREF(result); \
        return result; \
    }

#ifdef __clang__
#define START_ALLOW_CASE_RANGE _Pragma("clang diagnostic push") _Pragma("clang diagnostic ignored \"-Wpedantic\"")
#define END_ALLOW_CASE_RANGE _Pragma("clang diagnostic pop")
#define ALLOW_UNUSED_RESULT _Pragma("clang diagnostic push") _Pragma("clang diagnostic ignored \"-Wunused-result\"")
#define END_ALLOW_UNUSED_RESULT _Pragma("clang diagnostic pop")
#else
#define START_ALLOW_CASE_RANGE _Pragma("GCC diagnostic ignored \"-Wpedantic\"")
#define END_ALLOW_CASE_RANGE _Pragma("GCC diagnostic pop")
#define ALLOW_UNUSED_RESULT _Pragma("GCC diagnostic ignored \"-Wunused-result\"")
#define END_ALLOW_UNUSED_RESULT _Pragma("GCC diagnostic pop")
#endif

typedef struct {
    char_type ch;
    color_type fg, bg, decoration_fg;
    combining_type cc;
    sprite_index sprite_x, sprite_y, sprite_z;
} Cell;

typedef struct {
    PyObject_HEAD

    Cell *cells;
    index_type xnum, ynum;
    bool continued;
    bool needs_free;
} Line;
PyTypeObject Line_Type;


typedef struct {
    PyObject_HEAD

    Cell *buf;
    index_type xnum, ynum, *line_map, *scratch;
    bool *continued_map;
    Line *line;
} LineBuf;
PyTypeObject LineBuf_Type;


typedef struct {
    PyObject_HEAD

    Cell *buf;
    index_type xnum, ynum;
    Line *line;
    index_type start_of_data, count;
    bool *continued_map;
} HistoryBuf;
PyTypeObject HistoryBuf_Type;

typedef struct {
    PyObject_HEAD

    bool bold, italic, reverse, strikethrough, blink;
    unsigned int x, y;
    uint8_t decoration, shape;
    unsigned long fg, bg, decoration_fg;

} Cursor;
PyTypeObject Cursor_Type;

PyTypeObject Face_Type;
PyTypeObject Window_Type;

typedef struct {
    color_type default_fg, default_bg, cursor_color, highlight_fg, highlight_bg;
} DynamicColor;

typedef struct {
    PyObject_HEAD

    PyObject *dirty, *ubo;
    uint32_t color_table[256];
    uint32_t orig_color_table[256];
    DynamicColor configured, overridden;
} ColorProfile;
PyTypeObject ColorProfile_Type;

typedef struct {
    bool mLNM, mIRM, mDECTCEM, mDECSCNM, mDECOM, mDECAWM, mDECCOLM, mDECARM, mDECCKM,
         mBRACKETED_PASTE, mFOCUS_TRACKING, mEXTENDED_KEYBOARD;
    unsigned long mouse_tracking_mode, mouse_tracking_protocol;
} ScreenModes;
PyTypeObject ScreenModes_Type;

#define SAVEPOINTS_SZ 256

typedef struct {
    uint32_t utf8_state, utf8_codepoint, *g0_charset, *g1_charset, *g_charset;
    bool use_latin1;
    Cursor cursor;
    bool mDECOM, mDECAWM, mDECSCNM;

} Savepoint;


typedef struct {
    Savepoint buf[SAVEPOINTS_SZ];
    index_type start_of_data, count;
} SavepointBuffer;


#define PARSER_BUF_SZ (8 * 1024)
#define READ_BUF_SZ (1024*1024)

typedef struct {
    unsigned int x, y;
} SelectionBoundary;

typedef struct {
    unsigned int start_x, start_y, start_scrolled_by, end_x, end_y, end_scrolled_by;
    bool in_progress;
} Selection;
    
typedef struct {
    PyObject_HEAD

    unsigned int columns, lines, margin_top, margin_bottom, charset, scrolled_by, last_selection_scrolled_by;
    uint32_t utf8_state, utf8_codepoint, *g0_charset, *g1_charset, *g_charset;
    Selection selection;
    SelectionBoundary last_rendered_selection_start, last_rendered_selection_end;
    bool use_latin1, selection_updated_once;
    Cursor *cursor;
    SavepointBuffer main_savepoints, alt_savepoints;
    PyObject *callbacks, *is_dirty, *cursor_changed, *scroll_changed;
    LineBuf *linebuf, *main_linebuf, *alt_linebuf;
    HistoryBuf *historybuf;
    unsigned int history_line_added_count;
    bool *tabstops, *main_tabstops, *alt_tabstops;
    ScreenModes modes;
    ColorProfile *color_profile;

    uint32_t parser_buf[PARSER_BUF_SZ];
    unsigned int parser_state, parser_text_start, parser_buf_pos;
    bool parser_has_pending_text;
    uint8_t read_buf[READ_BUF_SZ], *write_buf;
    size_t read_buf_sz, write_buf_sz;
    pthread_mutex_t read_buf_lock, write_buf_lock;

} Screen;
PyTypeObject Screen_Type;

typedef struct {
    double at;
    PyObject *callback;
    PyObject *args;
} TimerEvent;

typedef struct {
    PyObject_HEAD

    TimerEvent *events, *buf1, *buf2;
    size_t capacity;
    size_t count;
    bool in_call;
} Timers;
PyTypeObject Timers_Type;

typedef struct {
    PyObject_HEAD

    PyObject *dump_callback, *update_screen, *death_notify, *render_func;
    Timers *timers;
    double repaint_delay;
    unsigned int count;
    bool shutting_down;
    pthread_t io_thread;
} ChildMonitor;
PyTypeObject ChildMonitor_Type;

#define clear_sprite_position(cell) (cell).sprite_x = 0; (cell).sprite_y = 0; (cell).sprite_z = 0; 

#define left_shift_line(line, at, num) \
    for(index_type __i__ = (at); __i__ < (line)->xnum - (num); __i__++) { \
        COPY_CELL(line, __i__ + (num), line, __i__) \
    } \
    if ((((line)->cells[(at)].ch >> ATTRS_SHIFT) & WIDTH_MASK) != 1) { \
        (line)->cells[(at)].ch = (1 << ATTRS_SHIFT) | BLANK_CHAR; \
        clear_sprite_position((line)->cells[(at)]); \
    }


// Global functions 
Line* alloc_line();
Cursor* alloc_cursor();
LineBuf* alloc_linebuf(unsigned int, unsigned int);
HistoryBuf* alloc_historybuf(unsigned int, unsigned int);
ColorProfile* alloc_color_profile();
PyObject* create_256_color_table();
void parse_worker(Screen *screen, PyObject *dump_callback);
void parse_worker_dump(Screen *screen, PyObject *dump_callback);
PyObject* parse_bytes_dump(PyObject UNUSED *, PyObject *);
PyObject* parse_bytes(PyObject UNUSED *, PyObject *);
uint32_t decode_utf8(uint32_t*, uint32_t*, uint8_t byte);
void cursor_reset(Cursor*);
Cursor* cursor_copy(Cursor*);
void cursor_copy_to(Cursor *src, Cursor *dest);
void cursor_reset_display_attrs(Cursor*);
void set_sprite_position(Cell *cell, Cell *previous_cell);

double monotonic();
double timers_timeout(Timers*);
void timers_call(Timers*);
bool timers_add(Timers *self, double delay, bool, PyObject *callback, PyObject *args);
bool timers_add_if_missing(Timers *self, double delay, PyObject *callback, PyObject *args);
bool timers_add_if_before(Timers *self, double delay, PyObject *callback, PyObject *args);
PyObject* cm_thread_write(PyObject *self, PyObject *args);
bool set_iutf8(int, bool);

color_type colorprofile_to_color(ColorProfile *self, color_type entry, color_type defval);

unsigned int safe_wcwidth(uint32_t ch);
void change_wcwidth(bool use9);

