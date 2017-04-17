//spencer jackson //tikloo toolkit

//tk_main.c

//this is approximately based on priciples of data-oriented design, but really doesn't have the performance benefits of DOD since everything is dynamically allocated anyway. The goal is that it will have the coding benefits though.

#include<stdlib.h>
#include<stdio.h>
#include<string.h>
#include<ctype.h>
#include<math.h>
#include"tk.h"
#include"tk_default_draw.h"
#include"tk_test.h"
#include"csleep.h"
#include"timer.h"



//forward declarations are all in tk_test or tk.h
//static void tk_callback (PuglView* view, const PuglEvent* event);
//void tk_nocallback(tk_t tk, const PuglEvent* e, uint16_t n);

tk_t tk_gimmeaTikloo(uint16_t w, uint16_t h, char* title)
{
    uint8_t starter_sz = 64;
    tk_t tk = (tk_t)malloc(sizeof(tk_stuff));

    //initialize the table in the struct
    tk->x = (float*)calloc(starter_sz,sizeof(float));
    tk->y = (float*)calloc(starter_sz,sizeof(float));
    tk->w = (float*)calloc(starter_sz,sizeof(float));
    tk->h = (float*)calloc(starter_sz,sizeof(float));

    tk->layer = (uint8_t*)calloc(starter_sz+1,sizeof(uint8_t)); 
    tk->value = (void**)calloc(starter_sz,sizeof(void*)); 
    tk->tip = (char**)calloc(starter_sz,sizeof(char*));
    tk->props = (uint16_t*)calloc(starter_sz,sizeof(uint16_t));
    tk->extras = (void**)calloc(starter_sz,sizeof(void*));
    tk->user = (void**)calloc(starter_sz,sizeof(void*));

    //lists always keep an extra 0 at the end so the end can be found even if full
    tk->hold_ratio = (uint16_t*)calloc(starter_sz+1,sizeof(float));
    tk->draw = (uint16_t*)calloc(starter_sz+1,sizeof(float));
    tk->redraw = (uint16_t*)calloc(starter_sz+1,sizeof(float));
    tk->timer = 0;//this list only gets initialized if there are timers

    tk->draw_f = (void(**)(cairo_t*,float,float,void*))calloc(starter_sz,sizeof(&tk_drawnothing));
    tk->cb_f = (void(**)(tk_t,const PuglEvent*,uint16_t))calloc(starter_sz,sizeof(&tk_callback));
    tk->callback_f = (void(**)(tk_t,const PuglEvent*,uint16_t))calloc(starter_sz,sizeof(&tk_callback));

    //now initialize the main window widget
    //TODO: check that everything is nz
    tk->w[0] = w;
    tk->h[0] = h;
    tk->w0 = w;
    tk->h0 = h;


    tk->layer[0] = 1;
    //we must do this so that we can draw the 0th widget, otherwise its an empty list;
    //rollit will fix TODO: so what about idle?
    tk->draw[0] = 0xffff;
    tk->draw[1] = 0;
    tk->redraw[0] = 0;
    tk->redraw[1] = 0;
    tk_setstring(&tk->tip[0],title);

    tk->cb_f[0] = tk_nocallback;
    tk->draw_f[0] = tk_drawbg;

    tk->drag = 0;
    tk->nwidgets = 1;
    tk->tablesize = starter_sz;
    tk->ttip = 0;
    tk->quit = 0;

    //start timer stuff
    timer_lib_initialize(&tk->tlibh);

    //start the pugl stuff 
    PuglView* view = puglInit(NULL, NULL);
    //puglInitWindowClass(view, tk->tip[0]);
    puglInitWindowSize(view, tk->w[0], tk->h[0]);
    //puglInitWindowMinSize(view, 256, 256);
    puglInitResizable(view, 1);
    
    puglIgnoreKeyRepeat(view, 0);
    puglSetEventFunc(view, tk_callback);
    puglSetHandle(view, tk);
    puglInitContextType(view, PUGL_CAIRO);//PUGL_CAIRO_GL
    tk->view = view; 
    
    //all set!
    puglCreateWindow(view, tk->tip[0]); 
    tk->cr = (cairo_t*)puglGetContext(tk->view);
    
    return (tk_t) tk;
}

