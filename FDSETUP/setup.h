//#define	DEBUG			// shows all windows

typedef unsigned char byte;

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
extern pup_t far askpres, far consel, far control,
	far idcard, far idkeysel, far idmain2,
	far idmousel, far mcard, far midiport,
	far mousentr, far mouspres,
	far numdig, far quitwin,
	far lptport, far cmsport, far show,
	far freqpcm, far freqall, far comport, far mididev,
	far title, far imfcport,
	far goldirq, far golddma;

#define MAXLAYERS 5 // max amount of screens to save
void SaveScreen(void);
void RestoreScreen(void);

void DrawPup(pup_t far *pup);
extern unsigned char mono;
extern char errorstring[80];
void Error(char *string);
int CheckParm(char *string);

extern char **myargv;
extern int myargc;
