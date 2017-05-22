//spencer jackson //tikloo toolkit

//tk_main.c

//this is approximately based on priciples of data-oriented design, but I'm not really shooting for the performance benefits of DOD. The goal is that it will have the coding benefits: powerful, flexible, simple to extend/tweak, and fewer LOC.

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


#ifndef TK_TOOLTIP_TIME
#define TK_TOOLTIP_TIME 1
#endif
#ifndef TK_STARTER_SIZE
#define TK_STARTER_SIZE 64
#endif
#ifndef TK_TEXT_STARTER_SIZE
#define TK_TEXT_STARTER_SIZE 16
#endif

//forward declarations are all in tk_test or tk.h
//static void tk_callback (PuglView* view, const PuglEvent* event);
//void tk_nocallback(tk_t tk, const PuglEvent* e, uint16_t n);

void tk_growprimarytable(tk_t tk)
{
    uint8_t osz,sz = TK_STARTER_SIZE;
    tk_table tmpt;
    if(tk->tablesize)
        sz = tk->tablesize*2;
    //initialize the table in the struct
    tmpt.x = (float*)calloc(sz,sizeof(float));
    tmpt.y = (float*)calloc(sz,sizeof(float));
    tmpt.w = (float*)calloc(sz,sizeof(float));
    tmpt.h = (float*)calloc(sz,sizeof(float));

    tmpt.layer =  (uint8_t*)calloc(sz+1,sizeof(uint8_t)); 
    tmpt.value =    (void**)calloc(sz,sizeof(void*)); 
    tmpt.tip =      (char**)calloc(sz,sizeof(char*));
    tmpt.props = (uint16_t*)calloc(sz,sizeof(uint16_t));
    tmpt.extras =   (void**)calloc(sz,sizeof(void*));
    tmpt.user =     (void**)calloc(sz,sizeof(void*));

    //init the lists
    //lists always keep an extra 0 at the end so the end can be found even if full
    tmpt.hold_ratio = (uint16_t*)calloc(sz+1,sizeof(float));
    tmpt.draw =       (uint16_t*)calloc(sz+1,sizeof(float));
    tmpt.redraw =     (uint16_t*)calloc(sz+1,sizeof(float));

    tmpt.draw_f = (void(**)(cairo_t*,float,float,void*))calloc(sz,sizeof(&tk_drawnothing));
    tmpt.cb_f = (void(**)(tk_t,const PuglEvent*,uint16_t))calloc(sz,sizeof(&tk_callback));
    tmpt.callback_f = (void(**)(tk_t,const PuglEvent*,uint16_t))calloc(sz,sizeof(&tk_callback));

    if(tk->tablesize)
    {
        osz = tk->tablesize;
        memcpy(tmpt.x,      tk->x,      osz*sizeof(float));
        memcpy(tmpt.y,      tk->y,      osz*sizeof(float));
        memcpy(tmpt.w,      tk->w,      osz*sizeof(float));
        memcpy(tmpt.h,      tk->h,      osz*sizeof(float));
        memcpy(tmpt.layer,  tk->layer,  osz*sizeof(uint8_t));
        memcpy(tmpt.value,  tk->value,  osz*sizeof(void*));
        memcpy(tmpt.tip,    tk->tip,    osz*sizeof(char*));
        memcpy(tmpt.props,  tk->props,  osz*sizeof(uint16_t));
        memcpy(tmpt.extras, tk->extras, osz*sizeof(void*));
        memcpy(tmpt.user,   tk->user,   osz*sizeof(void*));
        
        memcpy(tmpt.hold_ratio,tk->hold_ratio,osz*sizeof(uint16_t)+1);
        memcpy(tmpt.draw,      tk->draw,      osz*sizeof(uint16_t)+1);
        memcpy(tmpt.redraw,    tk->redraw,    osz*sizeof(uint16_t)+1);

        memcpy(tmpt.draw_f,    tk->draw_f,    osz*sizeof(void(*)(cairo_t*,float,float,void*)));
        memcpy(tmpt.cb_f,      tk->cb_f,      osz*sizeof(void(*)(tk_t,PuglEvent*,uint16_t)));
        memcpy(tmpt.callback_f,tk->callback_f,osz*sizeof(void(*)(tk_t,PuglEvent*,uint16_t)));
    }

    tk->x =      tmpt.x;
    tk->y =      tmpt.y;
    tk->w =      tmpt.w;
    tk->h =      tmpt.h;
    tk->layer =  tmpt.layer;
    tk->value =  tmpt.value;
    tk->tip =    tmpt.tip;
    tk->props =  tmpt.props;
    tk->extras = tmpt.extras;
    tk->user =   tmpt.user;

    tk->hold_ratio = tmpt.hold_ratio;
    tk->draw =       tmpt.draw;
    tk->redraw =     tmpt.redraw; 
    tk->draw_f =     tmpt.draw_f;
    tk->cb_f =       tmpt.cb_f;
    tk->callback_f = tmpt.callback_f;
}

