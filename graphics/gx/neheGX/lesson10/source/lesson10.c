/*---------------------------------------------------------------------------------

	nehe lesson 10 GX port by ccfreak2k

---------------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <math.h>
#include <gccore.h>
#include <wiiuse/wpad.h>


#include "mud_tpl.h"
#include "mud.h"
#include "world_txt.h"
 
#define DEFAULT_FIFO_SIZE	(256*1024)

/* DATA FILE FORMAT 
Each triangle in our data file is declared as follows:

X1 Y1 Z1 U1 V1
X2 Y2 Z2 U2 V2
X3 Y3 Z3 U3 V3 */
 
static void *frameBuffer[2] = { NULL, NULL};
GXRModeObj *rmode;

f32 xrot;   // x rotation
f32 yrot;   // y rotation
f32 xspeed; // x rotation speed
f32 yspeed; // y rotation speed

f32 walkbias = 0;
f32 walkbiasangle = 0;

f32 lookupdown = 0.0f;

float xpos, zpos;

f32 zdepth=0.0f; // depth into the screen

static GXColor LightColors[] = {
        { 0xFF, 0xFF, 0xFF, 0xFF }, // Light color 1
        { 0x80, 0x80, 0x80, 0xFF }, // Ambient 1
        { 0x80, 0x80, 0x80, 0xFF }  // Material 1
};

// A vertex is the basic element of our room.
typedef struct tagVERTEX // vertex coords - 3d and texture
{
	float x, y, z; // 3d coords
	float u, v;    // tex coords
} VERTEX;

// Triangle is a set of three vertices.
typedef struct tagTRIANGLE // triangle
{
	VERTEX vertex[3]; // 3 vertices
} TRIANGLE;

// Sector represents a room, i.e. series of tris.
typedef struct tagSECTOR
{
	int numtriangles;   // Number of tris in this sector
	TRIANGLE* triangle; // Ptr to array of tris
} SECTOR;

char* worldfile = "/lesson10/world.txt";
SECTOR sector1;

void DrawScene(Mtx v, GXTexObj texture);
void SetLight(Mtx view,GXColor litcol, GXColor ambcol,GXColor matcol);
int SetupWorld(void);
void readstr(FILE *f, char *string);
void initnetwork(void);

