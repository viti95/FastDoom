#include "main.h"
#include "default.h"
#include "keys.h"

//
// Choose MIDI port
//
enum
{
	MIDI_220,
	MIDI_230,
	MIDI_240,
	MIDI_250,
	MIDI_300,
	MIDI_320,
	MIDI_330,
	MIDI_332,
	MIDI_334,
	MIDI_336,
	MIDI_340,
	MIDI_360,
	MIDI_MAX
};

item_t midiportitems[] =
	{
		{MIDI_220, 32, 6, 13, -1, -1},
		{MIDI_230, 32, 7, 13, -1, -1},
		{MIDI_240, 32, 8, 13, -1, -1},
		{MIDI_250, 32, 9, 13, -1, -1},
		{MIDI_300, 32, 10, 13, -1, -1},
		{MIDI_320, 32, 11, 13, -1, -1},
		{MIDI_330, 32, 12, 13, -1, -1},
		{MIDI_332, 32, 13, 13, -1, -1},
		{MIDI_334, 32, 14, 13, -1, -1},
		{MIDI_336, 32, 15, 13, -1, -1},
		{MIDI_340, 32, 16, 13, -1, -1},
		{MIDI_360, 32, 17, 13, -1, -1}};

menu_t midiportmenu =
	{
		&midiportitems[0],
		MIDI_330,
		MIDI_MAX,
		0x7f};

item_t midideviceitems[] =
	{
		{MIDI_DEFAULT, 26, 5, 28, -1, -1},
		{MIDI_MT32, 26, 6, 28, -1, -1},
		{MIDI_SC55, 26, 7, 28, -1, -1},
		{MIDI_MU80, 26, 8, 28, -1, -1},
		{MIDI_TG300, 26, 9, 28, -1, -1}};

menu_t mididevicemenu =
	{
		&midideviceitems[0],
		MIDI_DEFAULT,
		MIDI_LAST,
		0x7f};

int ChooseMidiPort(DMXCARD *card)
{
	short field;
	short key;
	int rval = 0;

	SaveScreen();
	DrawPup(&midiport);

	// DEFAULT FIELD ========================================

	switch (card->midiport)
	{
	default:
	case 0x220:
		field = MIDI_220;
		break;

	case 0x230:
		field = MIDI_230;
		break;

	case 0x240:
		field = MIDI_240;
		break;

	case 0x250:
		field = MIDI_250;
		break;

	case 0x300:
		field = MIDI_300;
		break;

	case 0x320:
		field = MIDI_320;
		break;

	case 0x330:
		field = MIDI_330;
		break;

	case 0x332:
		field = MIDI_332;
		break;

	case 0x334:
		field = MIDI_334;
		break;

	case 0x336:
		field = MIDI_336;
		break;

	case 0x340:
		field = MIDI_340;
		break;

	case 0x360:
		field = MIDI_360;
		break;
	}

	while (1)
	{
		SetupMenu(&midiportmenu);
		field = GetMenuInput();
		key = menukey;
		switch (key)
		{
		case KEY_ESC:
			rval = -1;
			goto func_exit;

		case KEY_ENTER:
		case KEY_F10:
			switch (field)
			{
			case MIDI_220:
				card->midiport = 0x220;
				goto func_exit;

			case MIDI_230:
				card->midiport = 0x230;
				goto func_exit;

			case MIDI_240:
				card->midiport = 0x240;
				goto func_exit;

			case MIDI_250:
				card->midiport = 0x250;
				goto func_exit;

			case MIDI_300:
				card->midiport = 0x300;
				goto func_exit;

			case MIDI_320:
				card->midiport = 0x320;
				goto func_exit;

			case MIDI_330:
				card->midiport = 0x330;
				goto func_exit;

			case MIDI_332:
				card->midiport = 0x332;
				goto func_exit;

			case MIDI_334:
				card->midiport = 0x334;
				goto func_exit;

			case MIDI_336:
				card->midiport = 0x336;
				goto func_exit;

			case MIDI_340:
				card->midiport = 0x340;
				goto func_exit;

			case MIDI_360:
				card->midiport = 0x360;
				goto func_exit;

			default:
				break;
			}
			break;
		}
	}

func_exit:

	RestoreScreen();
	return (rval);
}

