// nand_browser - Demo ISFS to browse your NAND's files.
// Fairly long script, but the comments should help you understand.

// Basic Includes.
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <malloc.h>
#include <time.h>

// DevKitPPC Wii
#include <gccore.h>
#include <ogc/ios.h>
#include <ogc/isfs.h>
#include <ogc/system.h>
#include <wiiuse/wpad.h>

// ISFS Directory Reading Definitions.
#define FLAG_DIR 1                                  // Constant Value of a directory flag.
#define DIR_SEPARATOR '/'                           // Directory Seperator.
static fstats filest __attribute__((aligned(32)));  // Aligned file status variable.

// Directory Entry Structure.
typedef struct DIR_ENTRY_STRUCT {
	char *name;                         // Entry Name.
	char *abspath;                      // Absolute Path of Entry.
	u32 size;                           // Entry File Size.
	u8 flags;                           // Entry Flags.
	u32 childCount;                     // Entry Children Count.
	struct DIR_ENTRY_STRUCT *children;  // Entry Children.
} DIR_ENTRY;

// Enum for standard ANSI VT colors (0-7)
typedef enum {
    BLACK = 0,
    RED = 1,
    GREEN = 2,
    YELLOW = 3,
    BLUE = 4,
    MAGENTA = 5,
    CYAN = 6,
    WHITE = 7
} Color;

// Framebuffer and Render Mode.
static void *xfb = NULL;
static GXRModeObj *rmode = NULL;

// Initialize.
static void Init();

// Helper Functions.
static void StopCritical(const char* msg, const char* running_func, const char* occuring_file, int lcall, uint32_t code);
static void SetCursor(int x, int y);
static void SetColors(Color background, Color foreground);
static void ClearScreen();

// ISFS Directory Reading
static inline bool IsDir(DIR_ENTRY *entry);
static bool ReadDirectory(DIR_ENTRY *parent);

// Print paths menu
static void print_paths_menu(DIR_ENTRY* dir, int selected_index, int max_length) {
    // Reset Screen to Browse Neatly.
    ClearScreen();
    SetCursor(0,3);

    // Print App Title
    printf("ISFS NAND Browser Demo\n");
    printf("[D] = Directory, [F] = File. <D/F> = Selected.\n");
    printf("Press HOME to exit.\n\n");

    // Ensure we've been passed valid data.
    if (dir == NULL) return;
    if (dir->children == NULL) return;

    // Starting index to print entries.
    int start_index = 0;

    // If the total number of children is more than the max length, we adjust the start_index.
    if (dir->childCount > max_length) {
        // Adjust start_index based on the selected_index
        if (selected_index >= max_length / 2) {
            start_index = selected_index - max_length / 2;
            // Make sure start_index does not go negative
            if (start_index < 0) start_index = 0;
        }
    }

    // Print the browsing information
    printf("Browsing %s\n", dir->abspath);

    // Go through each entry from the start_index to the maximum of start_index + max_length
    for (int i = start_index; i < dir->childCount; i++) {
        bool is_entry_dir = IsDir(&(dir->children[i]));
        bool is_selected  = (i == selected_index);
        
        // Set colors based on selection
        if (is_selected) SetColors(WHITE, BLACK);
        
        // Print the entry
        printf("%c%c%c %s\n",
            is_selected     ? '<' : '[',
            is_entry_dir    ? 'D' : 'F',
            is_selected     ? '>' : ']',
            dir->children[i].name
        );

        // Back to normal colors.
        SetColors(BLACK, WHITE);
    }
}

// Main
int main(int argc, char** argv) {
    // Initialize hardware.
    Init();

    // Directory Entry
    DIR_ENTRY parent;
    parent.abspath = malloc(ISFS_MAXPATH);
    sprintf(parent.abspath, "/");
    ReadDirectory(&parent);

    // Print list
    int selected_index = 0;
    print_paths_menu(&parent, selected_index, 20);

    // Main Loop (Watchin for button presses).
    while(true) {
        // Scan WPAD
        WPAD_ScanPads();
        s32 pressed = WPAD_ButtonsDown(WPAD_CHAN_0);

        // Check for home on each remote.
        if(pressed & WPAD_BUTTON_HOME) break;

        // Menu navigation.
        if(pressed & WPAD_BUTTON_UP) {
            if(selected_index < 1) // Block going too far back.
                selected_index = 0;
            else
                selected_index--;
        } if(pressed & WPAD_BUTTON_DOWN) {
            if(selected_index >= parent.childCount-1)
                selected_index = parent.childCount-1;
            else
                selected_index++;
        } if(pressed & WPAD_BUTTON_A) {
            if(IsDir(&(parent.children[selected_index])))
            {
                // Check for . and .. on parent.children[selected_index].name
                if (strcmp(parent.children[selected_index].name, "..") == 0) {
                    // Chop off the last directory if it's ".."
                    char *last_slash = strrchr(parent.abspath, '/');
                    if (last_slash != NULL) {
                        *last_slash = '\0'; // Remove the last directory from the path
                    }
                    
                    // If the resulting path is empty, set it to root "/"
                    if (strlen(parent.abspath) == 0) {
                        strcpy(parent.abspath, "/");
                    }
                } else if (strcmp(parent.children[selected_index].name, ".") != 0) {
                    // If it's not ".", update abspath to the selected child
                    sprintf(parent.abspath, "%s", parent.children[selected_index].abspath);
                }
                selected_index = 0;
                ReadDirectory(&parent);
            }
        }

        // Update menu on press.
        if(pressed) print_paths_menu(&parent, selected_index, 5);
        
        // Wait VSync.
        VIDEO_WaitVSync();
    }
}

