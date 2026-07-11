// ID_VH.C

#include "ID_HEADS.H"
#include "assets.h"
#include "render.h"
#include <SDL2/SDL.h>

#define	SCREENWIDTH		80
#define CHARWIDTH		2
#define TILEWIDTH		4
#define GRPLANES		4
#define BYTEPIXELS		4

#define SCREENXMASK		(~3)
#define SCREENXPLUS		(3)
#define SCREENXDIV		(4)

#define VIEWWIDTH		80

#define PIXTOBLOCK		4		// 16 pixels to an update block

byte	update[UPDATEHIGH][UPDATEWIDE];

//==========================================================================

pictabletype	_seg *pictable;


int	px,py;
byte	fontcolor,backcolor;
int	fontnumber;
int bufferwidth,bufferheight;


//==========================================================================

void	VWL_UpdateScreenBlocks (void);

//==========================================================================

void VW_DrawPropString (char far *string)
{
	fontstruct	far	*font;
	int		width,step,height,i;
	byte	far *source, far *dest, far *origdest;
	byte	ch,mask;

	font = (fontstruct far *)AM_GetGraphicsAsset(STARTFONT+fontnumber);
	height = bufferheight = font->height;
	dest = origdest = MK_FP(SCREENSEG,bufferofs+ylookup[py]+(px>>2));
	mask = 1<<(px&3);


	while ((ch = *string++)!=0)
	{
		width = step = font->width[ch];
		source = ((byte far *)font)+font->location[ch];
		while (width--)
		{
			VGAMAPMASK(mask);

asm	mov	ah,[BYTE PTR fontcolor]
asm	mov	bx,[step]
asm	mov	cx,[height]
asm	mov	dx,[linewidth]
asm	lds	si,[source]
asm	les	di,[dest]

vertloop:
asm	mov	al,[si]
asm	or	al,al
asm	je	next
asm	mov	[es:di],ah			// draw color

next:
asm	add	si,bx
asm	add	di,dx
asm	loop	vertloop
asm	mov	ax,ss
asm	mov	ds,ax

			source++;
			px++;
			mask <<= 1;
			if (mask == 16)
			{
				mask = 1;
				dest++;
			}
		}
	}
bufferheight = height;
bufferwidth = ((dest+1)-origdest)*4;
}


void VW_DrawColorPropString (char far *string)
{
	fontstruct	far	*font;
	int		width,step,height,i;
	byte	far *source, far *dest, far *origdest;
	byte	ch,mask;

	font = (fontstruct far *)AM_GetGraphicsAsset(STARTFONT+fontnumber);
	height = bufferheight = font->height;
	dest = origdest = MK_FP(SCREENSEG,bufferofs+ylookup[py]+(px>>2));
	mask = 1<<(px&3);


	while ((ch = *string++)!=0)
	{
		width = step = font->width[ch];
		source = ((byte far *)font)+font->location[ch];
		while (width--)
		{
			VGAMAPMASK(mask);

asm	mov	ah,[BYTE PTR fontcolor]
asm	mov	bx,[step]
asm	mov	cx,[height]
asm	mov	dx,[linewidth]
asm	lds	si,[source]
asm	les	di,[dest]

vertloop:
asm	mov	al,[si]
asm	or	al,al
asm	je	next
asm	mov	[es:di],ah			// draw color

next:
asm	add	si,bx
asm	add	di,dx

asm rcr cx,1				// inc font color
asm jc  cont
asm	inc ah

cont:
asm rcl cx,1
asm	loop	vertloop
asm	mov	ax,ss
asm	mov	ds,ax

			source++;
			px++;
			mask <<= 1;
			if (mask == 16)
			{
				mask = 1;
				dest++;
			}
		}
	}
bufferheight = height;
bufferwidth = ((dest+1)-origdest)*4;
}


//==========================================================================


/*
=================
=
= VL_MungePic
=
=================
*/

void VL_MungePic (byte far *source, unsigned width, unsigned height)
{
	unsigned	x,y,plane,size,pwidth;
	void* temp;
	byte		_seg *temp, far *dest, far *srcline;

	size = width*height;

	if (width&3)
		MS_Quit ("VL_MungePic: Not divisable by 4!");

//
// copy the pic to a temp buffer
//
	temp = malloc(size);
	_fmemcpy (temp,source,size);

//
// munge it back into the original buffer
//
	dest = source;
	pwidth = width/4;

	for (plane=0;plane<4;plane++)
	{
		srcline = temp;
		for (y=0;y<height;y++)
		{
			for (x=0;x<pwidth;x++)
				*dest++ = *(srcline+x*4+plane);
			srcline+=width;
		}
	}

	free(temp);
}

