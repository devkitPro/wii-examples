// basic_threading.c - Basic usage of the LWP threading system.
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <time.h>
#include <gccore.h>
#include <wiiuse/wpad.h>
#include <ogc/lwp.h>

// Framebuffer and Render Mode.
static void *xfb = NULL;
static GXRModeObj *rmode = NULL;

// Initialization.
void Init();

// Example Data Type
typedef struct {
    int id;     // ID.
    int tg;     // Variable Target.
    int var;    // Some Variable.
} demo_data_t;

// Thread Delay Time
#define THREAD_SLEEP_TIME 1000*1000 // One Second

// Thread Context Handles.
static lwp_t handle_a = LWP_THREAD_NULL;
static lwp_t handle_b = LWP_THREAD_NULL;

// Example Thread
//  void* arg - Data to pass to the thread.
static void* demo_thread(void* arg) {
    // Get the data
    demo_data_t* demo_data;
    if(arg != NULL) demo_data = (demo_data_t*)arg;
    else return NULL;

    // Pre-Loop Code.
    printf("%s(#%d): Started!!! Reach Target: %d.\n",
        __FUNCTION__,
        demo_data->id,
        demo_data->tg
    );

    // Thread Loop
    bool thread_done = false;
    while(!thread_done) {
        // Thread Code
        printf("%s(#%d): var = %d.\n",
            __FUNCTION__,
            demo_data->id,
            demo_data->var
        );

        // Increment the variable until it reaches target.
        demo_data->var++;
        if(demo_data->var > demo_data->tg) {
            thread_done = true; // Reached Target.
        }

        // Thread Delay
        usleep(THREAD_SLEEP_TIME);
    }

    // Finished!
    printf("%s(#%d): Finished!\n",
        __FUNCTION__,
        demo_data->id
    );
    return NULL; // Thread Done!
}

// Main Code.
int main(int argc, char** argv) {
    // Initialize.
    Init();

    // Create some demo data.
    demo_data_t* demo_data = malloc(sizeof(demo_data_t) * 2); // Data for 2 threads.
    for(int idx=0; idx < 2; idx++) {
        printf("Setting up demo data[%d]...\n", idx);
        demo_data[idx].id = idx+1;          // Set ID
        demo_data[idx].tg = rand() % 10;    // Random Target Value.
    }

    // Init Thread Handles.
    // Thread Handle A
    if(LWP_CreateThread(
        &handle_a,              // Thread Handle
        demo_thread,            // Thread Function
        (void*)&demo_data[0],   // Argument Data

        NULL,0,                 // Stack and Stack Size.
        LWP_PRIO_HIGHEST        // Thread Priority.
    ) != LWP_SUCCESSFUL) fprintf(stderr, "Error setting up demo HandleA\n");

    // Thread Handle B
    if(LWP_CreateThread(
        &handle_b,
        demo_thread,
        (void*)&demo_data[1],

        NULL,0,
        LWP_PRIO_HIGHEST
    ) != LWP_SUCCESSFUL) fprintf(stderr, "Error setting up demo HandleB\n");

    // Main Loop.
    bool quit=false;
    while(!quit) {
        // Scan WPAD
        WPAD_ScanPads();
        s32 pressed = WPAD_ButtonsDown(WPAD_CHAN_0);
        if(pressed & WPAD_BUTTON_HOME) quit=true;

        // Stop Threads
        if(pressed & WPAD_BUTTON_B) {
            // Thread A
            if(!LWP_ThreadIsSuspended(handle_a)) { 
                if(LWP_SuspendThread(handle_a) != LWP_SUCCESSFUL)
                    fprintf(stderr, "Error suspending HandleA\n");
                else
                    printf("Suspended Thread Handle A\n");
            }
            else
                printf("Thread Handle A is already suspended.\n");
            
            // Thread B
            if(!LWP_ThreadIsSuspended(handle_b)) {
                if(LWP_SuspendThread(handle_b) != LWP_SUCCESSFUL)
                    fprintf(stderr, "Error suspending HandleB\n");
                else
                    printf("Suspended Thread Handle B\n");
            }
            else
                printf("Thread Handle B is already suspended.\n");
        }

        // Resume Threads.
        if(pressed & WPAD_BUTTON_A) {
            // Thread A
            if(LWP_ThreadIsSuspended(handle_a)) {
                if(LWP_ResumeThread(handle_a) != LWP_SUCCESSFUL)
                    fprintf(stderr, "Error resuming HandleA\n");
                else
                    printf("Resumed Thread Handle A\n"); 
            }
            else
                printf("Thread Handle A isn't suspended.\n");


            // Thread B
            if(LWP_ThreadIsSuspended(handle_b)) {
                if(LWP_ResumeThread(handle_b) != LWP_SUCCESSFUL)
                    fprintf(stderr, "Error resuming HandleB\n");
                else
                    printf("Resumed Thread Handle B\n");
            }
            else
                printf("Thread Handle B isn't suspended.\n");
        }

        // Loop Delay.
        usleep(100);
    }
}

// Init Code.
void Init() {
	// Initialise the video system
	VIDEO_Init();

	// This function initialises the attached controllers
	WPAD_Init();

	// Obtain the preferred video mode from the system
	// This will correspond to the settings in the Wii menu
	rmode = VIDEO_GetPreferredMode(NULL);

	// Allocate memory for the display in the uncached region
	xfb = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));

	// Initialise the console, required for printf
	console_init(xfb,20,20,rmode->fbWidth,rmode->xfbHeight,rmode->fbWidth*VI_DISPLAY_PIX_SZ);
	//SYS_STDIO_Report(true);

	// Set up the video registers with the chosen mode
	VIDEO_Configure(rmode);

	// Tell the video hardware where our display memory is
	VIDEO_SetNextFramebuffer(xfb);

	// Make the display visible
	VIDEO_SetBlack(false);

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

    // Seed the RNG
    srand(time(NULL));

    // Welcome
    printf("LWP Threading Demo\n");
    printf("Controls:\n\tB - Suspend Ongoing Threads\n\tA - Resume Suspended Threads\n\tHOME - Exit\n");
    usleep(1000*1000); // Wait one second.
}