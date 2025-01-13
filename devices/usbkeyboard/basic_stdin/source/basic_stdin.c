#include <gccore.h>
#include <wiiuse/wpad.h>
#include <wiikeyboard/keyboard.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static void *xfb = NULL;
static GXRModeObj *rmode = NULL;
bool quitapp=false;

void keyPress_cb(char sym) {
    if (sym == 0x1b) quitapp = true;
}

int main(int argc, char **argv) {
    VIDEO_Init();
    WPAD_Init();
    rmode = VIDEO_GetPreferredMode(NULL);
    xfb = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));
    console_init(xfb,20,20,rmode->fbWidth,rmode->xfbHeight,rmode->fbWidth*VI_DISPLAY_PIX_SZ);
    VIDEO_Configure(rmode);
    VIDEO_SetNextFramebuffer(xfb);
    VIDEO_SetBlack(false);
    VIDEO_Flush();
    VIDEO_WaitVSync();
    if(rmode->viTVMode&VI_NON_INTERLACE) VIDEO_WaitVSync();

    printf("\x1b[2;0HHello World!\n");
    if (KEYBOARD_Init(keyPress_cb) == 0) printf("keyboard initialised\n");

    do {
        WPAD_ScanPads();
        u32 pressed = WPAD_ButtonsDown(0);
        
        int key;
        if((key = getchar()) != EOF) {
            if(key > 31) putchar(key);
            if(key == 13) putchar('\n');
        }
        
        if (pressed & WPAD_BUTTON_HOME) quitapp=true;
        VIDEO_WaitVSync();
    } while(!quitapp);

    return 0;
}