void tk_cleanup(tk_t tk)
{
    uint16_t i;
    timer_lib_shutdown(tk->tlibh);
    for(i=0;tk->cb_f[i];i++)
    {
        if(tk->value[i])
            free(tk->value[i]);
        if(tk->tip[i])
            free(tk->tip[i]);
        if(tk->extras[i])
            free(tk->extras[i]);
        //we let the user free anything in user data 
    }
    //TODO: this is currently a sieve of memory
    free(tk->x); free(tk->y); free(tk->w); free(tk->h);
    free(tk->layer); free(tk->value); free(tk->tip);
    free(tk->props); free(tk->extras); free(tk->user);
    free(tk->draw_f); free(tk->cb_f); free(tk->callback_f);
    free(tk->hold_ratio);
    free(tk);
    puglDestroy(tk->view);
}

void tk_checktimers(tk_t tk)
{
    uint16_t i,n;
    float *nexttime,period, t = (float)timer_current_seconds(tk->tlibh);
    for(i=0;tk->timer[i];i++)
    {
        n = tk->timer[i];
        nexttime = (float*)tk->extras[n];
        if(t>=*nexttime)
        {
            tk->callback_f[n](tk,0,n);
            period = *(float*)tk->value[n];
            if(period)
            {
                *nexttime += period;//set timer for next tick
                if(*nexttime+period<t)
                    *nexttime = t+period;//we're way overdue, start again from now
            }
            else
                tk_removefromlist(tk->timer,tk->timer[i--]);//decrement since the current item was removed
        }
    } 
}

//for standalone apps
void tk_rollit(tk_t tk)
{ 
    PuglView* view = tk->view;

    puglShowWindow(view);
    tk->draw[0] = 0;//we faked a number here before so it wasn't an empty list

    if(tk->timer)
    {
        while (!tk->quit)
        {
            csleep(1);// these are crappy timers jsyk, we sleep for 1ms in between
            puglProcessEvents(view);
            tk_checktimers(tk);
            tk_redraw(tk);
        }
    }
    else
        while (!tk->quit)
        {
            //no timers
            puglWaitForEvent(view);
            puglProcessEvents(view);
            tk_redraw(tk);
        }
        
    tk_cleanup(tk);    
}

//for plugins
void tk_idle(tk_t tk)
{
//TODO: need to init timers
    puglProcessEvents(tk->view); 
    tk_checktimers(tk);
    tk_redraw(tk);
}

void tk_resizeeverything(tk_t tk,float w, float h)
{
    uint16_t i,n;
    float sx,sy,sx0,sy0,sx1,sy1,smx,smy,sm0,sm1,dx,dy;

    sx = w/(tk->w[0]);//scale change (relative)
    sy = h/(tk->h[0]);
    sx0 = (tk->w[0]+2*tk->x[0])/tk->w0;//old scaling (absolute)
    sy0 = (tk->h[0]+2*tk->y[0])/tk->h0;
    sx1 = w/tk->w0;//new scaling (absolute)
    sy1 = h/tk->h0;
    sm0 = sx0<sy0?sx0:sy0;//old small dim
    sm1 = sx1<sy1?sx1:sy1;//new small dim
    smx = (sm1/sm0)/sx;//min scale factor div. scale x
    smy = (sm1/sm0)/sy;

    if(tk->props[0]&TK_HOLD_RATIO)
    {
        if(sx<sy) sy = sx;
        else sx = sy;
        dx = tk->w0*(sx1-sm1)/2;//new window shift
        dy = tk->h0*(sy1-sm1)/2;
        tk->w[0] *= sx;
        tk->h[0] *= sy; 
    }
    else
    {
        dx = (1-smx)/2;//offset factor for individual items
        dy = (1-smy)/2;
        tk->w[0] = w;
        tk->h[0] = h;
        //prescale ones that hold ratio
        for(i=0;tk->hold_ratio[i];i++)
        {
            n = tk->hold_ratio[i];

            tk->x[n] += tk->w[n]*dx;//remove old offset, add new
            tk->y[n] += tk->h[n]*dy;
            tk->w[n] *= smx;
            tk->h[n] *= smy;
        }
        dx = 0;//window shift is actually 0
        dy = 0;
    } 

    //scale items
    for(i=1;tk->cb_f[i];i++)
    {
        tk->x[i] -= tk->x[0]; //remove old shift
        tk->y[i] -= tk->y[0];
        tk->x[i] *= sx;
        tk->y[i] *= sy;
        tk->w[i] *= sx;
        tk->h[i] *= sy;
        tk->x[i] += dx; //add new shift
        tk->y[i] += dy;
    } 

    //update window shift
    tk->x[0] = dx;
    tk->y[0] = dy;
}

