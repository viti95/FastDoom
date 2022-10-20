#include <stdlib.h>
#include <dos.h>
#include <conio.h>
#include "ns_dpmi.h"
#include "ns_task.h"
#include "ns_cards.h"
#include "ns_user.h"
#include "ns_cms.h"
#include "ns_muldf.h"
#include "ns_inter.h"
#include "ns_multi.h"
#include "ns_midif.h"

#include "m_misc.h"
#include "options.h"
#include "doomstat.h"

//#define CMS_DEBUG

#ifdef CMS_DEBUG
#include <stdio.h>
#include <stdarg.h>

static void debug_log(const char *fmt, ...)
{
   FILE *f_log;
   va_list ap;

	f_log = fopen("cmslog.log", "a");
	va_start(ap, fmt);
        vfprintf(f_log, fmt, ap);
	va_end(ap);
        fclose(f_log);
}
#endif

static int CMS_Installed = 0;

static short *CMS_BufferStart;
static short *CMS_CurrentBuffer;
static int CMS_BufferNum = 0;
static int CMS_NumBuffers = 0;
static int CMS_TransferLength = 0;
static int CMS_CurrentLength = 0;

unsigned short CMS_Port = 0x220;

static short *CMS_SoundPtr;
volatile int CMS_SoundPlaying;

static task *CMS_Timer;

void (*CMS_CallBack)(void);

// CMS synth
static mid_channel cms_synth[MAX_CMS_CHANNELS];	

static unsigned short freqtable[128] = {
    8,    9,    9,   10,   10,   11,   12,   12,   13,   14,   15,   15,
   16,   17,   18,   19,   21,   22,   23,   24,   26,   28,   29,   31,
   33,   35,   37,   39,   41,   44,   46,   49,   52,   55,   58,   62,
   65,   69,   73,   78,   82,   87,   92,   98,  104,  110,  117,  123,
  131,  139,  147,  156,  165,  175,  185,  196,  208,  220,  233,  247,
  262,  277,  294,  311,  330,  349,  370,  392,  415,  440,  466,  494,
  523,  554,  587,  622,  659,  698,  740,  784,  831,  880,  932,  988,
 1047, 1109, 1175, 1245, 1319, 1397, 1480, 1568, 1661, 1760, 1865, 1976,
 2093, 2217, 2349, 2489, 2637, 2794, 2960, 3136, 3322, 3520, 3729, 3951,
 4186, 4435, 4699, 4978, 5274, 5588, 5920, 6272, 6645, 7040, 7459, 7902,
 8372, 8870, 9397, 9956,10548,11175,11840,12544};