tk_t tk_gimmeaTikloo(uint16_t w, uint16_t h, char* title)
{
    tk_t tk = (tk_t)malloc(sizeof(tk_table));

    tk->tablesize = 0;
    tk_growprimarytable(tk);

    //init the text table to len 0
    tk->tkt.str = 0;
    tk->tkt.strchange = 0;
    tk->tkt.n = 0;
    tk->tkt.cursor = 0;
    tk->tkt.select = 0;
    tk->tkt.ln = 0;
    tk->tkt.col = 0;
    tk->tkt.brklen = 0;
    tk->tkt.brk = 0;
    tk->tkt.scale = 0;

    tk->tkt.tkf = 0;
    tk->tkt.glyphs = 0;
    tk->tkt.glyph_count = 0;
    tk->tkt.clusters = 0;
    tk->tkt.cluster_count = 0;

    tk->tkt.cursorstate = 0;
    tk->tkt.nitems = 0;
    tk->tkt.tablesize = 0;

    tk->timer = 0;//this list only gets initialized if there are timers

    tk->tkt.nitems = 0;
    tk->tkt.tablesize = 0;

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
    tk->nitems = 1;
    tk->tablesize = TK_STARTER_SIZE;
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
    for(i=0;tk->timer && tk->timer[i];i++)
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
    puglProcessEvents(tk->view); 
    tk_checktimers(tk);
    tk_redraw(tk);
}