void tk_draw(tk_t tk,uint16_t n)
{
    cairo_translate(tk->cr,tk->x[n],tk->y[n]);
    tk->draw_f[n](tk->cr,tk->w[n],tk->h[n],tk->value[n]); 
    cairo_translate(tk->cr,-tk->x[n],-tk->y[n]);
}
void tk_redraw(tk_t tk)
{
    uint16_t i;
    if( !tk->redraw[0] && !tk->redraw[1] )
        return;
    for(i=0; tk->redraw[i]||!i; i++)
    {
        tk_draw(tk,tk->redraw[i]);
        tk->redraw[i] = 0;
        //TODO: cache everything to avoid redraws?
    }
} 
void tk_draweverything(tk_t tk)
{
    uint16_t i;
    for(i=0; tk->draw[i]||!i; i++)
    {
        tk_draw(tk,tk->draw[i]);
        //TODO: cache everything to avoid redraws?
    }
}

uint16_t tk_dumbsearch(tk_t tk, const PuglEvent* event)
{
    uint16_t i,n=0,l=1;
    float x,y;
    switch (event->type) {
    case PUGL_BUTTON_PRESS:
    case PUGL_BUTTON_RELEASE:
        x = event->button.x;
        y = event->button.y;
        break;
    case PUGL_SCROLL:
        x = event->scroll.x;
        y = event->scroll.y;
        break;
    default:
        return 0;
    }
    for(i=1; tk->cb_f[i]; i++)
    {
        if( x >= tk->x[i] && x <= tk->x[i] + tk->w[i] &&
            y >= tk->y[i] && y <= tk->y[i] + tk->h[i] &&
            tk->layer[i] > l 
          )
        {
            l = tk->layer[i];
            n = i; 
        }
    }
    return n;
}

//primary callback for all events, sorts to appropriate widget
static void tk_callback (PuglView* view, const PuglEvent* event)
{ 
    uint16_t n;
    tk_t tk = (tk_t)puglGetHandle(view);
    switch (event->type) {
    case PUGL_NOTHING:
        break;
    case PUGL_CONFIGURE:
        if(event->configure.width == (tk->w[0]+2*tk->x[0]) &&
           event->configure.height == (tk->h[0]+2*tk->y[0]) )
           break;
        tk_resizeeverything(tk,event->configure.width,event->configure.height);
    case PUGL_EXPOSE:
        tk_draweverything(tk);
        break;
    case PUGL_CLOSE:
        tk->quit = 1;
        break;
    case PUGL_KEY_PRESS:
        fprintf(stderr, "Key %u (char %u) press (%s)%s\n",
                event->key.keycode, event->key.character, event->key.utf8,
                event->key.filter ? " (filtered)" : "");
        break;
    case PUGL_KEY_RELEASE:
        fprintf(stderr, "Key %u (char %u) release (%s)%s\n",
                event->key.keycode, event->key.character, event->key.utf8,
                event->key.filter ? " (filtered)" : "");
        break;
    case PUGL_MOTION_NOTIFY:
        if(tk->drag)
            tk->cb_f[tk->drag](tk,event,tk->drag);
        break;
    case PUGL_BUTTON_RELEASE:
        if(tk->drag)
        {
            tk->cb_f[tk->drag](tk,event,tk->drag);
            tk->drag = 0;
        }
        //no break
    case PUGL_BUTTON_PRESS:
        n = tk_dumbsearch(tk,event);
        if(n)
            tk->cb_f[n](tk,event,n);
        break;
    case PUGL_SCROLL:
        n = tk_dumbsearch(tk,event);
        if(n)
            tk->cb_f[n](tk,event,n);
        break;
    case PUGL_ENTER_NOTIFY:
        //fprintf(stderr, "Entered\n");
        break;
    case PUGL_LEAVE_NOTIFY:
        //fprintf(stderr, "Exited\n");
        break;
    case PUGL_FOCUS_IN:
        //fprintf(stderr, "Focus in\n");
        break;
    case PUGL_FOCUS_OUT:
        //fprintf(stderr, "Focus out\n");
        break;
    }
}