void VWL_MeasureString (char far *string, word *width, word *height
	, fontstruct _seg *font)
{
	*height = font->height;
	for (*width = 0;*string;string++)
		*width += font->width[*((byte far *)string)];	// proportional width
}

void	VW_MeasurePropString (char far *string, word *width, word *height)
{
	VWL_MeasureString(string,width,height,(fontstruct _seg *)AM_GetGraphicsAsset(STARTFONT+fontnumber));
}

void	VW_MeasureMPropString  (char far *string, word *width, word *height)
{
	VWL_MeasureString(string,width,height,(fontstruct _seg *)AM_GetGraphicsAsset(STARTFONTM+fontnumber));
}



/*
=============================================================================

				Double buffer management routines

=============================================================================
*/



void VWB_DrawTile8 (int x, int y, int tile)
{
	LatchDrawChar(x,y,tile);
}

void VWB_DrawTile8M (int x, int y, int tile)
{
	VL_MemToScreen (((byte far *)AM_GetGraphicsAsset(STARTTILE8M))+tile*64,8,8,x,y);
}

void VWB_DrawPropString	 (char far *string)
{
	int x;
	x=px;
	VW_DrawPropString (string);
}


void VWB_Bar (int x, int y, int width, int height, int color)
{
	VW_Bar (x,y,width,height,color);
}

void VWB_Plot (int x, int y, int color)
{
	VW_Plot(x,y,color);
}

void VWB_Hlin (int x1, int x2, int y, int color)
{
	VW_Hlin(x1,x2,y,color);
}

void VWB_Vlin (int y1, int y2, int x, int color)
{
	VW_Vlin(y1,y2,x,color);
}



/*
=============================================================================

						WOLFENSTEIN STUFF

=============================================================================
*/


/*
===================
=
= FizzleFade
=
= returns true if aborted
=
===================
*/

extern	ControlInfo	c;

static void ShufflePixels (unsigned *pixels, unsigned count) {
	unsigned i, j, tmp;

	// loop through every array element (pixel) and swap it with a random one.
	for (i = count-1; i > 0; i--) {
		j = rand() % (i+1);
		tmp = pixels[i];
		pixels[i] = pixels[j];
		pixels[j] = tmp;
	}
}

boolean FizzleFade (unsigned source, unsigned dest,
	unsigned width,unsigned height, unsigned frames, boolean abortable)
{
	static unsigned *order = NULL;
	static unsigned ordercount = 0;
	unsigned pixcount, pixperframe, reveal, goal, tics;
	uint32_t starttime;
	static uint8_t target[SCREEN_W * SCREEN_H];
	unsigned x0, y0;

	R_CaptureBackbuffer(target); // remember the frame we're fading TO
	R_RestoreShown(); // put the frame we're fading FROM back on the canvas
	
	x0 = dest % SCREEN_W; // dest (displayofs+screenofs) is a pixel offset in the port,
	y0 = dest / SCREEN_W; // so it directly encodes the region origin; source is unused

	pixcount = width*height;
	pixperframe = pixcount/frames + 1; //frames is still in 70hz tics

	if (ordercount != pixcount) { // rebuild only when the region size changes
		free(order);
		order = malloc(pixcount * sizeof(unsigned));
		ordercount = pixcount;
	}
	for (int i = 0; i < pixcount; i++) 
		order[i] = i;
	
	ShufflePixels(order, pixcount);

	IN_StartAck(); 

	reveal = 0;
	starttime = SDL_GetTicks();
	do {
		if (abortable && IN_CheckAck()) return true;

		// reveal as many pixels as the 70hz tic clock says we're due
		tics = (SDL_GetTicks() - starttime) * 70/1000 + 1;
		goal = tics * pixperframe;
		if (goal > pixcount)
			goal = pixcount;
		
		while (reveal < goal) {
			int i = order[reveal++];
			int x = x0 + i % width;
			int y = y0 + i / width;
			R_PutPixel(x, y, target[y * SCREEN_W + x]);
		}

		R_Present();
	} while(reveal < pixcount);

	return false;
}
