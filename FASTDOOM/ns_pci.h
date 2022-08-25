#ifndef __PCI_H
#define __PCI_H

#include "ns_muldf.h"

#define PCI_CLASS_MULTIMEDIA_AUDIO  0x0401
#define PCI_CLASS_MULTIMEDIA_OTHER  0x0480
#define PCI_VENDER_ID               0x00
#define PCI_REVISION_ID             0x08
#define PCI_COMMAND                 0x04
#define PCI_DEVICE_ID               0x02
#define PCI_INTERRUPT_LINE          0x3c
#define PCI_INT_LINE                0x3d

#define PCI_MEM_BASE_ADDRESS_0      0x10
#define PCI_MEM_BASE_ADDRESS_1      0x14
#define PCI_MEM_BASE_ADDRESS_2      0x18
#define PCI_MEM_BASE_ADDRESS_3      0x1c

#define PCI_BASE_ADDRESS_0          0x10
#define PCI_BASE_ADDRESS_1          0x14
#define PCI_BASE_ADDRESS_2          0x18
#define PCI_BASE_ADDRESS_3          0x1c
#define PCI_BASE_ADDRESS_4          0x20
#define PCI_BASE_ADDRESS_5          0x24

#define PCI_COMMAND_IO              0x01
#define PCI_COMMAND_MEMORY          0x02
#define PCI_COMMAND_MASTER          0x04
#define PCI_COMMAND_PARITY          0x40
#define PCI_ICH4_CFG_REG            0x41
#define PCI_COMMAND_SERR            0x100

#define PCI_STATUS                  0x06
#define PCI_SUBSYSTEM_VENDER_ID     0x2c
#define PCI_SUBSYSTEM_ID            0x2e

#define HDA_MAX_CONNECTIONS                 32
#define HDA_MAX_PCM_VOLS                    2

#define PCI_ANY_ID              ((WORD)(~0))

#pragma pack(push,1)
// 1 HDA node with it's data
struct hda_node {
	WORD nid; /* NID of this widget */
	WORD nconns; /* number of input connections */
	WORD conn_list[HDA_MAX_CONNECTIONS];
	DWORD wid_caps; /* widget capabilities */
	DWORD type; /* node/widget type - bits 20 - 23 of wid_caps */
	DWORD pin_caps; /* pin widget capabilities */
	DWORD pin_ctl; /* pin controls */
	DWORD def_config; /* default configuration */
	DWORD amp_out_caps; /* AMP out capabilities override over default */
	DWORD amp_in_caps; /* AMP in capabilities override over default */
	DWORD supported_formats; /* supported formats value */
	BYTE checked; /* flag indicator that node is already parsed */
};

struct pcm_vol_str {
	struct hda_node *node; /* Node for PCM volume */
	DWORD index; /* connection of PCM volume */
};

typedef struct {
	// PCI device IDs
	WORD vender_id;
	WORD device_id;
	WORD sub_vender_id;
	WORD sub_device_id;
	WORD device_bus_number;
	BYTE irq;                   //PCI IRQ
	BYTE pin;                   // PCI IRQ PIN
	WORD command;               // PCI command reg
	DWORD base0;                // PCI BAR for mixer registers NAMBAR_REG
	DWORD base1;                // PCI BAR for bus master registers NABMBAR_REG
	DWORD base2;
	DWORD base3;
	DWORD base4;
	DWORD base5;
	DWORD device_type;          // any

	// driver type flags
	int mem_mode;               // 0 for IO access, 1 for memory access
	int hda_mode;               // 0 for AC97 mode, 1 for HDA mode

	// memory allocated for BDL and PCM buffers
	DWORD *bdl_buffer; // Buffer Descriptor List for AC97 - 256 bytes / HDA - 8192 bytes
	DWORD *pcmout_buffer0; // Low DOS memory AC97/HDA buffer 0 for Bus Master DMA
	DWORD *pcmout_buffer1; // Low DOS memory AC97/HDA buffer 1 for Bus Master DMA
	DWORD *hda_buffer;          // base of HDA allocated memory non aligned
	DWORD pcmout_bufsize;       // size of PCM out buffer - obsolete
	DWORD pcmout_bdl_entries;   // number of BDL entries - obsolete
	DWORD pcmout_bdl_size;      // single BDL size - obsolete
	DWORD pcmout_dmasize;         // size of 1 BDL entry - obsolete
	DWORD pcmout_dma_lastgoodpos; // last good position in DMA - obsolete
	DWORD pcmout_dma_pos_ptr; // buffer for DMA position (not used in Judas) - obsolete

	// ac97 only properties
	int ac97_vra_supported;    // False by default

	// HDA only properties
	unsigned long codec_mask;                  // mask for all available codecs
	unsigned int codec_index;             // 1st codec that passed hardware init

	unsigned short afg_root_nodenum;           // Audio Function Group root node
	int afg_num_nodes; // number of subordinate nodes connected to AFG root node
	struct hda_node *afg_nodes;          // all nodes connected to root AFG node
	unsigned int def_amp_out_caps;         // default out amplifier capabilities
	unsigned int def_amp_in_caps;           // default in amplifier capabilities

	struct hda_node *dac_node[2];	           // DAC nodes
	struct hda_node *out_pin_node[2]; // Output pin nodes - all (Line-Out/hp-out, etc...)
	struct hda_node *adc_node[2];              // ADC nodes
	struct hda_node *in_pin_node[2]; // Input pin nodes - all (CD-In, etc...) - obsolete
	unsigned int input_items;                  // Input items for capture
	unsigned int pcm_num_vols;	               // number of PCM volumes
	struct pcm_vol_str pcm_vols[HDA_MAX_PCM_VOLS]; // PCM volume nodes

	unsigned int format_val;                  // stream type
	unsigned int dacout_num_bits;      // bits for playback (16 bits by default)
	unsigned int dacout_num_channels; // channels for playback (2 = stereo by default)
	unsigned int stream_tag;     // stream associated with our SD (1 by default)
	unsigned long supported_formats;
	unsigned long supported_max_freq;
	unsigned int supported_max_bits;

	unsigned long freq_card;               // current frequency 44100 by default
	unsigned int chan_card;
	unsigned int bits_card;

	// codec IDs and names
	WORD codec_id1;             // codec vender id
	WORD codec_id2;             // codec device id
	char device_name[128];      // controller name string
	char codec_name[128];       // codec name string
} AUDIO_PCI_DEV;
#pragma pack(pop)

extern AUDIO_PCI_DEV audio_pci;

BYTE pci_config_read_byte(int index);
WORD pci_config_read_word(int index);
DWORD pci_config_read_dword(int index);
void pci_config_write_byte(int index, BYTE data);
void pci_config_write_word(int index, WORD data);
void pci_config_write_dword(int index, DWORD data);
BOOL pci_check_bios(void);
BOOL pci_find_device();
void pci_enable_io_access();
void pci_enable_memory_access();
void pci_enable_busmaster();

#endif