//SUNDRY HELPER FUNCTIONS

void tk_addtolist(uint16_t* list, uint16_t n)
{
    uint16_t i;
    for(i=0;list[i];i++)//find end of list
        if(list[i]==n)
            return;
    list[i] = n;
}

void tk_removefromlist(uint16_t* list, uint16_t n)
{
    uint16_t i;
    for(i=0;list[i]&&list[i]!=n;i++);//find item in list
    if(list[i]==n)
        for(;list[i];i++)
            list[i] = list[i+1]; 
}

void tk_insertinlist(uint16_t* list, uint16_t n, uint16_t i)
{
    uint16_t j,k;
    for(j=0;list[j]&&list[j]!=n;j++);//find end of list
    if(list[j]==n)
    {
        //it's already in the list once
        if(j<i)
            for(k=j;k<i;k++)
                list[k] = list[k+1];
        else if(j>i)
            for(k=j;k>i;k--)
                list[k] = list[k-1];
    }
    else
    {
        list[j+1] = 0;//caution, don't call this if you are at the max length already
        for(;j>i;j--)
            list[j] = list[j-1];
    }
    list[i] = n;
}

void tk_setstring(char** str, char* msg)
{
    
    if( *str )
        free(*str);
    *str = (char*)calloc(strlen(msg),sizeof(char));
    strcpy(*str,msg);
}

void tk_changelayer(tk_t tk, uint16_t n, uint16_t layer)
{
    uint16_t i;
    if(!layer)
    {
        tk_removefromlist(tk->draw,n);
        tk_removefromlist(tk->redraw,n);
    }
    else
    {
        for(i=0;tk->layer[tk->draw[i]]<layer+1&&tk->draw[i];i++);//find end of others on same layer
        tk_insertinlist(tk->draw,n,i);
        for(i=0;tk->layer[tk->redraw[i]]<layer+1&&tk->redraw[i];i++);//find end of layer
        tk_insertinlist(tk->redraw,n,i);
    }
    tk->layer[n] = layer;
}

//WIDGET STUFF
void tk_nocallback(tk_t tk, const PuglEvent* e, uint16_t n)
{(void)tk;(void)e;(void)n;}

uint16_t tk_gimmeaWidget(tk_t tk, uint16_t x, uint16_t y, uint16_t w, uint16_t h)
{ 
    uint16_t n = tk->nwidgets++;
    tk->x[n] = x;
    tk->y[n] = y;
    tk->w[n] = w;
    tk->h[n] = h;
    tk->layer[n] = 2;
    tk_addtolist(tk->draw,n);
    tk->draw_f[n] = tk_drawnothing;
    tk->cb_f[n] = tk_nocallback;
    tk->callback_f[n] = tk_nocallback;
    return n;
}

uint16_t tk_gimmeaDecoration(tk_t tk, uint16_t x, uint16_t y, uint16_t w, uint16_t h)
{
    uint16_t n = tk->nwidgets;

    tk_gimmeaWidget(tk,x,y,w,h);
    tk_addtolist(tk->hold_ratio,n); 
    return n; 
}

float tk_dialValue(tk_t tk, uint16_t n)
{
    float *v = (float*)tk->value[n];
    tk_dial_stuff* tkd = (tk_dial_stuff*)tk->extras[n];
    if(tk->props[n]&TK_VALUE_PARABOLIC)
        return *v*(tkd->max-tkd->min)+ tkd->min;
    else
        return *v**v*(tkd->max-tkd->min)+ tkd->min; 
}