static unsigned short pitchtable[256] = {                    /* pitch wheel */
         29193U,29219U,29246U,29272U,29299U,29325U,29351U,29378U,  /* -128 */
         29405U,29431U,29458U,29484U,29511U,29538U,29564U,29591U,  /* -120 */
         29618U,29644U,29671U,29698U,29725U,29752U,29778U,29805U,  /* -112 */
         29832U,29859U,29886U,29913U,29940U,29967U,29994U,30021U,  /* -104 */
         30048U,30076U,30103U,30130U,30157U,30184U,30212U,30239U,  /*  -96 */
         30266U,30293U,30321U,30348U,30376U,30403U,30430U,30458U,  /*  -88 */
         30485U,30513U,30541U,30568U,30596U,30623U,30651U,30679U,  /*  -80 */
         30706U,30734U,30762U,30790U,30817U,30845U,30873U,30901U,  /*  -72 */
         30929U,30957U,30985U,31013U,31041U,31069U,31097U,31125U,  /*  -64 */
         31153U,31181U,31209U,31237U,31266U,31294U,31322U,31350U,  /*  -56 */
         31379U,31407U,31435U,31464U,31492U,31521U,31549U,31578U,  /*  -48 */
         31606U,31635U,31663U,31692U,31720U,31749U,31778U,31806U,  /*  -40 */
         31835U,31864U,31893U,31921U,31950U,31979U,32008U,32037U,  /*  -32 */
         32066U,32095U,32124U,32153U,32182U,32211U,32240U,32269U,  /*  -24 */
         32298U,32327U,32357U,32386U,32415U,32444U,32474U,32503U,  /*  -16 */
         32532U,32562U,32591U,32620U,32650U,32679U,32709U,32738U,  /*   -8 */
         32768U,32798U,32827U,32857U,32887U,32916U,32946U,32976U,  /*    0 */
         33005U,33035U,33065U,33095U,33125U,33155U,33185U,33215U,  /*    8 */
         33245U,33275U,33305U,33335U,33365U,33395U,33425U,33455U,  /*   16 */
         33486U,33516U,33546U,33576U,33607U,33637U,33667U,33698U,  /*   24 */
         33728U,33759U,33789U,33820U,33850U,33881U,33911U,33942U,  /*   32 */
         33973U,34003U,34034U,34065U,34095U,34126U,34157U,34188U,  /*   40 */
         34219U,34250U,34281U,34312U,34343U,34374U,34405U,34436U,  /*   48 */
         34467U,34498U,34529U,34560U,34591U,34623U,34654U,34685U,  /*   56 */
         34716U,34748U,34779U,34811U,34842U,34874U,34905U,34937U,  /*   64 */
         34968U,35000U,35031U,35063U,35095U,35126U,35158U,35190U,  /*   72 */
         35221U,35253U,35285U,35317U,35349U,35381U,35413U,35445U,  /*   80 */
         35477U,35509U,35541U,35573U,35605U,35637U,35669U,35702U,  /*   88 */
         35734U,35766U,35798U,35831U,35863U,35895U,35928U,35960U,  /*   96 */
         35993U,36025U,36058U,36090U,36123U,36155U,36188U,36221U,  /*  104 */
         36254U,36286U,36319U,36352U,36385U,36417U,36450U,36483U,  /*  112 */
         36516U,36549U,36582U,36615U,36648U,36681U,36715U,36748U}; /*  120 */

unsigned char	CMSFreqMap[128] = {
		0  ,  3,  7, 11, 15, 19, 23, 27,
		31 , 34, 38, 41, 45, 48, 51, 55,
		58 , 61, 64, 66, 69, 72, 75, 77,
		80 , 83, 86, 88, 91, 94, 96, 99,
		102,104,107,109,112,114,116,119,
		121,123,125,128,130,132,134,136,
		138,141,143,145,147,149,151,153,
		155,157,159,161,162,164,166,168,
		170,172,174,175,177,179,181,182,
		184,186,188,189,191,193,194,196,
		197,199,200,202,203,205,206,208,
		209,210,212,213,214,216,217,218,
		219,221,222,223,225,226,227,228,
		229,231,232,233,234,235,236,237,
		239,240,241,242,243,244,245,246,
		247,249,250,251,252,253,254,255
	};

// The 12 note-within-an-octave values for the SAA1099, starting at B
static unsigned char noteAdr[] = {5, 32, 60, 85, 110, 132, 153, 173, 192, 210, 227, 243};

// Volume
static unsigned char atten[128] = {
                 0,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,
                 3,3,3,3,3,3,3,3,4,4,4,4,4,4,4,4,
                 5,5,5,5,5,5,5,5,6,6,6,6,6,6,6,6,
                 7,7,7,7,7,7,7,7,8,8,8,8,8,8,8,8,
                 9,9,9,9,9,9,9,9,10,10,10,10,10,10,10,10,
                 11,11,11,11,11,11,11,11,12,12,12,12,12,12,12,12,
                 13,13,13,13,13,13,13,13,14,14,14,14,14,14,14,14,
        	 15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,
        };

// Logic channel - first chip/second chip
static unsigned char ChanReg[12] =  {000,001,002,003,004,005,000,001,002,003,004,005};

// Set octave command
static unsigned char OctavReg[12] = {0x10,0x10,0x11,0x11,0x12,0x12,0x10,0x10,0x11,0x11,0x12,0x12};

