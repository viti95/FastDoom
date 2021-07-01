//
// MAIN.C - Handles StartUp and the MainMenu
//
#include <process.h>
#include <io.h>
#include <dos.h>
#include <mem.h>
#include <conio.h>
#include <bios.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "main.h"
#include "default.h"
#include "scguess.h"

char keydesc[256][10];
unsigned char ASCIINames[] = // Unshifted ASCII for scan codes
	{
		//       0   1   2   3   4   5   6   7   8   9   A   B   C   D   E   F
		0, 0, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', 0, 0,		// 0
		'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '[', ']', 0, 0, 'A', 'S', // 1
		'D', 'F', 'G', 'H', 'J', 'K', 'L', ';', 0, '`', 0, 92, 'Z', 'X', 'C', 'V',	// 2
		'B', 'N', 'M', ',', '.', '/', 0, '*', 0, 0, 0, 0, 0, 0, 0, 0,				// 3
		0, 0, 0, 0, 0, 0, 0, '7', '8', '9', '-', '4', '5', '6', '+', '1',			// 4
		'2', '3', '0', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,						// 5
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,								// 6
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0								// 7
};

char cards[M_LAST][20] = {
	S_NONE,
	S_PCSP,
	"Adlib",
	"Sound Blaster",
	"Pro Audio Spectrum",
	"Gravis Ultra Sound",
	"WaveBlaster",
	"Roland Sound Canvas",
	"General Midi",
	"Sound Blaster AWE32",
	"Unused",
	"Unused",
	"Disney Sound Source",
	"Tandy Sound Source",
	"PC Speaker (1 bit)",
	"COVOX"};

char controls[C_LAST][20] = {
	S_CON1,
	S_CON2};

CONTS curk;

net_t netinfo;
net_t info; // in case ESC is pressed

serial_t modeminfo;
serial_t minfo; // in case ESC is pressed

serial_t serialinfo;
serial_t sinfo; // in case ESC is pressed

DMXINFO lastc;
DMXINFO newc;

void *swindow;
BOOL savemusic = FALSE;
BOOL savefx = FALSE;

#define KEYBOARDINT 0x9

int mousepresent = 0;

enum
{
	MAIN_CMUSIC,
	MAIN_CSFX,
	MAIN_TYPE,
	MAIN_CONFIG,
	MAIN_SAVE,
	MAIN_MAX
};

item_t mainitems[] =
	{
		{MAIN_CMUSIC, 21, 12, 39, -1, -1},
		{MAIN_CSFX, 21, 13, 39, -1, -1},
		{MAIN_TYPE, 21, 14, 39, -1, -1},
		{MAIN_CONFIG, 21, 15, 39, -1, -1},
		{MAIN_SAVE, 21, 16, 39, -1, -1},
};

menu_t mainmenu =
	{
		&mainitems[0],
		MAIN_CMUSIC,
		MAIN_MAX,
		0x7f};

//
//	Draw a PUP and get a keypress
//
void ErrorWindow(pup_t far *pup)
{
	SaveScreen();
	DrawPup(pup);
	while (kbhit())
		getch();
	while (getch() != 0x1b)
		;
	RestoreScreen();
	while (kbhit())
		getch();
	sound(3000);
	delay(10);
	nosound();
}

//
// Make ASCII names/key value lookup
//
void MakeKeyLookup(void)
{
	int loop;

	memset(keydesc, 0, sizeof(keydesc));

	for (loop = 0; loop < 128; loop++)
		keydesc[loop][0] = ASCIINames[loop];

	strcpy(keydesc[SC_CAPS_LOCK], "CAPSLOCK");
	strcpy(keydesc[SC_BACKSPACE], "BACKSP");
	strcpy(keydesc[SC_ENTER], "ENTER");
	strcpy(keydesc[SC_TAB], "TAB");
	strcpy(keydesc[SC_RIGHT_SHIFT], "RSHIFT");
	strcpy(keydesc[SC_SPACE], "SPACE");
	strcpy(keydesc[SC_CTRL], "CTRL");
	strcpy(keydesc[SC_ALT], "ALT");
	strcpy(keydesc[SC_INSERT], "INS");
	strcpy(keydesc[SC_DELETE], "DEL");
	strcpy(keydesc[SC_PAGEUP], "PGUP");
	strcpy(keydesc[SC_PAGEDN], "PGDN");
	strcpy(keydesc[SC_HOME], "HOME");
	strcpy(keydesc[SC_END], "END");
	strcpy(keydesc[SC_UP], "UP");
	strcpy(keydesc[SC_DOWN], "DOWN");
	strcpy(keydesc[SC_LEFT], "LEFT");
	strcpy(keydesc[SC_RIGHT], "RIGHT");
}

//
// Set funky blue color
//
void SetColor(void)
{
	return; // DON'T DO ANYTHING UNTIL I CAN SET IT BACK!
#if 0
_asm
	{
		push  ax
		push  dx

		mov   dx, 0x3C8
		mov   ax, 1
		out   dx, al
		inc   dx

		mov   ax, 1
		out   dx, al

		mov   ax, 5
		out   dx, al

		mov   ax, 16
		out   dx, al

		pop   dx
		pop   ax
	}
#endif
}

//
// Draw current config info in window
// MAKE SURE NO WINDOWS ARE ON TOP!
//
void DrawCurrentConfig(void)
{
	RestoreScreen();
	textcolor(8);
	textbackground(7);
	gotoxy(43, 6);
	cprintf("                    ");
	gotoxy(43, 6);
	cprintf("%s", controls[newc.control]);

	gotoxy(43, 7);
	cprintf("                    ");
	gotoxy(43, 7);
	cprintf("%s", cards[newc.m.card]);

	gotoxy(43, 8);
	cprintf("                    ");
	gotoxy(43, 8);
	cprintf("%s", cards[newc.d.card]);
	gotoxy(1, 25);
	SaveScreen();
}

