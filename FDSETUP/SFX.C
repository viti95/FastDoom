//
// Handle all SFX card menus
//
#include "main.h"

enum
{
	DCARD_GUS,
	DCARD_ENSONIQ,
	DCARD_PAS,
	DCARD_SB,
	DCARD_PC,
	DCARD_DISNEY,
	DCARD_TANDY,
	DCARD_OPL2LPT,
	DCARD_OPL3LPT,
	DCARD_PC1BIT,
	DCARD_COVOX,
	DCARD_SBDIRECT,
	DCARD_ADLIB,
	DCARD_PCPWM,
	DCARD_CMS,
	DCARD_NONE,
	DCARD_MAX
};

item_t idcarditems[] =
	{
		{DCARD_GUS, 27, 4, 25, -1, -1},
		{DCARD_ENSONIQ, 27, 5, 25, -1, -1},
		{DCARD_PAS, 27, 6, 25, -1, -1},
		{DCARD_SB, 27, 7, 25, -1, -1},
		{DCARD_PC, 27, 8, 25, -1, -1},
		{DCARD_DISNEY, 27, 9, 25, -1, -1},
		{DCARD_TANDY, 27, 10, 25, -1, -1},
		{DCARD_OPL2LPT, 27, 11, 25, -1, -1},
		{DCARD_OPL3LPT, 27, 12, 25, -1, -1},
		{DCARD_PC1BIT, 27, 13, 25, -1, -1},
		{DCARD_COVOX, 27, 14, 25, -1, -1},
		{DCARD_SBDIRECT, 27, 15, 25, -1, -1},
		{DCARD_ADLIB, 27, 16, 25, -1, -1},
		{DCARD_PCPWM, 27, 17, 25, -1, -1},
		{DCARD_CMS, 27, 18, 25, -1, -1},
		{DCARD_NONE, 27, 19, 25, -1, -1}
	};

menu_t idcardmenu =
	{
		&idcarditems[0],
		DCARD_NONE,
		DCARD_MAX,
		0x7f
	};

