// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id:$
//
// Copyright (C) 1993-1996 by id Software, Inc.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// $Log:$
//
// DESCRIPTION:
//	DOOM graphics stuff for X11, UNIX.
//
//-----------------------------------------------------------------------------

#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>

#include <Ph.h>
#include <Pt.h>

#include <stdarg.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <netinet/in.h>
#include <errno.h>
#include <signal.h>

#include "doomstat.h"
#include "i_system.h"
#include "v_video.h"
#include "m_argv.h"
#include "d_main.h"
#include "g_game.h"

#include "doomdef.h"

#define POINTER_WARP_COUNTDOWN	1

#define PkIsRepeated( f ) ((f & Pk_KF_Key_Repeat) != 0)
#define PkIsReleased( f ) ((f & (Pk_KF_Key_Down|Pk_KF_Key_Repeat)) == 0)

static PtWidget_t *Window;
static PtWidget_t *FrameLabel;

extern struct _Ph_ctrl *_Ph_;
static PtAppContext_t app;
static PhImage_t *image;
static int input_group;
static boolean window_focused = false;
static int	lastmousex;
static int	lastmousey;
static boolean novert = false;
int		Ph_width;
int		Ph_height;


// MIT SHared Memory extension.
boolean		doShm;

//XShmSegmentInfo	X_shminfo;
int		X_shmeventtype;

// Fake mouse handling.
// This cannot work properly w/o DGA.
// Needs an invisible mouse cursor at least.
boolean		grabMouse;
int		doPointerWarp = POINTER_WARP_COUNTDOWN;

// Blocky mode,
// replace each 320x200 pixel with multiply*multiply pixels.
// According to Dave Taylor, it still is a bonehead thing
// to use ....
static int	multiply=1;


static void MoveCursorToCenter(void)
{
  short x, y;

  PtGetAbsPosition(Window, &x, &y);
  lastmousex = x + Window->area.size.w / 2;
  lastmousey = y + Window->area.size.h / 2;
  PhMoveCursorAbs(input_group, lastmousex, lastmousey);
}

typedef enum
{
  cursor_hide,
  cursor_display
} cursortype_t;

static void ChangeCursorAppearance(cursortype_t cursortype)
{
  PtArg_t arg;
  
  PtSetArg(&arg, Pt_ARG_CURSOR_TYPE, 
           (cursortype == cursor_hide) ? Ph_CURSOR_NONE : Ph_CURSOR_INHERIT, 0);
  PtSetResources(Window, 1, &arg);
}

//
//  Translates the key
//  Use keycap, becase keysym is not valid on release
//
int translatekey(unsigned long keycap)
{
    int rc;
    
    switch(keycap)
    {
      case Pk_Left: rc = KEY_LEFTARROW; break;
      case Pk_Right: rc = KEY_RIGHTARROW; break;
      case Pk_Down: rc = KEY_DOWNARROW; break;
      case Pk_Up: rc = KEY_UPARROW; break;
      case Pk_Escape: rc = KEY_ESCAPE; break;
      case Pk_Return: rc = KEY_ENTER; break;
      case Pk_Tab: rc = KEY_TAB; break;
      case Pk_F1: rc = KEY_F1; break;
      case Pk_F2: rc = KEY_F2; break;
      case Pk_F3: rc = KEY_F3; break;
      case Pk_F4: rc = KEY_F4; break;
      case Pk_F5: rc = KEY_F5; break;
      case Pk_F6: rc = KEY_F6; break;
      case Pk_F7: rc = KEY_F7; break;
      case Pk_F8: rc = KEY_F8; break;
      case Pk_F9: rc = KEY_F9; break;
      case Pk_F10: rc = KEY_F10; break;
      case Pk_F11: rc = KEY_F11; break;
      case Pk_F12: rc = KEY_F12; break;
	
      case Pk_BackSpace:
      case Pk_Delete: rc = KEY_BACKSPACE; break;

      case Pk_Pause: rc = KEY_PAUSE; break;

      case Pk_KP_Equal:
      case Pk_equal: rc = KEY_EQUALS; break;

      case Pk_KP_Subtract:
      case Pk_minus: rc = KEY_MINUS; break;

      case Pk_Shift_L:
      case Pk_Shift_R: rc = KEY_RSHIFT; break;
	
      case Pk_Control_L:
      case Pk_Control_R: rc = KEY_RCTRL; break;
	
      case Pk_Alt_L:
      case Pk_Meta_L:
      case Pk_Alt_R:
      case Pk_Meta_R: rc = KEY_RALT; break;
	
      default:
        rc = keycap;
	      if (rc >= Pk_space && rc <= Pk_asciitilde)
	        rc = rc - Pk_space + ' ';
	      break;
    }
    return rc;
}