void tk_dialcallback(tk_t tk, const PuglEvent* event, uint16_t n)
{
    float s = tk->w[n],*v = (float*)tk->value[n];
    tk_dial_stuff* tkd = (tk_dial_stuff*)tk->extras[n];
    switch (event->type) {
    case PUGL_MOTION_NOTIFY:
        if(s < tk->h[n]) s= tk->h[n]; //here we make the assumptions dials will usually be approximately round (not slider shaped)
        *v = tkd->v0 +
             (event->motion.x - tkd->x0)/(30.f*s) + 
             (tkd->y0 - event->motion.y)/(3.f*s);
        if(*v > 1) *v = 1;
        if(*v < 0) *v = 0;
        //fprintf(stderr, "%f ",*v);
        tk->callback_f[n](tk,event,n);
        tk_addtolist(tk->redraw,n);//TODO: damage
        break;
    case PUGL_BUTTON_PRESS:
        tk->drag = n;
        tkd->x0 = event->button.x;
        tkd->y0 = event->button.y;
        tkd->v0 = *v;
        break;
    case PUGL_BUTTON_RELEASE:
        tkd->x0 = 0;
        tkd->y0 = 0;
        break;
    case PUGL_SCROLL:
        //TODO: scroll
        *v += event->scroll.dx/(30.f*s)+ event->scroll.dy/(3.f*s);
        if(*v > 1) *v = 1;
        if(*v < 0) *v = 0;
        //fprintf(stderr, "%f ",*v);
        tk->callback_f[n](tk,event,n);
        tk_addtolist(tk->redraw,n);
        break;
    default:
        break;
    }
}


uint16_t tk_gimmeaDial(tk_t tk, uint16_t x, uint16_t y, uint16_t w, uint16_t h, float min, float max, float val)
{
    uint16_t n = tk->nwidgets;
    tk_dial_stuff* tkd = (tk_dial_stuff*)malloc(sizeof(tk_dial_stuff));

    tk_gimmeaWidget(tk,x,y,w,h);
    tk->extras[n] = (void*)tkd;
    tkd->min = min;
    tkd->max = max;
    tk->value[n] = (void*)malloc(sizeof(float));
    *(float*)tk->value[n] = (val-min)/(max-min); 

    tk->draw_f[n] = tk_drawdial;//default
    tk_addtolist(tk->hold_ratio,n);

    tk->cb_f[n] = tk_dialcallback;
    return n;

}

void tk_buttoncallback(tk_t tk, const PuglEvent* event, uint16_t n)
{
    uint8_t *v = (uint8_t*)tk->value[n];
    switch (event->type) {
    case PUGL_MOTION_NOTIFY:
        if( tk->props[n]&TK_BUTTON_MOMENTARY &&
            (event->motion.x < tk->x[n] || event->motion.x > tk->x[n] + tk->w[n] ||
            event->motion.y < tk->y[n] || event->motion.y > tk->y[n] + tk->h[n])
          )
        {
            //click has left the widget
            tk->drag = 0;
            *v ^= 0x01;
            tk->callback_f[n](tk,event,n);
            tk_addtolist(tk->redraw,n);
        }
        break;
    case PUGL_BUTTON_PRESS:
        //TODO: decide if being dragged
        tk->drag = n;
        if(tk->props[n]&TK_BUTTON_MOMENTARY)
        {
            *v ^= 0x01;
            tk->callback_f[n](tk,event,n);
            tk_addtolist(tk->redraw,n);
        }
        break;
    case PUGL_BUTTON_RELEASE:
        if(tk->drag == n &&
           (event->button.x >= tk->x[n] && event->button.x <= tk->x[n] + tk->w[n] &&
            event->button.y >= tk->y[n] && event->button.y <= tk->y[n] + tk->h[n])
          )
        {
            *v ^= 0x01;
            tk->callback_f[n](tk,event,n);
            tk_addtolist(tk->redraw,n);
        }
        break;
    default:
        break;
    }
}

uint16_t tk_gimmeaButton(tk_t tk, uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint8_t val)
{
    uint16_t n = tk->nwidgets;

    tk_gimmeaWidget(tk,x,y,w,h);
    tk->value[n] = (void*)malloc(sizeof(uint8_t));
    *(uint8_t*)tk->value[n] = val&0x1;

    tk->draw_f[n] = tk_drawbutton;//default

    tk->cb_f[n] = tk_buttoncallback;
    return n; 
}


