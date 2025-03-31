#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include <gccore.h>
#include <wiiuse/wpad.h>

static const s32 messageSize 		        = 0x20000;
static void *xfb = NULL;
static GXRModeObj *rmode = NULL;
static void* input = NULL;
static void* buffer = NULL;
static void* iv = NULL;
static sha_context context ATTRIBUTE_ALIGN(32);
static const u8 key[0x10]			        = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F };
static const u8 initialIv[0x10]		        = { 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F };
static const u32 expectedSha1[4] 	        = { 0x9E4EE857, 0x2F0568A6, 0x206743F4, 0x2BD62721 };
static const u32 expectedDecryptedSha1[4] 	= { 0xB3440B4A, 0xE59A2FB4, 0x1A52DEB8, 0x37634C70 };
static u32 hash[4] = {};
void wait_for_input()
{
	while(1) {

		// Call WPAD_ScanPads each loop, this reads the latest controller states
		WPAD_ScanPads();
		PAD_ScanPads();

		// WPAD_ButtonsDown tells us which buttons were pressed in this loop
		// this is a "one shot" state which will not fire again until the button has been released
		u32 pressed = WPAD_ButtonsDown(0);
		u32 gcPressed = PAD_ButtonsDown(0);

		// We return to the launcher application via exit
		if ( pressed & WPAD_BUTTON_HOME || gcPressed & PAD_BUTTON_START ) exit(0);

		// Wait for the next frame
		VIDEO_WaitVSync();
	}
}
void cleanup_and_wait()
{
	if(input != NULL)
		free(input);
	if(buffer != NULL)
		free(buffer);
	AES_Close();
	SHA_Close();
	wait_for_input();
}
//---------------------------------------------------------------------------------
int main(int argc, char **argv) {
//---------------------------------------------------------------------------------

	// Initialise the video system
	VIDEO_Init();

	// This function initialises the attached controllers
	WPAD_Init();
	PAD_Init();

	// Obtain the preferred video mode from the system
	// This will correspond to the settings in the Wii menu
	rmode = VIDEO_GetPreferredMode(NULL);

	// Allocate memory for the display in the uncached region
	xfb = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));

	// Initialise the console, required for printf
	console_init(xfb,20,20,rmode->fbWidth-20,rmode->xfbHeight-20,rmode->fbWidth*VI_DISPLAY_PIX_SZ);

	// Set up the video registers with the chosen mode
	VIDEO_Configure(rmode);

	// Tell the video hardware where our display memory is
	VIDEO_SetNextFramebuffer(xfb);

	// Clear the framebuffer
	VIDEO_ClearFrameBuffer(rmode, xfb, COLOR_BLACK);

	// Make the display visible
	VIDEO_SetBlack(FALSE);

	// Flush the video register changes to the hardware
	VIDEO_Flush();

	// Wait for Video setup to complete
	VIDEO_WaitVSync();
	if(rmode->viTVMode&VI_NON_INTERLACE) VIDEO_WaitVSync();

	// The console understands VT terminal escape codes
	// This positions the cursor on row 2, column 0
	// we can use variables for this with format codes too
	// e.g. printf ("\x1b[%d;%dH", row, column );
	printf("\x1b[2;0H");
	
	// Applications should init the AES & SHA1 code before usage
	if (AES_Init() < 0)
	{
		printf("failed to init AES engine");
		cleanup_and_wait();
	}
	
	if (SHA_Init() < 0)
	{
		printf("failed to init SHA engine");
		cleanup_and_wait();
	}
	
	//the aes engine requires data to be 32-byte aligned.
	input = memalign(32, messageSize);
	buffer = memalign(32, messageSize);
	iv = memalign(32, sizeof(initialIv));
	if(input == NULL || buffer == NULL)
	{
		printf("failed to allocate the buffers");
		cleanup_and_wait();
	}
	
	memset(buffer, 0, messageSize);
	memcpy(iv, initialIv, sizeof(initialIv));
	memset(input, 0xA5A5A5A5, messageSize);
	
	s32 ret = AES_Encrypt(key, 0x10, iv, 0x10, input, buffer, messageSize);
	if(ret < 0)
	{
		printf("failed to encrypt input: %d", ret);
		cleanup_and_wait();
	}
	
	//SHA can be done in a manual way, or automatically.
	//when doing it the manual way, the application needs to manage the context
	//this example uses both, first via the 1-call function and later full manual.
	ret = SHA_Calculate(buffer, messageSize, hash);
	if(ret < 0)
	{
		printf("failed to calculate hash : %d", ret);
		cleanup_and_wait();
	}
	
	printf("calculated hash : %08X%08X%08X%08X\n", hash[0], hash[1], hash[2], hash[3]);
	printf("expected hash   : %08X%08X%08X%08X\n", expectedSha1[0], expectedSha1[1], expectedSha1[2], expectedSha1[3]);
	
	//AES_Encrypt returned the changed IV after encryption, so lets reset the IV and decrypt
	memcpy(iv, initialIv, sizeof(initialIv));
	ret = AES_Decrypt(key, 0x10, iv, 0x10, buffer, input, messageSize);
	if(ret < 0)
	{
		printf("failed to decrypt input");
		cleanup_and_wait();
	}

	for(int i = 0; i*4 < messageSize; i++)
	{
		if(((u32*)input)[i] != 0xA5A5A5A5)
		{
			printf("decrypted data isn't correct : %d - %08X", i, ((u32*)input)[i]);
			cleanup_and_wait();  
		}
	}

	//and this is how to use the SHA interface manually
	if(SHA_InitializeContext(&context) < 0)
	{
		printf("failed to setup sha1 context");
		cleanup_and_wait();
	}

	//adding data can be done part by part
	int i = 0;
	for(; i < 0x400; i+=0x200)
	{
		ret = SHA_Input(&context, input+i, 0x200);
		if(ret < 0)
		{
			printf("failed to calculate hash : %d", ret);
			cleanup_and_wait();
		}
	}

	//and when the last chunk is to be added, call finalize to add the data and return the hash
	ret = SHA_Finalize(&context, input+i, messageSize-i, hash);
	if(ret < 0)
	{
		printf("failed to finalize hash : %d", ret);
		cleanup_and_wait();
	}

	DCFlushRange(hash, sizeof(hash));
	printf("decrypted hash  : %08X%08X%08X%08X\n", hash[0], hash[1], hash[2], hash[3]);
	printf("expected hash   : %08X%08X%08X%08X\n", expectedDecryptedSha1[0], expectedDecryptedSha1[1], expectedDecryptedSha1[2], expectedDecryptedSha1[3]);

	//cleanup everything
	cleanup_and_wait();
	return 0;
}