unsigned char CmsOctaveStore[12];

unsigned char ChanEnableReg[2] = {0,0};

// Note priority
unsigned char NotePriority;

unsigned short channelpitch[NUM_MIDI_CHANNELS];


/*---------------------------------------------------------------------
   Function: CMS_ServiceInterrupt

   Handles interrupt generated by sound card at the end of a voice
   transfer.  Calls the user supplied callback function.
---------------------------------------------------------------------*/

static void CMS_SetRegister(unsigned short regAddr, unsigned char reg, unsigned char val)
{
    outp(regAddr + 1, reg); /* Select the register */
    outp(regAddr, val);     /* Set the value of the register */
}

static void CMS_Reset(void)
{
    int i;
    unsigned short port = CMS_Port;

    // Null all 32 registers
    for (i = 0; i < 32; i++){
        CMS_SetRegister(port, i, 0x00);
        CMS_SetRegister(port + 2, i, 0x00);
    }

    // Reset chip
    CMS_SetRegister(port, 0x1C, 0x02);
    CMS_SetRegister(port + 2, 0x1C, 0x02);

    // Enable chip
    CMS_SetRegister(port, 0x1C, 0x01);
    CMS_SetRegister(port + 2, 0x1C, 0x01);
}

static void CMS_ServiceInterrupt(task *Task)
{   
    char *ptr = CMS_SoundPtr;

    unsigned char value1 = (unsigned char)*(ptr);
    unsigned char value2 = (unsigned char)*(ptr + MV_RightChannelOffset);
    
    value1 = value1 >> 4;
    value2 = value2 & 0xF0;

    outp(CMS_Port, value1);
    outp(CMS_Port + 2, value2);

    CMS_SoundPtr++;

    CMS_CurrentLength--;
    if (CMS_CurrentLength == 0)
    {
        // Keep track of current buffer
        CMS_CurrentBuffer += CMS_TransferLength;
        CMS_BufferNum++;
        if (CMS_BufferNum >= CMS_NumBuffers)
        {
            CMS_BufferNum = 0;
            CMS_CurrentBuffer = CMS_BufferStart;
        }

        CMS_CurrentLength = CMS_TransferLength;
        CMS_SoundPtr = CMS_CurrentBuffer;

        // Call the caller's callback function
        if (CMS_CallBack != NULL)
        {
            MV_ServiceVoc();
        }
    }
}

/*---------------------------------------------------------------------
   Function: CMS_StopPlayback

   Ends the transfer of digitized sound to the Sound Source.
---------------------------------------------------------------------*/

void CMS_StopPlayback(void)
{
    if (CMS_SoundPlaying)
    {
        TS_Terminate(CMS_Timer);
        CMS_SoundPlaying = 0;
        CMS_BufferStart = NULL;
    }
}

/*---------------------------------------------------------------------
   Function: CMS_BeginBufferedPlayback

   Begins multibuffered playback of digitized sound on the Sound Source.
---------------------------------------------------------------------*/

int CMS_BeginBufferedPlayback(
    char *BufferStart,
    int BufferSize,
    int NumDivisions,
    void (*CallBackFunc)(void))
{
    if (CMS_SoundPlaying)
    {
        CMS_StopPlayback();
    }

    CMS_CallBack = CallBackFunc;

    CMS_BufferStart = BufferStart;
    CMS_CurrentBuffer = BufferStart;
    CMS_SoundPtr = BufferStart;
    // VITI95: OPTIMIZE
    CMS_TransferLength = (BufferSize / NumDivisions) / 2;
    CMS_CurrentLength = CMS_TransferLength;
    CMS_BufferNum = 0;
    CMS_NumBuffers = NumDivisions;

    CMS_SoundPlaying = 1;

    CMS_Timer = TS_ScheduleTask(CMS_ServiceInterrupt, CMS_SampleRate, 1, NULL);
    TS_Dispatch();

    return (CMS_Ok);
}