//timers are an entirely different beast from other widgets
void tk_settimer(tk_t tk, uint16_t n, float s)
{
    *(float*)tk->value[n] = s;
    *(float*)tk->extras[n] = (float)timer_current_seconds(tk->tlibh)+s;
    if(s)
        tk_addtolist(tk->timer,n);
    else
        tk_removefromlist(tk->timer,n);
}

uint16_t tk_gimmeaTimer(tk_t tk, float s)
{
    //may want to make this actually off the window
    uint16_t n = tk->nwidgets; 
    tk->x[n] = 0;
    tk->y[n] = 0;
    tk->w[n] = 0;
    tk->h[n] = 0;
    tk->layer[n] = 0;
    tk->draw_f[n] = tk_drawnothing;
    tk->cb_f[n] = tk_nocallback;
    tk->callback_f[n] = tk_nocallback;
    tk->value[n] = malloc(sizeof(float));
    tk->extras[n] = malloc(sizeof(float));
    if(!tk->timer)
        tk->timer = (uint16_t*)calloc(tk->tablesize,sizeof(uint16_t));//probably won't need this many, manually allocate this if you want to use a little less memory
    tk_settimer(tk,n,s);
    return n;
}

//this function just makes the font stuff
tk_font_stuff* tk_gimmeaFont(tk_t tk, char* fontpath, uint16_t h) 
{
    int fontsize = h;
    tk_font_stuff* tkf = (tk_font_stuff*)malloc(sizeof(tk_font_stuff));

    //freetype stuff 
    FT_Library  library;   /* handle to library     */
    FT_Face     face;      /* handle to face object */
    FT_Error    error;
    //cairo stuff
    cairo_font_face_t* fontface;
    cairo_scaled_font_t* scaledface; 


    //now font setup stuff 
    error = FT_Init_FreeType( &library );
    if ( error ) 
    { 
        fprintf(stderr, "OH NO, Freetype init problem!");
        free(tkf);
        return 0;
    }

    error = FT_New_Face( library,
         fontpath,
         0,
         &face );
    if ( error == FT_Err_Unknown_File_Format )
    {
      //... the font file could be opened and read, but it appears
      //  ... that its font format is unsupported
        fprintf(stderr, "OH NO, Font problem!");
        free(tkf);
        return 0;
    }
    else if ( error )
    {
          //... another error code means that the font file could not
          //  ... be opened or read, or that it is broken...
        fprintf(stderr, "OH NO, Font not found!");
        free(tkf);
        return 0;
    } 

    // get glyphs for the text
    // get the scaled font object
    fontface = cairo_font_face_reference(cairo_ft_font_face_create_for_ft_face(face,0));
    cairo_set_font_face(tk->cr, fontface);
    cairo_set_font_size(tk->cr, fontsize);
    scaledface = cairo_scaled_font_reference(cairo_get_scaled_font(tk->cr));

    tkf->library = library;
    tkf->face = face;
    tkf->fontsize = fontsize;
    tkf->fontface = fontface;
    tkf->scaledface = scaledface;

    return tkf; 
}

//TODO: add Nlines to decide size of font
//TODO: autodetect width?
uint16_t tk_gimmeaText(tk_t tk, uint16_t x, uint16_t y, uint16_t w, uint16_t h, tk_font_stuff* font, char* str)
{
    uint16_t n = tk->nwidgets; 
    tk_text_stuff* tkt = (tk_text_stuff*)calloc(1,sizeof(tk_text_stuff));

    //cairo stuff
    cairo_glyph_t* glyphs = NULL;
    int glyph_count = 0;
    cairo_text_cluster_t* clusters = NULL;
    int cluster_count = 0;
    cairo_text_cluster_flags_t clusterflags;
    cairo_status_t stat;

    tk_gimmeaWidget(tk,x,y,w,h);

    tk_setstring(&tk->tip[n],str);
    tkt->str = tk->tip[n];
    tk_addtolist(tk->hold_ratio,n);


    // get glyphs for the text
    stat = cairo_scaled_font_text_to_glyphs(font->scaledface, 0, 0, 
                    str, strlen(str), 
                    &glyphs, &glyph_count, 
                    &clusters, &cluster_count, &clusterflags);

    tkt->tkf = font;
    tkt->glyphs = glyphs;
    tkt->glyph_count = glyph_count;
    tkt->clusters = clusters;
    tkt->cluster_count = cluster_count;

    // check if conversion was successful
    if (stat != CAIRO_STATUS_SUCCESS) 
    {
        //TODO: not sure if this is cause for abort
        fprintf(stderr, "OH NO, Text conversion failed!");
    }

    //TODO: cleanup properly glyphs and clusters, others?

    tk->draw_f[n] = tk_drawtext;
    tk->value[n] = tkt;

    return n; } 