void tk_resizeeverything(tk_t tk,float w, float h)
{
    uint16_t i,n,tw,th;
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

    //scale text
    tk->tkt.scale = sm1;
    for(i=0;i<tk->tkt.nitems;i++)
    {
        n = tk->tkt.n[i];
        tw = tk->w[n];
        th = tk->h[n];
        //TODO: unless they've changed ratio they don't actually need a re-layout
        //TODO: do anything if it doesn't fit?
        tk_textlayout(tk->cr,&tk->tkt,i,&tw,&th,tk->props[n]&TK_TEXT_WRAP);
        
    }
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
        return;//empty list
    //TODO: if we have to redraw the bg, we probably have to draweverything anyway, no?
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
void tk_damage(tk_t tk, uint16_t n)
{
    uint16_t i,l,lmx;
    uint16_t x,y,w,h,x2,y2;
    x = tk->x[n];
    y = tk->y[n];
    w = tk->w[n];
    h = tk->h[n];
    lmx = tk->layer[n];
    x2 = x--+w+1;
    y2 = y--+h+1;

    //set up clip area
    cairo_new_path(tk->cr);
    cairo_move_to(tk->cr, x,y);
    cairo_line_to(tk->cr, x2, y);
    cairo_line_to(tk->cr, x2, y2);
    cairo_line_to(tk->cr, x, y2);
    cairo_close_path(tk->cr);
    cairo_clip(tk->cr);
        
    for(l=1;l<=lmx;l++)
    {
        for(i=0; tk->cb_f[i]; i++)
        {
            if( (tk->x[i] < x2 || tk->x[i] + tk->w[i] > x) &&
                (tk->y[i] < y2 || tk->y[i] + tk->h[i] > y) &&
                tk->layer[i] == l )
            {
                tk_draw(tk,i);
            }
        }
    }

    cairo_reset_clip(tk->cr);
}

uint16_t tk_eventsearch(tk_t tk, const PuglEvent* event)
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
    case PUGL_MOTION_NOTIFY:
        x = event->motion.x;
        y = event->motion.y;
        break;
    default:
        return 0;
    }
    for(i=1; tk->cb_f[i]; i++)
    {
        if( x >= tk->x[i] && x <= tk->x[i] + tk->w[i] &&
            y >= tk->y[i] && y <= tk->y[i] + tk->h[i] &&
            tk->layer[i] > l )
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
        else 
        {//tooltip
            n=tk_eventsearch(tk,event);
            tk->tover = n;
            if(tk->ttip)
            {//ttip enabled
                if(n && tk->tip[n] && strlen(tk->tip[n]))//mouse is over a widget with a tip   
                    tk_settimer(tk,tk->ttip,TK_TOOLTIP_TIME);
                else
                {
                    tk_settimer(tk,tk->ttip,0);
                    if(tk->layer[tk->ttip-1])
                        tk_changelayer(tk,tk->ttip-1,0);
                }
            }
        }
        break;
    case PUGL_BUTTON_RELEASE:
        if(tk->drag)
        {
            tk->cb_f[tk->drag](tk,event,tk->drag);
            tk->drag = 0;
        }
        //no break
    case PUGL_BUTTON_PRESS:
        n = tk_eventsearch(tk,event);
        if(n)
            tk->cb_f[n](tk,event,n);
        break;
    case PUGL_SCROLL:
        n = tk_eventsearch(tk,event);
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
void tk_addtogrowlist(uint16_t** list, uint16_t *len, uint16_t n)
{
    uint16_t i=0;
    uint16_t *newlist;
    if(!*list || !*len)
        *list = (uint16_t*)calloc(sizeof(uint16_t),10);
    else if(list[*len-1])
    {//list is full
        newlist = (uint16_t*)calloc(sizeof(uint16_t),2**len);
        memcpy(newlist,*list,sizeof(uint16_t)**len);
        free(*list);
        *list = newlist;
        i = *len;
        *len *=2;
    }
    else
    {
        for(i=0;*list[i];i++)//find end of list
            if(*list[i]==n)
                return;
    }
    *list[i] = n;
}

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

//insert n in position i
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
        tk_damage(tk,n); 
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

uint16_t tk_addaWidget(tk_t tk, uint16_t x, uint16_t y, uint16_t w, uint16_t h)
{ 
    uint16_t n = tk->nitems++;
    if (tk->nitems == tk->tablesize)
        tk_growprimarytable(tk);
    tk->x[n] = x;
    tk->y[n] = y;
    tk->w[n] = w;
    tk->h[n] = h;
    tk->layer[n] = 2;//bg is layer 0
    tk_addtolist(tk->draw,n);
    tk->draw_f[n] = tk_drawnothing;
    tk->cb_f[n] = tk_nocallback;
    tk->callback_f[n] = tk_nocallback;
    return n;
}

uint16_t tk_addaDecoration(tk_t tk, uint16_t x, uint16_t y, uint16_t w, uint16_t h)
{
    uint16_t n = tk->nitems;

    tk_addaWidget(tk,x,y,w,h);
    tk->layer[n] = 1;
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
        if(s < tk->h[n]) s= tk->h[n]; //here we make the assumption dials will usually be approximately round (not slider shaped)
        *v = tkd->v0 +
             (event->motion.x - tkd->x0)/(30.f*s) + 
             (tkd->y0 - event->motion.y)/(3.f*s);
        if(*v > 1) *v = 1;
        if(*v < 0) *v = 0;
        //fprintf(stderr, "%f ",*v);
        tk->callback_f[n](tk,event,n);
        tk_addtolist(tk->redraw,n);
        if(!tk->props[n]&TK_NO_DAMAGE)
            tk_damage(tk,n);
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
        if(!tk->props[n]&TK_NO_DAMAGE)
            tk_damage(tk,n); 
        break;
    default:
        break;
    }
}


uint16_t tk_addaDial(tk_t tk, uint16_t x, uint16_t y, uint16_t w, uint16_t h, float min, float max, float val)
{
    uint16_t n = tk->nitems;
    tk_dial_stuff* tkd = (tk_dial_stuff*)malloc(sizeof(tk_dial_stuff));

    tk_addaWidget(tk,x,y,w,h);
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
            if(!tk->props[n]&TK_NO_DAMAGE)
                tk_damage(tk,n);
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
            if(!tk->props[n]&TK_NO_DAMAGE)
                tk_damage(tk,n);
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
            if(!tk->props[n]&TK_NO_DAMAGE)
                tk_damage(tk,n);
        }
        break;
    default:
        break;
    }
}

