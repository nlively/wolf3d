// ID_VH.C

#include "ID_HEADS.H"
#include "assets.h"

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


/*
=======================
=
= VW_MarkUpdateBlock
=
= Takes a pixel bounded block and marks the tiles in bufferblocks
= Returns 0 if the entire block is off the buffer screen
=
=======================
*/

int VW_MarkUpdateBlock (int x1, int y1, int x2, int y2)
{
	int	x,y,xt1,yt1,xt2,yt2,nextline;
	byte *mark;

	xt1 = x1>>PIXTOBLOCK;
	yt1 = y1>>PIXTOBLOCK;

	xt2 = x2>>PIXTOBLOCK;
	yt2 = y2>>PIXTOBLOCK;

	if (xt1<0)
		xt1=0;
	else if (xt1>=UPDATEWIDE)
		return 0;

	if (yt1<0)
		yt1=0;
	else if (yt1>UPDATEHIGH)
		return 0;

	if (xt2<0)
		return 0;
	else if (xt2>=UPDATEWIDE)
		xt2 = UPDATEWIDE-1;

	if (yt2<0)
		return 0;
	else if (yt2>=UPDATEHIGH)
		yt2 = UPDATEHIGH-1;

	mark = updateptr + uwidthtable[yt1] + xt1;
	nextline = UPDATEWIDE - (xt2-xt1) - 1;

	for (y=yt1;y<=yt2;y++)
	{
		for (x=xt1;x<=xt2;x++)
			*mark++ = 1;			// this tile will need to be updated

		mark += nextline;
	}

	return 1;
}

void VWB_DrawTile8 (int x, int y, int tile)
{
	if (VW_MarkUpdateBlock (x,y,x+7,y+7))
		LatchDrawChar(x,y,tile);
}

void VWB_DrawTile8M (int x, int y, int tile)
{
	if (VW_MarkUpdateBlock (x,y,x+7,y+7))
		VL_MemToScreen (((byte far *)AM_GetGraphicsAsset(STARTTILE8M))+tile*64,8,8,x,y);
}

void VWB_DrawPropString	 (char far *string)
{
	int x;
	x=px;
	VW_DrawPropString (string);
	VW_MarkUpdateBlock(x,py,px-1,py+bufferheight-1);
}


void VWB_Bar (int x, int y, int width, int height, int color)
{
	if (VW_MarkUpdateBlock (x,y,x+width,y+height-1) )
		VW_Bar (x,y,width,height,color);
}

void VWB_Plot (int x, int y, int color)
{
	if (VW_MarkUpdateBlock (x,y,x,y))
		VW_Plot(x,y,color);
}

void VWB_Hlin (int x1, int x2, int y, int color)
{
	if (VW_MarkUpdateBlock (x1,y,x2,y))
		VW_Hlin(x1,x2,y,color);
}

void VWB_Vlin (int y1, int y2, int x, int color)
{
	if (VW_MarkUpdateBlock (x,y1,x,y2))
		VW_Vlin(y1,y2,x,color);
}

/*
=====================
=
= VH_UpdateScreen
=
= Copies all the dirty update tiles from the build buffer (bufferofs) to the
= visible page (displayofs).  Originally hand-written in ID_VH_A.ASM; ported
= to C as part of the modernization so the port target is a single language.
=
= Each update tile is 4 bytes wide (16 pixels across all four planes) and 16
= scanlines tall, copied with a VGA latch copy (write mode 1).
=
=====================
*/

void VH_UpdateScreen (void)
{
	byte	*tiles = (byte *)update;
	byte	far *src, far *dst;
	int		tile,i;

	for (tile = 0 ; tile < UPDATEWIDE*UPDATEHIGH ; tile++)
	{
		if (!(tiles[tile] & 1))
			continue;

		tiles[tile] = 0;
		src = MK_FP(SCREENSEG,blockstarts[tile] + bufferofs);
		dst = MK_FP(SCREENSEG,blockstarts[tile] + displayofs);

		for (i = 0 ; i < 16 ; i++)
		{
			dst[0] = src[0];
			dst[1] = src[1];
			dst[2] = src[2];
			dst[3] = src[3];
			src += linewidth;
			dst += linewidth;
		}
	}

	VGAWRITEMODE(0);
}


void VW_UpdateScreen (void)
{
	VH_UpdateScreen ();
}


/*
=============================================================================

						WOLFENSTEIN STUFF

=============================================================================
*/


//==========================================================================

//==========================================================================

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

boolean FizzleFade (unsigned source, unsigned dest,
	unsigned width,unsigned height, unsigned frames, boolean abortable)
{
	int			pixperframe;
	unsigned	drawofs,pagedelta;
	byte 		mask,maskb[8] = {1,2,4,8};
	unsigned	x,y,p,frame;
	long		rndval;

	pagedelta = dest-source;
	rndval = 1;
	y = 0;
	pixperframe = 64000/frames;

	IN_StartAck ();

	TimeCount=frame=0;
	do	// while (1)
	{
		if (abortable && IN_CheckAck () )
			return true;

		asm	mov	es,[screenseg]

		for (p=0;p<pixperframe;p++)
		{
			//
			// seperate random value into x/y pair
			//
			asm	mov	ax,[WORD PTR rndval]
			asm	mov	dx,[WORD PTR rndval+2]
			asm	mov	bx,ax
			asm	dec	bl
			asm	mov	[BYTE PTR y],bl			// low 8 bits - 1 = y xoordinate
			asm	mov	bx,ax
			asm	mov	cx,dx
			asm	mov	[BYTE PTR x],ah			// next 9 bits = x xoordinate
			asm	mov	[BYTE PTR x+1],dl
			//
			// advance to next random element
			//
			asm	shr	dx,1
			asm	rcr	ax,1
			asm	jnc	noxor
			asm	xor	dx,0x0001
			asm	xor	ax,0x2000
noxor:
			asm	mov	[WORD PTR rndval],ax
			asm	mov	[WORD PTR rndval+2],dx

			if (x>width || y>height)
				continue;
			drawofs = source+ylookup[y] + (x>>2);

			//
			// copy one pixel
			//
			mask = x&3;
			VGAREADMAP(mask);
			mask = maskb[mask];
			VGAMAPMASK(mask);

			asm	mov	di,[drawofs]
			asm	mov	al,[es:di]
			asm add	di,[pagedelta]
			asm	mov	[es:di],al

			if (rndval == 1)		// entire sequence has been completed
				return false;
		}
		frame++;
		while (TimeCount<frame)		// don't go too fast
		;
	} while (1);


}
