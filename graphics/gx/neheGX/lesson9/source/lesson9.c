/*---------------------------------------------------------------------------------

	nehe lesson 9 port to GX by shagkur

---------------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <math.h>
#include <gccore.h>
#include <wiiuse/wpad.h>

#include "startex_tpl.h"
#include "startex.h" 

#define DEFAULT_FIFO_SIZE	(256*1024)
#define NUM_STARS			50

typedef struct tagtexdef
{
	void *tex_data;
	u32 sz_x;
	u32 sz_y;
	u32 fmt;
	u32 min_lod;
	u32 max_lod;
	u32 min;
	u32 mag;
	u32 wrap_s;
	u32 wrap_t;
	void *nextdef;
} texdef;

typedef struct 
{
	u8 r,g,b;
	f32 dist;
	f32 ang;
} star;


static star stars[NUM_STARS];
static GXRModeObj *rmode = NULL;
static void *frameBuffer[2] = { NULL, NULL};

int main(int argc,char **argv)
{
	f32 yscale,spin = 0.0f;
	u32 xfbHeight,i;
	u32 fb = 0;
	u32 first_frame = 1;
	f32 zoom = -15.0f;
	GXTexObj texture;
	Mtx view; // view and perspective matrices
	Mtx model, modelview;
	Mtx44 perspective;
	void *gpfifo = NULL;
	GXColor background = {0, 0, 0, 0xff};
	guVector cam = {0.0F, 0.0F, 0.0F},
			up = {0.0F, 1.0F, 0.0F},
		  look = {0.0F, 0.0F, -1.0F};
	TPLFile starTPL;


	VIDEO_Init();
	WPAD_Init();

	rmode = VIDEO_GetPreferredMode(NULL);

	// allocate the fifo buffer
	gpfifo = memalign(32,DEFAULT_FIFO_SIZE);
	memset(gpfifo,0,DEFAULT_FIFO_SIZE);

	// allocate 2 framebuffers for double buffering
	frameBuffer[0] = SYS_AllocateFramebuffer(rmode);
	frameBuffer[1] = SYS_AllocateFramebuffer(rmode);

	// configure video
	VIDEO_Configure(rmode);
	VIDEO_SetNextFramebuffer(frameBuffer[fb]);
	VIDEO_Flush();
	VIDEO_WaitVSync();
	if(rmode->viTVMode&VI_NON_INTERLACE) VIDEO_WaitVSync();

	fb ^= 1;

	// init the flipper
	GX_Init(gpfifo,DEFAULT_FIFO_SIZE);
 
	// clears the bg to color and clears the z buffer
	GX_SetCopyClear(background, 0x00ffffff);
 
	// other gx setup
	GX_SetViewport(0,0,rmode->fbWidth,rmode->efbHeight,0,1);
	yscale = GX_GetYScaleFactor(rmode->efbHeight,rmode->xfbHeight);
	xfbHeight = GX_SetDispCopyYScale(yscale);
	GX_SetScissor(0,0,rmode->fbWidth,rmode->efbHeight);
	GX_SetDispCopySrc(0,0,rmode->fbWidth,rmode->efbHeight);
	GX_SetDispCopyDst(rmode->fbWidth,xfbHeight);
	GX_SetCopyFilter(rmode->aa,rmode->sample_pattern,GX_TRUE,rmode->vfilter);
	GX_SetFieldMode(rmode->field_rendering,((rmode->viHeight==2*rmode->xfbHeight)?GX_ENABLE:GX_DISABLE));
 
	if (rmode->aa)
        GX_SetPixelFmt(GX_PF_RGB565_Z16, GX_ZC_LINEAR);
    else
        GX_SetPixelFmt(GX_PF_RGB8_Z24, GX_ZC_LINEAR);

	GX_SetCullMode(GX_CULL_NONE);
	GX_CopyDisp(frameBuffer[fb],GX_TRUE);
	GX_SetDispCopyGamma(GX_GM_1_0);

	// setup the vertex attribute table
	// describes the data
	// args: vat location 0-7, type of data, data format, size, scale
	// so for ex. in the first call we are sending position data with
	// 3 values X,Y,Z of size F32. scale sets the number of fractional
	// bits for non float data.
	GX_ClearVtxDesc();
	GX_SetVtxDesc(GX_VA_POS, GX_DIRECT);
	GX_SetVtxDesc(GX_VA_CLR0, GX_DIRECT);
	GX_SetVtxDesc(GX_VA_TEX0, GX_DIRECT);

	GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
	GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0);
	GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGB8, 0);

	// set number of rasterized color channels
	GX_SetNumChans(1);

	//set number of textures to generate
	GX_SetNumTexGens(1);

	// setup texture coordinate generation
	// args: texcoord slot 0-7, matrix type, source to generate texture coordinates from, matrix to use
	GX_SetTexCoordGen(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_IDENTITY);


    GX_InvVtxCache();
	GX_InvalidateTexAll();
	TPL_OpenTPLFromMemory(&starTPL, (void *)startex_tpl,startex_tpl_size);
	TPL_GetTexture(&starTPL,startex,&texture);
	// setup our camera at the origin
	// looking down the -z axis with y up
	guLookAt(view, &cam, &up, &look);
 
	// setup our projection matrix
	// this creates a perspective matrix with a view angle of 90,
	// and aspect ratio based on the display resolution
    f32 w = rmode->viWidth;
    f32 h = rmode->viHeight;
	guPerspective(perspective, 45, (f32)w/h, 0.1F, 300.0F);
	GX_LoadProjectionMtx(perspective, GX_PERSPECTIVE);

	srand(time(NULL));
	for(i=0;i<NUM_STARS;i++) {
		stars[i].ang = 0.0f;
		stars[i].dist = (f32)i/(f32)NUM_STARS;
		stars[i].r = rand()%256;			// Give star[loop] A Random Red Intensity
		stars[i].g = rand()%256;			// Give star[loop] A Random Green Intensity
		stars[i].b = rand()%256;			// Give star[loop] A Random Blue Intensity
	}

	guVector starAxis1 = {0,1,0};
	guVector starAxis2 = {0,0,1};
	while(1) {

		WPAD_ScanPads();
		if(WPAD_ButtonsDown(0) & WPAD_BUTTON_HOME) exit(0);

		GX_SetTevOp(GX_TEVSTAGE0,GX_MODULATE);
		GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);

		GX_LoadTexObj(&texture, GX_TEXMAP0);

		for(i=0;i<NUM_STARS;i++) {
			Mtx mry,mrz;

			guMtxIdentity(model);
			guMtxRotAxisDeg(mry, &starAxis1, stars[i].ang);
			guMtxRotAxisDeg(mrz, &starAxis2, spin);
			guMtxConcat(mry,mrz,model);
			guMtxTransApply(model, model, stars[i].dist,0.0f,zoom);
			guMtxConcat(view,model,modelview);

			GX_LoadPosMtxImm(modelview, GX_PNMTX0);

			GX_Begin(GX_QUADS, GX_VTXFMT0, 4);					// Draw a quad
				GX_Position3f32(-1.0f, -1.0f, 0.0f);			// Top Left of the quad (top)
				GX_Color3u8(stars[i].r,stars[i].g,stars[i].b);	// Set The Color To Green
				GX_TexCoord2f32(0.0f,0.0f);
				GX_Position3f32(1.0f, -1.0f, 0.0f);				// Top Right of the quad (top)
				GX_Color3u8(stars[i].r,stars[i].g,stars[i].b);	// Set The Color To Green
				GX_TexCoord2f32(1.0f,0.0f);
				GX_Position3f32(1.0f, 1.0f, 0.0f);				// Bottom Right of the quad (top)
				GX_Color3u8(stars[i].r,stars[i].g,stars[i].b);	// Set The Color To Green
				GX_TexCoord2f32(1.0f,1.0f);
				GX_Position3f32(-1.0f, 1.0f, 0.0f);				// Bottom Left of the quad (top)
				GX_Color3u8(stars[i].r,stars[i].g,stars[i].b);	// Set The Color To Green
				GX_TexCoord2f32(0.0f,1.0f);
			GX_End();											// Done Drawing The Quad 

			spin += 0.01f;
			stars[i].ang += (f32)i/(f32)NUM_STARS;
			stars[i].dist -= 0.01f;

			if(stars[i].dist<0.0f)			// Is The Star In The Middle Yet
			{
				stars[i].dist += 5.0f;			// Move The Star 5 Units From The Center
				stars[i].r = rand()%256;		// Give It A New Red Value
				stars[i].g = rand()%256;		// Give It A New Green Value
				stars[i].b = rand()%256;		// Give It A New Blue Value
			}
		}

		GX_SetZMode(GX_TRUE, GX_LEQUAL, GX_TRUE);
		GX_SetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_ONE, GX_LO_SET);
		GX_SetColorUpdate(GX_TRUE);
		GX_SetAlphaUpdate(GX_TRUE);
		GX_CopyDisp(frameBuffer[fb],GX_TRUE);

		GX_DrawDone();

		VIDEO_SetNextFramebuffer(frameBuffer[fb]);
		if(first_frame) {
			first_frame = 0;
			VIDEO_SetBlack(FALSE);
		}
		VIDEO_Flush();
 		VIDEO_WaitVSync();
		fb ^= 1;
	}
}
