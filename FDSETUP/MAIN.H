//
// MAIN.H
//
#include "setup.h"
#include "menu.h"
#include "keys.h"

#define EXENAME "FDOOM.EXE"
#define DEFAULTNAME "FDOOM.CFG"
#define DEFAULTPATH "C:\\DOOMDATA"
#define SAVENAME "FDOOMSV%d.DSG"

#define S_NONE "None"
#define S_PCSP "PC Speaker"
#define S_CON1 "Keyboard only"
#define S_CON2 "Keyboard + Mouse"

#define MAXARGS 12

typedef enum
{
	FALSE,
	TRUE
} BOOL;

typedef enum
{
	M_NONE,
	M_PC,
	M_ADLIB,
	M_SB,
	M_PAS,
	M_GUS,
	M_WAVE,
	M_CANVAS,
	M_GMIDI,
	M_SBAWE32,
	M_UNUSED1,
	M_UNUSED2,
	M_DISNEYSS,
	M_TANDYSS,
	M_PC1BIT,
	M_COVOX,
	M_SBDIRECT,
	M_ADBFX,
	M_LAST
} SCARD;

typedef enum
{
	C_KEY,
	C_MOUSE,
	C_LAST
} CONTROL;

typedef struct
{
	SCARD card;
	short port;
	short midiport;
	short irq;
	short dma;
} DMXCARD;

typedef struct
{
	CONTROL control;
	DMXCARD m;
	DMXCARD d;
	short numdig;
} DMXINFO;

typedef struct
{
	int numberOfPlayers;
	int skillLevel;
	int deathMatch;
	unsigned networkSocket;
	int episode;
} net_t;

typedef struct
{
	int skillLevel;
	int deathMatch;
	int episode;
	int comtype; // 0 = connected,1 = answer,2 = call
	char phonenum[16];
} serial_t;

typedef struct
{
	int up;
	int down;
	int left;
	int right;
	int fire;
	int use;  // ID = USE         CYGNUS = USE SPECIAL WEAPON
	int key1; // ID = STRAFE ON   CYGNUS = CHANGE SPECIAL WEAPON
	int key2; // ID = SPEED ON    CYGNUS = MEGA BOMB HOT KEY
	int key3; // ID = STRAFE LEFT
	int key4; // ID = STRAFE RIGHT
	int mouse[3];
} CONTS;

typedef enum
{
	ID_FIRE,	// def button 1
	ID_STRAFE,	// def button 3
	ID_FORWARD, // def button 2
	ID_USE		// def button 4 joystick only
} IDCONTS;

extern int usemouse;

typedef struct
{
	char name[64];
	char init[64];
	char hangup[64];
	char baud[8];
} modem_t;

#define MAXMODEMS 200

typedef struct
{
	char name[32];
	char number[16];
} phonelist_t;

#define MAXPHNLIST 40

extern char keydesc[256][10];
extern int mousepresent;
extern DMXINFO lastc;
extern DMXINFO newc;
extern BOOL savemusic;
extern BOOL savefx;
extern CONTS curk;

//
// Network macros
//

extern net_t netinfo;
extern net_t info; // in case ESC is pressed

extern serial_t modeminfo;
extern serial_t minfo; // in case ESC is pressed

extern serial_t serialinfo;
extern serial_t sinfo; // in case ESC is pressed

void ErrorWindow(pup_t far *pup);
void StartUp(void);
void MainMenu(void);
void DrawCurrentConfig(void);
int QuitAndSave(void);

//
// Functions in other modules
//

// MUSIC.C
int SetupMusic(void);
int ChooseSbPort(DMXCARD *card);
int ChooseMidiPort(DMXCARD *card);

// SFX.C
int SetupFX(void);

// CONTROL.C
int ChooseController(void);

// CONFIG.C
void ConfigControl(void);
void Pos(item_t *item);
void Clear(item_t *item);