/*---------------------------------------------------------------------
   Function: CMS_Init

   Initializes the Sound Source prepares the module to play digitized
   sounds.
---------------------------------------------------------------------*/

int CMS_Init(int soundcard, int port)
{
    if (CMS_Installed)
    {
        CMS_Shutdown();
    }

    CMS_Reset();
    CMS_SetRegister(CMS_Port, 0x18, 0x82);
    CMS_SetRegister(CMS_Port + 2, 0x18, 0x82);

    outp(CMS_Port + 1, 0x02);
    outp(CMS_Port + 3, 0x02);

    CMS_SoundPlaying = 0;

    CMS_CallBack = NULL;

    CMS_BufferStart = NULL;

    if (port != -1)
        CMS_Port = port;

    CMS_Installed = 1;

    return (CMS_Ok);
}

/*---------------------------------------------------------------------
   Function: CMS_Shutdown

   Ends transfer of sound data to the Sound Source.
---------------------------------------------------------------------*/

void CMS_Shutdown(void)
{
    CMS_StopPlayback();

    CMS_SetRegister(CMS_Port, 0x1C, 0x02);
    CMS_SetRegister(CMS_Port + 2, 0x1C, 0x02);

    CMS_SoundPlaying = 0;

    CMS_BufferStart = NULL;

    CMS_CallBack = NULL;

    CMS_Installed = 0;
}

// Low-lewel CMS-synth func

void cmsDisableVoice(unsigned char voice)
{
	if (voice > 5)
	    {
		ChanEnableReg[1] &= ~(1 << ChanReg[voice]);
		CMS_SetRegister(CMS_Port + 2, 0x14, ChanEnableReg[1]);
	    }
	else
	    {
		ChanEnableReg[0] &= ~(1 << ChanReg[voice]);
		CMS_SetRegister(CMS_Port, 0x14, ChanEnableReg[0]);
	    }
}

void cmsSound(unsigned char voice,unsigned char freq,unsigned char octave,unsigned char amplitudeLeft,unsigned char amplitudeRight)
{
	if (voice > 5)
	    {
		if ((ChanReg[voice]&0x1) == 0)
			CmsOctaveStore[OctavReg[voice]-0x10+3] = (CmsOctaveStore[OctavReg[voice]-0x10+3] & 0xF0) | octave;
		else
			CmsOctaveStore[OctavReg[voice]-0x10+3] = (CmsOctaveStore[OctavReg[voice]-0x10+3] & 0xF) | (octave << 4);
		CMS_SetRegister(CMS_Port + 2, OctavReg[voice], CmsOctaveStore[OctavReg[voice]-0x10+3]);
		CMS_SetRegister(CMS_Port + 2, ChanReg[voice], (amplitudeLeft << 4) | amplitudeRight);
		CMS_SetRegister(CMS_Port + 2, ChanReg[voice] | 0x8, freq);
		ChanEnableReg[1] |= 1 << ChanReg[voice];
		CMS_SetRegister(CMS_Port + 2, 0x14, ChanEnableReg[1]);
	    }
	else
	    {
		if ((ChanReg[voice]&0x1) == 0)
			CmsOctaveStore[OctavReg[voice]-0x10] = (CmsOctaveStore[OctavReg[voice]-0x10] & 0xF0) | octave;
		else
			CmsOctaveStore[OctavReg[voice]-0x10] = (CmsOctaveStore[OctavReg[voice]-0x10] & 0xF) | (octave << 4);
		CMS_SetRegister(CMS_Port, OctavReg[voice], CmsOctaveStore[OctavReg[voice]-0x10]);
		CMS_SetRegister(CMS_Port, ChanReg[voice], (amplitudeLeft << 4) | amplitudeRight);
		CMS_SetRegister(CMS_Port, ChanReg[voice] | 0x8, freq);
		ChanEnableReg[0] |= 1 << ChanReg[voice];
		CMS_SetRegister(CMS_Port, 0x14, ChanEnableReg[0]);
	    }
}

