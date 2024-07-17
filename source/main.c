#include <gccore.h>
#include <wiiuse/wpad.h>
#include <inttypes.h>
#include <fat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <malloc.h>
#include <asndlib.h>
#include <mp3player.h>

// include generated header for the music
#include "music_mp3.h"
#include "background_jpg.h"     //This background image went unused (didnt find any need for it)


static void *xfb = NULL;			//Set up the DevKit functions to draw to the screen
static GXRModeObj *rmode = NULL;


u32 SysMenu()
{
	// This function will get the System Menu version from TMD (title metadata)
	// This does not work on Dolphin since it is not real hardware

	static u64 TitleID ATTRIBUTE_ALIGN(32) = 0x0000000100000002LL;  //Fetch the system menu title
	static u32 tmdSize ATTRIBUTE_ALIGN(32);							//Create a variable to store the size of said title
	
	if (ES_GetStoredTMDSize(TitleID, &tmdSize) < 0)	              //If the size is empty or nonexistent, print error and proceed
	{
		printf("\nError! Failed to get the stored TMD size for the system menu\n\n");
		
		return false;
	}
	

	signed_blob *TMD = (signed_blob *)memalign(32, (tmdSize+32)&(~31));
	memset(TMD, 0, tmdSize);
	
	if (ES_GetStoredTMD(TitleID, TMD, tmdSize) < 0)                //If the stored title metadata can't even be retrieved, print the error. Operations will still continue
	{
		printf("\nError! Failed to get the stored TMD for the system menu\n\n");
		
		return false;
	}
}



void loopsong(){
	    MP3Player_PlayBuffer(music_mp3, music_mp3_size, NULL);
}


int initialize(void){

	// Initialise the video system
	VIDEO_Init();

	// Initialises the attached controllers
	WPAD_Init();

	//Initialize the audio player and sound capabilities
    ASND_Init();
	MP3Player_Init();


	// Obtain the preferred video mode from the system
	// This will correspond to the settings in the Wii menu
	rmode = VIDEO_GetPreferredMode(NULL);

	// Allocate memory for the display in the uncached region
	xfb = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));

	// Initialise the console, required for printf and our banner
	console_init(xfb,20,20,rmode->fbWidth,rmode->xfbHeight,rmode->fbWidth*VI_DISPLAY_PIX_SZ);

	// Set up the video registers with the chosen mode
	VIDEO_Configure(rmode);

	// Tell the video hardware where our display memory is
	VIDEO_SetNextFramebuffer(xfb);

	// Make the display visible
	VIDEO_SetBlack(FALSE);
	
	// Flush the video register changes to the hardware
	VIDEO_Flush();
	
	// Wait for Video setup to complete
	VIDEO_WaitVSync();

	// Start the music
	loopsong();
	return 0;
}