uint16_t tk_addaButton(tk_t tk, uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint8_t val)
{
    uint16_t n = tk->nitems;

    tk_addaWidget(tk,x,y,w,h);
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

uint16_t tk_addaTimer(tk_t tk, float s)
{
    //may want to make this actually off the window
    uint16_t n = tk->nitems++; 
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

//we assume there is a valid font with set size and a string, we will set line brks 
//we will pass back the actual dimensions in w and h, and 
//return  1 if the text fits in the provided size
uint8_t tk_textlayout(cairo_t* cr, tk_text_table* tkt, uint16_t n, uint16_t *w, uint16_t *h, uint8_t wrap)
{
    uint8_t fit;
    uint16_t i,size,glyph_index,str_index;
    uint16_t x,y,lastwhite,deltax,xmax;

    cairo_scaled_font_t* scaled_face = tkt->tkf[n]->scaledface;
    cairo_glyph_t* glyphs = tkt->glyphs[n];
    int glyph_count = tkt->glyph_count[n];
    cairo_text_cluster_t* clusters = tkt->clusters[n]; int cluster_count = tkt->cluster_count[n];
    cairo_text_extents_t* extents = tkt->extents[n];
    int extents_count = tkt->extents_count[n];
    cairo_text_cluster_flags_t clusterflags;
    cairo_status_t stat;

    *w /= tkt->scale;
    *h /= tkt->scale;

#if 0
//we don't resize here (anymore)
    vlines = tkt->vlines;
    if(vlines>=1)
    {
        size = tkt->scale*(tkt->tkf->fontsize)*.86;
        space = *h/tkt->vlines - size;
        if(size != tkt->tkf->fontsize)
        {
            cairo_set_font_face(cr, tkt->tkf->fontface);
            tkt->tkf->fontsize = size; 
            cairo_set_font_size(cr, size); 
            cairo_scaled_font_destroy(tkt->tkf->scaledface);
            tkt->tkf->scaledface = cairo_scaled_font_reference(cairo_get_scaled_font(cr));
            tkt->strchange = 1;
        }
    }
    else if(*h < tkt->tkf->fontsize*tkt->scale)
    {//it doesn't fit
        return 1;
    }
    else
    {
        size = tkt->tkf->fontsize;
        space = .14*size;
        vlines = (*h-space)/(size+space);
        //TODO: autosize, for now assume size is fixed
    }
#endif

//TODO: we need to still calculate size and space 
    size = tkt->tkf[n]->fontsize;
    if(tkt->strchange[n])
    {
        stat = cairo_scaled_font_text_to_glyphs(scaled_face, 0, 0, tkt->str[n], strlen(tkt->str[n]), 
                                                &glyphs, &glyph_count, 
                                                &clusters, &cluster_count,
                                                &clusterflags); 
        if (stat == CAIRO_STATUS_SUCCESS)
            tkt->strchange[n] = 0;
        //else return 0;
        //TODO: cleanup old buffers
        if(glyphs != tkt->glyphs[n])
            free(tkt->glyphs[n]);
        if(clusters != tkt->clusters[n])
            free(tkt->clusters[n]);
        if(cluster_count > extents_count)
        {
            free(tkt->extents[n]);
            extents = (cairo_text_extents_t*)calloc(cluster_count,sizeof(cairo_text_extents_t)); 
            extents_count = cluster_count;
        }
    }
    tkt->brk[n][0] = 0; //clear list

    x = xmax = 0;
    y = size;
    glyph_index = str_index = 0;
    for (i = 0; i < cluster_count; i++) 
    { 
        // get extents for the glyphs in the cluster
        cairo_scaled_font_glyph_extents(scaled_face, &glyphs[glyph_index], clusters[i].num_glyphs, &extents[i]);
        glyph_index += clusters[i].num_glyphs;

        if(clusters[i].num_bytes == 1 && isspace(tkt->str[n][str_index]))
        { 
            deltax = 0;
            lastwhite = str_index;
            if (tkt->str[n][str_index] == '\n') //newline
            {
                tk_addtogrowlist(&tkt->brk[n],&tkt->brklen[n],str_index);
                y += size;
            }
        }
        else 
            deltax += extents[i].x_advance;

        str_index += clusters[i].num_bytes;

        if(wrap && x + extents[i].x_advance > *w)
        {
            //go back to last whitespace put the rest on a newline
            if(deltax == x)
                //single word doesn't fit on a line
                x = extents[i].x_advance;
            else
                x = deltax;
            tk_addtogrowlist(&tkt->brk[n],&tkt->brklen[n],lastwhite);
            y += size;
        }
        else
        {
            x += extents[i].x_advance;
            if(x > xmax)
                xmax = x;
        }
    }

    tkt->glyphs[n] = glyphs;
    tkt->clusters[n] = clusters;
    tkt->extents[n] = extents;
    tkt->glyph_count[n] = glyph_count;
    tkt->cluster_count[n] = cluster_count;
    tkt->extents_count[n] = extents_count;

    fit = 1;
    if(xmax > *w)
        fit = 0;
    if(y > *h)
        fit = 0; 

    *w = xmax*tkt->scale;
    *h = y*tkt->scale;
    return fit;
}

void tk_growtexttable(tk_text_table* tkt)
{
    uint8_t osz,sz = TK_TEXT_STARTER_SIZE;
    tk_text_table tmpt;
    if(tkt->tablesize)
        sz = 2*tkt->tablesize;
    tmpt.str =         (char**)calloc(sz,sizeof(char*));
    tmpt.strchange = (uint8_t*)calloc(sz,sizeof(uint8_t));
    tmpt.n =        (uint16_t*)calloc(sz,sizeof(uint16_t));
    tmpt.cursor =   (uint16_t*)calloc(sz,sizeof(uint16_t));
    tmpt.select =   (uint16_t*)calloc(sz,sizeof(uint16_t));
    tmpt.ln =       (uint16_t*)calloc(sz,sizeof(uint16_t));
    tmpt.col =      (uint16_t*)calloc(sz,sizeof(uint16_t));
    tmpt.brklen =   (uint16_t*)calloc(sz,sizeof(uint16_t));
    tmpt.brk =     (uint16_t**)calloc(sz,sizeof(uint16_t*));

    tmpt.tkf =    (tk_font_stuff**)calloc(sz,sizeof(tk_font_stuff*));
    tmpt.glyphs = (cairo_glyph_t**)calloc(sz,sizeof(cairo_glyph_t*));
    tmpt.clusters = (cairo_text_cluster_t**)calloc(sz,sizeof(cairo_text_cluster_t*));
    tmpt.extents = (cairo_text_extents_t**)calloc(sz,sizeof(cairo_text_extents_t*));
    tmpt.glyph_count =  (uint16_t*)calloc(sz,sizeof(uint16_t));
    tmpt.cluster_count = (uint16_t*)calloc(sz,sizeof(uint16_t));
    tmpt.extents_count = (uint16_t*)calloc(sz,sizeof(uint16_t));

    if(tkt->tablesize)
    {
        osz = tkt->tablesize;
        memcpy(tmpt.str,      tkt->str,      osz*sizeof(char*));
        memcpy(tmpt.strchange,tkt->strchange,osz*sizeof(uint8_t));
        memcpy(tmpt.n,        tkt->n,        osz*sizeof(uint16_t));
        memcpy(tmpt.cursor,   tkt->cursor,   osz*sizeof(uint16_t));
        memcpy(tmpt.select,   tkt->select,   osz*sizeof(uint16_t));
        memcpy(tmpt.ln,       tkt->ln,       osz*sizeof(uint16_t));
        memcpy(tmpt.col,      tkt->col,      osz*sizeof(uint16_t));
        memcpy(tmpt.brklen,   tkt->brklen,   osz*sizeof(uint16_t));
        memcpy(tmpt.brk,      tkt->brk,      osz*sizeof(uint16_t*));

        memcpy(tmpt.tkf,      tkt->tkf,      osz*sizeof(tk_font_stuff*));
        memcpy(tmpt.glyphs,   tkt->glyphs,   osz*sizeof(cairo_glyph_t*));
        memcpy(tmpt.clusters, tkt->clusters, osz*sizeof(cairo_text_cluster_t*));
        memcpy(tmpt.extents,  tkt->extents,  osz*sizeof(cairo_text_cluster_t*));
        memcpy(tmpt.glyph_count,  tkt->glyph_count,  osz*sizeof(uint16_t));
        memcpy(tmpt.cluster_count,tkt->cluster_count,osz*sizeof(uint16_t));
        memcpy(tmpt.extents_count,tkt->extents_count,osz*sizeof(uint16_t));
    }

    tkt->str = tmpt.str;
    tkt->strchange = tmpt.strchange;
    tkt->n = tmpt.n;
    tkt->cursor = tmpt.cursor;
    tkt->select = tmpt.select;
    tkt->ln = tmpt.ln;
    tkt->col = tmpt.col;
    tkt->brklen = tmpt.brklen;
    tkt->brk = tmpt.brk;

    tkt->tkf = tmpt.tkf;
    tkt->glyphs = tmpt.glyphs;
    tkt->clusters = tmpt.clusters;
    tkt->extents = tmpt.extents;
    tkt->glyph_count = tmpt.glyph_count;
    tkt->cluster_count = tmpt.cluster_count;
    tkt->extents_count = tmpt.extents_count;

    tkt->tablesize = sz;
}

uint16_t tk_addaText(tk_t tk, uint16_t x, uint16_t y, uint16_t w, uint16_t h, tk_font_stuff* font, char* str)
{
    uint16_t n = tk->nitems; 
    uint16_t s = tk->tkt.nitems++;
    uint16_t w2 = w;
    uint16_t h2 = h;
    tk_text_table* tkt = &tk->tkt;

    tk_text_stuff* tkts = (tk_text_stuff*)malloc(sizeof(tk_text_stuff));
    
    tk_addaWidget(tk,x,y,w,h);
    
    if(!tkt->tablesize || s >= tkt->tablesize)
        tk_growtexttable(&tk->tkt);

    tk_setstring(&tkt->str[s],str);

    tk_addtogrowlist(&tkt->brk[s],&tkt->brklen[s],0);//alloc list for linebreaks

    // get glyphs for the text
    tkt->tkf[s] = font;
    tkt->scale = 1;
    tkt->strchange[s] = 1;
    tk_textlayout(tk->cr,tkt,s,&w2,&h2,0); 
    //TODO: what if w and h don't fit?

    tk->draw_f[n] = tk_drawtext;
    tkts->tkt = tkt;
    tkts->n = s;
    tkt->n[s] = n;
    tk->value[n] = tkts;

    return n; 
} 

void tk_showtipcallback(tk_t tk, const PuglEvent* e, uint16_t n)
{
    uint16_t w,h,s;
    n--;//text widget is 1 before timer part of ttip
    tk_text_stuff* tkts = (tk_text_stuff*)tk->value[n];
    s = tkts->n;
    if(!tk->tover) return;//no tip
    tkts->tkt->str[s] = tk->tip[tk->tover];
    tkts->tkt->strchange[s] = 1;
    tk_settimer(tk,tk->ttip,0);//disable timer

    tk->x[n] = tk->x[tk->tover];
    tk->y[n] = tk->y[tk->tover];

    //find best place to put the tip
    //first try to the right
    h = tk->h[0];
    w = tk->w[0]-tk->x[tk->tover]-tk->w[tk->tover];
    if(h>4) h-=4;
    if(w>4) w-=4;
    if(tk_textlayout(tk->cr,tkts->tkt,s,&w,&h,1))
    {//it fits
        tk->x[n] += tk->w[tk->tover]+2;
    } 
    else
    {//try on the left side
        h = tk->h[0];
        w = tk->x[tk->tover];
        if(h>4) h-=4;
        if(w>4) w-=4;
        if(tk_textlayout(tk->cr,tkts->tkt,s,&w,&h,1))
        {//it fits
            tk->x[n] -= w+2;
        }
        else
        {//try above
            h = tk->y[tk->tover];
            w = tk->w[0];
            if(h>4) h-=4;
            if(w>4) w-=4;
            if(tk_textlayout(tk->cr,tkts->tkt,s,&w,&h,1))
            {//it fits
                tk->y[n] -= h+2;
            }
            else
            {//try below
                h = tk->y[tk->tover];
                w = tk->w[0];
                if(h>4) h-=4;
                if(w>4) w-=4;

                if(tk_textlayout(tk->cr,tkts->tkt,s,&w,&h,1))
                {//it fits
                    tk->y[n] += tk->h[tk->tover]+h+2;
                }
                else
                {//don't show it
                    tk->w[n] = 0;
                    tk->h[n] = 0;
                    tk->x[n] = 0;
                    tk->y[n] = 0;
                    return;
                }
            }//below
        }//above
    }//left
    tk->w[n] = w;
    tk->h[n] = h;
    if(tk->y[n]+h > tk->h[0])
        tk->y[n] = tk->h[0]-h+2;
    if(tk->x[n]+w > tk->w[0])
        tk->x[n] = tk->w[0]-w+2;

    tk_changelayer(tk,n,3);
}

uint16_t tk_addaTooltip(tk_t tk, tk_font_stuff* font)
{
    //need text 
    uint16_t n = tk_addaText(tk, 0, 0, 0, 12, font, " ");
    tk->h[n] = 0;
    tk->layer[n] = 0;
    free(tk->tip[n]);
    tk->tip[n] = 0;

    //tk_text_stuff* tkt = (tk_text_stuff*)tk->value[n];

    //tk_addtolist(tk->hold_ratio,n);//?

    tk->draw_f[n] = tk_drawtip;

    //need timer
    n = tk_addaTimer(tk, 0);
    tk->ttip = n; 
    tk->callback_f[n] = tk_showtipcallback;

    return n;
}