int ChooseMidiDevice(short *device)
{
	short field;
	short key;
	int rval = 0;

	SaveScreen();
	DrawPup(&mididev);

	// DEFAULT FIELD ========================================

	switch (*(device))
	{
	default:
	case MIDI_DEFAULT:
		field = MIDI_DEFAULT;
		break;

	case MIDI_MT32:
		field = MIDI_MT32;
		break;

	case MIDI_SC55:
		field = MIDI_SC55;
		break;

	case MIDI_MU80:
		field = MIDI_MU80;
		break;
	
	case MIDI_TG300:
		field = MIDI_TG300;
		break;
	}

	while (1)
	{
		SetupMenu(&mididevicemenu);
		field = GetMenuInput();
		key = menukey;
		switch (key)
		{
		case KEY_ESC:
			rval = -1;
			goto func_exit;

		case KEY_ENTER:
		case KEY_F10:
			switch (field)
			{
			case MIDI_DEFAULT:
				*(device) = MIDI_DEFAULT;
				goto func_exit;

			case MIDI_MT32:
				*(device) = MIDI_MT32;
				goto func_exit;

			case MIDI_SC55:
				*(device) = MIDI_SC55;
				goto func_exit;

			case MIDI_MU80:
				*(device) = MIDI_MU80;
				goto func_exit;

			case MIDI_TG300:
				*(device) = MIDI_TG300;
				goto func_exit;

			default:
				break;
			}
			break;
		}
	}

func_exit:

	RestoreScreen();
	return (rval);
}

enum
{
	COM1,
	COM2,
	COM3,
	COM4,
	COM_MAX
};

item_t comportitems[] =
	{
		{COM1, 32, 6, 14, -1, -1},
		{COM2, 32, 7, 14, -1, -1},
		{COM3, 32, 8, 14, -1, -1},
		{COM4, 32, 9, 14, -1, -1}};

menu_t comportmenu =
	{
		&comportitems[0],
		COM1,
		COM_MAX,
		0x7f};

int ChooseCOMPort(DMXCARD *card)
{
	short field;
	short key;
	int rval = 0;

	SaveScreen();
	DrawPup(&comport);

	// DEFAULT FIELD ========================================

	switch (card->midiport)
	{
	default:
	case 0x3F8:
		field = COM1;
		break;

	case 0x2F8:
		field = COM2;
		break;

	case 0x3E8:
		field = COM3;
		break;

	case 0x2E8:
		field = COM4;
		break;
	}

	while (1)
	{
		SetupMenu(&comportmenu);
		field = GetMenuInput();
		key = menukey;
		switch (key)
		{
		case KEY_ESC:
			rval = -1;
			goto func_exit;

		case KEY_ENTER:
		case KEY_F10:
			switch (field)
			{
			case COM1:
				card->midiport = 0x3F8;
				goto func_exit;

			case COM2:
				card->midiport = 0x2F8;
				goto func_exit;

			case COM3:
				card->midiport = 0x3E8;
				goto func_exit;

			case COM4:
				card->midiport = 0x2E8;
				goto func_exit;

			default:
				break;
			}
			break;
		}
	}

func_exit:

	RestoreScreen();
	return (rval);
}

//
// Menu for choosing Music Card
//
enum
{
	MCARD_GMIDI,
	MCARD_ENSONIQ,
	MCARD_SBAWE32,
	MCARD_GUS,
	MCARD_PAS,
	MCARD_SB,
	MCARD_ADLIB,
	MCARD_OPL2LPT,
	MCARD_OPL3LPT,
	MCARD_CMS,
	MCARD_CD,
	MCARD_WAV,
	MCARD_SBMIDI,
	MCARD_RS232MIDI,
	MCARD_LPTMIDI,
	MCARD_NONE,
	MCARD_MAX
};

item_t mcarditems[] =
	{
		{MCARD_GMIDI, 26, 5, 28, -1, -1},
		{MCARD_ENSONIQ, 26, 6, 28, -1, -1},
		{MCARD_SBAWE32, 26, 7, 28, -1, -1},
		{MCARD_GUS, 26, 8, 28, -1, -1},
		{MCARD_PAS, 26, 9, 28, -1, -1},
		{MCARD_SB, 26, 10, 28, -1, -1},
		{MCARD_ADLIB, 26, 11, 28, -1, -1},
		{MCARD_OPL2LPT, 26, 12, 28, -1, -1},
		{MCARD_OPL3LPT, 26, 13, 28, -1, -1},
		{MCARD_CMS, 26, 14, 28, -1, -1},
		{MCARD_CD, 26, 15, 28, -1, -1},
		{MCARD_WAV, 26, 16, 28, -1, -1},
		{MCARD_SBMIDI, 26, 17, 28, -1, -1},
		{MCARD_RS232MIDI, 26, 18, 28, -1, -1},
		{MCARD_LPTMIDI, 26, 19, 28, -1, -1},
		{MCARD_NONE, 26, 20, 28, -1, -1}};

