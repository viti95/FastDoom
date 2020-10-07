#include <conio.h>
#include <dos.h>
#include <stdio.h>
#include <io.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include "ns_usrho.h"
#include "ns_inter.h"
#include "ns_gf1.h"
#include "ns_gusmi.h"

#define TRUE (1 == 1)
#define FALSE (!TRUE)

// size of DMA buffer for patch loading
#define DMABUFFSIZE 2048U

#define MAX_MEM_CONFIG 3

// size of patch array (128 perc, 128 melodic)
#define NUM_PATCHES 256

// size of largest patch name
#define BIGGEST_NAME 9

#define UNUSED_PATCH -1

static struct patch Patch[NUM_PATCHES];
static unsigned char *PatchWaves[NUM_PATCHES];

static int PatchMap[NUM_PATCHES][MAX_MEM_CONFIG + 1];
static char ProgramName[NUM_PATCHES][BIGGEST_NAME];
static char PatchLoaded[NUM_PATCHES];

static char ConfigFileName[] = "ULTRAMID.INI";
static char ConfigDirectory[80] = {'\0'};

// The name of the configuration directory
static char InstrumentDirectory[80];

extern struct gf1_dma_buff GUS_HoldBuffer;

extern unsigned long GUS_TotalMemory;
extern int GUS_MemConfig;

static int GUSMIDI_Volume = 255;

extern int GUS_AuxError;

int GUSMIDI_Installed = FALSE;

/*---------------------------------------------------------------------
   Function: GUS_GetPatchMap

   Reads the patch map from disk.
---------------------------------------------------------------------*/

int GUS_GetPatchMap(
    char *name)

{
   char text[80];
   char *ud;
   int index;
   int ignore;
   FILE *fp;

   for (index = 0; index < NUM_PATCHES; index++)
   {
      PatchMap[index][0] = UNUSED_PATCH;
      PatchMap[index][1] = UNUSED_PATCH;
      PatchMap[index][2] = UNUSED_PATCH;
      PatchMap[index][3] = UNUSED_PATCH;
      ProgramName[index][0] = 0;
   }

   ud = getenv("ULTRADIR");
   if (ud == NULL)
   {
      return (GUS_Error);
   }

   strcpy(InstrumentDirectory, ud);
   strcat(InstrumentDirectory, "\\midi\\");
   strcpy(ConfigDirectory, ud);
   strcat(ConfigDirectory, "\\midi\\");
   strcpy(text, name);

   fp = fopen(text, "r");
   if (fp == NULL)
   {
      strcpy(text, InstrumentDirectory);
      strcat(text, name);

      fp = fopen(text, "r");
      if (fp == NULL)
      {
         return (GUS_Error);
      }
   }

   while (1)
   {
      if (fgets(text, 80, fp) == NULL)
      {
         break;
      }

      if (text[0] == '#')
      {
         continue;
      }

      if (sscanf(text, "%d", &index) != 1)
      {
         continue;
      }

      sscanf(text, "%d, %d, %d, %d, %d, %s\n", &ignore,
             &PatchMap[index][0],
             &PatchMap[index][1],
             &PatchMap[index][2],
             &PatchMap[index][3],
             ProgramName[index]);
   }

   fclose(fp);

   return (GUS_Ok);
}

/*---------------------------------------------------------------------
   Function: GUSMIDI_UnloadPatch

   Unloads a patch from the GUS's memory.
---------------------------------------------------------------------*/

int GUSMIDI_UnloadPatch(
    int prognum)

{
   int prog;
   unsigned flags;

   prog = PatchMap[prognum][GUS_MemConfig];

   if (PatchLoaded[prog])
   {
      flags = DisableInterrupts();

      gf1_unload_patch(&Patch[prog]);
      if (PatchWaves[prog] != NULL)
      {
         USRHOOKS_FreeMem(PatchWaves[prog]);
         PatchWaves[prog] = NULL;
      }

      // just in case sequence is still playing
      Patch[prog].nlayers = 0;
      PatchLoaded[prog] = FALSE;

      RestoreInterrupts(flags);
   }

   return (GUS_Ok);
}

/*---------------------------------------------------------------------
   Function: GUSMIDI_LoadPatch

   Loads a patch into the GUS's memory.
---------------------------------------------------------------------*/

int GUSMIDI_LoadPatch(
    int prognum)

{
   int prog;
   char text[80];
   int ret;
   unsigned char *wave_buff;
   struct patchinfo patchi;
   int status;

   prog = PatchMap[prognum][GUS_MemConfig];

   if ((PatchLoaded[prog]) || (prog == UNUSED_PATCH))
   {
      return (GUS_Ok);
   }

   if (!ProgramName[prog][0])
   {
      return (GUS_Ok);
   }

   strcpy(text, InstrumentDirectory);
   strcat(text, ProgramName[prog]);
   strcat(text, ".pat");

   ret = gf1_get_patch_info(text, &patchi);
   if (ret != OK)
   {
      return (GUS_Error);
   }

   status = USRHOOKS_GetMem(&wave_buff, 512); // 512 bytes
   if (status != USRHOOKS_Ok)
   {
      return (GUS_Error);
   }

   ret = gf1_load_patch(text, &patchi, &Patch[prog], &GUS_HoldBuffer, DMABUFFSIZE, (unsigned char *)wave_buff, PATCH_LOAD_8_BIT);

   if (ret != OK)
   {
      USRHOOKS_FreeMem(wave_buff);
      return (GUS_Error);
   }

   PatchWaves[prog] = wave_buff;
   PatchLoaded[prog] = TRUE;

   return (GUS_Ok);
}

