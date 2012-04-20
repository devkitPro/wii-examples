/*---------------------------------------------------------------------------------

	nehe lesson 6 port to GX by shagkur

---------------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <math.h>
#include <gccore.h>
#include <wiiuse/wpad.h>

#include "crate_tpl.h"
#include "crate.h"

#define DEFAULT_FIFO_SIZE	(256*1024)

typedef struct tagtexdef
{
	void *pal_data;
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

static GXRModeObj *rmode = NULL;
static void *frameBuffer[2] = { NULL, NULL};
//static texdef *txdef = (texdef*)crate0_texture;

int main(int argc,char **argv) {

	f32 yscale,zt = 0;
	u32 xfbHeight;
	u32 fb = 0;
	f32 rquad = 0.0f;
	u32 first_frame = 1;
	GXTexObj texture;
	Mtx view; // view and perspective matrices
	Mtx model, modelview;
	Mtx44 perspective;
	void *gpfifo = NULL;
	GXColor background = {0, 0, 0, 0xff};
	guVector cam = {0.0F, 0.0F, 0.0F},
			up = {0.0F, 1.0F, 0.0F},
		  look = {0.0F, 0.0F, -1.0F};
	TPLFile crateTPL;

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

    GX_InvVtxCache();
	GX_InvalidateTexAll();
	TPL_OpenTPLFromMemory(&crateTPL, (void *)crate_tpl,crate_tpl_size);
	TPL_GetTexture(&crateTPL,crate,&texture);

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

	guVector cubeAxis = {1,1,1};

	while(1) {

		WPAD_ScanPads();
		if(WPAD_ButtonsDown(0) & WPAD_BUTTON_HOME) exit(0);
		else if (WPAD_ButtonsHeld(0)&WPAD_BUTTON_UP) zt -= 0.25f;
		else if (WPAD_ButtonsHeld(0)&WPAD_BUTTON_DOWN) zt += 0.25f;

		// set number of rasterized color channels
		GX_SetNumChans(1);

		//set number of textures to generate
		GX_SetNumTexGens(1);

		// setup texture coordinate generation
		// args: texcoord slot 0-7, matrix type, source to generate texture coordinates from, matrix to use
		GX_SetTexCoordGen(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_IDENTITY);

		GX_SetTevOp(GX_TEVSTAGE0,GX_REPLACE);
		GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);

		GX_LoadTexObj(&texture, GX_TEXMAP0);

		guMtxIdentity(model);
		guMtxRotAxisDeg(model, &cubeAxis, rquad);
		guMtxTransApply(model, model, 0.0f,0.0f,zt-7.0f);
		guMtxConcat(view,model,modelview);
		// load the modelview matrix into matrix memory
		GX_LoadPosMtxImm(modelview, GX_PNMTX3);
		GX_SetCurrentMtx(GX_PNMTX3);

		GX_Begin(GX_QUADS, GX_VTXFMT0, 24);			// Draw a Cube

			GX_Position3f32(-1.0f, 1.0f, -1.0f);	// Top Left of the quad (top)
			GX_Color3f32(0.0f,1.0f,0.0f);			// Set The Color To Green
			GX_TexCoord2f32(0.0f,0.0f);
			GX_Position3f32(-1.0f, 1.0f, 1.0f);	// Top Right of the quad (top)
			GX_Color3f32(0.0f,1.0f,0.0f);			// Set The Color To Green
			GX_TexCoord2f32(1.0f,0.0f);
			GX_Position3f32(-1.0f, -1.0f, 1.0f);	// Bottom Right of the quad (top)
			GX_Color3f32(0.0f,1.0f,0.0f);			// Set The Color To Green
			GX_TexCoord2f32(1.0f,1.0f);
			GX_Position3f32(- 1.0f, -1.0f, -1.0f);		// Bottom Left of the quad (top)
			GX_Color3f32(0.0f,1.0f,0.0f);			// Set The Color To Green
			GX_TexCoord2f32(0.0f,1.0f);

			GX_Position3f32( 1.0f,1.0f, -1.0f);	// Top Left of the quad (bottom)
			GX_Color3f32(1.0f,0.5f,0.0f);			// Set The Color To Orange
			GX_TexCoord2f32(0.0f,0.0f);
			GX_Position3f32(1.0f,-1.0f, -1.0f);	// Top Right of the quad (bottom)
			GX_Color3f32(1.0f,0.5f,0.0f);			// Set The Color To Orange
			GX_TexCoord2f32(1.0f,0.0f);
			GX_Position3f32(1.0f,-1.0f,1.0f);	// Bottom Right of the quad (bottom)
			GX_Color3f32(1.0f,0.5f,0.0f);			// Set The Color To Orange
			GX_TexCoord2f32(1.0f,1.0f);
			GX_Position3f32( 1.0f,1.0f,1.0f);	// Bottom Left of the quad (bottom)
			GX_Color3f32(1.0f,0.5f,0.0f);			// Set The Color To Orange
			GX_TexCoord2f32(0.0f,1.0f);

			GX_Position3f32( -1.0f, -1.0f, 1.0f);		// Top Right Of The Quad (Front)
			GX_Color3f32(1.0f,0.0f,0.0f);			// Set The Color To Red
			GX_TexCoord2f32(0.0f,0.0f);
			GX_Position3f32(1.0f, -1.0f, 1.0f);	// Top Left Of The Quad (Front)
			GX_Color3f32(1.0f,0.0f,0.0f);			// Set The Color To Red
			GX_TexCoord2f32(1.0f,0.0f);
			GX_Position3f32(1.0f,-1.0f, -1.0f);	// Bottom Left Of The Quad (Front)
			GX_Color3f32(1.0f,0.0f,0.0f);			// Set The Color To Red
			GX_TexCoord2f32(1.0f,1.0f);
			GX_Position3f32( -1.0f,-1.0f, -1.0f);	// Bottom Right Of The Quad (Front)
			GX_Color3f32(1.0f,0.0f,0.0f);			// Set The Color To Red
			GX_TexCoord2f32(0.0f,1.0f);

			GX_Position3f32( -1.0f,1.0f,1.0f);	// Bottom Left Of The Quad (Back)
			GX_Color3f32(1.0f,1.0f,0.0f);			// Set The Color To Yellow
			GX_TexCoord2f32(0.0f,0.0f);
			GX_Position3f32(-1.0f,1.0f,-1.0f);	// Bottom Right Of The Quad (Back)
			GX_Color3f32(1.0f,1.0f,0.0f);			// Set The Color To Yellow
			GX_TexCoord2f32(1.0f,0.0f);
			GX_Position3f32(1.0f, 1.0f,-1.0f);	// Top Right Of The Quad (Back)
			GX_Color3f32(1.0f,1.0f,0.0f);			// Set The Color To Yellow
			GX_TexCoord2f32(1.0f,1.0f);
			GX_Position3f32( 1.0f, 1.0f,1.0f);	// Top Left Of The Quad (Back)
			GX_Color3f32(1.0f,1.0f,0.0f);			// Set The Color To Yellow
			GX_TexCoord2f32(0.0f,1.0f);

			GX_Position3f32(1.0f, -1.0f, -1.0f);	// Top Right Of The Quad (Left)
			GX_Color3f32(0.0f,0.0f,1.0f);			// Set The Color To Blue
			GX_TexCoord2f32(0.0f,0.0f);
			GX_Position3f32(1.0f, 1.0f,-1.0f);	// Top Left Of The Quad (Left)
			GX_Color3f32(0.0f,0.0f,1.0f);			// Set The Color To Blue
			GX_TexCoord2f32(1.0f,0.0f);
			GX_Position3f32(-1.0f,1.0f,-1.0f);	// Bottom Left Of The Quad (Left)
			GX_Color3f32(0.0f,0.0f,1.0f);			// Set The Color To Blue
			GX_TexCoord2f32(1.0f,1.0f);
			GX_Position3f32(-1.0f,-1.0f, -1.0f);	// Bottom Right Of The Quad (Left)
			GX_Color3f32(0.0f,0.0f,1.0f);			// Set The Color To Blue
			GX_TexCoord2f32(0.0f,1.0f);

			GX_Position3f32( 1.0f, -1.0f,1.0f);	// Top Right Of The Quad (Right)
			GX_Color3f32(1.0f,0.0f,1.0f);			// Set The Color To Violet
			GX_TexCoord2f32(0.0f,0.0f);
			GX_Position3f32( -1.0f, -1.0f, 1.0f);		// Top Left Of The Quad (Right)
			GX_Color3f32(1.0f,0.0f,1.0f);			// Set The Color To Violet
			GX_TexCoord2f32(1.0f,0.0f);
			GX_Position3f32( -1.0f,1.0f, 1.0f);	// Bottom Left Of The Quad (Right)
			GX_Color3f32(1.0f,0.0f,1.0f);			// Set The Color To Violet
			GX_TexCoord2f32(1.0f,1.0f);
			GX_Position3f32( 1.0f,1.0f,1.0f);	// Bottom Right Of The Quad (Right)		
			GX_Color3f32(1.0f,0.0f,1.0f);			// Set The Color To Violet
			GX_TexCoord2f32(0.0f,1.0f);

		GX_End();									// Done Drawing The Quad 

		GX_SetZMode(GX_TRUE, GX_LEQUAL, GX_TRUE);
		GX_SetColorUpdate(GX_TRUE);
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

		rquad -= 0.15f;				// Decrease The Rotation Variable For The Quad     ( NEW )
	}
}