// Initializer.
static void Init() {
	// Initialise the video system
	VIDEO_Init();

	// Please refer to templates to understand this.
	rmode = VIDEO_GetPreferredMode(NULL);
	xfb = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));
	console_init(xfb,20,20,rmode->fbWidth,rmode->xfbHeight,rmode->fbWidth*VI_DISPLAY_PIX_SZ);
	//SYS_STDIO_Report(true);
	VIDEO_Configure(rmode);
	VIDEO_SetNextFramebuffer(xfb);
	VIDEO_SetBlack(false);
	VIDEO_Flush();
	VIDEO_WaitVSync();
	if(rmode->viTVMode&VI_NON_INTERLACE) VIDEO_WaitVSync();

    // Jump to another IOS (Because some IOS's just don't like you, no offence).
    SetColors(BLUE, WHITE);    // Fancy Colors.
    ClearScreen();             // Fill the screen with the color mode.
    SetCursor(0,2);            // Reset cursor position.


    // Get the better prefered IOS (Default 56).
    s32 ios_needs = 56; // Default 56
    printf("Preparing to jump IOS..\n");
    ios_needs = IOS_GetPreferredVersion();

    // Reload into the new IOS.
    printf("Jumping to IOS(%u)...\n", ios_needs);
    IOS_ReloadIOS(ios_needs);

	// This function initialises the attached controllers
	WPAD_Init();

    // Initialize ISFS
    s32 init_res = ISFS_Initialize();
    if(init_res != ISFS_OK) StopCritical("Failed to initialize ISFS.\n", __FUNCTION__, __FILE__, __LINE__, init_res);
    
    // Welcome
    SetColors(BLACK, WHITE);   // Default color mode.
    ClearScreen();             // Clear Screen.
    SetCursor(0,2);            // Reset Cursor Position.
}

// Function to set the terminal cursor position at X,Y cordinates.
static void SetCursor(int x, int y) {
    printf("\x1b[%d;%dH", y,x);
}

// Function to set background and foreground color on console.
static void SetColors(Color background, Color foreground) {
    printf("\x1b[%dm", 40 + background); // BG
    printf("\x1b[%dm", 30 + foreground); // FG
}

// Function to clear the screen.
static void ClearScreen() {
    printf("\x1b[2J");
}

// Critical Error Display.
static void StopCritical(const char* msg, const char* running_func, const char* occuring_file, int lcall, uint32_t code) {
    // Clear the display with blue.
    SetColors(BLUE, WHITE);
    ClearScreen();
    SetCursor(0,2);

    // Error
    printf("DEMO ERROR 0x%04x:\n\n", code);
    SetColors(BLACK, YELLOW);
    printf(msg);
    SetColors(BLUE, WHITE);
    printf("\n*DONT PANIC* If you see this, its nothing\nharmful, this in-fact is a safety measure taken to\nprevent damage to your console/emulator.\n");
    printf("\nPress HOME or RESET to exit.\n\n");

    // Print Extra Info (Useful for debugging).
    printf("This error occured in '%s' (Called from L%d by '%s()')\nThis demo was built '%s %s', refer to source for debugging.\nBuilt by '%s'\n",
        occuring_file,
        lcall,
        running_func,
        __DATE__, __TIME__,
        __HOST__
    );

    // Critial Loop.
    while(true) {
        // Scan WPAD
        WPAD_ScanPads();
        s32 pressed[4]; // State on every remote.

        // Get the remote states.
        for(int rmt = 0; rmt < 4; rmt++)
            pressed[rmt] = WPAD_ButtonsDown(rmt);

        // Check for home on each remote, and check reset button.
        for(int rmt = 0; rmt < 4; rmt++)
            if(pressed[rmt] & WPAD_BUTTON_HOME || SYS_ResetButtonDown()) break;
        
        // Wait VSync.
        VIDEO_WaitVSync();
    }

    // Notify that the system is exiting.
    printf("\nEXITING...\n");
    exit(code);
}