void I_ShutdownGraphics(void)
{
  // Remove shared memory references;
  PgShmemCleanup();
  // Paranoia.
  if (image)
    image->image = NULL;
}


//
// I_StartFrame
//
void I_StartFrame (void)
{
    // er?

}

boolean		mousemoved = false;
boolean		shmFinished;

void I_GetEvent(void)
{
  event_t event;
  union
  {
    void *raw;
    PhPointerEvent_t *ptr_ev;
    PhKeyEvent_t     *key_ev;
    PhWindowEvent_t  *win_ev;
  } ph_ev;
  
  app->event->processing_flags = 0;
  ph_ev.raw = PhGetData(app->event);
  switch(app->event->type)
  {
    case Ph_EV_BUT_RELEASE:
      if (app->event->subtype == Ph_EV_RELEASE_ENDCLICK)
        break;
    case Ph_EV_BUT_PRESS:
      event.type = ev_mouse;
      event.data1 = (ph_ev.ptr_ev->button_state & Ph_BUTTON_SELECT ? 1 : 0)
                  | (ph_ev.ptr_ev->button_state & Ph_BUTTON_MENU   ? 2 : 0)
                  | (ph_ev.ptr_ev->button_state & Ph_BUTTON_ADJUST ? 4 : 0);
      event.data2 = event.data3 = 0;
      D_PostEvent(&event);
      break;
    case Ph_EV_KEY:
      if (PkIsFirstDown(ph_ev.key_ev->key_flags))
      {
        event.type = ev_keydown;
        event.data1 = translatekey(ph_ev.key_ev->key_cap);
        D_PostEvent(&event);
      }
      else if (PkIsReleased(ph_ev.key_ev->key_flags))
      {
        event.type = ev_keyup;
        event.data1 = translatekey(ph_ev.key_ev->key_cap);
        D_PostEvent(&event);
      }
      break;
    case Ph_EV_PTR_MOTION_BUTTON:
    case Ph_EV_PTR_MOTION_NOBUTTON:
      if (window_focused)
      {
        event.type = ev_mouse;
        event.data1 = (ph_ev.ptr_ev->button_state & Ph_BUTTON_SELECT ? 1 : 0)
                    | (ph_ev.ptr_ev->button_state & Ph_BUTTON_MENU   ? 2 : 0)
                    | (ph_ev.ptr_ev->button_state & Ph_BUTTON_ADJUST ? 4 : 0);
        event.data2 = (ph_ev.ptr_ev->pos.x - lastmousex) << 2;
        event.data3 = (lastmousey - ph_ev.ptr_ev->pos.y) << 2;
        if (event.data2 || event.data3)
        {
          mousemoved = true;
          lastmousex = ph_ev.ptr_ev->pos.x;
          lastmousey = ph_ev.ptr_ev->pos.y;
          if (novert)
          {
            event.data3 = 0;
            if (!event.data2)
              break;
          }
          D_PostEvent(&event);
        }
      }
      break;
    case Ph_EV_WM:
      if (ph_ev.win_ev->event_f & Ph_WM_FOCUS)
      {
        window_focused = (ph_ev.win_ev->event_state ==
                          Ph_WM_EVSTATE_FOCUSLOST) ? false : true;
        if (!window_focused && (ph_ev.win_ev->state_f & Ph_WM_STATE_ISFOCUS))
        {
          //Window lost focus
          G_ReleaseAllButtonsKeys();
          ChangeCursorAppearance(cursor_display);
        }
        else if (window_focused && !(ph_ev.win_ev->state_f & Ph_WM_STATE_ISFOCUS))
        {
          //Window get focus
          ChangeCursorAppearance(cursor_hide);
        }
        
      }
         
      break;
  }
  PtEventHandler(app->event);
  if (_Pt_->destroyed_widgets)
    PtRemoveWidget();
}


//
// I_StartTic
//

void I_StartTic (void)
{
  register int ret;

  if (!_Ph_)
    return;

  //delay(10);

  while((ret = PhEventPeek(app->event, app->event_size)))
  {
    switch(ret)
    {
      case Ph_EVENT_MSG:
        I_GetEvent();
        break;
      case Ph_RESIZE_MSG:
        if (PtResizeEventMsg(app, PhGetMsgSize(app->event)) == -1)
          I_Error("Can not reallocate event buffer");
        break;
      case -1:
        I_Error("Receiving Photon event");
        break;
    }
  }
  if (window_focused && mousemoved)
    MoveCursorToCenter();
    
  mousemoved = false;
}