//---------------------------------------------------------------------------------
int main( int argc, char **argv ){
//---------------------------------------------------------------------------------
	f32 yscale;

	u32 xfbHeight;

	// various matrices for things like view
	Mtx	view,mv,mr;
	Mtx44 perspective;

	// the texure we're going to paint
	GXTexObj texture;
	TPLFile mudTPL;

	u32	fb = 0; 	// initial framebuffer index
	GXColor background = {0, 0, 0, 0xff};

	// init the vi.
	VIDEO_Init();

	rmode = VIDEO_GetPreferredMode(NULL);
	WPAD_Init();
	
	// allocate 2 framebuffers for double buffering
	frameBuffer[0] = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));
	frameBuffer[1] = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));

	// configure video and wait for the screen to blank
	VIDEO_Configure(rmode);
	VIDEO_SetNextFramebuffer(frameBuffer[fb]);
	VIDEO_SetBlack(FALSE);
	VIDEO_Flush();
	VIDEO_WaitVSync();
	if(rmode->viTVMode&VI_NON_INTERLACE) VIDEO_WaitVSync();

	// setup the fifo...
	void *gp_fifo = NULL;
	gp_fifo = memalign(32,DEFAULT_FIFO_SIZE);
	memset(gp_fifo,0,DEFAULT_FIFO_SIZE);
 
	// ...then init the flipper
	GX_Init(gp_fifo,DEFAULT_FIFO_SIZE);
 
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
	GX_SetTexCoordGen(GX_TEXCOORD0, GX_TG_MTX3x4, GX_TG_TEX0, GX_IDENTITY);

	f32 w = rmode->viWidth;
	f32 h = rmode->viHeight;
	guLightPerspective(mv,45, (f32)w/h, 1.05F, 1.0F, 0.0F, 0.0F);
    guMtxTrans(mr, 0.0F, 0.0F, -1.0F);
    guMtxConcat(mv, mr, mv);
    GX_LoadTexMtxImm(mv, GX_TEXMTX0, GX_MTX3x4);

	GX_InvalidateTexAll();
	TPL_OpenTPLFromMemory(&mudTPL, (void *)mud_tpl,mud_tpl_size);
	TPL_GetTexture(&mudTPL,mud,&texture);
 
	// setup our camera at the origin
	// looking down the -z axis with y up
	guVector cam = {0.0F, 0.0F, 0.0F},
			up = {0.0F, 1.0F, 0.0F},
		  look = {0.0F, 0.0F, -1.0F};
	guLookAt(view, &cam, &up, &look);
 

	// setup our projection matrix
	// this creates a perspective matrix with a view angle of 90,
	// and aspect ratio based on the display resolution
	guPerspective(perspective, 45, (f32)w/h, 0.1F, 300.0F);
	GX_LoadProjectionMtx(perspective, GX_PERSPECTIVE);

	// get the room ready to render
	SetupWorld();
 
	while(1) {

		WPAD_ScanPads();

		s8 tpad = PAD_StickX(0);
		// Rotate left or right.
		if ((tpad < -8) || (tpad > 8)) yrot -= (float)tpad / 50.f;

		// NOTE: walkbiasangle = head bob
		tpad = PAD_StickY(0);
		// Go forward.
		if(tpad > 50) {
			xpos -= (float)sin(DegToRad(yrot)) * 0.05f; // Move on the x-plane based on player direction
			zpos -= (float)cos(DegToRad(yrot)) * 0.05f; // Move on the z-plane based on player direction
			if (walkbiasangle >= 359.0f) walkbiasangle = 0.0f; // Bring walkbiasangle back around
			else walkbiasangle += 10; // if walkbiasangle < 359 increase it by 10
			walkbias = (float)sin(DegToRad(walkbiasangle))/20.0f;
		}

		// Go backward
		if(tpad < -50) {
			xpos += (float)sin(DegToRad(yrot)) * 0.05f;
			zpos += (float)cos(DegToRad(yrot)) * 0.05f;
			if (walkbiasangle <= 1.0f) walkbiasangle = 359.0f;
			else walkbiasangle -= 10;
			walkbias = (float)sin(DegToRad(walkbiasangle))/20.0f;
		}

		tpad = PAD_SubStickY(0);
		// Tilt up/down
		if (((tpad > 8) || (tpad < -8)) && ((90 >= lookupdown) && (lookupdown >= -90))) {
			zdepth -= ((f32)tpad * 0.01f);
			lookupdown -= ((f32)tpad * 0.01f);
			if (lookupdown > 90)  lookupdown = 90.0F;
			if (lookupdown < -90) lookupdown = -90.0F;
		}

		if ( PAD_ButtonsDown(0) & PAD_BUTTON_START) {
			exit(0);
		}

		// do this before drawing
		GX_SetViewport(0,0,rmode->fbWidth,rmode->efbHeight,0,1);

		//set number of textures to generate
		GX_SetNumTexGens(1);

		// Draw things
		DrawScene(view,texture);

		GX_SetZMode(GX_TRUE, GX_LEQUAL, GX_TRUE);
		GX_SetColorUpdate(GX_TRUE);
		GX_CopyDisp(frameBuffer[fb],GX_TRUE);

		// do this stuff after drawing
		GX_DrawDone();
		
		fb ^= 1; // flip framebuffer

		VIDEO_SetNextFramebuffer(frameBuffer[fb]);
 
		VIDEO_Flush();
 
		VIDEO_WaitVSync();


	}
	return 0;
}