menu_t mcardmenu =
	{
		&mcarditems[0],
		MCARD_NONE,
		MCARD_MAX,
		0x7f};

int ChooseMusicCard(void) // RETURN: 0 = OK, -1 == ABORT
{
	short key;
	short field;
	int rval = 0;

	switch (newc.m.card)
	{
	default:
	case M_NONE:
		field = MCARD_NONE;
		break;

	case M_ADLIB:
		field = MCARD_ADLIB;
		break;
	
	case M_OPL2LPT:
		field = MCARD_OPL2LPT;
		break;
	
	case M_OPL3LPT:
		field = MCARD_OPL3LPT;
		break;

	case M_ENSONIQ:
		field = MCARD_ENSONIQ;
		break;

	case M_CMS:
		field = MCARD_CMS;
		break;

	case M_CD:
		field = MCARD_CD;
		break;

	case M_WAV:
		field = MCARD_WAV;
		break;

	case M_SBMIDI:
		field = MCARD_SBMIDI;
		break;

	case M_RS232MIDI:
		field = MCARD_RS232MIDI;
		break;
	
	case M_LPTMIDI:
		field = MCARD_LPTMIDI;
		break;

	case M_PAS:
		field = MCARD_PAS;
		break;

	case M_GUS:
		field = MCARD_GUS;
		break;

	case M_SB:
		field = MCARD_SB;
		break;

	case M_SBAWE32:
		field = MCARD_SBAWE32;
		break;

	case M_GMIDI:
		field = MCARD_GMIDI;
		break;
	}

	mcardmenu.startitem = field;
	SaveScreen();
	DrawPup(&mcard);

	while (1)
	{
		SetupMenu(&mcardmenu);
		field = GetMenuInput();
		key = menukey;

		switch (key)
		{
		case KEY_ESC:
			rval = -1;
			goto func_exit;

		case KEY_ENTER:
		case KEY_F10:
			switch (field)
			{
			case MCARD_SBAWE32:
				newc.m.card = M_SBAWE32;
				newc.m.soundport = -1;
				goto func_exit;

			case MCARD_GMIDI:
				newc.m.card = M_GMIDI;
				newc.m.soundport = -1;
				goto func_exit;

			case MCARD_SB:
				newc.m.card = M_SB;
				goto func_exit;

			case MCARD_PAS:
				newc.m.card = M_PAS;
				newc.m.midiport = -1;
				goto func_exit;

			case MCARD_GUS:
				newc.m.card = M_GUS;
				newc.m.midiport = -1;
				goto func_exit;

			case MCARD_ADLIB:
				newc.m.card = M_ADLIB;
				newc.m.soundport = -1;
				newc.m.midiport = -1;
				goto func_exit;

			case MCARD_ENSONIQ:
				newc.m.card = M_ENSONIQ;
				newc.m.soundport = -1;
				newc.m.midiport = -1;
				goto func_exit;

			case MCARD_OPL2LPT:
				newc.m.card = M_OPL2LPT;
				newc.m.midiport = -1;
				goto func_exit;
			
			case MCARD_OPL3LPT:
				newc.m.card = M_OPL3LPT;
				newc.m.midiport = -1;
				goto func_exit;

			case MCARD_CMS:
				newc.m.card = M_CMS;
				newc.m.midiport = -1;
				goto func_exit;

			case MCARD_CD:
				newc.m.card = M_CD;
				newc.m.midiport = -1;
				goto func_exit;

			case MCARD_WAV:
				newc.m.card = M_WAV;
				newc.m.midiport = -1;
				newc.m.pcmrate = 0;
				goto func_exit;

			case MCARD_SBMIDI:
				newc.m.card = M_SBMIDI;
				newc.m.midiport = -1;
				goto func_exit;

			case MCARD_RS232MIDI:
				newc.m.card = M_RS232MIDI;
				newc.m.soundport = -1;
				goto func_exit;
			
			case MCARD_LPTMIDI:
				newc.m.card = M_LPTMIDI;
				newc.m.soundport = -1;
				goto func_exit;

			case MCARD_NONE:
				newc.m.card = M_NONE;
				newc.m.soundport = -1;
				newc.m.midiport = -1;
				goto func_exit;

			default:
				break;
			}
			break;
		}
	}

func_exit:

	RestoreScreen();
	return (rval);
}

