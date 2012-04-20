/*---------------------------------------------------------------------------------

	nehe lesson 8 port to GX by shagkur

---------------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <math.h>
#include <gccore.h>
#include <wiiuse/wpad.h>

#include "glass_tpl.h"
#include "glass.h"
 
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

static GXColor litcolors[] = {
        { 0xD0, 0xD0, 0xD0, 0xFF }, // Light color 1
        { 0x40, 0x40, 0x40, 0xFF }, // Ambient 1
        { 0x80, 0x80, 0x80, 0xFF }  // Material 1
};

static GXRModeObj *rmode = NULL;
static void *frameBuffer[2] = { NULL, NULL};

void setlight(Mtx view,u32 theta,u32 phi,GXColor litcol, GXColor ambcol,GXColor matcol)
{
	guVector lpos;
	f32 _theta,_phi;
	GXLightObj lobj;

	_theta = (f32)theta*M_PI/180.0f;
	_phi = (f32)phi*M_PI/180.0f;
	lpos.x = 1000.0f * cosf(_phi) * sinf(_theta);
	lpos.y = 1000.0f * sinf(_phi);
	lpos.z = 1000.0f * cosf(_phi) * cosf(_theta);

	guVecMultiply(view,&lpos,&lpos);

	GX_InitLightPos(&lobj,lpos.x,lpos.y,lpos.z);
	GX_InitLightColor(&lobj,litcol);
	GX_LoadLightObj(&lobj,GX_LIGHT0);
	
	// set number of rasterized color channels
	GX_SetNumChans(1);
    GX_SetChanCtrl(GX_COLOR0A0,GX_ENABLE,GX_SRC_REG,GX_SRC_REG,GX_LIGHT0,GX_DF_CLAMP,GX_AF_NONE);
    GX_SetChanAmbColor(GX_COLOR0A0,ambcol);
    GX_SetChanMatColor(GX_COLOR0A0,matcol);
}

int main(int argc,char **argv)
{
	f32 yscale,zt = 0;
	u32 xfbHeight;
	u32 fb = 0;
	f32 rquad = 0.0f;
	u32 first_frame = 1;
	GXTexObj texture;
	Mtx view,mv,mr,mvi; // view and perspective matrices
	Mtx model, modelview;
	Mtx44 perspective;
	void *gpfifo = NULL;
	GXColor background = {0, 0, 0, 0xff};
	guVector cam = {0.0F, 0.0F, 0.0F},
			up = {0.0F, 1.0F, 0.0F},
		  look = {0.0F, 0.0F, -1.0F};
	TPLFile glassTPL;

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
    GX_InvVtxCache();
	GX_ClearVtxDesc();
	GX_SetVtxDesc(GX_VA_POS, GX_DIRECT);
	GX_SetVtxDesc(GX_VA_NRM, GX_DIRECT);
	GX_SetVtxDesc(GX_VA_TEX0, GX_DIRECT);

	GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
	GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_NRM, GX_NRM_XYZ, GX_F32, 0);
	GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0);

	// setup texture coordinate generation
	// args: texcoord slot 0-7, matrix type, source to generate texture coordinates from, matrix to use
	GX_SetTexCoordGen(GX_TEXCOORD0, GX_TG_MTX3x4, GX_TG_TEX0, GX_TEXMTX0);

	f32 w = rmode->viWidth;
	f32 h = rmode->viHeight;
	guLightPerspective(mv,45, (f32)w/h, 1.05F, 1.0F, 0.0F, 0.0F);
    guMtxTrans(mr, 0.0F, 0.0F, -1.0F);
    guMtxConcat(mv, mr, mv);
    GX_LoadTexMtxImm(mv, GX_TEXMTX0, GX_MTX3x4);

	GX_InvalidateTexAll();
	TPL_OpenTPLFromMemory(&glassTPL, (void *)glass_tpl,glass_tpl_size);
	TPL_GetTexture(&glassTPL,glass,&texture);
	
	// setup our camera at the origin
	// looking down the -z axis with y up
	guLookAt(view, &cam, &up, &look);
 
	// setup our projection matrix
	// this creates a perspective matrix with a view angle of 90,
	// and aspect ratio based on the display resolution
	guPerspective(perspective, 45, (f32)w/h, 0.1F, 300.0F);
	GX_LoadProjectionMtx(perspective, GX_PERSPECTIVE);

	guVector cubeAxis = {1,1,1};

	while(1) {
		WPAD_ScanPads();
		if(WPAD_ButtonsDown(0) & WPAD_BUTTON_HOME) exit(0);
		else if (WPAD_ButtonsHeld(0)&WPAD_BUTTON_UP) zt -= 0.25f;
		else if (WPAD_ButtonsHeld(0)&WPAD_BUTTON_DOWN) zt += 0.25f;

		// set number of rasterized color channels
		//GX_SetNumChans(1);
		setlight(view,8,20,litcolors[0],litcolors[1],litcolors[2]);

		//set number of textures to generate
		GX_SetNumTexGens(1);

		GX_SetTevOp(GX_TEVSTAGE0,GX_BLEND);
		GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);

		GX_LoadTexObj(&texture, GX_TEXMAP0);

		guMtxIdentity(model);
		guMtxRotAxisDeg(model, &cubeAxis, rquad);
		guMtxTransApply(model, model, 0.0f,0.0f,zt-7.0f);
		guMtxConcat(view,model,modelview);

		// load the modelview matrix into matrix memory
		GX_LoadPosMtxImm(modelview, GX_PNMTX0);
		
		guMtxInverse(modelview,mvi);
		guMtxTranspose(mvi,modelview);
	    GX_LoadNrmMtxImm(modelview, GX_PNMTX0);

		GX_Begin(GX_QUADS, GX_VTXFMT0, 24);			// Draw a Cube

			GX_Position3f32(-1.0f,1.0f,1.0f);	// Top Left of the quad (top)
			GX_Normal3f32(-1.0f,1.0f,1.0f);
			GX_TexCoord2f32(1.0f,0.0f);
			GX_Position3f32(1.0f,1.0f,1.0f);	// Top Right of the quad (top)
			GX_Normal3f32(1.0f,1.0f,1.0f);	// Top Right of the quad (top)
			GX_TexCoord2f32(1.0f,1.0f);
			GX_Position3f32(1.0f,1.0f,-1.0f);	// Bottom Right of the quad (top)
			GX_Normal3f32(1.0f,1.0f,-1.0f);	// Bottom Right of the quad (top)
			GX_TexCoord2f32(0.0f,1.0f);
			GX_Position3f32(-1.0f,1.0f,-1.0f);		// Bottom Left of the quad (top)
			GX_Normal3f32(-1.0f,1.0f,-1.0f);		// Bottom Left of the quad (top)
			GX_TexCoord2f32(0.0f,0.0f);

			GX_Position3f32(-1.0f,-1.0f,1.0f);	// Top Left of the quad (bottom)
			GX_Normal3f32(-1.0f,-1.0f,1.0f);	// Top Left of the quad (bottom)
			GX_TexCoord2f32(1.0f,0.0f);
			GX_Position3f32(1.0f,-1.0f,1.0f);	// Top Right of the quad (bottom)
			GX_Normal3f32(1.0f,-1.0f,1.0f);	// Top Right of the quad (bottom)
			GX_TexCoord2f32(1.0f,1.0f);
			GX_Position3f32(1.0f,-1.0f,-1.0f);	// Bottom Right of the quad (bottom)
			GX_Normal3f32(1.0f,-1.0f,-1.0f);	// Bottom Right of the quad (bottom)
			GX_TexCoord2f32(0.0f,1.0f);
			GX_Position3f32(-1.0f,-1.0f,-1.0f);		// Bottom Left of the quad (bottom)
			GX_Normal3f32(-1.0f,-1.0f,-1.0f);		// Bottom Left of the quad (bottom)
			GX_TexCoord2f32(0.0f,0.0f);

			GX_Position3f32(-1.0f,1.0f,1.0f);	// Top Left of the quad (front)
			GX_Normal3f32(-1.0f,1.0f,1.0f);	// Top Left of the quad (front)
			GX_TexCoord2f32(1.0f,0.0f);
			GX_Position3f32(-1.0f,-1.0f,1.0f);	// Top Right of the quad (front)
			GX_Normal3f32(-1.0f,-1.0f,1.0f);	// Top Right of the quad (front)
			GX_TexCoord2f32(1.0f,1.0f);
			GX_Position3f32(1.0f,-1.0f,1.0f);	// Bottom Right of the quad (front)
			GX_Normal3f32(1.0f,-1.0f,1.0f);	// Bottom Right of the quad (front)
			GX_TexCoord2f32(0.0f,1.0f);
			GX_Position3f32(1.0f,1.0f,1.0f);		// Bottom Left of the quad (front)
			GX_Normal3f32(1.0f,1.0f,1.0f);		// Bottom Left of the quad (front)
			GX_TexCoord2f32(0.0f,0.0f);

			GX_Position3f32(-1.0f,1.0f,-1.0f);	// Top Left of the quad (back)
			GX_Normal3f32(-1.0f,1.0f,-1.0f);	// Top Left of the quad (back)
			GX_TexCoord2f32(1.0f,0.0f);
			GX_Position3f32(-1.0f,-1.0f,-1.0f);	// Top Right of the quad (back)
			GX_Normal3f32(-1.0f,-1.0f,-1.0f);	// Top Right of the quad (back)
			GX_TexCoord2f32(1.0f,1.0f);
			GX_Position3f32(1.0f,-1.0f,-1.0f);	// Bottom Right of the quad (back)
			GX_Normal3f32(1.0f,-1.0f,-1.0f);	// Bottom Right of the quad (back)
			GX_TexCoord2f32(0.0f,1.0f);
			GX_Position3f32(1.0f,1.0f,-1.0f);		// Bottom Left of the quad (back)
			GX_Normal3f32(1.0f,1.0f,-1.0f);		// Bottom Left of the quad (back)
			GX_TexCoord2f32(0.0f,0.0f);

			GX_Position3f32(-1.0f,1.0f,1.0f);	// Top Left of the quad (left)
			GX_Normal3f32(-1.0f,1.0f,1.0f);	// Top Left of the quad (left)
			GX_TexCoord2f32(1.0f,0.0f);
			GX_Position3f32(-1.0f,1.0f,-1.0f);	// Top Right of the quad (back)
			GX_Normal3f32(-1.0f,1.0f,-1.0f);	// Top Right of the quad (back)
			GX_TexCoord2f32(1.0f,1.0f);
			GX_Position3f32(-1.0f,-1.0f,-1.0f);	// Bottom Right of the quad (back)
			GX_Normal3f32(-1.0f,-1.0f,-1.0f);	// Bottom Right of the quad (back)
			GX_TexCoord2f32(0.0f,1.0f);
			GX_Position3f32(-1.0f,-1.0f,1.0f);		// Bottom Left of the quad (back)
			GX_Normal3f32(-1.0f,-1.0f,1.0f);		// Bottom Left of the quad (back)
			GX_TexCoord2f32(0.0f,0.0f);

			GX_Position3f32(1.0f,1.0f,1.0f);	// Top Left of the quad (right)
			GX_Normal3f32(1.0f,1.0f,1.0f);	// Top Left of the quad (right)
			GX_TexCoord2f32(1.0f,0.0f);
			GX_Position3f32(1.0f,1.0f,-1.0f);	// Top Right of the quad (right)
			GX_Normal3f32(1.0f,1.0f,-1.0f);	// Top Right of the quad (right)
			GX_TexCoord2f32(1.0f,1.0f);
			GX_Position3f32(1.0f,-1.0f,-1.0f);	// Bottom Right of the quad (right)
			GX_Normal3f32(1.0f,-1.0f,-1.0f);	// Bottom Right of the quad (right)
			GX_TexCoord2f32(0.0f,1.0f);
			GX_Position3f32(1.0f,-1.0f,1.0f);		// Bottom Left of the quad (right)
			GX_Normal3f32(1.0f,-1.0f,1.0f);		// Bottom Left of the quad (right)
			GX_TexCoord2f32(0.0f,0.0f);

		GX_End();									// Done Drawing The Quad 

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

		rquad -= 0.15f;				// Decrease The Rotation Variable For The Quad     ( NEW )
	}
}
