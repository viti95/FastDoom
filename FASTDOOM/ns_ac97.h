#ifndef __AC97_H
#define __AC97_H

#include "ns_muldf.h"
#include "ns_pci.h"

#define MONO_8BIT 0

#define AC97_SampleRate 11025

enum AC97_Errors {
	AC97_Warning = -2,
	AC97_Error = -1,
	AC97_Ok = 0,
	AC97_NoVoices,
	AC97_VoiceNotFound,
	AC97_DPMI_Error
};

#define DEVICE_INTEL                       0    /* AC97 device Intel ICH compatible */
#define DEVICE_SIS                         1    /* AC97 device SIS compatible */
#define DEVICE_INTEL_ICH4                  2    /* AC97 device Intel ICH4 compatible */
#define DEVICE_NFORCE                      3    /* AC97 device nForce compatible */
#define DEVICE_HDA_INTEL                   4    /* HDA audio device for Intel  and others */
#define DEVICE_HDA_ATI                     5
#define DEVICE_HDA_ATIHDMI                 6
#define DEVICE_HDA_NVIDIA                  7
#define DEVICE_HDA_SIS                     8
#define DEVICE_HDA_ULI                     9
#define DEVICE_HDA_VIA                     10

typedef struct {
	WORD vender_id;
	WORD device_id;
	WORD sub_vender_id;
	WORD sub_device_id;
	int type;
	char *string;
} AUDIO_DEVICE_LIST;

typedef struct {
	char *string;
} AUDIO_STEREO_TECHNOLOGY;

typedef struct {
	WORD vender_id1;
	WORD vender_id2;
	char *string;
} AUDIO_CODEC_LIST;