// ****
// High-level CMS-synth functions
// ****

int CMS_MIDI_Init(int port)
{
    int i;

    CMS_Port = port;

#ifdef CMS_DEBUG_X
    debug_log("CMS_MIDI_Init port= %i (CMS_Port = %i)\r\n",port,CMS_Port);
#endif

    CMS_Reset();
    // prepare MIDI synth
    for (i=0;i<11;i++)
	CmsOctaveStore[i]=0;

    ChanEnableReg[0]=0;
    ChanEnableReg[1]=0;

    for (i=0;i<MAX_CMS_CHANNELS;i++)
        {
                cms_synth[i].note=0;
                cms_synth[i].priority=0;
    		cms_synth[i].ch = 0;
    		cms_synth[i].voice = 0;
    		cms_synth[i].velocity = 0;
        }
        for (i=0;i<NUM_MIDI_CHANNELS;i++)
        {
            channelpitch[i] = 8192;
        }
    NotePriority = 0;

    return (CMS_Ok);
}

void CMS_MIDI_Shutdown(void)
{
    CMS_Reset();
}

void CMS_NoteOff(int channel, int key, int velocity)
{
  unsigned char i;
  unsigned char voice;

#ifdef CMS_DEBUG
    debug_log("CMS_NoteOff\r\n");
#endif
    voice = MAX_CMS_CHANNELS;
    for(i=0; i<MAX_CMS_CHANNELS; i++)
    {
      	if(cms_synth[i].note==key)
      	{
        	voice = i;
        	break;
      	}
    }


    // Note not found, ignore note off command
    if(voice==MAX_CMS_CHANNELS)
    {
#ifdef CMS_DEBUG
	debug_log("not found Ch=%u,note=%u\n",channel & 0xFF,key & 0xFF);
#endif
    	return;
    }

    // decrease priority for all notes greater than current
    for (i=0; i<MAX_CMS_CHANNELS; i++)
	if (cms_synth[i].priority > cms_synth[voice].priority )
		cms_synth[i].priority = cms_synth[i].priority - 1;

    
    if (NotePriority != 0) NotePriority--;

    cmsDisableVoice(voice);
    cms_synth[voice].note = 0;
    cms_synth[voice].priority = 0;
    cms_synth[voice].ch = 0;
    cms_synth[voice].voice = 0;
    cms_synth[voice].velocity = 0;
}