//
// I_UpdateNoBlit
//
void I_UpdateNoBlit (void)
{
    // what is this?
}

//
// I_FinishUpdate
//

void I_FinishUpdate (void)
{
  static int lasttic;
  int tics;
  int i;
  // UNUSED static unsigned char *bigscreen=0;
  // draws little dots on the bottom of the screen
  if (devparm)
  {
    i = I_GetTime();
    tics = i - lasttic;
    lasttic = i;
    if (tics > 20)
      tics = 20;
    for (i=0 ; i<tics*2 ; i+=2)
      screens[0][ (SCREENHEIGHT-1)*SCREENWIDTH + i] = 0xff;
    for ( ; i<20*2 ; i+=2)
      screens[0][ (SCREENHEIGHT-1)*SCREENWIDTH + i] = 0x0;
  }
  switch(multiply)
  {
  case 2:
    {
      unsigned int *olineptrs[2];
      unsigned int *ilineptr;
      int x, y, i;
      unsigned int twoopixels;
      unsigned int twomoreopixels;
      unsigned int fouripixels;

      ilineptr = (unsigned int *) (screens[0]);
      for (i=0 ; i<2 ; i++)
        olineptrs[i] = (unsigned int *) &image->image[i*Ph_width];

      y = SCREENHEIGHT;
      while (y--)
      {
        x = SCREENWIDTH;
        do
        {
          fouripixels = *ilineptr++;
          twoopixels = (fouripixels & 0xff000000)
                       | ((fouripixels>>8) & 0xffff00)
                       | ((fouripixels>>16) & 0xff);
          twomoreopixels =	((fouripixels<<16) & 0xff000000)
                       | ((fouripixels<<8) & 0xffff00)
                       | (fouripixels & 0xff);

          *olineptrs[0]++ = twomoreopixels;
          *olineptrs[1]++ = twomoreopixels;
          *olineptrs[0]++ = twoopixels;
          *olineptrs[1]++ = twoopixels;
        } while (x-=4);
        olineptrs[0] += Ph_width/4;
        olineptrs[1] += Ph_width/4;
      }
    }
    break;
  case 3:
    {
      unsigned int *olineptrs[3];
      unsigned int *ilineptr;
      int x, y, i;
      unsigned int fouropixels[3];
      unsigned int fouripixels;

      ilineptr = (unsigned int *) (screens[0]);
      for (i=0 ; i<3 ; i++)
        olineptrs[i] = (unsigned int *) &image->image[i*Ph_width];

      y = SCREENHEIGHT;
      while (y--)
      {
        x = SCREENWIDTH;
        do
        {
          fouripixels = *ilineptr++;
          fouropixels[0] = (fouripixels & 0xff000000)
                         | ((fouripixels>>8) & 0xff0000)
                         | ((fouripixels>>16) & 0xffff);
          fouropixels[1] = ((fouripixels<<8) & 0xff000000)
                         | (fouripixels & 0xffff00)
                         | ((fouripixels>>8) & 0xff);
          fouropixels[2] = ((fouripixels<<16) & 0xffff0000)
                         | ((fouripixels<<8) & 0xff00)
                         | (fouripixels & 0xff);

          *olineptrs[0]++ = fouropixels[2];
          *olineptrs[1]++ = fouropixels[2];
          *olineptrs[2]++ = fouropixels[2];
          *olineptrs[0]++ = fouropixels[1];
          *olineptrs[1]++ = fouropixels[1];
          *olineptrs[2]++ = fouropixels[1];
          *olineptrs[0]++ = fouropixels[0];
          *olineptrs[1]++ = fouropixels[0];
          *olineptrs[2]++ = fouropixels[0];
        } while (x-=4);
        olineptrs[0] += 2*Ph_width/4;
        olineptrs[1] += 2*Ph_width/4;
        olineptrs[2] += 2*Ph_width/4;
      }
    }
    break;
  case 4:
    {
	    // Broken. Gotta fix this some day.
      void Expand4(unsigned *, double *);
  	  Expand4 ((unsigned *)(screens[0]), (double *) (image->image));
    }
    break;
  }
  //TODO: calculate image-data and pallete-data tags
  PtDamageWidget(FrameLabel);
  PtFlush();
  //PtSyncPhoton();
}

//
// I_ReadScreen
//
void I_ReadScreen (byte* scr)
{
    memcpy (scr, screens[0], SCREENWIDTH*SCREENHEIGHT);
}


//
// Palette stuff.
//
static PgColor_t	colors[256];