/*---------------------------------------------------------------------
   Function: GUSMIDI_ProgramChange

   Selects the instrument to use on the specified MIDI channel.
---------------------------------------------------------------------*/

void GUSMIDI_ProgramChange(
    int channel,
    int prognum)

{
   int prog;

   prog = PatchMap[prognum][GUS_MemConfig];

   if (PatchLoaded[prog])
   {
      gf1_midi_change_program(&Patch[prog], channel);
   }
   else
   {
      gf1_midi_change_program(NULL, channel);
   }
}

/*---------------------------------------------------------------------
   Function: GUSMIDI_NoteOn

   Plays a note on the specified channel.
---------------------------------------------------------------------*/

void GUSMIDI_NoteOn(
    int chan,
    int note,
    int velocity)

{
   int prog;

   if (chan == 9)
   {
      prog = PatchMap[note + 128][GUS_MemConfig];

      if (PatchLoaded[prog])
      {
         gf1_midi_note_on(&Patch[note + 128], 1,
                          note, velocity, 9);
      }
   }
   else
   {
      gf1_midi_note_on(0L, 1, note, velocity, chan);
   }
}

/*---------------------------------------------------------------------
   Function: GUSMIDI_NoteOff

   Turns off a note on the specified channel.
---------------------------------------------------------------------*/
#pragma warn - par
void GUSMIDI_NoteOff(
    int chan,
    int note,
    int velocity)

{
   gf1_midi_note_off(note, chan);
}
#pragma warn.par

/*---------------------------------------------------------------------
   Function: GUSMIDI_ControlChange

   Sets the value of a controller on the specified channel.
---------------------------------------------------------------------*/

void GUSMIDI_ControlChange(
    int channel,
    int number,
    int value)

{
   gf1_midi_parameter(channel, number, value);
}

/*---------------------------------------------------------------------
   Function: GUSMIDI_PitchBend

   Sets the pitch bend on the specified MIDI channel.
---------------------------------------------------------------------*/

void GUSMIDI_PitchBend(
    int channel,
    int lsb,
    int msb)

{
   gf1_midi_pitch_bend(channel, lsb, msb);
}

/*---------------------------------------------------------------------
   Function: GUSMIDI_ReleasePatches

   Removes all the instruments from the GUS's memory.
---------------------------------------------------------------------*/

void GUSMIDI_ReleasePatches(
    void)

{
   int i;

   for (i = 0; i < 256; i++)
   {
      GUSMIDI_UnloadPatch(i);
   }
}

/*---------------------------------------------------------------------
   Function: GUSMIDI_SetVolume

   Sets the total music volume.
---------------------------------------------------------------------*/

void GUSMIDI_SetVolume(
    int volume)

{
   // Set the minimum to 2 because 0 has a tremolo problem
   volume = max(2, volume);
   volume = min(volume, 255);

   GUSMIDI_Volume = volume;

   // range = 0 to 127
   gf1_midi_synth_volume(0, volume >> 1);
}

/*---------------------------------------------------------------------
   Function: GUSMIDI_GetVolume

   Returns the total music volume.
---------------------------------------------------------------------*/

int GUSMIDI_GetVolume(
    void)

{
   return (GUSMIDI_Volume);
}

/*---------------------------------------------------------------------
   Function: GUSMIDI_Init

   Initializes the Gravis Ultrasound for music playback.
---------------------------------------------------------------------*/

int GUSMIDI_Init(
    void)

{
   int ret;
   int i;
   int startmem;
   //   unsigned long mem;
   extern int GUSWAVE_Installed;

   if (GUSMIDI_Installed)
   {
      GUSMIDI_Shutdown();
   }

   ret = GUS_Init();
   if (ret != GUS_Ok)
   {
      return (ret);
   }

   if (GUS_MemConfig < 0)
   {
      GUS_MemConfig = 0;
   }

   if (GUS_MemConfig > MAX_MEM_CONFIG)
   {
      GUS_MemConfig = MAX_MEM_CONFIG;
   }

   for (i = 0; i < NUM_PATCHES; i++)
   {
      ProgramName[i][0] = '\0';
      PatchWaves[i] = NULL;
      PatchLoaded[i] = FALSE;
   }

   GUSMIDI_SetVolume(255);

   GUSMIDI_Installed = TRUE;

   ret = GUS_GetPatchMap(ConfigFileName);
   if (ret != GUS_Ok)
   {
      GUSMIDI_Shutdown();
      return (ret);
   }

   //   if ( !GUSWAVE_Installed )
   //      {
   //      mem = gf1_malloc( 8192 );
   //      }

   startmem = gf1_mem_avail();
   for (i = 0; i < NUM_PATCHES; i++)
   {
      ret = GUSMIDI_LoadPatch(i);
      if (ret != GUS_Ok)
      {
      }
   }

   //   if ( !GUSWAVE_Installed )
   //      {
   //      gf1_free( mem );
   //      }

   GUSMIDI_Installed = TRUE;

   return (GUS_Ok);
}

/*---------------------------------------------------------------------
   Function: GUSMIDI_Shutdown

   Ends use of the Gravis Ultrasound for music playback.
---------------------------------------------------------------------*/

void GUSMIDI_Shutdown(
    void)

{
   GUSMIDI_ReleasePatches();
   GUS_Shutdown();
   GUSMIDI_Installed = FALSE;
}