void CMS_NoteOn(int channel, int key, int velocity)
{
  unsigned char octave;
  unsigned char noteVal;
  unsigned char i;
  unsigned char voice;
  unsigned char note_cms;
  unsigned notefreq;
  int pitch;

#ifdef CMS_DEBUG
    debug_log("CMS_NoteOn channel=%i;key=%i;velocity=%i\r\n",channel,key,velocity);
#endif
  if (velocity != 0)
  {
	//note_cms = key + 1;

	//octave = (note_cms / 12) - 1; //Some fancy math to get the correct octave
  	//noteVal = note_cms - ((octave + 1) * 12); //More fancy math to get the correct note

	NotePriority++;

        voice = MAX_CMS_CHANNELS;
    	for(i=0; i<MAX_CMS_CHANNELS; i++)
    	{
      		if(cms_synth[i].note==0)
      		{
        		voice = i;
        		break;
      		}
    	}


  	// We run out of voices, find low priority voice
  	if(voice==MAX_CMS_CHANNELS)
  	{
		unsigned char min_prior = cms_synth[0].priority;

#ifdef CMS_DEBUG
	debug_log("out of voice. NotePriority=%u\n",NotePriority);
    		for (i=0; i<MAX_CMS_CHANNELS; i++)
		  debug_log("%u ",cms_synth[i].priority);
	debug_log("\n");	
#endif
		// find note with min prioryty
		voice = 0;
    		for (i=1; i<MAX_CMS_CHANNELS; i++)
		  if (cms_synth[i].priority < min_prior) 
		  {
			voice = i;
                        min_prior = cms_synth[i].priority;
                  }

		// decrease all notes priority by one
    		for (i=0; i<MAX_CMS_CHANNELS; i++)
		  if (cms_synth[i].priority != 0)
			cms_synth[i].priority = cms_synth[i].priority - 1;

		// decrease current priority
		if (NotePriority != 0) NotePriority--;

#ifdef CMS_DEBUG
	debug_log("find low priority voice %u, NotePriority=%u\n",voice,NotePriority);
    		for (i=0; i<MAX_CMS_CHANNELS; i++)
		  debug_log("%u ",cms_synth[i].priority);
	debug_log("\n");	
#endif
  	}

        pitch = channelpitch[channel];
        if (pitch != 0) {
           if (pitch > 127) {
              pitch = 127;
           } else if (pitch < -128) {
              pitch = -128;
           }
        }

        notefreq = ((unsigned long)freqtable[key] * pitchtable[pitch + 128]) >> 15;

	if ((notefreq > 31) && (notefreq < 7824))
	{
		octave=4;
		while (notefreq<489)
	 		{
				notefreq=notefreq*2;
				octave--;
			}
		while (notefreq>977)
			{
				notefreq=notefreq / 2;
				octave++;
			}

		cmsSound(voice,CMSFreqMap[((notefreq-489)*128) / 489],octave,atten[velocity],atten[velocity]); 

	}

	cms_synth[voice].note = key;
	cms_synth[voice].priority = NotePriority;
	cms_synth[voice].velocity = velocity;
        cms_synth[voice].ch = channel;
        cms_synth[voice].voice = voice;

  } 
  else
        CMS_NoteOff(channel,key,0);
}

void CMS_ControlChange(int channel, int number, int value)
{
  unsigned char i;
    if ((number==MIDI_RESET_ALL_CONTROLLERS) || (number==MIDI_ALL_NOTES_OFF)) // All Sound/Notes Off
    {
  		for (i=0;i<MAX_CMS_CHANNELS;i++) 
    		  if (cms_synth[i].note != 0)
		 	{
    				cmsDisableVoice(i);
				cms_synth[i].note = 0;
				cms_synth[i].priority = 0;
    				cms_synth[i].ch = 0;
    				cms_synth[i].voice = 0;
    				cms_synth[i].velocity = 0;
		 	}
		if (number==MIDI_RESET_ALL_CONTROLLERS)
		{
        		for (i=0;i<NUM_MIDI_CHANNELS;i++)
            			channelpitch[i] = 8192;
		}
    }
}

void CMS_ProgramChange(int channel, int program)
{
}

void CMS_PitchBend(int channel, int lsb, int msb)
{
  unsigned char i;
  unsigned short notefreq;
  unsigned short period;
  unsigned char mixer;
  unsigned char note;
  int pitch;
  unsigned char octave;
  int pitchwheel;

    pitchwheel = lsb + (msb << 8);

    channelpitch[channel] = pitchwheel;

    for(i=0; i<MAX_CMS_CHANNELS; i++)
    {
        if ((cms_synth[i].ch==channel) && (cms_synth[i].note != 0))
        {
         note = cms_synth[i].note;

         pitch = pitchwheel;
         if (pitch != 0) {
           if (pitch > 127) {
              pitch = 127;
           } else if (pitch < -128) {
              pitch = -128;
           }
         }

        notefreq = ((unsigned long)freqtable[note] * pitchtable[pitch + 128]) >> 15;

	if ((notefreq > 31) && (notefreq < 7824))
	{
		octave=4;
		while (notefreq<489)
	 		{
				notefreq=notefreq*2;
				octave--;
			}
		while (notefreq>977)
			{
				notefreq=notefreq / 2;
				octave++;
			}

		cmsSound(cms_synth[i].voice,CMSFreqMap[((notefreq-489)*128) / 489],octave,atten[cms_synth[i].velocity],atten[cms_synth[i].velocity]); 

        }
      }
   }
}