void UploadNewPalette(byte *palette)
{
  register int i;
  register PgColor_t color;

  for (i=0 ; i<256 ; i++)
	{
    color = PgRGB(gammatable[usegamma][palette[i*3]],
                  gammatable[usegamma][palette[i*3 + 1]],
                  gammatable[usegamma][palette[i*3 + 2]]);
    colors[i] = color;
	}
  memcpy(image->palette, colors, 256 * sizeof(PgColor_t));
}


//
// I_SetPalette
//
void I_SetPalette (byte* palette)
{
    UploadNewPalette(palette);
}



void I_InitGraphics(void)
{
  static int firsttime=1;
  char *displayname, *env_var;
  int pnum;
  PhRid_t rid;
  PhRegion_t region;
  
  PhChannelParms_t parms = {0, 0, Ph_DYNAMIC_BUFFER};
  PtArg_t arg[10];
  PhDim_t dim;

  if (!firsttime)
    return; 
  firsttime = 0;
  
  //signal(SIGINT, (void (*)(int)) I_Quit);

  if (M_CheckParm("-2"))
	  multiply = 2;

  if (M_CheckParm("-3"))
	  multiply = 3;

  if (M_CheckParm("-4"))
	  multiply = 4;
  
  dim.w = Ph_width = SCREENWIDTH * multiply;
  dim.h = Ph_height = SCREENHEIGHT * multiply;
  
  // check for command-line display name
  if ( (pnum=M_CheckParm("-disp")) ) // suggest parentheses around assignment
	  displayname = myargv[pnum+1];
  else
	  displayname = NULL;
  
  // check if the user wants to ignore mouse y-axis
  if (M_CheckParm("-novert"))
    novert = true;

  if (!PhAttach(displayname, &parms))
  {
    if (displayname)
      I_Error("Could not attach to Photon manager [%s]", displayname);
    else if ((displayname = getenv("PHOTON")))
      I_Error("Could not attach to Photon manager (PHOTON=[%s])", displayname);
    else
      I_Error("Could not attach to Photon manager [/dev/photon]");
  }
  
  PtSetArg(&arg[0], Pt_ARG_WINDOW_TITLE, "PhDoom", 0);
  PtSetArg(&arg[1], Pt_ARG_DIM, &dim, 0);
  PtSetArg(&arg[2], Pt_ARG_WINDOW_RENDER_FLAGS,
                    Ph_WM_RENDER_ASAPP |
                    Ph_WM_RENDER_CLOSE |
                    Ph_WM_RENDER_TITLE |
                    Ph_WM_RENDER_MIN,
                    Pt_TRUE);
  PtSetArg(&arg[3], Pt_ARG_WINDOW_NOTIFY_FLAGS,
                    Ph_WM_CLOSE | Ph_WM_FOCUS,
                    Pt_TRUE);       
  PtSetArg(&arg[4], Pt_ARG_MIN_HEIGHT, dim.h, 0);
  PtSetArg(&arg[5], Pt_ARG_MIN_WIDTH,  dim.w, 0);
  PtSetArg(&arg[6], Pt_ARG_MAX_HEIGHT, dim.h, 0);
  PtSetArg(&arg[7], Pt_ARG_MAX_WIDTH,  dim.w, 0);
  PtSetArg(&arg[8], Pt_ARG_CURSOR_TYPE, Ph_CURSOR_NONE, 0);

  Window = PtAppInit(NULL, NULL, NULL, 9, arg);
  
  image = calloc(1, sizeof(*image));
  if (!image)
    I_Error("Could not allocate memory for image header");
  image->type = Pg_IMAGE_PALETTE_BYTE;
  image->size = dim;
  image->bpl = image->size.w;
  image->image = PgShmemCreate(image->size.w * image->size.h, NULL);
  if (!image->image)
    I_Error("Could not allocate shared memory for image pixel data");
  if (multiply == 1)
	  screens[0] = (unsigned char *) (image->image);

  image->colors = 256;
  image->palette = calloc(image->colors, sizeof(PgColor_t));
  if (!image->palette)
    I_Error("Could not allocate memory for image palette");
  
  PtSetArg(&arg[0], Pt_ARG_DIM, &dim, 0);
  PtSetArg(&arg[1], Pt_ARG_LABEL_TYPE, Pt_IMAGE, 0);
  PtSetArg(&arg[2], Pt_ARG_LABEL_DATA, image, sizeof(*image));
  PtSetArg(&arg[3], Pt_ARG_MARGIN_HEIGHT, 0, 0);
  PtSetArg(&arg[4], Pt_ARG_MARGIN_WIDTH, 0, 0);
  PtSetArg(&arg[5], Pt_ARG_BORDER_WIDTH, 0, 0);
  
  FrameLabel = PtCreateWidget(PtLabel, Window, 6, arg);
  
  PtRealizeWidget(Window);
  
  rid = PtWidgetRid(Window);
  PhRegionQuery(rid, &region, NULL, NULL, 0);
  region.events_sense |= Ph_EV_PTR_MOTION_BUTTON | Ph_EV_PTR_MOTION_NOBUTTON;
  PhRegionChange(Ph_REGION_EV_SENSE, Ph_EXPOSE_REGION, &region, NULL, NULL);

  if (!((env_var = getenv("PHIG")) && (input_group = atoi(env_var))))
    input_group = 1;
  MoveCursorToCenter();

  app = PtDefaultAppContext();
}



