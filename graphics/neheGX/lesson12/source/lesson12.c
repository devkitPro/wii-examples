/*---------------------------------------------------------------------------------

	nehe lesson 12 port to GX by ccfreak2k
	based on the lesson 6 port by shagkur

---------------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <math.h>
#include <gccore.h>
#include <wiiuse/wpad.h>

#include "Cube_tpl.h"
#include "Cube.h"

#define DEFAULT_FIFO_SIZE	(256*1024)

static GXRModeObj *rmode = NULL;
static void *frameBuffer[2] = { NULL, NULL};

u32 texture[1];   // Storage for one texture
void *boxList[5]; // Storage for the display lists
u32 boxSize[5];   // Real display list sizes
u32 xloop;        // Loop for x axis
u32 yloop;        // Loop for y axis

f32 xrot = 0.0f; // Rotates cube on the x axis
f32 yrot = 0.0f; // Rotates cube on the y axis

/* TODO:
	* Convert colors into indexed arrays.
	* Fix cubes not rotating vertically. */

static f32 BoxColors[5][3] = { // Bright
	{1.0f,0.0f,0.0f}, // Red
	{1.0f,0.5f,0.0f}, // Orange
	{1.0f,1.0f,0.0f}, // Yellow
	{0.0f,1.0f,0.0f}, // Green
	{0.0f,1.0f,1.0f}  // Blue
};

static f32 TopColors[5][3] = { // Dark
	{0.5f,0.0f,0.0f},
	{0.5f,0.25f,0.0f},
	{0.5f,0.5f,0.0f},
	{0.0f,0.5f,0.0f},
	{0.0f,0.5f,0.5f}
};

static GXColor lightColor[] = {
	{0x80,0x80,0x80,0xFF}, // Light color
	{0x80,0x80,0x80,0xFF}, // Ambient color
	{0x80,0x80,0x80,0xFF}  // Mat color
};

void DrawScene  (Mtx view);
int  BuildLists (GXTexObj texture);
void SetLight   (Mtx view);

int main(int argc,char **argv)
{
	f32 yscale;
	u32 xfbHeight;
	u32 fb = 0;
	u32 first_frame = 1;
	GXTexObj texture;
	Mtx view; // view and perspective matrices
	Mtx44 perspective;
	void *gpfifo = NULL;
	GXColor background = {0x00, 0x00, 0x00, 0xFF};
	guVector cam = {0.0F, 0.0F, 0.0F},
			up = {0.0F, 1.0F, 0.0F},
		  look = {0.0F, 0.0F, -1.0F};

	TPLFile cubeTPL;

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
	GX_SetCopyClear(background,0x00FFFFFF);

	// other gx setup
	GX_SetViewport(0,0,rmode->fbWidth,rmode->efbHeight,0,1);
	yscale = GX_GetYScaleFactor(rmode->efbHeight,rmode->xfbHeight);
	xfbHeight = GX_SetDispCopyYScale(yscale);
	GX_SetScissor(0,0,rmode->fbWidth,rmode->efbHeight);
	GX_SetDispCopySrc(0,0,rmode->fbWidth,rmode->efbHeight);
	GX_SetDispCopyDst(rmode->fbWidth,xfbHeight);
	GX_SetCopyFilter(rmode->aa,rmode->sample_pattern,GX_TRUE,rmode->vfilter);
	GX_SetFieldMode(rmode->field_rendering,((rmode->viHeight==2*rmode->xfbHeight)?GX_ENABLE:GX_DISABLE));

	if (rmode->aa) {
		GX_SetPixelFmt(GX_PF_RGB565_Z16, GX_ZC_LINEAR);
	} else {
		GX_SetPixelFmt(GX_PF_RGB8_Z24, GX_ZC_LINEAR);
	}

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
	GX_SetVtxDesc(GX_VA_NRM, GX_DIRECT);
	GX_SetVtxDesc(GX_VA_CLR0, GX_DIRECT);
	GX_SetVtxDesc(GX_VA_TEX0, GX_DIRECT);

	GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
	GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_NRM, GX_NRM_XYZ, GX_F32, 0);
	GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGB, GX_RGB8, 0);
	GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0);

	// set number of rasterized color channels
	GX_SetNumChans(1);

	//set number of textures to generate
	GX_SetNumTexGens(1);

	GX_InvVtxCache();
	GX_InvalidateTexAll();

	TPL_OpenTPLFromMemory(&cubeTPL, (void *)Cube_tpl,Cube_tpl_size);
	TPL_GetTexture(&cubeTPL,cube,&texture);
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

	if (BuildLists(texture)) { // Build the display lists
		exit(1);        // Exit if failed.
	}

	while(1) {

		WPAD_ScanPads();
		if(WPAD_ButtonsDown(0) & WPAD_BUTTON_HOME) exit(0);

		u16 directions = PAD_ButtonsHeld(0);
		if ( directions & PAD_BUTTON_LEFT ) yrot += 0.5f;
		if ( directions & PAD_BUTTON_RIGHT ) yrot -= 0.5f;
		if ( yrot > 360.f ) yrot -= 360.f;
		if ( yrot < 0 ) yrot += 360.f;

		if ( directions & PAD_BUTTON_UP ) xrot -= 0.5f;
		if ( directions & PAD_BUTTON_DOWN ) xrot += 0.5f;
		if ( xrot > 360.f ) xrot -= 360.f;
		if ( xrot < 0 ) xrot += 360.f;

		if(first_frame) {
			first_frame = 0;
			VIDEO_SetBlack(FALSE);
		}

		// draw things
		DrawScene(view);

		GX_SetZMode(GX_TRUE, GX_LEQUAL, GX_TRUE);
		GX_SetColorUpdate(GX_TRUE);
		GX_CopyDisp(frameBuffer[fb],GX_TRUE);

		GX_DrawDone();

		VIDEO_SetNextFramebuffer(frameBuffer[fb]);
		VIDEO_Flush();
		VIDEO_WaitVSync();
		fb ^= 1;
	}
}