int ChooseFxCard(void)
{
	short field;
	short key;
	int rval = 0;

	switch (newc.d.card)
	{
	default:
	case M_NONE:
		field = DCARD_NONE;
		break;

	case M_PC:
		field = DCARD_PC;
		break;

	case M_GUS:
		field = DCARD_GUS;
		break;

	case M_SB:
		field = DCARD_SB;
		break;

	case M_PAS:
		field = DCARD_PAS;
		break;

	case M_DISNEYSS:
		field = DCARD_DISNEY;
		break;

	case M_TANDY3VOICE:
		field = DCARD_TANDY;
		break;

	case M_PC1BIT:
		field = DCARD_PC1BIT;
		break;

	case M_ADLIB:
		field = DCARD_ADLIB;
		break;
	
	case M_PCPWM:
		field = DCARD_PCPWM;
		break;
	
	case M_CMS:
		field = DCARD_CMS;
		break;

	case M_COVOX:
		field = DCARD_COVOX;
		break;

	case M_SBDIRECT:
		field = DCARD_SBDIRECT;
		break;

	case M_ENSONIQ:
		field = DCARD_ENSONIQ;
		break;
	
	case M_OPL2LPT:
		field = DCARD_OPL2LPT;
		break;
	
	case M_OPL3LPT:
		field = DCARD_OPL3LPT;
		break;
	}

	SaveScreen();
	DrawPup(&idcard);
	idcardmenu.startitem = field;

	while (1)
	{
		SetupMenu(&idcardmenu);
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
			case DCARD_PAS:
				newc.d.card = M_PAS;
				goto func_exit;

			case DCARD_SB:
				newc.d.card = M_SB;
				goto func_exit;

			case DCARD_GUS:
				newc.d.card = M_GUS;
				newc.d.midiport = -1;
				goto func_exit;

			case DCARD_PC:
				newc.d.card = M_PC;
				newc.d.soundport = -1;
				newc.d.midiport = -1;
				goto func_exit;

			case DCARD_DISNEY:
				newc.d.card = M_DISNEYSS;
				newc.d.soundport = -1;
				newc.d.midiport = -1;
				goto func_exit;

			case DCARD_TANDY:
				newc.d.card = M_TANDY3VOICE;
				newc.d.soundport = -1;
				newc.d.midiport = -1;
				goto func_exit;

			case DCARD_PC1BIT:
				newc.d.card = M_PC1BIT;
				newc.d.soundport = -1;
				newc.d.midiport = -1;
				goto func_exit;

			case DCARD_ADLIB:
				newc.d.card = M_ADLIB;
				newc.d.soundport = -1;
				newc.d.midiport = -1;
				goto func_exit;

			case DCARD_PCPWM:
				newc.d.card = M_PCPWM;
				newc.d.soundport = -1;
				newc.d.midiport = -1;
				goto func_exit;

			case DCARD_CMS:
				newc.d.card = M_CMS;
				newc.d.soundport = -1;
				newc.d.midiport = -1;
				goto func_exit;

			case DCARD_COVOX:
				newc.d.card = M_COVOX;
				newc.d.soundport = -1;
				newc.d.midiport = -1;
				goto func_exit;

			case DCARD_SBDIRECT:
				newc.d.card = M_SBDIRECT;
				newc.d.soundport = -1;
				newc.d.midiport = -1;
				goto func_exit;

			case DCARD_ENSONIQ:
				newc.d.card = M_ENSONIQ;
				newc.d.soundport = -1;
				newc.d.midiport = -1;
				goto func_exit;

			case DCARD_OPL2LPT:
				newc.d.card = M_OPL2LPT;
				newc.d.soundport = -1;
				newc.d.midiport = -1;
				goto func_exit;

			case DCARD_OPL3LPT:
				newc.d.card = M_OPL3LPT;
				newc.d.soundport = -1;
				newc.d.midiport = -1;
				goto func_exit;

			case DCARD_NONE:
				newc.d.card = M_NONE;
				newc.d.soundport = -1;
				newc.d.midiport = -1;
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

item_t lptportitems[] =
	{
		{LPT_PORT_3BC, 32, 9, 13, -1, -1},
		{LPT_PORT_378, 32, 10, 13, -1, -1},
		{LPT_PORT_278, 32, 11, 13, -1, -1}};

menu_t lptportmenu =
	{
		&lptportitems[0],
		LPT_PORT_378,
		LPT_PORT_MAX,
		0x7f};

int ChooseLPTPort(DMXCARD *card) // RETURN: 0 = OK, -1 == ABORT
{
	short field;
	short key;
	int rval = 0;

	SaveScreen();
	DrawPup(&lptport);

	// DEFAULT FIELD ========================================

	switch (card->soundport)
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

	lptportmenu.startitem = field;
	while (1)
	{
		SetupMenu(&lptportmenu);
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
				card->soundport = 0x378;
				goto func_exit;

			case LPT_PORT_278:
				card->soundport = 0x278;
				goto func_exit;

			case LPT_PORT_3BC:
				card->soundport = 0x3BC;
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

item_t cmsportitems[] =
	{
		{CMS_PORT_210, 32, 9, 13, -1, -1},
		{CMS_PORT_220, 32, 10, 13, -1, -1},
		{CMS_PORT_230, 32, 11, 13, -1, -1},
		{CMS_PORT_240, 32, 12, 13, -1, -1},
		{CMS_PORT_250, 32, 13, 13, -1, -1},
		{CMS_PORT_260, 32, 14, 13, -1, -1}};

menu_t cmsportmenu =
	{
		&cmsportitems[0],
		CMS_PORT_220,
		CMS_PORT_MAX,
		0x7f};

int ChooseCMSPort(DMXCARD *card) // RETURN: 0 = OK, -1 == ABORT
{
	short field;
	short key;
	int rval = 0;

	SaveScreen();
	DrawPup(&cmsport);

	// DEFAULT FIELD ========================================

	switch (card->soundport)
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

	cmsportmenu.startitem = field;
	while (1)
	{
		SetupMenu(&cmsportmenu);
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
				card->soundport = 0x210;
				goto func_exit;

			case CMS_PORT_220:
				card->soundport = 0x220;
				goto func_exit;

			case CMS_PORT_230:
				card->soundport = 0x230;
				goto func_exit;

			case CMS_PORT_240:
				card->soundport = 0x240;
				goto func_exit;

			case CMS_PORT_250:
				card->soundport = 0x250;
				goto func_exit;

			case CMS_PORT_260:
				card->soundport = 0x260;
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
// Choose SB DMA channel
//
enum
{
	SB_DMA_0,
	SB_DMA_1,
	SB_DMA_3,
	SB_DMA_5,
	SB_DMA_6,
	SB_DMA_7,
	SB_DMA_MAX
};

item_t sbdmaitems[] =
	{
		{SB_DMA_0, 35, 9, 7, -1, -1},
		{SB_DMA_1, 35, 10, 7, -1, -1},
		{SB_DMA_3, 35, 11, 7, -1, -1},
		{SB_DMA_5, 35, 12, 7, -1, -1},
		{SB_DMA_6, 35, 13, 7, -1, -1},
		{SB_DMA_7, 35, 14, 7, -1, -1}};

menu_t sbdmamenu =
	{
		&sbdmaitems[0],
		SB_DMA_5,
		SB_DMA_MAX,
		0x7f};

/*int ChooseSbDma(DMXCARD *card)
{
	short key;
	short field;
	int rval = 0;

	switch (card->dma)
	{
	case 0:
		field = SB_DMA_0;
		break;

	default:
	case 1:
		field = SB_DMA_1;
		break;

	case 3:
		field = SB_DMA_3;
		break;

	case 5:
		field = SB_DMA_5;
		break;

	case 6:
		field = SB_DMA_6;
		break;

	case 7:
		field = SB_DMA_7;
		break;
	}

	SaveScreen();
	DrawPup(&sbdma);
	sbdmamenu.startitem = field;

	while (1)
	{
		SetupMenu(&sbdmamenu);
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
			case SB_DMA_0:
				card->dma = 0;
				goto func_exit;

			case SB_DMA_1:
				card->dma = 1;
				goto func_exit;

			case SB_DMA_3:
				card->dma = 3;
				goto func_exit;

			case SB_DMA_5:
				card->dma = 5;
				goto func_exit;

			case SB_DMA_6:
				card->dma = 6;
				goto func_exit;

			case SB_DMA_7:
				card->dma = 7;
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
}*/

//
// Choose SB IRQ channel
//

enum
{
	SB_IRQ_2,
	SB_IRQ_5,
	SB_IRQ_7,
	SB_IRQ_MAX
};
item_t sbirqitems[] =
	{
		{SB_IRQ_2, 35, 11, 7, -1, -1},
		{SB_IRQ_5, 35, 12, 7, -1, -1},
		{SB_IRQ_7, 35, 13, 7, -1, -1}};
menu_t sbirqmenu =
	{
		&sbirqitems[0],
		SB_IRQ_5,
		SB_IRQ_MAX,
		0x7f};

/*int ChooseSbIrq(DMXCARD *card)
{
	short field;
	short key;
	int rval = 0;

	if (card->irq > 8)
		ErrorWindow(&irqerr);

	switch (card->irq)
	{
	case 2:
		field = SB_IRQ_2;
		break;

	default:
	case 5:
		field = SB_IRQ_5;
		break;

	case 7:
		field = SB_IRQ_7;
		break;
	}

	SaveScreen();
	DrawPup(&sbirq);
	sbirqmenu.startitem = field;

	while (1)
	{
		SetupMenu(&sbirqmenu);
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
			case SB_IRQ_2:
				card->irq = 2;
				goto func_exit;

			case SB_IRQ_5:
				card->irq = 5;
				goto func_exit;

			case SB_IRQ_7:
				card->irq = 7;
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
}*/

//
//	Choose # of simultaneous digital channels
//

enum
{
	DIG_1,
	DIG_2,
	DIG_3,
	DIG_4,
	DIG_5,
	DIG_6,
	DIG_7,
	DIG_8,
	DIG_MAX
};
item_t numdigitems[] =
	{
		{DIG_1, 36, 8, 7, -1, -1},
		{DIG_2, 36, 9, 7, -1, -1},
		{DIG_3, 36, 10, 7, -1, -1},
		{DIG_4, 36, 11, 7, -1, -1},
		{DIG_5, 36, 12, 7, -1, -1},
		{DIG_6, 36, 13, 7, -1, -1},
		{DIG_7, 36, 14, 7, -1, -1},
		{DIG_8, 36, 15, 7, -1, -1}};
menu_t numdigmenu =
	{
		&numdigitems[0],
		DIG_4,
		DIG_MAX,
		0x7f};

int ChooseNumDig(void)
{
	short key;
	short field;
	int rval = 0;

	SaveScreen();
	DrawPup(&numdig);

	// DEFAULT FIELD ========================================

	switch (newc.numdig)
	{
	default:
		field = DIG_4;
		break;

	case 1:
		field = DIG_1;
		break;

	case 2:
		field = DIG_2;
		break;

	case 3:
		field = DIG_3;
		break;

	case 4:
		field = DIG_4;
		break;

	case 5:
		field = DIG_5;
		break;

	case 6:
		field = DIG_6;
		break;

	case 7:
		field = DIG_7;
		break;

	case 8:
		field = DIG_8;
		break;
	}
	numdigmenu.startitem = field;

	while (1)
	{
		SetupMenu(&numdigmenu);
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
			case DIG_1:
				newc.numdig = 1;
				goto func_exit;

			case DIG_2:
				newc.numdig = 2;
				goto func_exit;

			case DIG_3:
				newc.numdig = 3;
				goto func_exit;

			case DIG_4:
				newc.numdig = 4;
				goto func_exit;

			case DIG_5:
				newc.numdig = 5;
				goto func_exit;

			case DIG_6:
				newc.numdig = 6;
				goto func_exit;

			case DIG_7:
				newc.numdig = 7;
				goto func_exit;

			case DIG_8:
				newc.numdig = 8;
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
	FREQ_7000,
	FREQ_8000,
	FREQ_11025,
	FREQ_12000,
	FREQ_16000,
	FREQ_22050,
	FREQ_24000,
	FREQ_32000,
	FREQ_44100,
	FREQ_MAX
};

item_t freqitems[] =
	{
		{FREQ_7000, 27, 9, 26, -1, -1},
		{FREQ_8000, 27, 10, 26, -1, -1},
		{FREQ_11025, 27, 11, 26, -1, -1},
		{FREQ_12000, 27, 12, 26, -1, -1},
		{FREQ_16000, 27, 13, 26, -1, -1},
		{FREQ_22050, 27, 14, 26, -1, -1},
		{FREQ_24000, 27, 15, 26, -1, -1},
		{FREQ_32000, 27, 16, 26, -1, -1},
		{FREQ_44100, 27, 17, 26, -1, -1}
	};

menu_t freqmenu =
	{
		&freqitems[0],
		FREQ_11025,
		FREQ_MAX,
		0x7f};

int ChooseFreq(void)
{
	short key;
	short field;
	int rval = 0;

	SaveScreen();
	DrawPup(&freqall);

	// DEFAULT FIELD ========================================

	switch (newc.d.rate)
	{
	default:
		field = FREQ_11025;
		break;

	case 0:
		field = FREQ_7000;
		break;

	case 1:
		field = FREQ_8000;
		break;

	case 2:
		field = FREQ_11025;
		break;

	case 3:
		field = FREQ_12000;
		break;

	case 4:
		field = FREQ_16000;
		break;

	case 5:
		field = FREQ_22050;
		break;

	case 6:
		field = FREQ_24000;
		break;

	case 7:
		field = FREQ_32000;
		break;
	
	case 8:
		field = FREQ_44100;
		break;
	}

	freqmenu.startitem = field;

	while (1)
	{
		SetupMenu(&freqmenu);
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
			case FREQ_7000:
				newc.d.rate = 0;
				goto func_exit;

			case FREQ_8000:
				newc.d.rate = 1;
				goto func_exit;

			case FREQ_11025:
				newc.d.rate = 2;
				goto func_exit;

			case FREQ_12000:
				newc.d.rate = 3;
				goto func_exit;

			case FREQ_16000:
				newc.d.rate = 4;
				goto func_exit;

			case FREQ_22050:
				newc.d.rate = 5;
				goto func_exit;

			case FREQ_24000:
				newc.d.rate = 6;
				goto func_exit;

			case FREQ_32000:
				newc.d.rate = 7;
				goto func_exit;

			case FREQ_44100:
				newc.d.rate = 8;
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
// Setup Sound Effects card
//
int SetupFX(void)
{
	if (ChooseFxCard() == -1)
		return (-1);

	DrawCurrentConfig();

	switch (newc.d.card)
	{
	default:
	case M_NONE:
	case M_PC:
	case M_CD:
	case M_WAV:
	case M_SBMIDI:
		savefx = FALSE;
		break;

	case M_DISNEYSS:
		if (ChooseLPTPort(&newc.d) == -1)
			return (-1);
		newc.d.rate = 0; // 7KHz
		ChooseNumDig();
		savefx = TRUE;
		break;

	case M_COVOX:
	case M_OPL2LPT:
	case M_OPL3LPT:
		if (ChooseLPTPort(&newc.d) == -1)
			return (-1);
		ChooseFreq();
		ChooseNumDig();
		savefx = TRUE;
		break;

	case M_PC1BIT:
	case M_ADLIB:
	case M_SBDIRECT:
	case M_GUS:
	case M_PAS:
	case M_SB:
	case M_TANDY3VOICE:
	case M_ENSONIQ:
		ChooseFreq();
		ChooseNumDig();
		savefx = TRUE;
		break;

	case M_PCPWM:
		newc.d.rate = 4; // 16KHz
		ChooseNumDig();
		savefx = TRUE;
		break;
	
	case M_CMS:
		if (ChooseCMSPort(&newc.d) == -1)
			return (-1);
		ChooseFreq();
		ChooseNumDig();
		savefx = TRUE;
		break;
		
	case M_GMIDI:
		if (ChooseMidiPort(&newc.d) == -1)
			return (-1);
		savefx = TRUE;
		break;
	}
	return 0;
}