static AUDIO_DEVICE_LIST audio_dev_list[] =
		{
				// supported controllers AC97 INTEL
				{ 0x8086, 0x2415, PCI_ANY_ID, PCI_ANY_ID, DEVICE_INTEL,
						"Intel 82801AA (ICH) integrated AC97 audio codec" }, {
						0x8086, 0x2425, PCI_ANY_ID, PCI_ANY_ID, DEVICE_INTEL,
						"Intel 82801AB (ICH0) integrated AC97 audio codec" }, {
						0x8086, 0x2445, PCI_ANY_ID, PCI_ANY_ID, DEVICE_INTEL,
						"Intel 82801BA (ICH2) integrated AC97 audio codec" }, {
						0x8086, 0x2485, PCI_ANY_ID, PCI_ANY_ID, DEVICE_INTEL,
						"Intel 82801CA (ICH3) integrated AC97 audio codec" }, {
						0x8086, 0x24c5, PCI_ANY_ID, PCI_ANY_ID,
						DEVICE_INTEL_ICH4,
						"Intel 82801DB (ICH4) integrated AC97 audio codec" },
				{ 0x8086, 0x24d5, PCI_ANY_ID, PCI_ANY_ID, DEVICE_INTEL_ICH4,
						"Intel 82801EB/ER (ICH5/ICH5R) integrated AC97 audio codec" },
				{ 0x8086, 0x25a6, PCI_ANY_ID, PCI_ANY_ID, DEVICE_INTEL_ICH4,
						"Intel 6300ESB integrated AC97 audio codec" }, { 0x8086,
						0x266e, PCI_ANY_ID, PCI_ANY_ID, DEVICE_INTEL_ICH4,
						"Intel 82801FB (ICH6) integrated AC97 audio codec" }, {
						0x8086, 0x27de, PCI_ANY_ID, PCI_ANY_ID,
						DEVICE_INTEL_ICH4,
						"Intel 82801GB (ICH7) integrated AC97 audio codec" }, {
						0x8086, 0x7195, PCI_ANY_ID, PCI_ANY_ID, DEVICE_INTEL,
						"Intel 82443MX integrated AC97 audio codec" },

				// supported controllers AC97 other (AMD/NVIDIA/SIS)
				{ 0x1022, 0x7445, PCI_ANY_ID, PCI_ANY_ID, DEVICE_INTEL,
						"AMD 768 integrated AC97 audio codec" }, { 0x1022,
						0x746d, PCI_ANY_ID, PCI_ANY_ID, DEVICE_INTEL,
						"AMD 8111 integrated AC97 audio codec" },

				{ 0x10de, 0x01b1, PCI_ANY_ID, PCI_ANY_ID, DEVICE_NFORCE,
						"Nvidia nForce integrated AC97 audio codec" }, { 0x10de,
						0x006a, PCI_ANY_ID, PCI_ANY_ID, DEVICE_NFORCE,
						"Nvidia nForce2 integrated AC97 audio codec" },
				{ 0x10de, 0x008a, PCI_ANY_ID, PCI_ANY_ID, DEVICE_NFORCE,
						"Nvidia CK8 (nForce compatible) integrated AC97 audio codec" },
				{ 0x10de, 0x00da, PCI_ANY_ID, PCI_ANY_ID, DEVICE_NFORCE,
						"Nvidia nForce3 integrated AC97 audio codec" },
				{ 0x10de, 0x00ea, PCI_ANY_ID, PCI_ANY_ID, DEVICE_NFORCE,
						"Nvidia CK8S (nForce compatible) integrated AC97 audio codec" },
				{ 0x10de, 0x0059, PCI_ANY_ID, PCI_ANY_ID, DEVICE_NFORCE,
						"Nvidia nForce4 integrated AC97 audio codec" }, {
						0x10de, 0x026b, PCI_ANY_ID, PCI_ANY_ID, DEVICE_NFORCE,
						"Nvidia nForce MCP51 integrated AC97 audio codec" }, {
						0x10de, 0x003a, PCI_ANY_ID, PCI_ANY_ID, DEVICE_NFORCE,
						"Nvidia nForce MCP04 integrated AC97 audio codec" }, {
						0x10de, 0x006b, PCI_ANY_ID, PCI_ANY_ID, DEVICE_NFORCE,
						"Nvidia nForce MCP integrated AC97 audio codec" },

				{ 0x1039, 0x7012, PCI_ANY_ID, PCI_ANY_ID, DEVICE_SIS,
						"SiS SI7012 integrated AC97 audio codec" },

				// supported controllers HDA INTEL
				{ 0x8086, 0x2668, PCI_ANY_ID, PCI_ANY_ID, DEVICE_HDA_INTEL,
						"Intel 82801F (ICH6) integrated High Definition Audio controller" },
				{ 0x8086, 0x27d8, PCI_ANY_ID, PCI_ANY_ID, DEVICE_HDA_INTEL,
						"Intel 82801G (ICH7) integrated High Definition Audio controller" },
				{ 0x8086, 0x269a, PCI_ANY_ID, PCI_ANY_ID, DEVICE_HDA_INTEL,
						"Intel ESB2 integrated High Definition Audio controller" },
				{ 0x8086, 0x284b, PCI_ANY_ID, PCI_ANY_ID, DEVICE_HDA_INTEL,
						"Intel 82801H (ICH8) integrated High Definition Audio controller" },
				{ 0x8086, 0x293e, PCI_ANY_ID, PCI_ANY_ID, DEVICE_HDA_INTEL,
						"Intel 82801I (ICH9) integrated High Definition Audio controller" },
				{ 0x8086, 0x293f, PCI_ANY_ID, PCI_ANY_ID, DEVICE_HDA_INTEL,
						"Intel 82801I (ICH9) integrated High Definition Audio controller" },
				{ 0x8086, 0x3a3e, PCI_ANY_ID, PCI_ANY_ID, DEVICE_HDA_INTEL,
						"Intel 82801J (ICH10R) integrated High Definition Audio controller" },
				{ 0x8086, 0x3a6e, PCI_ANY_ID, PCI_ANY_ID, DEVICE_HDA_INTEL,
						"Intel 82801J (ICH10) integrated High Definition Audio controller" },
				{ 0x8086, 0x3b56, PCI_ANY_ID, PCI_ANY_ID, DEVICE_HDA_INTEL,
						"Intel 5-series integrated High Definition Audio controller" },
				{ 0x8086, 0x1c20, PCI_ANY_ID, PCI_ANY_ID, DEVICE_HDA_INTEL,
						"Intel 6-series integrated High Definition Audio controller" },
				{ 0x8086, 0x1d20, PCI_ANY_ID, PCI_ANY_ID, DEVICE_HDA_INTEL,
						"Intel 7-series integrated High Definition Audio controller" },

				// supported controllers HDA other (ATI/NVIDIA/SIS/ULI/VIA)
				{ 0x1002, 0x437b, PCI_ANY_ID, PCI_ANY_ID, DEVICE_HDA_ATI,
						"ATI Technologies SB450 integrated High Definition Audio controller" },
				{ 0x1002, 0x4383, PCI_ANY_ID, PCI_ANY_ID, DEVICE_HDA_ATI,
						"ATI Technologies SB600 integrated High Definition Audio controller" },

				{ 0x1002, 0x793b, PCI_ANY_ID, PCI_ANY_ID, DEVICE_HDA_ATIHDMI,
						"ATI Technologies RS600 integrated High Definition Audio controller", },
				{ 0x1002, 0x7919, PCI_ANY_ID, PCI_ANY_ID, DEVICE_HDA_ATIHDMI,
						"ATI Technologies RS690 integrated High Definition Audio controller", },
				{ 0x1002, 0x960c, PCI_ANY_ID, PCI_ANY_ID, DEVICE_HDA_ATIHDMI,
						"ATI Technologies RS780 integrated High Definition Audio controller", },
				{ 0x1002, 0xaa00, PCI_ANY_ID, PCI_ANY_ID, DEVICE_HDA_ATIHDMI,
						"ATI Technologies R600 integrated High Definition Audio controller", },

				{ 0x10de, 0x026c, PCI_ANY_ID, PCI_ANY_ID, DEVICE_HDA_NVIDIA,
						"Nvidia nForce MCP51 integrated High Definition Audio controller" },
				{ 0x10de, 0x0371, PCI_ANY_ID, PCI_ANY_ID, DEVICE_HDA_NVIDIA,
						"Nvidia nForce MCP55 integrated High Definition Audio controller" },
				{ 0x10de, 0x03e4, PCI_ANY_ID, PCI_ANY_ID, DEVICE_HDA_NVIDIA,
						"Nvidia nForce MCP61a integrated High Definition Audio controller" },
				{ 0x10de, 0x03f0, PCI_ANY_ID, PCI_ANY_ID, DEVICE_HDA_NVIDIA,
						"Nvidia nForce MCP61b integrated High Definition Audio controller" },
				{ 0x10de, 0x044a, PCI_ANY_ID, PCI_ANY_ID, DEVICE_HDA_NVIDIA,
						"Nvidia nForce MCP65a integrated High Definition Audio controller" },
				{ 0x10de, 0x044b, PCI_ANY_ID, PCI_ANY_ID, DEVICE_HDA_NVIDIA,
						"Nvidia nForce MCP65b integrated High Definition Audio controller" },
				{ 0x10de, 0x055c, PCI_ANY_ID, PCI_ANY_ID, DEVICE_HDA_NVIDIA,
						"Nvidia nForce MCP67a integrated High Definition Audio controller" },
				{ 0x10de, 0x055d, PCI_ANY_ID, PCI_ANY_ID, DEVICE_HDA_NVIDIA,
						"Nvidia nForce MCP67b integrated High Definition Audio controller" },
				{ 0x10de, 0x07fc, PCI_ANY_ID, PCI_ANY_ID, DEVICE_HDA_NVIDIA,
						"Nvidia nForce MCP73a integrated High Definition Audio controller" },
				{ 0x10de, 0x07fd, PCI_ANY_ID, PCI_ANY_ID, DEVICE_HDA_NVIDIA,
						"Nvidia nForce MCP73b integrated High Definition Audio controller" },
				{ 0x10de, 0x0774, PCI_ANY_ID, PCI_ANY_ID, DEVICE_HDA_NVIDIA,
						"Nvidia nForce MCP77a integrated High Definition Audio controller" },
				{ 0x10de, 0x0775, PCI_ANY_ID, PCI_ANY_ID, DEVICE_HDA_NVIDIA,
						"Nvidia nForce MCP77b integrated High Definition Audio controller" },
				{ 0x10de, 0x0776, PCI_ANY_ID, PCI_ANY_ID, DEVICE_HDA_NVIDIA,
						"Nvidia nForce MCP77c integrated High Definition Audio controller" },
				{ 0x10de, 0x0777, PCI_ANY_ID, PCI_ANY_ID, DEVICE_HDA_NVIDIA,
						"Nvidia nForce MCP77d integrated High Definition Audio controller" },
				{ 0x10de, 0x0ac0, PCI_ANY_ID, PCI_ANY_ID, DEVICE_HDA_NVIDIA,
						"Nvidia nForce MCP79a integrated High Definition Audio controller" },
				{ 0x10de, 0x0ac1, PCI_ANY_ID, PCI_ANY_ID, DEVICE_HDA_NVIDIA,
						"Nvidia nForce MCP79b integrated High Definition Audio controller" },
				{ 0x10de, 0x0ac2, PCI_ANY_ID, PCI_ANY_ID, DEVICE_HDA_NVIDIA,
						"Nvidia nForce MCP79c integrated High Definition Audio controller" },
				{ 0x10de, 0x0ac3, PCI_ANY_ID, PCI_ANY_ID, DEVICE_HDA_NVIDIA,
						"Nvidia nForce MCP79d integrated High Definition Audio controller" },

				{ 0x1039, 0x7502, PCI_ANY_ID, PCI_ANY_ID, DEVICE_HDA_SIS,
						"SIS 966 integrated High Definition Audio controller" },

				{ 0x10b9, 0x5461, PCI_ANY_ID, PCI_ANY_ID, DEVICE_HDA_ULI,
						"ULI M5461 integrated High Definition Audio controller" },

				{ 0x1106, 0x3288, PCI_ANY_ID, PCI_ANY_ID, DEVICE_HDA_VIA,
						"VIA 8251/8237 integrated High Definition Audio controller" },

				// null entry
				{ 0x0000, 0x0000, PCI_ANY_ID, PCI_ANY_ID, 0, "" } };

