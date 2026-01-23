#include <stdio.h>
#include <string.h>
#include <wiiuse/wpad.h>
#include <ogc/wd.h>

static void *xfb = NULL;
static GXRModeObj *rmode = NULL;

void ParseScanBuff(u8* ScanBuff, u16 ScanBuffSize)
{
    u16 APs = ScanBuff[0] << 8 | ScanBuff[1];
    BSSDescriptor* ptr = (BSSDescriptor*)((u32)ScanBuff + 2);

    // We check if the first two bytes of the buffer aren't 0.
    // The bytes makes a 16 bit value that represents the number of APs detected.
    if(APs) { 
        printf("Found %d APs", APs);
    } else {
        printf("No APs were found.");
    }
    
    for (size_t i = 0; i < APs; i++)
    {
        char ssid[32];

        WD_GetIE(ptr, IEID_SSID, (u8*)ssid, 32); // Can be retrieved with ptr->SSID instead.
        printf("\n\n  AP %d", i + 1);
        printf("\n\tBSSID : %02X:%02X:%02X:%02X:%02X:%02X", ptr->BSSID[0], ptr->BSSID[1], ptr->BSSID[2], ptr->BSSID[3], ptr->BSSID[4], ptr->BSSID[5]);
        printf("\n\tSSID : %s", ssid);
        printf("\n\tStrength : ");
        switch(WD_GetRadioLevel(ptr)) {
            case WD_SIGNAL_STRONG:
                printf("Strong");
            break;

            case WD_SIGNAL_NORMAL:
                printf("Normal");
            break;

            case WD_SIGNAL_FAIR:
                printf("Fair");
            break;

            case WD_SIGNAL_WEAK:
                printf("Weak");
            break;
        }
        printf("\n\tChannel : %d", ptr->channel);
        printf("\n\tIs Encrypted? %s\n", ptr->Capabilities & CAPAB_SECURED_FLAG ? "Yes" : "No");
        
        // Sometimes length can be 0, which is wrong.
        // And in that case we can use ptr->IEs_length to get the correct length. 
        if (ptr->length == 0) {
            if((ptr->IEs_length + 0x3E) % 2 == 0) {
                ptr = (BSSDescriptor*)((u32)ptr + ptr->IEs_length + 0x3E);
            } else {
                ptr = (BSSDescriptor*)((u32)ptr + ptr->IEs_length + 0x3F);
            }            
        } else { // If it's not then we just use it as it is.
            // Doubling ptr->length seems to match the BSSDescriptor length + IEs_length.
            ptr = (BSSDescriptor*)((u32)ptr + ptr->length*2);
        }

        if(APs > 1 && APs > i + 1) {
            printf("\n\tPress A to get the next AP...");
            while(1) {
                WPAD_ScanPads();
                u32 pressed = WPAD_ButtonsDown(0);
                if (pressed & WPAD_BUTTON_A) break;
            }
        }
    }
}

int main(int argc, char *argv[]) {
    VIDEO_Init();

    rmode = VIDEO_GetPreferredMode(NULL);

    xfb = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));

    console_init(xfb,20,20,rmode->fbWidth - 20,rmode->xfbHeight - 20,rmode->fbWidth*VI_DISPLAY_PIX_SZ);

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

    // In order to perform a scan, we need to get the EnableChannels Mask from 
    // WD_GetInfo.
    WD_GetInfo(&inf);

    printf("\n\n\t\tWD Info :\n\n");
    printf("\tIs Initialized? %s\n", inf.initialized ? "Yes" : "No");
    printf("\tChannels Mask 0x%X\n", inf.EnableChannelsMask);
    printf("\tCountry Code : %s\n\n", inf.CountryCode);

    printf("\tPress A on the Wiimote to scan for WiFi Access Points\n");
    printf("\tPress Home to return to loader\n");
    
    ScanParameters Param;

    // We set the default scan parameters :
    WD_SetDefaultScanParameters(&Param);
    
    // We set the channel bitmap with the EnableChannel mask we got from
    // WD_GetInfo.
    Param.ChannelBitmap = inf.EnableChannelsMask; 

    // The buffer that will hold the data comming from WD_ScanOnce().
    // It needs to be big enough to hold the APs data.
    u8 buff[4096]; 
    
    while (1)
    {
        WPAD_ScanPads();
        u32 pressed = WPAD_ButtonsDown(0);
        if (pressed & WPAD_BUTTON_A) {
            printf("\x1b[2J\n\n Scanning...\n");
            // WD_ScanOnce starts by locking WD then it initializes it. 
            WD_ScanOnce(&Param, buff, 4096);
            // After that it performs a scan using WD_Scan which performs an IOCTLV call 
            // to the wd_fd, when it returns the buffer we close WD and unlock the driver
            // and we pass it to ParseScanBuff to print the content :
            ParseScanBuff(buff, 4096);
        }
        if (pressed & WPAD_BUTTON_HOME) break; 
    }

    return 0;
}
