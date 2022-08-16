//#define	DEBUG			// shows all windows

typedef unsigned char byte;

typedef struct
{
	int x;	   // xcoord of radio
	int y;	   // ycoord of radio
	int value; // value to match
} radio_t;

typedef struct
{
	radio_t *radios; // start of radio buttons
	int amount;		 // # of radio buttons
	int *master;	 // master value that radios must match
	int fgcolor;	 // text color
	int bgcolor;	 // background color
} radiogroup_t;

typedef struct
{
	short pup_id;
	char width;
	char height;
	char x;
	char y;
	short mystery1;
	short mystery2;
} pup_t;

typedef enum
{
	normal,
	stringdraw,
	repeat
} pup_e;

// ALL THE WINDOWS
extern pup_t far askpres, far cmodem, far consel, far control, far cserial,
	far cwarp, far gusirqer, far idcard, far idjoysel, far idkeysel, far idmain2,
	far idmousel, far irqerr, far macros, far mcard, far midiport, far modemchs,
	far modsave, far mousentr, far mouspres, far netplay, far netplay2,
	far netsave, far netserr, far netwk2, far numdig, far phonelst, far quitwin,
	far sbdma, far sbirq, far sbport, far lptport, far sersave, far show, far sockerr,
	far title;

#define MAXLAYERS 5 // max amount of screens to save
void SaveScreen(void);
void RestoreScreen(void);
void DrawRadios(radiogroup_t *rg);

void DrawPup(pup_t far *pup);
extern char errorstring[80];
void Error(char *string);

extern char **myargv;
extern int myargc;