enum
{
	LPT_PORT_3BC,
	LPT_PORT_378,
	LPT_PORT_278,
	LPT_PORT_MAX
};

item_t lptportitemsm[] =
	{
		{LPT_PORT_3BC, 32, 9, 13, -1, -1},
		{LPT_PORT_378, 32, 10, 13, -1, -1},
		{LPT_PORT_278, 32, 11, 13, -1, -1}};

menu_t lptportmenum =
	{
		&lptportitemsm[0],
		LPT_PORT_378,
		LPT_PORT_MAX,
		0x7f};

int ChooseLPTPortMusic(DMXCARD *card) // RETURN: 0 = OK, -1 == ABORT
{
	short field;
	short key;
	int rval = 0;

	SaveScreen();
	DrawPup(&lptport);

	// DEFAULT FIELD ========================================

	switch (card->midiport)
	{
	default:
	case 0x378:
		field = LPT_PORT_378;
		break;

	case 0x278:
		field = LPT_PORT_278;
		break;

	case 0x3BC:
		field = LPT_PORT_3BC;
		break;
	}

	lptportmenum.startitem = field;
	while (1)
	{
		SetupMenu(&lptportmenum);
		field = GetMenuInput();
		key = menukey;
		switch (key)
		{
		case KEY_ESC:
			rval = -1;
			goto func_exit;

		case KEY_ENTER:
		case KEY_F10:
			switch (field)
			{
			case LPT_PORT_378:
				card->midiport = 0x378;
				goto func_exit;

			case LPT_PORT_278:
				card->midiport = 0x278;
				goto func_exit;

			case LPT_PORT_3BC:
				card->midiport = 0x3BC;
				goto func_exit;

			default:
				break;
			}
			break;
		}
	}

func_exit:

	RestoreScreen();
	return (rval);
}

enum
{
	CMS_PORT_210,
	CMS_PORT_220,
	CMS_PORT_230,
	CMS_PORT_240,
	CMS_PORT_250,
	CMS_PORT_260,
	CMS_PORT_MAX
};

item_t cmsportitemsm[] =
	{
		{CMS_PORT_210, 32, 9, 13, -1, -1},
		{CMS_PORT_220, 32, 10, 13, -1, -1},
		{CMS_PORT_230, 32, 11, 13, -1, -1},
		{CMS_PORT_240, 32, 12, 13, -1, -1},
		{CMS_PORT_250, 32, 13, 13, -1, -1},
		{CMS_PORT_260, 32, 14, 13, -1, -1}};

menu_t cmsportmenum =
	{
		&cmsportitemsm[0],
		CMS_PORT_220,
		CMS_PORT_MAX,
		0x7f};

int ChooseCMSPortMusic(DMXCARD *card) // RETURN: 0 = OK, -1 == ABORT
{
	short field;
	short key;
	int rval = 0;

	SaveScreen();
	DrawPup(&cmsport);

	// DEFAULT FIELD ========================================

	switch (card->midiport)
	{
	default:
	case 0x210:
		field = CMS_PORT_210;
		break;

	case 0x220:
		field = CMS_PORT_220;
		break;

	case 0x230:
		field = CMS_PORT_230;
		break;

	case 0x240:
		field = CMS_PORT_240;
		break;

	case 0x250:
		field = CMS_PORT_250;
		break;

	case 0x260:
		field = CMS_PORT_260;
		break;
	}

	cmsportmenum.startitem = field;
	while (1)
	{
		SetupMenu(&cmsportmenum);
		field = GetMenuInput();
		key = menukey;
		switch (key)
		{
		case KEY_ESC:
			rval = -1;
			goto func_exit;

		case KEY_ENTER:
		case KEY_F10:
			switch (field)
			{
			case CMS_PORT_210:
				card->midiport = 0x210;
				goto func_exit;

			case CMS_PORT_220:
				card->midiport = 0x220;
				goto func_exit;

			case CMS_PORT_230:
				card->midiport = 0x230;
				goto func_exit;

			case CMS_PORT_240:
				card->midiport = 0x240;
				goto func_exit;

			case CMS_PORT_250:
				card->midiport = 0x250;
				goto func_exit;

			case CMS_PORT_260:
				card->midiport = 0x260;
				goto func_exit;

			default:
				break;
			}
			break;
		}
	}

func_exit:

	RestoreScreen();
	return (rval);
}

