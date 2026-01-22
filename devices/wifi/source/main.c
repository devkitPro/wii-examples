#include <stdio.h>
#include <string.h>
#include <wiiuse/wpad.h>
#include <ogc/wd.h>
#include <fat.h>

static void *xfb = NULL;
static GXRModeObj *rmode = NULL;

void ParseScanBuff(u8* ScanBuff, u16 ScanBuffSize)
{
    u16 APs = ScanBuff[0] << 8 | ScanBuff[1];
    BSSDescriptor* ptr = (BSSDescriptor*)((u32)ScanBuff + 2);
    if(APs) {
        printf("Found %d APs", APs);
    } else {
        printf("No APs were found.");
    }
    
    for (size_t i = 0; i < APs; i++)
    {
        char ssid[32];

        WD_GetIE(ptr, IEID_SSID, (u8*)ssid, 32);
        printf("\n\n  AP %d", i + 1);
        printf("\n\tBSSID : %02X:%02X:%02X:%02X:%02X:%02X", ptr->BSSID[0], ptr->BSSID[1], ptr->BSSID[2], ptr->BSSID[3], ptr->BSSID[4], ptr->BSSID[5]);
        printf("\n\tSSID : %s", ssid);
        printf("\n\tRSSI : -%d", 65535 - ptr->RSSI);
        printf("\n\tChannel : %d", ptr->channel);
        printf("\n\tNum of IEs : %d", WD_GetNumberOfIEs(ptr));
        u8 len = 0;
        WD_GetIELength(ptr, 0x30, &len);
        printf("\n\tIE 0x30 size : 0x%X", len);
        printf("\n\tIs Encrypted? %s\n", ptr->Capabilities & CAPAB_SECURED_FLAG ? "Yes" : "No");
        if (ptr->length == 0) {
            if((ptr->IEs_length + 0x3E) % 2 == 0) {
                ptr = (BSSDescriptor*)((u32)ptr + ptr->IEs_length + 0x3E);
            } else {
                ptr = (BSSDescriptor*)((u32)ptr + ptr->IEs_length + 0x3F);
            }            
        } else {
            ptr = (BSSDescriptor*)((u32)ptr + ptr->length*2);
        }     
    }
}

int main(int argc, char *argv[]) {
    VIDEO_Init();

    rmode = VIDEO_GetPreferredMode(NULL);

    xfb = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));

    console_init(xfb,20,20,rmode->fbWidth,rmode->xfbHeight,rmode->fbWidth*VI_DISPLAY_PIX_SZ);

    VIDEO_Configure(rmode);

    VIDEO_SetNextFramebuffer(xfb);

    VIDEO_ClearFrameBuffer(rmode, xfb, COLOR_BLACK);

    VIDEO_SetBlack(false);

    VIDEO_Flush();

    VIDEO_WaitVSync();
    if(rmode->viTVMode&VI_NON_INTERLACE) VIDEO_WaitVSync();

    WPAD_Init();

    printf("\n  WiFi (WD) Scan Example\n");
    printf("  Created by Abdelali221.");

    WDInfo inf;

    WD_GetInfo(&inf);

    printf("\n\n\t\tWD Info :\n\n");
    printf("\tIs Initialized? %s\n", inf.initialized ? "Yes" : "No");
    printf("\tChannels Mask 0x%X\n", inf.EnableChannelsMask);
    printf("\tCountry Code : %s\n", inf.CountryCode);
    printf("Press A on the wiimote to scan for wifi SSID's'\n");
    printf("Press Home to return to loader\n");    ScanParameters Param;
    WD_SetDefaultScanParameters(&Param);
    Param.ChannelBitmap = inf.EnableChannelsMask;
    u8 buff[4096] __attribute__((aligned(32)));
    
    while (1)
    {
        WPAD_ScanPads();
        u32 pressed = WPAD_ButtonsDown(0);
        if (pressed & WPAD_BUTTON_A) {
            printf("\n\n Scanning...\n");
            WD_ScanOnce(&Param, buff, 4096);
            ParseScanBuff(buff, 4096);
        }
        if (pressed & WPAD_BUTTON_HOME) break; 
    }

    return 0;
}