int main(int argc, char **argv, char** env) {

	//Run the initialization function above
	initialize();

	//Define some functions to get the information we need.
	//Some of which uses DevKitPro functions, such as IOS_GetRevision
	//The values of these variables will be printed to the screen
	u8 nick[22];
	char outstr[1024];
	char tmpstr[1024];
	int curr_ios = IOS_GetVersion();
	int curr_ios_ver = IOS_GetRevisionMajor();
	int curr_ios_rev = IOS_GetRevision();	

	//Print the software banner	
	printf("\n _ _ _ _ _ ____  _                 _");
	printf("\n| | | |_|_|     |_|___ ___ ___ ___| |_"); 
	printf("\n| | | | | |  |  | |_ -|_ -| -_|  _|  _|");
	printf("\n|_____|_|_|____/|_|___|___|___|___|_|  ");
	printf("\n'Whats inside YOUR Nintendo Wii?'");
	printf("\n");
	printf("\n--------------------------------");
	printf("\n       Created by Tenor-Z       ");
	printf("\n          June 28 2024          ");
	printf("\n--------------------------------");

	//Define a few more variables that will gather info on the Wii console. This includes the Hollywood Revision
	//and the system menu version.
	u32 hw_rev = SYS_GetHollywoodRevision();
	u32 serial_no,boot2_ver,num_titles;
	u32 region = CONF_GetRegion();
	u32 sys_version = SysMenu();
	
	CONF_GetNickName(nick);
	ES_GetDeviceID(&serial_no);
	ES_GetBoot2Version(&boot2_ver);
	ES_GetNumTitles(&num_titles);
    
	//This block of code will get the version of IOS being used, using simple process of elimination.
	//If it cannot find the version, it will define it as unknown (often the case if Dolphin is used)
	//By default, an unknown version is set unless specified elsewhere
	if(rmode->viTVMode&VI_NON_INTERLACE) VIDEO_WaitVSync();
    printf("\nIOS version: ");
	switch (sys_version)
	{
		case 33: printf("1.0 (%d)\n", sys_version); break;
		case 128: case 97: case 130: printf("2.0 (%d)\n", sys_version); break;
		case 162: printf("2.1 (%d)\n", sys_version); break;
		case 192: case 193: case 194: printf("2.2 (%d)\n", sys_version); break;
		case 224: case 225: case 226: printf("3.0 (%d)\n", sys_version); break;
		case 256: case 257: case 258: printf("3.1 (%d)\n", sys_version); break;
		case 288: case 289: case 290: printf("3.2 (%d)\n", sys_version); break;
		case 352: case 353: case 354: case 326: printf("3.3 (%d)\n", sys_version); break;
		case 384: case 385: case 386: printf("3.4 (%d)\n", sys_version); break;
		case 390: printf("3.5 (%d)\n", sys_version); break;
		case 416: case 417: case 418: printf("4.0 (%d)\n", sys_version); break;
		case 448: case 449: case 450: case 454: printf("4.1 (%d)\n", sys_version); break;
		case 480: case 481: case 482: case 486: printf("4.2 (%d)\n", sys_version); break;
		case 512: case 513: case 514: case 518: printf("4.3 (%d)\n", sys_version); break;
		default: printf("Unknown Version (who knows?) (%d)\n", sys_version); break;
	}

	//This code block attempts to get the console's registered region code.
	//This is easily performed by using their numerical equivalents
	//i.e, region 0 means Japan, region 4 is Korea, etc.
	if (region == 0) {
		strcat(outstr,"\nConsole Region - NTSC-J");	
	} else if (region == 1) {
		strcat(outstr,"\nConsole Region - NTSC-U");	
	} else if (region == 2) {
		strcat(outstr,"\nConsole Region - PAL");	
	} else if (region == 4) {
		strcat(outstr,"\nConsole Region - NTSC-K");	
	} else if (region == 5) {
		strcat(outstr,"\nConsole Region - NTSC-C");	
	}

	
	sprintf(tmpstr,"\nConsole ID -  %u ",serial_no);
	strcat(outstr,tmpstr);
	sprintf(tmpstr,"\nConsole Name -  %s",nick);
	strcat(outstr,tmpstr);
	sprintf(tmpstr,"\nHollywood Rev -  %X",hw_rev);
	strcat(outstr,tmpstr);
	sprintf(tmpstr,"\nCurrent IOS -  %d v%d r%d",curr_ios,curr_ios_ver,curr_ios_rev);
	strcat(outstr,tmpstr);
	if (num_titles != 0) {
		sprintf(tmpstr,"\nTitles Installed - %d ",num_titles);
		strcat(outstr,tmpstr);
		}
	sprintf(tmpstr,"\nBoot2 - v%u \n",boot2_ver);
	strcat(outstr,tmpstr);
	
	printf("%s",outstr);
		
	while(1) {
		// Call WPAD_ScanPads each loop, this reads the latest controller states
		WPAD_ScanPads();
		// WPAD_ButtonsDown tells us which buttons were pressed in this loop
		// this is a "one shot" state which will not fire again until the button has been released
		u32 pressed = WPAD_ButtonsDown(0);
		// We return to the launcher application via exit
		if ( pressed & WPAD_BUTTON_HOME ) exit(0);
		// Wait for the next frame
		VIDEO_WaitVSync();
	}
	return 0;
}