uint16_t tk_gimmeaTooltip(tk_t tk, tk_font_stuff* font)
{
    //need text, timer
    tk_text_stuff* tkt = (tk_text_stuff*)calloc(1,sizeof(tk_text_stuff));

    uint16_t n = tk->nwidgets; 
    tk->x[n] = 0;
    tk->y[n] = 0;
    tk->w[n] = 0;
    tk->h[n] = 0;
    tk->layer[n] = 0;

    tk_addtolist(tk->hold_ratio,n);//?

    tkt->tkf = font;

    tk->draw_f[n] = tk_drawtip;
    tk->value[n] = tkt;
}

//we assume there is a valid font with size and a string, we may change line brks
// fontsize 0 will autoscale to fit
//we will pass back the actual dimensions in w and h, and 
//return  1 if the text fits in the provided size
uint8_t tk_textlayout(tk_text_stuff* tkt, uint16_t *w, uint16_t *h)
{
    //TODO: get CR in here
    uint8_t raster;
    uint16_t i,size,glyph_index,byte_index,x,y,ln,deltax;

    cairo_scaled_font_t* scaled_face = tkt->tkf->scaledface;
    cairo_glyph_t* glyphs = tkt->glyphs;
    int glyph_count = tkt->glyph_count;
    cairo_text_cluster_t* clusters = tkt->clusters;
    int cluster_count = tkt->cluster_count;
    cairo_text_cluster_flags_t clusterflags = tkt->clusterflags;
    cairo_status_t stat;
    cairo_text_extents_t extents;

    if(tkt->nlines>=1)
    {
        size = (*h/tkt->nlines)*.86;
        if(size != tkt->tkf->fontsize)
        {
            cairo_set_font_face(cr, tkt->tkf->fontface);
            tkt->tkf->fontsize = size; 
            cairo_set_font_size(cr, h); 
            cairo_scaled_font_destroy(tkt->tkf->scaledface);
            tkt->tkf->scaledface = cairo_scaled_font_reference(cairo_get_scaled_font(cr));
            raster = 1;
        }
    }
    //TODO: else autosize, for now assume size is fixed
    if(raster)
    {
        stat = cairo_scaled_font_text_to_glyphs(scaled_face, 0, 0, tkt->str, strlen(tkt->str), 
                                                &glyphs, &glyph_count, 
                                                &clusters, &cluster_count,
                                                &clusterflags); 
        if (stat == CAIRO_STATUS_SUCCESS)
            tkt->strchange = 0;
        //else return 0;
        //TODO: cleanup old buffers
    }

    x = y = 0;
    glyph_index = byte_index = 0;
    for (i = 0; i < cluster_count; i++) 
    { 
        // get extents for the glyphs in the cluster
        cairo_scaled_font_glyph_extents(scaled_face, &glyphs[glyph_index], clusters[i].num_glyphs, &extents);

        if(isspace(tkt->str[byte_index]))
        { 
            deltax = 0;
        }
	    //if (cluster->num_bytes == 1 && text[byte_pos] == '\n') //newline

        if(x + extents.x_advance > *w)
            continue;
        else
        {
            x += extents.x_advance;
            deltax += extents.x_advance;
            glyph_index += clusters[i].num_glyphs;
            byte_index += clusters[i].num_bytes;
        }
    }
}