// Determine weather or not an entry is a directory.
static inline bool IsDir(DIR_ENTRY *entry) {
	return entry->flags & FLAG_DIR; // If the entry flags has the DIR flag, assume dir.
}

// Chop off any double slashes (//) from a string.
static inline void RemoveDoubleSlash(char *str)
{
	if(!str) return;
	int count = 0;
	const char *ptr = str;
	while(*ptr != 0)
	{
		if(*ptr == '/' && ptr[1] == '/') {
			ptr++;
			continue;
		}
		str[count] = *ptr;
		ptr++;
		count++;
	}

	while(count > 2 && str[count-1] == '/')
		count--;

	str[count] = 0;
}

// Add a child entry to the main parent entry.
static DIR_ENTRY *add_child_entry(DIR_ENTRY *dir, const char *name) {
    // If the children section is NULL, allocate one child entry.
	if(!dir->children)
		dir->children = malloc(sizeof(DIR_ENTRY));
    
    // Reallocate the children array to fit one more child entry.
	DIR_ENTRY *newChildren = realloc(dir->children, (dir->childCount + 1) * sizeof(DIR_ENTRY));
	if (!newChildren) return NULL;

    // Zero out memory for additional children entries and update the directory's children pointer.
	bzero(newChildren + dir->childCount, sizeof(DIR_ENTRY));
	dir->children = newChildren;

    // Get our new child as a pointer, while incrementing the parent child count.
	DIR_ENTRY *child = &dir->children[dir->childCount++];

    // Dump the new child name into the actual child.
	child->name = strdup(name);
	if (!child->name) return NULL;

    // Allocate space for the child's absolute path.
	child->abspath = malloc(strlen(dir->abspath) + strlen(name) + 2);
	if (!child->abspath) return NULL;

    // Set the child's absolute path, and remove double slashes.
	sprintf(child->abspath, "%s/%s", dir->abspath, name);
	RemoveDoubleSlash(child->abspath);
	return child;
}

// Read a parent directory.
static bool ReadDirectory(DIR_ENTRY *parent) {
    // Don't proceed if parent or parent's absolute path is NULL.
	if(!parent || !parent->abspath)
		return false;
    
    // Have we already read this dir?
	u32 fileCount;
	if(parent->size != 0 && IsDir(parent))
	{
		fileCount = parent->size;
    } else
	{
        // Read the directory file count.
		s32 ret = ISFS_ReadDir(parent->abspath, NULL, &fileCount);
		if (ret != ISFS_OK) {
			return false;
		}
	}

    // Set newly discovered parent parameters.
	parent->flags = FLAG_DIR;
	parent->size = fileCount;
	parent->childCount = 0;

    // Add a ./ and ../ if we aren't inside the root directory.
	if(strcmp(parent->abspath, "/") != 0)
	{
        // Add the ./
		DIR_ENTRY *child = add_child_entry(parent, ".");
		if (!child) return false;
		child->flags = FLAG_DIR;
		child->size = 0;

        // Add the ../
		child = add_child_entry(parent, "..");
		if (!child) return false;
		child->flags = FLAG_DIR;
		child->size = 0;
	}

    // If we ACTUALLY HAVE FILES, proceed.
	if (fileCount > 0)
	{
        // Create an aligned buffer for file names.
		char *buffer = (char *) memalign(32, ISFS_MAXPATH * fileCount);
		if(!buffer) return false;

        // Read the parent directory to the name buffer.
		s32 ret = ISFS_ReadDir(parent->abspath, buffer, &fileCount);
		if (ret != ISFS_OK)
		{
			free(buffer);
			return false;
		}

        // Loop through each file count and add them as a child.
        // File names are read like aa|bb|cc| where | means \0
		u32 fileNum;
		char *name = buffer;
		for (fileNum = 0; fileNum < fileCount; fileNum++)
		{
            // Create a child entry.
			DIR_ENTRY *child = add_child_entry(parent, name);
			if (!child)
			{
				free(buffer);
				return false;
			}

            // Set the position of the entry name to the next one.
			name += strlen(name) + 1;

            // Check to see how many entries are in the child (if any)
			u32 childFileCount;
			ret = ISFS_ReadDir(child->abspath, NULL, &childFileCount);
			if (ret == ISFS_OK)
			{
				child->flags = FLAG_DIR;
				child->size = childFileCount;
			}
			else // Assume it's a file instead.
			{
                // Open the file, and read it's length.
				s32 fd = ISFS_Open(child->abspath, ISFS_OPEN_READ);
				if (fd >= 0) {
					if (ISFS_GetFileStats(fd, &filest) == ISFS_OK)
						child->size = filest.file_length;
					ISFS_Close(fd);
				}
			}
		}
		free(buffer);
	}
	return true;
}