/*---------------------------------------------------------------------------------

	nehe lesson 19 port to GX by ccfreak2k
	Based on the lesson 1 port by WinterMute

---------------------------------------------------------------------------------*/

/*
	D-pad up increases gravity
	D-pad down decreases gravity
	A "holds" particles
	L decreases particle speed
	R increases particle speed
	Y zooms in
	Z zooms out
	B toggles "rainbow mode"
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <math.h>
#include <gccore.h>
#include <wiiuse/wpad.h>


#include "Particle_tpl.h"
#include "Particle.h"

#define DEFAULT_FIFO_SIZE	(256*1024)

#define MAX_PARTICLES 1000

static void *frameBuffer[2] = { NULL, NULL};
GXRModeObj *rmode;

bool rainbow = true; // Rainbow mode

f32 slowdown = 2.0f; // slow down particles
f32 xspeed; // base x speed
f32 yspeed; // base y speed
f32 zoom = -40.0f; // used to zoom out

u32 col; // current color selection
u32 delay = 0; // rainbow effect delay
u32 texture[1]; // storage for our particle texture

// Particle structure
typedef struct
{
	bool active; // active
	f32 life; // particle life
	f32 fade; // fade speed

	// particle color
	f32 r;
	f32 g;
	f32 b;

	// particle position
	f32 x;
	f32 y;
	f32 z;

	// particle position delta (movement in direction)
	f32 xi;
	f32 yi;
	f32 zi;

	// particle gravity
	f32 xg;
	f32 yg;
	f32 zg;
} particles;

static particles particle_array[MAX_PARTICLES];

static f32 colors[12][3] = // rainbow of colors
{
	{1.0f,0.5f,0.5f},{1.0f,0.75f,0.5f},{1.0f,1.0f,0.5f},{0.75f,1.0f,0.5f},
	{0.5f,1.0f,0.5f},{0.5f,1.0f,0.75f},{0.5f,1.0f,1.0f},{0.5f,0.75f,1.0f},
	{0.5f,0.5f,1.0f},{0.75f,0.5f,1.0f},{1.0f,0.5f,1.0f},{1.0f,0.5f,0.75f}
};

// const char *tcp_localip = "10.1.10.103";
// const char *tcp_netmask = "255.0.0.0";
// const char *tcp_gateway = "10.1.10.100";

//---------------------------------------------------------------------------------
int main( int argc, char **argv ){
//---------------------------------------------------------------------------------
	f32 yscale;

	u32 xfbHeight;

	Mtx	view,mv;
	Mtx44 perspective;

	u32	fb = 0; 	// initial framebuffer index
	GXColor background = {0, 0, 0, 0xff};

	GXTexObj texture;
	TPLFile ParticleTPL;

	int i;

	//DEBUG_Init(GDBSTUB_DEVICE_TCP,GDBSTUB_DEF_TCPPORT);
	//_break();

	// init the vi.
	VIDEO_Init();
	WPAD_Init();

	rmode = VIDEO_GetPreferredMode(NULL);

	// allocate 2 framebuffers for double buffering
	frameBuffer[0] = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));
	frameBuffer[1] = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));

	VIDEO_Configure(rmode);
	VIDEO_SetNextFramebuffer(frameBuffer[fb]);
	VIDEO_SetBlack(FALSE);
	VIDEO_Flush();
	VIDEO_WaitVSync();
	if(rmode->viTVMode&VI_NON_INTERLACE) VIDEO_WaitVSync();

	// setup the fifo and then init the flipper
	void *gp_fifo = NULL;
	gp_fifo = memalign(32,DEFAULT_FIFO_SIZE);
	memset(gp_fifo,0,DEFAULT_FIFO_SIZE);

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
	GX_SetVtxDesc(GX_VA_POS,  GX_DIRECT);
	GX_SetVtxDesc(GX_VA_CLR0, GX_DIRECT);
	GX_SetVtxDesc(GX_VA_TEX0, GX_DIRECT);

	GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS,  GX_POS_XYZ,  GX_F32,   0);
	GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);
	GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST,   GX_F32,   0);

	GX_SetNumTexGens(1);
	GX_SetNumChans(1); // You need this for colorized textures to work.

	// setup texture coordinate generation
	// args: texcoord slot 0-7, matrix type, source to generate texture coordinates from, matrix to use
	GX_SetTexCoordGen(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_IDENTITY);

	// Set up TEV to paint the textures properly.
	GX_SetTevOp(GX_TEVSTAGE0,GX_MODULATE);
	GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);

	GX_InvalidateTexAll();

	TPL_OpenTPLFromMemory(&ParticleTPL, (void *)Particle_tpl,Particle_tpl_size);
	TPL_GetTexture(&ParticleTPL,particle,&texture);

	// setup our camera at the origin
	// looking down the -z axis with y up
	guVector cam = {0.0F, 0.0F, 0.0F},
			up = {0.0F, 1.0F, 0.0F},
		  look = {0.0F, 0.0F, -1.0F};
	guLookAt(view, &cam, &up, &look);


	// setup our projection matrix
	// this creates a perspective matrix with a view angle of 90,
	// and aspect ratio based on the display resolution
	f32 w = rmode->viWidth;
	f32 h = rmode->viHeight;
	guMtxIdentity(mv);
	guPerspective(perspective, 45, (f32)w/h, 0.1F, 300.0F);
	GX_LoadProjectionMtx(perspective, GX_PERSPECTIVE);
	GX_LoadTexMtxImm(mv, GX_TEXMTX0, GX_MTX3x4);

	// Initialize all particles.
	for ( i = 0; i < MAX_PARTICLES; i++ )
	{
		particle_array[i].active = true; // Activate all
		particle_array[i].life = 1.0f; // full life time
		particle_array[i].fade = (f32)(rand()%100)/1000.0f+0.003f; // random fade speed
		particle_array[i].r = colors[i*(12/MAX_PARTICLES)][0]; // Select rainbow color
		particle_array[i].g = colors[i*(12/MAX_PARTICLES)][1];
		particle_array[i].b = colors[i*(12/MAX_PARTICLES)][2];
		particle_array[i].xi = (f32)((rand()%50)-26.0f)*10.0f; // Set random velocity
		particle_array[i].yi = (f32)((rand()%50)-25.0f)*10.0f;
		particle_array[i].zi = (f32)((rand()%50)-26.0f)*10.0f;
		particle_array[i].xg = 0.0f; // Set downward gravity.
		particle_array[i].yg = -0.8f;
		particle_array[i].zg = 0.0f;
	}

	while(1) {

		WPAD_ScanPads();
		int pressed = WPAD_ButtonsDown(0);

		if(pressed & WPAD_BUTTON_HOME) exit(0);

		// do this before drawing
		GX_SetViewport(0,0,rmode->fbWidth,rmode->efbHeight,0,1);

		GX_LoadTexObj(&texture, GX_TEXMAP0);

		// This block changes the overall state.
		if ( (pressed & WPAD_BUTTON_PLUS) && (slowdown > 1.0f) ) slowdown -= 0.1f; // Speed up if R is pressed
		if ( (pressed & WPAD_BUTTON_MINUS) && (slowdown < 4.0f) ) slowdown += 0.1f; // Slow down if L is pressed

		if ( pressed & WPAD_BUTTON_1 ) zoom += 0.1f;
		if ( pressed & WPAD_BUTTON_2 ) zoom -= 0.1f;

		if ( pressed & WPAD_BUTTON_B ) rainbow = !rainbow;

		if ( rainbow && (delay > 25) )
		{
			delay = 0;
			col++;
			if ( col > 11 ) col = 0;
		} else delay++;

		// Update all particles.
		for ( i = 0; i < MAX_PARTICLES; i++ )
		{
			if ( particle_array[i].active == TRUE )
			{
				f32 x = particle_array[i].x; // Get the particle values.
				f32 y = particle_array[i].y;
				f32 z = particle_array[i].z + zoom;

				// Draw the particle using our RGB values and fade it based on its life.
				GX_Begin(GX_TRIANGLESTRIP,GX_VTXFMT0,4);
					GX_Position3f32(x+0.5f,y+0.5f,z); // Top right
					GX_Color4u8((u8)(particle_array[i].r * 255.0),(u8)(particle_array[i].g * 255.0),(u8)(particle_array[i].b * 255.0),(u8)(particle_array[i].life * 255.0));
					//GX_Color3f32(particle_array[i].r,particle_array[i].g,particle_array[i].b);
					GX_TexCoord2f32(1.0f,1.0f);
					GX_Position3f32(x-0.5f,y+0.5f,z); // Top left
					GX_Color4u8((u8)(particle_array[i].r * 255.0),(u8)(particle_array[i].g * 255.0),(u8)(particle_array[i].b * 255.0),(u8)(particle_array[i].life * 255.0));
					//GX_Color3f32(particle_array[i].r,particle_array[i].g,particle_array[i].b);
					GX_TexCoord2f32(0.0f,1.0f);
					GX_Position3f32(x+0.5f,y-0.5f,z); // Bottom right
					GX_Color4u8((u8)(particle_array[i].r * 255.0),(u8)(particle_array[i].g * 255.0),(u8)(particle_array[i].b * 255.0),(u8)(particle_array[i].life * 255.0));
					//GX_Color3f32(particle_array[i].r,particle_array[i].g,particle_array[i].b);
					GX_TexCoord2f32(1.0f,0.0f);
					GX_Position3f32(x-0.5f,y-0.5f,z); // Bottom left
					GX_Color4u8((u8)(particle_array[i].r * 255.0),(u8)(particle_array[i].g * 255.0),(u8)(particle_array[i].b * 255.0),(u8)(particle_array[i].life * 255.0));
					//GX_Color3f32(particle_array[i].r,particle_array[i].g,particle_array[i].b);
					GX_TexCoord2f32(0.0f,0.0f);
				GX_End();

				// Move the particle
				particle_array[i].x += particle_array[i].xi / (slowdown * 1000);
				particle_array[i].y += particle_array[i].yi / (slowdown * 1000);
				particle_array[i].z += particle_array[i].zi / (slowdown * 1000);

				// Gravitate the particle
				particle_array[i].xi += particle_array[i].xg;
				particle_array[i].yi += particle_array[i].yg;
				particle_array[i].zi += particle_array[i].zg;

				// Reduce particle life
				particle_array[i].life -= particle_array[i].fade;

				// Reset particle if dead
				if (particle_array[i].life < 0.0f)
				{
					particle_array[i].life = 1.0f;
					particle_array[i].fade = (f32)(rand()%100)/1000.0f+0.003f;
					particle_array[i].x = 0.0f;
					particle_array[i].y = 0.0f;
					particle_array[i].z = 0.0f;
					particle_array[i].xi = xspeed + (f32)((rand()%60)-23.0f);
					particle_array[i].yi = yspeed + (f32)((rand()%60)-30.0f);
					particle_array[i].zi = (f32)((rand()%60)-30.0f);
					particle_array[i].r = colors[col][0];
					particle_array[i].g = colors[col][1];
					particle_array[i].b = colors[col][2];
				}

				// FIXME: Make sure this is the right direction.
				// Increase gravity if down arrow is pressed.
				if ( (pressed & WPAD_BUTTON_DOWN) && (particle_array[i].yg < 1.5f) ) particle_array[i].yg += 0.1f;
				// Decrease if up arrow.
				if ( (pressed & WPAD_BUTTON_UP) && (particle_array[i].yg > -1.5f) ) particle_array[i].yg -= 0.1f;

				if ( pressed & WPAD_BUTTON_A )
				{
					particle_array[i].x = 0.0f;
					particle_array[i].y = 0.0f;
					particle_array[i].z = 0.0f;
					particle_array[i].xi = (f32)((rand()%50)-26.0f)*10.0f;
					particle_array[i].yi = (f32)((rand()%50)-25.0f)*10.0f;
					particle_array[i].zi = (f32)((rand()%50)-26.0f)*10.0f;
				}
			}
		}

		// do this stuff after drawing
		GX_DrawDone();

		fb ^= 1;		// flip framebuffer
		GX_SetZMode(GX_TRUE, GX_LEQUAL, GX_TRUE);
		GX_SetColorUpdate(GX_TRUE);
		GX_CopyDisp(frameBuffer[fb],GX_TRUE);

		VIDEO_SetNextFramebuffer(frameBuffer[fb]);

		VIDEO_Flush();

		VIDEO_WaitVSync();


	}
	return 0;
}