static AUDIO_STEREO_TECHNOLOGY audio_stereo_technology[] = { {
		"No 3D Stereo Enhancement" }, { "Analog Devices Phat Stereo" },   // 1
		{ "Creative Stereo Enhancement" },   // 2
		{ "National Semi 3D Stereo Enhancement" },   // 3
		{ "YAMAHA Ymersion" },   // 4
		{ "BBE 3D Stereo Enhancement" },   // 5
		{ "Crystal Semi 3D Stereo Enhancement" },   // 6
		{ "Qsound QXpander" },   // 7
		{ "Spatializer 3D Stereo Enhancement" },   // 8
		{ "SRS 3D Stereo Enhancement" },   // 9
		{ "Platform Tech 3D Stereo Enhancement" },   // 10
		{ "AKM 3D Audio" },   // 11
		{ "Aureal Stereo Enhancement" },   // 12
		{ "Aztech 3D Enhancement" },   // 13
		{ "Binaura 3D Audio Enhancement" },   // 14
		{ "ESS Technology Stereo Enhancement" },   // 15
		{ "Harman International VMAx" },   // 16
		{ "Nvidea 3D Stereo Enhancement" },   // 17
		{ "Philips Incredible Sound" },   // 18
		{ "Texas Instruments 3D Stereo Enhancement" },   // 19
		{ "VLSI Technology 3D Stereo Enhancement" },   // 20
		{ "TriTech 3D Stereo Enhancement" },   // 21
		{ "Realtek 3D Stereo Enhancement" },   // 22
		{ "Samsung 3D Stereo Enhancement" },   // 23
		{ "Wolfson Microelectronics 3D Enhancement" },   // 24
		{ "Delta Integration 3D Enhancement" },   // 25
		{ "SigmaTel 3D Enhancement" },   // 26
		{ "Winbond 3D Stereo Enhancement" },   // 27
		{ "Rockwell 3D Stereo Enhancement" },   // 28
		{ "Unknown 3D Stereo Enhancement" },   // 29
		{ "Unknown 3D Stereo Enhancement" },   // 30
		{ "Unknown 3D Stereo Enhancement" },   // 31
		};