void DrawScene(Mtx view) {
	Mtx model,modelview; // Various matrices
	guVector axis;                       // Axis to rotate on

	// BUG: Light ignores underlying polygon colors.
	SetLight(view); // Setup the light

	for (yloop = 1; yloop < 6; yloop++) { // Loop through the y plane
		for (xloop = 0; xloop < yloop; xloop++) { // Loop through the x plane
			// Position the cubes on the screen
			guMtxIdentity(model);

			axis.x = 1.0f;
			axis.y = 0;
			axis.z = 0;
			guMtxRotAxisDeg(model,&axis,(45.0f-(2.0f*(float)yloop)+xrot)); // Tilt the cubes up and down

			axis.x = 0;
			axis.y = 1.0f;
			guMtxRotAxisDeg(model,&axis,(45.0f+yrot)); // Spin cubes left and right

			guMtxTransApply(model,model,(1.4f+((float)xloop*2.8f)-((float)yloop*1.4f)),(((6.0f-(float)yloop)*2.2f)-7.0f),-20.0f);

			guMtxConcat(model,view,modelview);
			GX_LoadPosMtxImm(modelview, GX_PNMTX0);

			GX_CallDispList(boxList[yloop-1],boxSize[yloop-1]); // Draw the box
		}
	}
}

int BuildLists(GXTexObj texture) {
	// Make the new display list
	// For display lists, each command has an associated "cost" in bytes.
	// Add all these up to calculate the size of your display list before rounding up.
	// eke-eke says GX_Begin() costs 3 bytes (u8 + u16)
	// According to my research:
	// GX_Position3f32() is 12 bytes (f32*3)
	// GX_Normal3f32() is 12 bytes (f32*3)
	// GX_Color3f32() is actually 3 bytes ((f32 -> u8) * 3)
	// GX_TexCoord2f32() is 8 bytes (f32*2)
	// GX_End() seems to cost zero (there's no actual code in it)
	// Size -must- be multiple of 32, so (12*24) + (12*24) + (3*24) + (8*24) + 3 = 843
	// Rounded up to the nearest 32 is 864.
	// NOTE: Actual size may be up to 63 bytes -larger- than you calculate it to be due to padding and cache alignment.
	for (int i=0; i<5;i++) {
		boxList[i] = memalign(32,896);
		memset(boxList[i],0,896);
		DCInvalidateRange(boxList[i],896);
		GX_BeginDispList(boxList[i],896);
		GX_Begin(GX_QUADS,GX_VTXFMT0,24); // Start drawing
			// Bottom face
			GX_Position3f32(-1.0f,-1.0f,-1.0f); GX_Normal3f32((f32)0,(f32)0,(f32)1);
			GX_Color3f32(BoxColors[i][0],BoxColors[i][1],BoxColors[i][2]); GX_TexCoord2f32(1.0f,1.0f); // Top right
			GX_Position3f32( 1.0f,-1.0f,-1.0f); GX_Normal3f32((f32)0,(f32)0,(f32)1);
			GX_Color3f32(BoxColors[i][0],BoxColors[i][1],BoxColors[i][2]); GX_TexCoord2f32(0.0f,1.0f); // Top left
			GX_Position3f32( 1.0f,-1.0f, 1.0f); GX_Normal3f32((f32)0,(f32)0,(f32)1);
			GX_Color3f32(BoxColors[i][0],BoxColors[i][1],BoxColors[i][2]); GX_TexCoord2f32(0.0f,0.0f); // Bottom left
			GX_Position3f32(-1.0f,-1.0f, 1.0f); GX_Normal3f32((f32)0,(f32)0,(f32)1);
			GX_Color3f32(BoxColors[i][0],BoxColors[i][1],BoxColors[i][2]); GX_TexCoord2f32(1.0f,0.0f); // Bottom right
			// Front face
			GX_Position3f32(-1.0f,-1.0f, 1.0f); GX_Normal3f32((f32)0,(f32)0,(f32)1);
			GX_Color3f32(BoxColors[i][0],BoxColors[i][1],BoxColors[i][2]); GX_TexCoord2f32(0.0f,0.0f); // Bottom left
			GX_Position3f32( 1.0f,-1.0f, 1.0f); GX_Normal3f32((f32)0,(f32)0,(f32)1);
			GX_Color3f32(BoxColors[i][0],BoxColors[i][1],BoxColors[i][2]); GX_TexCoord2f32(1.0f,0.0f); // Bottom right
			GX_Position3f32( 1.0f, 1.0f, 1.0f); GX_Normal3f32((f32)0,(f32)0,(f32)1);
			GX_Color3f32(BoxColors[i][0],BoxColors[i][1],BoxColors[i][2]); GX_TexCoord2f32(1.0f,1.0f); // Top right
			GX_Position3f32(-1.0f, 1.0f, 1.0f); GX_Normal3f32((f32)0,(f32)0,(f32)1);
			GX_Color3f32(BoxColors[i][0],BoxColors[i][1],BoxColors[i][2]); GX_TexCoord2f32(0.0f,1.0f); // Top left
			// Back face
			GX_Position3f32(-1.0f,-1.0f,-1.0f); GX_Normal3f32((f32)0,(f32)0,(f32)1);
			GX_Color3f32(BoxColors[i][0],BoxColors[i][1],BoxColors[i][2]); GX_TexCoord2f32(1.0f,0.0f); // Bottom right
			GX_Position3f32(-1.0f, 1.0f,-1.0f); GX_Normal3f32((f32)0,(f32)0,(f32)1);
			GX_Color3f32(BoxColors[i][0],BoxColors[i][1],BoxColors[i][2]); GX_TexCoord2f32(1.0f,1.0f); // Top right
			GX_Position3f32( 1.0f, 1.0f,-1.0f); GX_Normal3f32((f32)0,(f32)0,(f32)1);
			GX_Color3f32(BoxColors[i][0],BoxColors[i][1],BoxColors[i][2]); GX_TexCoord2f32(0.0f,1.0f); // Top left
			GX_Position3f32( 1.0f,-1.0f,-1.0f); GX_Normal3f32((f32)0,(f32)0,(f32)1);
			GX_Color3f32(BoxColors[i][0],BoxColors[i][1],BoxColors[i][2]); GX_TexCoord2f32(0.0f,0.0f); // Bottom left
			// Right face
			GX_Position3f32( 1.0f,-1.0f,-1.0f); GX_Normal3f32((f32)0,(f32)0,(f32)1);
			GX_Color3f32(BoxColors[i][0],BoxColors[i][1],BoxColors[i][2]); GX_TexCoord2f32(1.0f,0.0f); // Bottom right
			GX_Position3f32( 1.0f, 1.0f,-1.0f); GX_Normal3f32((f32)0,(f32)0,(f32)1);
			GX_Color3f32(BoxColors[i][0],BoxColors[i][1],BoxColors[i][2]); GX_TexCoord2f32(1.0f,1.0f); // Top right
			GX_Position3f32( 1.0f, 1.0f, 1.0f); GX_Normal3f32((f32)0,(f32)0,(f32)1);
			GX_Color3f32(BoxColors[i][0],BoxColors[i][1],BoxColors[i][2]); GX_TexCoord2f32(0.0f,1.0f); // Top left
			GX_Position3f32( 1.0f,-1.0f, 1.0f); GX_Normal3f32((f32)0,(f32)0,(f32)1);
			GX_Color3f32(BoxColors[i][0],BoxColors[i][1],BoxColors[i][2]); GX_TexCoord2f32(0.0f,0.0f); // Bottom left
			// Left face
			GX_Position3f32(-1.0f,-1.0f,-1.0f); GX_Normal3f32((f32)0,(f32)0,(f32)1);
			GX_Color3f32(BoxColors[i][0],BoxColors[i][1],BoxColors[i][2]); GX_TexCoord2f32(0.0f,0.0f); // Bottom right
			GX_Position3f32(-1.0f,-1.0f, 1.0f); GX_Normal3f32((f32)0,(f32)0,(f32)1);
			GX_Color3f32(BoxColors[i][0],BoxColors[i][1],BoxColors[i][2]); GX_TexCoord2f32(1.0f,0.0f); // Top right
			GX_Position3f32(-1.0f, 1.0f, 1.0f); GX_Normal3f32((f32)0,(f32)0,(f32)1);
			GX_Color3f32(BoxColors[i][0],BoxColors[i][1],BoxColors[i][2]); GX_TexCoord2f32(1.0f,1.0f); // Top left
			GX_Position3f32(-1.0f, 1.0f,-1.0f); GX_Normal3f32((f32)0,(f32)0,(f32)1);
			GX_Color3f32(BoxColors[i][0],BoxColors[i][1],BoxColors[i][2]); GX_TexCoord2f32(0.0f,1.0f); // Bottom left
			// Top face
			GX_Position3f32(-1.0f, 1.0f,-1.0f); GX_Normal3f32((f32)0,(f32)0,(f32)1);
			GX_Color3f32(TopColors[i][0],TopColors[i][1],TopColors[i][2]); GX_TexCoord2f32(0.0f,1.0f); // Top left
			GX_Position3f32(-1.0f, 1.0f, 1.0f); GX_Normal3f32((f32)0,(f32)0,(f32)1);
			GX_Color3f32(TopColors[i][0],TopColors[i][1],TopColors[i][2]); GX_TexCoord2f32(0.0f,0.0f); // Bottom left
			GX_Position3f32( 1.0f, 1.0f, 1.0f); GX_Normal3f32((f32)0,(f32)0,(f32)1);
			GX_Color3f32(TopColors[i][0],TopColors[i][1],TopColors[i][2]); GX_TexCoord2f32(1.0f,0.0f); // Bottom rught
			GX_Position3f32( 1.0f, 1.0f,-1.0f); GX_Normal3f32((f32)0,(f32)0,(f32)1);
			GX_Color3f32(TopColors[i][0],TopColors[i][1],TopColors[i][2]); GX_TexCoord2f32(1.0f,1.0f); // Top right
		GX_End();         // Done drawing quads
		// GX_EndDispList() returns the size of the display list, so store that value and use it with GX_CallDispList().
		boxSize[i] = GX_EndDispList(); // Done building the box list
		if (boxSize[i] == 0) return 1;
	}

	// setup texture coordinate generation
	// args: texcoord slot 0-7, matrix type, source to generate texture coordinates from, matrix to use
	GX_SetTexCoordGen(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_IDENTITY);

	// Set up TEV to paint the textures properly.
	GX_SetTevOp(GX_TEVSTAGE0,GX_MODULATE);
	GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);

	// Load up the textures (just one this time).
	GX_LoadTexObj(&texture, GX_TEXMAP0);

	return 0;
}

void SetLight(Mtx view)
{
	guVector lpos;
	GXLightObj lobj;

	lpos.x = 0;
	lpos.y = 0;
	lpos.z = 2.0f;

	guVecMultiply(view,&lpos,&lpos);

	GX_InitLightPos(&lobj,lpos.x,lpos.y,lpos.z);
	GX_InitLightColor(&lobj,lightColor[0]);
	GX_LoadLightObj(&lobj,GX_LIGHT0);

	// set number of rasterized color channels
	GX_SetNumChans(1);
	GX_SetChanCtrl(GX_COLOR0A0,GX_ENABLE,GX_SRC_VTX,GX_SRC_VTX,GX_LIGHT0,GX_DF_CLAMP,GX_AF_NONE);
	GX_SetChanAmbColor(GX_COLOR0A0,lightColor[1]);
	GX_SetChanMatColor(GX_COLOR0A0,lightColor[2]);
}
