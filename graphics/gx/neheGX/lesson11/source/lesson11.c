/*---------------------------------------------------------------------------------

	nehe lesson 11 port to GX by ccfreak2k
	based on the lesson 6 port by shagkur

---------------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <math.h>
#include <gccore.h>
#include <wiiuse/wpad.h>

#include "Tim_tpl.h"
#include "Tim.h"

#define DEFAULT_FIFO_SIZE	(256*1024)

static GXRModeObj *rmode = NULL;
static void *frameBuffer[2] = { NULL, NULL};

f32 points[45][45][3]; // The array for the points on the grid of our "wave"

void DrawFlag(Mtx view, GXTexObj texture);

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
	GXColor background = {0, 0, 0, 0xff};
	guVector cam = {0.0F, 0.0F, 0.0F},
			up = {0.0F, 1.0F, 0.0F},
		  look = {0.0F, 0.0F, -1.0F};

	TPLFile timTPL;

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
	GX_SetVtxDesc(GX_VA_TEX0, GX_DIRECT);

	GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
	GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0);

	// set number of rasterized color channels
	GX_SetNumChans(1);

	//set number of textures to generate
	GX_SetNumTexGens(1);

	// setup texture coordinate generation
	// args: texcoord slot 0-7, matrix type, source to generate texture coordinates from, matrix to use
	GX_SetTexCoordGen(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_IDENTITY);

	GX_InvVtxCache();
	GX_InvalidateTexAll();

	TPL_OpenTPLFromMemory(&timTPL, (void *)Tim_tpl,Tim_tpl_size);
	TPL_GetTexture(&timTPL,tim,&texture);
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

	// Setup flag roll
	for(int x = 0; x < 45; x++) { // Loop through the x plane
		for (int y = 0; y < 45; y++) {// Loop through the y plane
			// Apply the wave to our mesh
			points[x][y][0] = (f32)((x/5.0f)-4.5f);
			points[x][y][1] = (f32)((y/5.0f)-4.5f);
			points[x][y][2] = (f32)(sin(DegToRad((x/5.0f)*40.0f)));
		}
	}

	while(1) {

		WPAD_ScanPads();
		if(WPAD_ButtonsDown(0) & WPAD_BUTTON_HOME) exit(0);

		if(first_frame) {
			first_frame = 0;
			VIDEO_SetBlack(FALSE);
		}

		// draw things
		DrawFlag(view,texture);

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

void DrawFlag(Mtx view, GXTexObj texture) {
	int x,y;                               // Loop vars
	f32 float_x,float_y,float_xb,float_yb; // Used to break the flag into tiny quads
	Mtx model1,model2,model12,modelview;   // Model and model+view matrices
	guVector axis;                         // Axes to spin on.
	static float xrot = 0;                 // Rotation values
	static float yrot = 0;
	static float zrot = 0;
	static int wiggle_count = 0;           // Counter used to control how fast flag waves
	f32 hold;                              // Temporarily holds a floating-point value

	axis.x = 0;
	axis.y = 0;
	axis.z = 0;

	// glTranslatef(0.0f,0.0f,-12.0f);				// Translate 12 Units Into The Screen

	// glRotatef(xrot,1.0f,0.0f,0.0f);				// Rotate On The X Axis
	// glRotatef(yrot,0.0f,1.0f,0.0f);				// Rotate On The Y Axis
	// glRotatef(zrot,0.0f,0.0f,1.0f);				// Rotate On The Z Axis

	guMtxIdentity(model2);

	axis.x = 1.0f; // Rotate on the x axis
	guMtxIdentity(model1);
	guMtxRotAxisDeg(model1,&axis,xrot);
	guMtxConcat(model1,model2,model12);
	axis.x = 0;

	axis.y = 1.0f; // Rotate on the y axis
	guMtxIdentity(model1);
	guMtxRotAxisDeg(model1,&axis,yrot);
	guMtxConcat(model1,model12,model2);
	axis.y = 0;

	axis.z = 1.0f; // Rotate on the z axis
	guMtxIdentity(model1);
	guMtxRotAxisDeg(model1,&axis,zrot);
	guMtxConcat(model1,model2,model12);

	guMtxTransApply(model12,model12,0,0,-12.0f); // Translate 12 units into the screen

	// cat model and view, then load it into memory
	guMtxConcat(model12,view,modelview);
	GX_LoadPosMtxImm(modelview,GX_PNMTX0);

	// Set up the TEV operations
	GX_SetTevOp(GX_TEVSTAGE0,GX_REPLACE);
	GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);

	// Load in the texture
	GX_LoadTexObj(&texture, GX_TEXMAP0);

	// Start drawing quads
	for (x = 0; x < 44; x++) {
		for (y = 0; y < 44; y++) {
			float_x = (f32)(x)/44.0f; // Create floating point values
			float_y = (f32)(y)/44.0f; // These are used for the tex coords
			float_xb = (f32)(x+1)/44.0f;
			float_yb = (f32)(y+1)/44.0f;

			GX_Begin(GX_QUADS,GX_VTXFMT0,4);
			// FIXME: Make sure render order is correct (clockwise or counterclock, etc)
			GX_Position3f32(points[x][y][0],points[x][y][1],points[x][y][2]);
			GX_TexCoord2f32(float_x,float_y); // Tex coords: bottom left

			GX_Position3f32(points[x][y+1][0],points[x][y+1][1],points[x][y+1][2]);
			GX_TexCoord2f32(float_x,float_yb); // top left

			GX_Position3f32(points[x+1][y+1][0],points[x+1][y+1][1],points[x+1][y+1][2]);
			GX_TexCoord2f32(float_xb,float_yb); // top right

			GX_Position3f32(points[x+1][y][0],points[x+1][y][1],points[x+1][y][2]);
			GX_TexCoord2f32(float_xb,float_y); // bottom right
			GX_End();
		}
	}

	//
	if (wiggle_count == 2 ) {
		for (y = 0; y < 45; y++) { // Loop through the y plane
			hold = points[0][y][2]; // Store current value one left side of wave
			for (x = 0; x < 44; x++) {
				// Current wave value equals value to the right
				points[x][y][2] = points[x+1][y][2];
			}
			points[44][y][2] = hold; // Last value becomes the far left stored value
		}
		wiggle_count = 0; // Set counter back to zero
	} else { wiggle_count++; }

	xrot += 0.3f; // Increase rotation values
	yrot += 0.2f;
	zrot += 0.4f;

	if (xrot > 360.0f) xrot -= 360.0f;
	if (yrot > 360.0f) yrot -= 360.0f;
	if (zrot > 360.0f) zrot -= 360.0f;
}
