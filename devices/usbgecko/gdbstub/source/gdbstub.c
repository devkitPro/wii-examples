#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <ogcsys.h>
#include <gccore.h>
#include <debug.h>
#include <wiiuse/wpad.h>

static void *xfb = NULL;
static GXRModeObj *rmode = NULL;

int main() {

	VIDEO_Init();
	WPAD_Init();
	
	rmode = VIDEO_GetPreferredMode(NULL);

	xfb = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));
	console_init(xfb,20,20,rmode->fbWidth,rmode->xfbHeight,rmode->fbWidth*VI_DISPLAY_PIX_SZ);
		
	VIDEO_Configure(rmode);
	VIDEO_SetNextFramebuffer(xfb);
	VIDEO_SetBlack(FALSE);
	VIDEO_Flush();
	VIDEO_WaitVSync();
	if(rmode->viTVMode&VI_NON_INTERLACE) VIDEO_WaitVSync();

	/* Configure for use with USB on EXI channel 1 (memcard slot B) */
	/* Other option: GDBSTUB_DEVICE_TCP. Note: second parameter acts as port for this type of device */
	DEBUG_Init(GDBSTUB_DEVICE_USB,1);


	printf("Waiting for debugger ...\n");
	/* This function call enters the debug stub for the first time */
	/* It's needed to call this if one wants to start debugging. */
	_break();

	printf("debugger connected ...\n");
     
	while(1) {

		VIDEO_WaitVSync();
		WPAD_ScanPads();

		int buttons = WPAD_ButtonsDown(0);

		if(buttons & WPAD_BUTTON_A) {
			printf("Button A pressed.\n");
		}

		if (buttons & WPAD_BUTTON_HOME) break;
	}

	return 0;
}