unsigned	exptable[256];

void InitExpand (void)
{
    int		i;
	
    for (i=0 ; i<256 ; i++)
	exptable[i] = i | (i<<8) | (i<<16) | (i<<24);
}

double		exptable2[256*256];

void InitExpand2 (void)
{
    int		i;
    int		j;
    // UNUSED unsigned	iexp, jexp;
    double*	exp;
    union
    {
	double 		d;
	unsigned	u[2];
    } pixel;
	
    printf ("building exptable2...\n");
    exp = exptable2;
    for (i=0 ; i<256 ; i++)
    {
	pixel.u[0] = i | (i<<8) | (i<<16) | (i<<24);
	for (j=0 ; j<256 ; j++)
	{
	    pixel.u[1] = j | (j<<8) | (j<<16) | (j<<24);
	    *exp++ = pixel.d;
	}
    }
    printf ("done.\n");
}

static int	inited;

void
Expand4
( unsigned*	lineptr,
  double*	xline )
{
    double	dpixel;
    unsigned	x;
    unsigned 	y;
    unsigned	fourpixels;
    unsigned	step;
    double*	exp;
	
    exp = exptable2;
    if (!inited)
    {
	inited = 1;
	InitExpand2 ();
    }
		
		
    step = 3*SCREENWIDTH/2;
	
    y = SCREENHEIGHT-1;
    do
    {
	x = SCREENWIDTH;

	do
	{
	    fourpixels = lineptr[0];
			
	    dpixel = *(double *)( (int)exp + ( (fourpixels&0xffff0000)>>13) );
	    xline[0] = dpixel;
	    xline[160] = dpixel;
	    xline[320] = dpixel;
	    xline[480] = dpixel;
			
	    dpixel = *(double *)( (int)exp + ( (fourpixels&0xffff)<<3 ) );
	    xline[1] = dpixel;
	    xline[161] = dpixel;
	    xline[321] = dpixel;
	    xline[481] = dpixel;

	    fourpixels = lineptr[1];
			
	    dpixel = *(double *)( (int)exp + ( (fourpixels&0xffff0000)>>13) );
	    xline[2] = dpixel;
	    xline[162] = dpixel;
	    xline[322] = dpixel;
	    xline[482] = dpixel;
			
	    dpixel = *(double *)( (int)exp + ( (fourpixels&0xffff)<<3 ) );
	    xline[3] = dpixel;
	    xline[163] = dpixel;
	    xline[323] = dpixel;
	    xline[483] = dpixel;

	    fourpixels = lineptr[2];
			
	    dpixel = *(double *)( (int)exp + ( (fourpixels&0xffff0000)>>13) );
	    xline[4] = dpixel;
	    xline[164] = dpixel;
	    xline[324] = dpixel;
	    xline[484] = dpixel;
			
	    dpixel = *(double *)( (int)exp + ( (fourpixels&0xffff)<<3 ) );
	    xline[5] = dpixel;
	    xline[165] = dpixel;
	    xline[325] = dpixel;
	    xline[485] = dpixel;

	    fourpixels = lineptr[3];
			
	    dpixel = *(double *)( (int)exp + ( (fourpixels&0xffff0000)>>13) );
	    xline[6] = dpixel;
	    xline[166] = dpixel;
	    xline[326] = dpixel;
	    xline[486] = dpixel;
			
	    dpixel = *(double *)( (int)exp + ( (fourpixels&0xffff)<<3 ) );
	    xline[7] = dpixel;
	    xline[167] = dpixel;
	    xline[327] = dpixel;
	    xline[487] = dpixel;

	    lineptr+=4;
	    xline+=8;
	} while (x-=16);
	xline += step;
    } while (y--);
}


