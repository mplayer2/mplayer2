/* 
 *  video_out_sage.c
 *
 *	Copyright (C) Aaron Holtzman - June 2000
 *
 *  This file is part of mpeg2dec, a free MPEG-2 video stream decoder.
 *	
 *  mpeg2dec is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *   
 *  mpeg2dec is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *   
 *  You should have received a copy of the GNU General Public License
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA. 
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
extern "C" {
#include "config.h"
#include "video_out.h"
#include "video_out_internal.h"
#include "sub/sub.h"
#include "osd.h"
}

// headers for SAGE
#include "sail.h"
#include "misc.h"
// sail object
sail sageInf; 
unsigned char *rgbBuffer = NULL;
int sage_ready = 0;

static const vo_info_t info = 
{
	"SAGE video output",
	"sage",
	"Luc RENAMBOT",
	""
};

extern "C" {
struct vo_old_functions _vf={
	preinit,
	config,
	control,
	draw_frame,
	draw_slice,
     	draw_osd,
	flip_page,
	check_events,
        uninit,
};
/**
 * As C++ doesn't support designated initializers, this should be
 * changed with changing vo_driver struct
 * FIXME: do the stuff to be c|c99|c++ independent
 */
extern const /*LIBVO_EXTERN(sage)*/struct vo_driver video_out_sage ={
    /*is_new*/0,
    /*buffer_frames*/0,
    /*old_functions*/&_vf,
    /*info*/&info,
    /*preinit*/old_vo_preinit,
    /*config*/old_vo_config,
    /*control*/old_vo_control,
    /*draw_image*/0,
    /*get_buffered_frame*/0,
    /*draw_slice*/old_vo_draw_slice,
    /*draw_osd*/old_vo_draw_osd,
    /*flip_page*/old_vo_flip_page,
    /*flip_page_timed*/0,
    /*check_events*/old_vo_check_events,
    /*uninit*/old_vo_uninit,
    /*privsize*/0,
    /*options*/0
};
}


static uint32_t image_width, image_height;
static void (*draw_osd_fnc) (int x0,int y0, int w,int h, unsigned char* src, unsigned char *srca, int stride);

static void draw_osd_24(int x0,int y0, int w,int h, unsigned char* src, unsigned char *srca, int stride){
   vo_draw_alpha_rgb24(w,h,src,srca,stride,rgbBuffer+3*(y0*image_width+x0),3*image_width);
}

static int preinit(const char *arg)
{
    fprintf(stderr, "reached here ->%s:%d ??\n", __FILE__, __LINE__);
    if(arg) 
    {
        printf("vo_sage: Unknown subdevice: %s\n",arg);
        return ENOSYS;
    }
    return 0;
}

static int config(uint32_t width, uint32_t height, uint32_t d_width, uint32_t d_height, uint32_t fullscreen, char *title, uint32_t format)
{
        //     Set up the video system. You get the dimensions and flags.
        //     width, height: size of the source image
        //     d_width, d_height: wanted scaled/display size (it's a hint)

	sailConfig scfg;
	sageRect renderImageMap;

	image_width = width;
	image_height = height;

	int bpp = IMGFMT_IS_BGR(format)?IMGFMT_BGR_DEPTH(format):IMGFMT_RGB_DEPTH(format);
	fprintf(stderr, "SAGE: bpp %d\n", bpp);

	switch (format) {
		case IMGFMT_RGB24:
            if (! sage_ready)
            {
                fprintf(stderr, "SAGE: we're good with 24bpp\n");
		
		
                scfg.init("mplayer.conf");
                scfg.setAppName("mplayer");
                scfg.rank = 0;

                scfg.resX = image_width;
                scfg.resY = image_height;

		// set the window size to double the movie size
		scfg.winWidth = image_width*2;
		scfg.winHeight = image_height*2;
		
                renderImageMap.left = 0.0;
                renderImageMap.right = 1.0;
                renderImageMap.bottom = 0.0;
                renderImageMap.top = 1.0;

                scfg.imageMap = renderImageMap;
                scfg.pixFmt = PIXFMT_888;
                scfg.rowOrd = TOP_TO_BOTTOM;

                    // SAGE init
                sageInf.init(scfg);
                fprintf(stderr, "SAGE: sage init done\n");

                if (rgbBuffer)
                    free(rgbBuffer);
                rgbBuffer = (unsigned char*)sageInf.getBuffer();  //malloc(image_width*image_height*3);
                memset(rgbBuffer, 0, image_width*image_height*3);
                fprintf(stderr, "SAGE: create buffer %d %d\n", image_width,image_height);
                sage_ready = 1;

                //choose osd drawing function
                draw_osd_fnc=draw_osd_24;
            }
            
			break;
		case IMGFMT_RGBA:
			break;
		case IMGFMT_BGR15:
			break;
		case IMGFMT_BGR16:
			break;
		case IMGFMT_BGR24:
			break;
		case IMGFMT_BGRA:
			break;
		default:
			break;
	}

	return 0;
}

static int query_format(uint32_t format)
{
        //fprintf(stderr, "SAGE: query_format %d\n", format);

	if( (!IMGFMT_IS_RGB(format)) && (!IMGFMT_IS_BGR(format)) ) return 0;

	int caps = VFCAP_CSP_SUPPORTED | VFCAP_CSP_SUPPORTED_BY_HW;
	if (format == IMGFMT_RGB24)
		return caps;

	return 1;
}

static void draw_osd(void)
{
  if (rgbBuffer && sage_ready)
    vo_draw_text(image_width,image_height,draw_osd_fnc);
}

static int draw_frame(uint8_t *src[])
{    
    uint8_t *ImageData=src[0];
    if (rgbBuffer && sage_ready)
    {
            //fprintf(stderr, "SAGE: draw_frame %d %d\n", image_width,image_height);
            //memset(rgbBuffer, 0, image_width*image_height*3);
        rgbBuffer = (unsigned char*)sageInf.getBuffer();
        memcpy(rgbBuffer, (unsigned char *)ImageData, image_width*image_height*3);
    }
    
	return 0;
}

static int draw_slice(uint8_t *image[], int stride[], int w,int h,int x,int y)
{
        //fprintf(stderr, "SAGE: draw_slice\n");
	return 0;
}


static void check_events(void)
{
}

static void flip_page(void)
{
        //     flip_page(): this is called after each frame, this diplays the buffer for
        //        real. This is 'swapbuffers' when doublebuffering.

  // check for SAGE messages
  // this is where the UI messages will be checked for as well
  sageMessage msg;
  if (sageInf.checkMsg(msg, false) > 0) {
    switch (msg.getCode()) {
      case APP_QUIT : {
	exit(1);
	break;
      }
    }	
  }

    if (rgbBuffer && sage_ready)
    {
            //fprintf(stderr, "SAGE: swapBuffer\n");
        sageInf.swapBuffer(); 
    }
}

static void uninit(void)
{
}

static int control(uint32_t request, void *data)
{
    switch (request) {
        case VOCTRL_QUERY_FORMAT:
            return query_format(*((uint32_t*)data));
    }
    return VO_NOTIMPL;
}