// Perform the actual scene drawing.
void DrawScene(Mtx v, GXTexObj texture) {
	// Draw things
	// FIXME: Need to clear first?
	// FIXME: Check datatype sizes
	f32 x_m,y_m,z_m,u_m,v_m;       // Float types for temp x, y, z, u and v vertices
	f32 xtrans = -xpos;            // Used for player translation on the x axis
	f32 ztrans = -zpos;            // Used for player translation on the z axis
	f32 ytrans = -walkbias-0.25f;  // Used for bouncing motion up and down
	f32 sceneroty = 360.0f - yrot; // 360 degree angle for player direction
	int numtriangles;              // Integer to hold the number of triangles
	Mtx m; // Model matrix
	Mtx mt; // Model rotated matrix
	Mtx mv; // Modelview matrix
	guVector axis;                 // Vector for axis we're rotating on

	SetLight(v,LightColors[0],LightColors[1],LightColors[2]);

	// Set up TEV to paint the textures properly.
	GX_SetTevOp(GX_TEVSTAGE0,GX_MODULATE);
	GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);

	// Load up the textures (just one this time).
	GX_LoadTexObj(&texture, GX_TEXMAP0);

	//glRotatef(lookupdown,1.0f,0,0);
	axis.x = 1.0f;
	axis.y = 0;
	axis.z = 0;
	guMtxIdentity(m);
	guMtxRotAxisDeg(m, &axis, lookupdown);
	guMtxConcat(m,v,mv);

	//glrotatef(sceneroty,0,1.0f,0);
	axis.x = 0;
	axis.y = 1.0f;
	axis.z = 0;
	guMtxIdentity(m);
	guMtxRotAxisDeg(m, &axis, sceneroty);
	guMtxConcat(mv,m,mv);

	// Translate the camera view
	guMtxApplyTrans(mv,mt,xtrans,ytrans,ztrans);

	//glTranslatef(xtrans,ytrans,ztrans);
	//guMtxIdentity(m);
	//guMtxTrans(m, xtrans, ytrans, ztrans);	
	//guMtxConcat(v,m,v);

	// load the modelview matrix into matrix memory
	GX_LoadPosMtxImm(mt, GX_PNMTX0);

	numtriangles = sector1.numtriangles;

	// HACK: v tex coord is inverted so textures are rightside up.
	for (int loop_m = 0; loop_m < numtriangles; loop_m++) {
		GX_Begin(GX_TRIANGLES,GX_VTXFMT0,3);
			x_m = sector1.triangle[loop_m].vertex[0].x;
			y_m = sector1.triangle[loop_m].vertex[0].y;
			z_m = sector1.triangle[loop_m].vertex[0].z;
			u_m = sector1.triangle[loop_m].vertex[0].u;
			v_m = sector1.triangle[loop_m].vertex[0].v;
			GX_Position3f32(x_m,y_m,z_m);
			GX_Normal3f32((f32)0,(f32)0,(f32)1);
			//GX_Color3f32(0.7f,0.7f,0.7f);
			GX_TexCoord2f32(u_m,-v_m);

			x_m = sector1.triangle[loop_m].vertex[1].x;
			y_m = sector1.triangle[loop_m].vertex[1].y;
			z_m = sector1.triangle[loop_m].vertex[1].z;
			u_m = sector1.triangle[loop_m].vertex[1].u;
			v_m = sector1.triangle[loop_m].vertex[1].v;
			GX_Position3f32(x_m,y_m,z_m);
			GX_Normal3f32((f32)0,(f32)0,(f32)1);
			//GX_Color3f32(0.7f,0.7f,0.7f);
			GX_TexCoord2f32(u_m,-v_m);

			x_m = sector1.triangle[loop_m].vertex[2].x;
			y_m = sector1.triangle[loop_m].vertex[2].y;
			z_m = sector1.triangle[loop_m].vertex[2].z;
			u_m = sector1.triangle[loop_m].vertex[2].u;
			v_m = sector1.triangle[loop_m].vertex[2].v;
			GX_Position3f32(x_m,y_m,z_m);
			GX_Normal3f32((f32)0,(f32)0,(f32)1);
			//GX_Color3f32(0.7f,0.7f,0.7f);
			GX_TexCoord2f32(u_m,-v_m);
		GX_End();
	}

	return;
}

// This one originally written by shagkur
void SetLight(Mtx view,GXColor litcol, GXColor ambcol,GXColor matcol)
{
	guVector lpos;
	GXLightObj lobj;

	lpos.x = 0;
	lpos.y = 0;
	lpos.z = 2.0f;

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

// Read in and parse world info.
int SetupWorld(void) {
	FILE *filein;
	int numtriangles; // Number of triangles in sector
	char line[255];   // String to store data in
	float x = 0;      // 3D coords
	float y = 0;
	float z = 0;
	float u = 0;      // tex coords
	float v = 0;

	// open file in memory
	filein = fmemopen((void *)world_txt, world_txt_size, "rb");

	// read in data
	readstr(filein, line); // Get single line of data
	sscanf(line, "NUMPOLYS %d\n", &numtriangles); // Read in number of triangles

	// allocate new triangle objects
	sector1.triangle = (TRIANGLE*)malloc(sizeof(TRIANGLE)*numtriangles);
	sector1.numtriangles = numtriangles;

	// Step through each tri in sector
	for (int triloop = 0; triloop < numtriangles; triloop++) {
		// Step through each vertex in tri
		for (int vertloop = 0; vertloop < 3; vertloop++) {
			readstr(filein,line); // Read string
			if (line[0] == '\r' || line[0] == '\n') { // Ignore blank lines.
				vertloop--;
				continue;
			}
			if (line[0] == '/') { // Ignore lines with comments.
				vertloop--;
				continue;
			}
			sscanf(line, "%f %f %f %f %f", &x, &y, &z, &u, &v); // Read in data from string
			// Store values into respective vertices
			sector1.triangle[triloop].vertex[vertloop].x = x;
			sector1.triangle[triloop].vertex[vertloop].y = y;
			sector1.triangle[triloop].vertex[vertloop].z = z;
			sector1.triangle[triloop].vertex[vertloop].u = u;
			sector1.triangle[triloop].vertex[vertloop].v = v;
		}
	}

	fclose(filein);
	return 0;
}

// Read in each line.
void readstr(FILE *f, char *string) {
	do {
		fgets(string, 255, f);
	} while ((string[0] == '/') || (string[0] == '\n'));
	return;
}