//
// Quitting - save changes?
//
enum
{
	SAVEYES,
	SAVENO,
	SAVEMAX
};
item_t quitwinitems[] =
	{
		{SAVEYES, 35, 11, 7, -1, -1},
		{SAVENO, 35, 12, 7, -1, -1}};
menu_t quitwinmenu =
	{
		&quitwinitems[0],
		SAVEYES,
		SAVEMAX,
		0x7f};

int QuitAndSave(void)
{
	short key;
	short field;

	SaveScreen();
	DrawPup(&quitwin);

	quitwinmenu.startitem = SAVEYES;

	while (1)
	{
		SetupMenu(&quitwinmenu);
		field = GetMenuInput();
		key = menukey;

		switch (key)
		{
		case KEY_ESC:
			RestoreScreen();
			return -1;

		case KEY_ENTER:
			if (field == SAVEYES)
				M_SaveDefaults();
			RestoreScreen();
			return 0;
		}
	}
}

//
// Start up and initialize SETUP
//
void StartUp(void)
{
	int addr;
	int irq;
	int dma;
	int midi;
	union REGS r;

	r.x.ax = 0;
	int86(0x33, &r, &r);
	if (r.x.ax == 0xffff)
		mousepresent = 1;
	else
		mousepresent = 0;

	r.x.ax = 2;
	int86(0x33, &r, &r);

	SetColor();

	memset(&newc, 0, sizeof(DMXINFO));
	memset(&lastc, 0, sizeof(DMXINFO));

	MakeKeyLookup();
	midi = addr = irq = dma = 0; // WILL BE INITED LATER

	curk.up = SC_UP;
	curk.down = SC_DOWN;
	curk.left = SC_LEFT;
	curk.right = SC_RIGHT;
	curk.fire = SC_CTRL;
	curk.use = SC_SPACE;
	curk.key1 = SC_ALT;
	curk.key2 = SC_RIGHT_SHIFT;
	curk.key3 = SC_COMMA;
	curk.key4 = SC_PERIOD;
	curk.mouse[0] = 0;
	curk.mouse[1] = 1;
	curk.mouse[2] = 2;

	newc.m.card = M_NONE;
	newc.m.port = -1;
	newc.m.midiport = -1;
	newc.m.irq = -1;
	newc.m.dma = -1;
	newc.d = newc.m;
	newc.numdig = 2;

	DrawPup(&title);
	DrawPup(&show);
	DrawPup(&idmain2);
	SaveScreen();

	if (!M_LoadDefaults())
	{
		//
		// Auto-detect ONLY first time through
		//
		if (SmellsLikeGUS(&addr, &irq, &dma))
		{
			if (newc.m.irq > 7)
				ErrorWindow(&gusirqer);

			newc.m.card = M_GUS;
			newc.m.port = addr;
			newc.m.midiport = -1;
			newc.m.irq = irq;
			newc.m.dma = dma;
			newc.d = newc.m;
			//fprintf(stderr, "GUS: addr=%x, irq=%d, dma=%d\n", addr, irq, dma);
		}
		else if (SmellsLikeSB(&addr, &irq, &dma, &midi))
		{
			newc.m.card = M_SB;
			newc.m.port = addr;
			newc.m.midiport = midi;
			newc.m.irq = irq;
			newc.m.dma = dma;
			newc.d = newc.m;
		}

		ChooseController();
		SetupMusic();
		SetupFX();

		lastc.control = newc.control;
		lastc.numdig = newc.numdig;
		lastc.m = newc.m;
		lastc.d = newc.d;
		mainmenu.startitem = MAIN_SAVE;
	}
	else
	{
		lastc.control = newc.control;
		lastc.numdig = newc.numdig;
		lastc.m = newc.m;
		lastc.d = newc.d;

		if (newc.m.irq > 7)
			ErrorWindow(&gusirqer);

		mainmenu.startitem = MAIN_CMUSIC;
	}

	DrawCurrentConfig();

	//
	// GLOBALS
	//

	MainMenu();
	clrscr();
}

//
// Main menu
//
void MainMenu(void)
{
	int i;
	int field;
	short key;
	char *args[10];
	int argcount;

	// ASSUME THAT THE MAIN MENU HAS BEEN DRAWN IN STARTUP

	while (1)
	{
		SetupMenu(&mainmenu);
		field = GetMenuInput();
		key = menukey;

		if (key == KEY_ESC)
		{
			if (QuitAndSave() < 0)
				continue;
			break;
		}

		if (key != KEY_ENTER && key != KEY_F10)
			continue;

		switch (field)
		{
		case MAIN_CMUSIC:
			SetupMusic();
			break;

		case MAIN_CSFX:
			SetupFX();
			break;

		case MAIN_TYPE:
			ChooseController();
			break;

		case MAIN_CONFIG:
			ConfigControl();
			break;

		case MAIN_SAVE:

			M_SaveDefaults();

			textbackground(0);
			textcolor(7);
			clrscr();

			argcount = 1;

			for (i = 1; i < myargc; i++)
				args[argcount++] = myargv[i];

			args[0] = "fdoom.exe ";
			args[argcount] = NULL;
			execv("fdoom.exe", args);

			printf("Problem EXECing " EXENAME "!  Probably not in same directory!\n");
			exit(0);

			goto func_exit;

		default:
			break;
		}
	}

func_exit:

	textbackground(0);
	textcolor(7);
	clrscr();

	return;
}