enum
{
	PCM_FREQ_11025,
	PCM_FREQ_22050,
	PCM_FREQ_44100,
	PCM_FREQ_MAX
};

item_t pcmfreqitemsm[] =
	{
		{PCM_FREQ_11025, 32, 9, 13, -1, -1},
		{PCM_FREQ_22050, 32, 10, 13, -1, -1},
		{PCM_FREQ_44100, 32, 11, 13, -1, -1}
	};

menu_t pcmfreqmenum =
	{
		&pcmfreqitemsm[0],
		PCM_FREQ_11025,
		PCM_FREQ_MAX,
		0x7f};

int ChoosePCMFreqMusic(DMXCARD *card) // RETURN: 0 = OK, -1 == ABORT
{
	short field;
	short key;
	int rval = 0;

	SaveScreen();
	DrawPup(&freqpcm);

	// DEFAULT FIELD ========================================

	switch (card->pcmrate)
	{
	default:
	case 0:
		field = PCM_FREQ_11025;
		break;

	case 1:
		field = PCM_FREQ_22050;
		break;

	case 2:
		field = PCM_FREQ_44100;
		break;
	}

	pcmfreqmenum.startitem = field;
	while (1)
	{
		SetupMenu(&pcmfreqmenum);
		field = GetMenuInput();
		key = menukey;
		switch (key)
		{
		case KEY_ESC:
			rval = -1;
			goto func_exit;

		case KEY_ENTER:
		case KEY_F10:
			switch (field)
			{
			case PCM_FREQ_11025:
				card->pcmrate = 0;
				goto func_exit;

			case PCM_FREQ_22050:
				card->pcmrate = 1;
				goto func_exit;

			case PCM_FREQ_44100:
				card->pcmrate = 2;
				goto func_exit;

			default:
				break;
			}
			break;
		}
	}

func_exit:

	RestoreScreen();
	return (rval);
}

//
// Choose Music Card menu
//
int SetupMusic(void)
{
	if (ChooseMusicCard() == -1)
		return (-1);

	DrawCurrentConfig();

	switch (newc.m.card)
	{
	default:
		savemusic = FALSE;
		break;

	case M_OPL2LPT:
	case M_OPL3LPT:
		if (ChooseLPTPortMusic(&newc.m) == -1)
			return (-1);
		savemusic = TRUE;
		break;
	
	case M_LPTMIDI:
		if (ChooseLPTPortMusic(&newc.m) == -1)
			return (-1);
		if (ChooseMidiDevice(&newc.md) == -1)
			return (-1);
		savemusic = TRUE;
		break;

	case M_CMS:
		if (ChooseCMSPortMusic(&newc.m) == -1)
			return (-1);
		savemusic = TRUE;
		break;

	case M_NONE:
	case M_ADLIB:
	case M_CD:
	case M_PAS:
	case M_GUS:
	case M_ENSONIQ:
	case M_SBAWE32:
	case M_SB:
		savemusic = TRUE;
		break;

	case M_SBMIDI:
		if (ChooseMidiDevice(&newc.md) == -1)
			return (-1);
		savemusic = TRUE;
		break;

	case M_WAV:
		if (ChoosePCMFreqMusic(&newc.m) == -1)
			return (-1);
		savemusic = TRUE;
		break;

	case M_GMIDI:
		if (ChooseMidiPort(&newc.m) == -1)
			return (-1);
		if (ChooseMidiDevice(&newc.md) == -1)
			return (-1);
		savemusic = TRUE;
		break;

	case M_RS232MIDI:
		if (ChooseCOMPort(&newc.m) == -1)
			return (-1);
		if (ChooseMidiDevice(&newc.md) == -1)
			return (-1);
		savemusic = TRUE;
		break;
	}
	return 0;
}