// vender id, device id, string
static AUDIO_CODEC_LIST audio_codec_list[] = {

		// AC97 codecs
		{ 0x4144, 0x5303, "Analog Devices AD1819" }, { 0x4144, 0x5340,
				"Analog Devices AD1881" }, { 0x4144, 0x5348,
				"Analog Devices AD1881A" }, { 0x4144, 0x5360,
				"Analog Devices AD1885" }, { 0x4144, 0x5361,
				"Analog Devices AD1886" }, { 0x4144, 0x5362,
				"Analog Devices AD1887" }, { 0x4144, 0x5363,
				"Analog Devices AD1886A" }, { 0x4144, 0x5370,
				"Analog Devices AD1980" },

		{ 0x414B, 0x4D00, "Asahi Kasei AK4540" }, { 0x414B, 0x4D01,
				"Asahi Kasei AK4542" },
		{ 0x414B, 0x4D02, "Asahi Kasei AK4543" },

		{ 0x414C, 0x4300, "ALC100" }, { 0x414C, 0x4326, "ALC100P" }, { 0x414C,
				0x4710, "ALC200/200P" }, { 0x414C, 0x4720, "ALC650" }, { 0x414C,
				0x4721, "ALC650D" }, { 0x414C, 0x4722, "ALC650E" }, { 0x414C,
				0x4723, "ALC650F" }, { 0x414C, 0x4760, "ALC655" }, { 0x414C,
				0x4780, "ALC658" }, { 0x414C, 0x4781, "ALC658D" }, { 0x414C,
				0x4790, "ALC850" },

		{ 0x434D, 0x4941, "CMedia CM9738" }, { 0x434D, 0x4942, "CMedia" }, {
				0x434D, 0x4961, "CMedia CM9739" }, { 0x434D, 0x4978,
				"CMedia CM9761 rev A" },
		{ 0x434D, 0x4982, "CMedia CM9761 rev B" }, { 0x434D, 0x4983,
				"CMedia CM9761 rev C" },

		{ 0x4352, 0x5900, "Cirrus Logic CS4297" }, { 0x4352, 0x5903,
				"Cirrus Logic CS4297" }, { 0x4352, 0x5910,
				"Cirrus Logic CS4297A" }, { 0x4352, 0x5913,
				"Cirrus Logic CS4297A rev A" }, { 0x4352, 0x5914,
				"Cirrus Logic CS4297A rev B" }, { 0x4352, 0x5923,
				"Cirrus Logic CS4298" },
		{ 0x4352, 0x592B, "Cirrus Logic CS4294" }, { 0x4352, 0x592D,
				"Cirrus Logic CS4294" },
		{ 0x4352, 0x5930, "Cirrus Logic CS4299" }, { 0x4352, 0x5931,
				"Cirrus Logic CS4299 rev A" }, { 0x4352, 0x5933,
				"Cirrus Logic CS4299 rev C" }, { 0x4352, 0x5934,
				"Cirrus Logic CS4299 rev D" }, { 0x4352, 0x5948,
				"Cirrus Logic CS4201" },
		{ 0x4352, 0x5958, "Cirrus Logic CS4205" },

		{ 0x4358, 0x5442, "CXT66" },

		{ 0x4454, 0x3031, "Diamond Technology DT0893" },

		{ 0x4583, 0x8308, "ESS Allegro ES1988" },

		{ 0x4943, 0x4511, "ICEnsemble ICE1232" },
		{ 0x4943, 0x4541, "VIA Vinyl" }, { 0x4943, 0x4551, "VIA Vinyl" }, {
				0x4943, 0x4552, "VIA Vinyl" }, { 0x4943, 0x6010, "VIA Vinyl" },
		{ 0x4943, 0x6015, "VIA Vinyl" }, { 0x4943, 0x6017, "VIA Vinyl" },

		{ 0x4e53, 0x4331, "National Semiconductor LM4549" },

		{ 0x5349, 0x4c22, "Silicon Laboratory Si3036" }, { 0x5349, 0x4c23,
				"Silicon Laboratory Si3038" },

		{ 0x5452, 0x00FF, "TriTech TR?????" }, { 0x5452, 0x4102,
				"TriTech TR28022" }, { 0x5452, 0x4103, "TriTech TR28023" }, {
				0x5452, 0x4106, "TriTech TR28026" }, { 0x5452, 0x4108,
				"TriTech TR28028" }, { 0x5452, 0x4123, "TriTech TR A5" },

		{ 0x5745, 0x4301, "Winbond 83971D" },

		{ 0x574D, 0x4C00, "Wolfson WM9704" }, { 0x574D, 0x4C03,
				"WM9703/07/08/17" }, { 0x574D, 0x4C04, "WM9704M/WM9704Q" }, {
				0x574D, 0x4C05, "Wolfson WM9705/WM9710" },

		{ 0x594D, 0x4803, "Yamaha YMF753" },

		{ 0x8384, 0x7600, "SigmaTel STAC9700" }, { 0x8384, 0x7604,
				"SigmaTel STAC9704" }, { 0x8384, 0x7605, "SigmaTel STAC9705" },
		{ 0x8384, 0x7608, "SigmaTel STAC9708" }, { 0x8384, 0x7609,
				"SigmaTel STAC9721/23" }, { 0x8384, 0x7644,
				"SigmaTel STAC9744/45" }, { 0x8384, 0x7656,
				"SigmaTel STAC9756/57" },
		{ 0x8384, 0x7666, "SigmaTel STAC9750T" }, { 0x8384, 0x7684,
				"SigmaTel STAC9783/84" },

		// HDA codecs
		{ 0x10ec, 0x0260, "ALC260 Realtek High Definition Audio" }, { 0x10ec,
				0x0268, "ALC268 Realtek High Definition Audio" }, { 0x10ec,
				0x0660, "ALC660 Realtek High Definition Audio" }, { 0x10ec,
				0x0861, "ALC861 Realtek High Definition Audio" }, { 0x10ec,
				0x0862, "ALC861VD Realtek High Definition Audio" }, { 0x10ec,
				0x0880, "ALC880 Realtek High Definition Audio" }, { 0x10ec,
				0x0882, "ALC882 Realtek High Definition Audio" }, { 0x10ec,
				0x0883, "ALC883 Realtek High Definition Audio" }, { 0x10ec,
				0x0885, "ALC885 Realtek High Definition Audio" }, { 0x10ec,
				0x0888, "ALC888 Realtek High Definition Audio" },

		{ 0x11d4, 0x1981, "AD1981HD Analog Devices High Definition Audio" }, {
				0x11d4, 0x1983, "AD1983 Analog Devices High Definition Audio" },
		{ 0x11d4, 0x1984, "AD1984 Analog Devices High Definition Audio" },
		{ 0x11d4, 0x1986, "AD1986A Analog Devices High Definition Audio" }, {
				0x11d4, 0x1988, "AD1988 Analog Devices High Definition Audio" },
		{ 0x11d4, 0x198b, "AD1988B Analog Devices High Definition Audio" },

		{ 0x434d, 0x4980, "CMI9880 CMedia High Definition Audio" },

		{ 0x8384, 0x7680, "STAC9221 Sigmatel High Definition Audio" }, { 0x8384,
				0x7683, "STAC9221D Sigmatel High Definition Audio" }, { 0x8384,
				0x7690, "STAC9220 Sigmatel High Definition Audio" }, { 0x8384,
				0x7681, "STAC9220D Sigmatel High Definition Audio" }, { 0x8384,
				0x7618, "STAC9227 Sigmatel High Definition Audio" }, { 0x8384,
				0x7627, "STAC9271D Sigmatel High Definition Audio" },

		{ 0x14f1, 0x5045, "CXVenice Conexant High Definition Audio" }, { 0x14f1,
				0x5047, "CXWaikiki Conexant High Definition Audio" },

		{ 0x1106, 0x1708, "VT1708 rev8 High Definition Audio" }, { 0x1106,
				0x1709, "VT1708 rev9 High Definition Audio" }, { 0x1106, 0x170a,
				"VT1708 rev10 High Definition Audio" }, { 0x1106, 0x170b,
				"VT1708 rev11 High Definition Audio" }, { 0x1106, 0xe710,
				"VT1709 rev0 High Definition Audio" }, { 0x1106, 0xe711,
				"VT1709 rev1 High Definition Audio" }, { 0x1106, 0xe712,
				"VT1709 rev2 High Definition Audio" }, { 0x1106, 0xe713,
				"VT1709 rev3 High Definition Audio" }, { 0x1106, 0xe714,
				"VT1709 rev4 High Definition Audio" }, { 0x1106, 0xe715,
				"VT1709 rev5 High Definition Audio" }, { 0x1106, 0xe716,
				"VT1709 rev6 High Definition Audio" }, { 0x1106, 0xe717,
				"VT1709 rev7 High Definition Audio" }, { 0x1106, 0xe718,
				"VT1709 rev8 High Definition Audio" }, { 0x1106, 0xe719,
				"VT1709 rev9 High Definition Audio" },

		{ 0x0000, 0x0000, "Unknown Device" }, };

void AC97_StopPlayback(void);
int AC97_BeginBufferedPlayback(char *BufferStart, int BufferSize,
		int NumDivisions, void (*CallBackFunc)(void));
int AC97_Init(int soundcard, int port);
void AC97_Shutdown(void);

#endif
