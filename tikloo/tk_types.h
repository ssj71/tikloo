//spencer jackson
//tikloo toolkit

//tk_types.h

//this header describes the main type and what is available within the struct as well as supporting types

#ifndef TK_TYPES_H
#define TK_TYPES_H
#include<stdint.h>
#include<cairo/cairo.h>
#include<cairo/cairo-ft.h>
#include<ft2build.h> 
#include FT_FREETYPE_H
#include "pugl/pugl.h"
#include "timer.h"

typedef struct tk_stuff
{
    //////primary table, each index correlates across arrays
    //size and position
    float *x,*y,*w,*h;
    //items on higher layers recieve callbacks, never set to 0
    uint8_t *layer;
    //value could be any type so be careful here
    void **value;
    //a string pointer for tooltips
    char **tip;
    //property flags
    uint16_t *props;
    void (**draw_f)(cairo_t*, float, float, void*); //surface, w, h, value
    //built in callback, must not be 0
    void (**cb_f)(struct tk_stuff*, const PuglEvent*, uint16_t); 
    //user callback, run after built in
    void (**callback_f)(struct tk_stuff*, const PuglEvent*, uint16_t);
    //item specific data of any type, take care
    void **extras;
    //opaque data set by user
    void **user;

    //////lists, values are indices of items with common properties
    uint16_t *hold_ratio; //items that don't change aspect ratio when scaling
    uint16_t *draw; //full list of items that are drawn
    uint16_t *redraw; //list of items needing redraw
    uint16_t* timer; //list of active timers

    //////global stuff
    float w0,h0;
    uint16_t nwidgets,tablesize;
    uint16_t drag;//index of widgets being dragged
    uint16_t ttip;//index of tooltip widget
    timer_lib_handle_t tlibh;
    PuglView* view;
    cairo_t* cr;
    uint8_t quit;

}tk_stuff;

typedef tk_stuff* tk_t;

typedef enum
{
    //NOTE that some of these values are not unique, because the properties only apply in certain contexts. Pay attention
    //Main Window Properties
    TK_HOLD_RATIO = 0x1,

    //Any Item Properties
    TK_NO_DAMAGE = 0x2,

    //Button Properties
    TK_BUTTON_MOMENTARY = 0x1,

    //Dial Properties
    TK_VALUE_PARABOLIC = 0x1,//TODO: what about for xy points?

    
}TK_PROPERTIES;

typedef struct
{
    float min, max;
    float x0, y0, v0;//drag initiation point
    char* units;
}tk_dial_stuff;

typedef struct
{
    uint16_t fontsize;

    FT_Library library;
    FT_Face face;
    cairo_font_face_t* fontface;
    cairo_scaled_font_t* scaledface;
}tk_font_stuff;

typedef struct
{
    char* str;//pointer to tip
    uint8_t strchange,cursorstate;
    uint16_t cursor;

    tk_font_stuff* tkf;
    cairo_glyph_t* glyphs;
    int glyph_count;
    cairo_text_cluster_t* clusters;
    int cluster_count;
    cairo_text_cluster_flags_t clusterflags;
}tk_text_stuff;


#endif
