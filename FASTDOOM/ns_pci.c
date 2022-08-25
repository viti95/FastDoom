#include <dos.h>
#include <string.h>
//#include "judasac.h"
#include "ns_pci.h"
#include "options.h"

AUDIO_PCI_DEV audio_pci = {0}; /*  Pci device structure for AC97/HDA */

BYTE pci_config_read_byte(int index)
{
    union REGS r;

    memset(&r, 0, sizeof(r));
    r.x.eax = 0x0000B108; // config read byte
    r.x.ebx = (DWORD)audio_pci.device_bus_number;
    r.x.edi = (DWORD)index;
    int386(0x1a, &r, &r);
    if (r.h.ah != 0)
    {
        printf("Error : PCI read config byte failed\n");
        r.x.ecx = 0;
    }
    return (BYTE)r.x.ecx;
}

WORD pci_config_read_word(int index)
{
    union REGS r;

    memset(&r, 0, sizeof(r));
    r.x.eax = 0x0000B109; // config read word
    r.x.ebx = (DWORD)audio_pci.device_bus_number;
    r.x.edi = (DWORD)index;
    int386(0x1a, &r, &r);
    if (r.h.ah != 0)
    {
        printf("Error : PCI read config word failed\n");
        r.x.ecx = 0;
    }
    return (WORD)r.x.ecx;
}

DWORD pci_config_read_dword(int index)
{
    union REGS r;

    memset(&r, 0, sizeof(r));
    r.x.eax = 0x0000B10A; // config read dword
    r.x.ebx = (DWORD)audio_pci.device_bus_number;
    r.x.edi = (DWORD)index;
    int386(0x1a, &r, &r);
    if (r.h.ah != 0)
    {
        printf("Error : PCI read config dword failed\n");
        r.x.ecx = 0;
    }
    return (DWORD)r.x.ecx;
}

void pci_config_write_byte(int index, BYTE data)
{
    union REGS r;

    memset(&r, 0, sizeof(r));
    r.x.eax = 0x0000B10B; // config write byte
    r.x.ebx = (DWORD)audio_pci.device_bus_number;
    r.x.ecx = (DWORD)data;
    r.x.edi = (DWORD)index;
    int386(0x1a, &r, &r);
    if (r.h.ah != 0)
    {
        printf("Error : PCI write config byte failed\n");
    }
}

void pci_config_write_word(int index, WORD data)
{
    union REGS r;

    memset(&r, 0, sizeof(r));
    r.x.eax = 0x0000B10C; // config write word
    r.x.ebx = (DWORD)audio_pci.device_bus_number;
    r.x.ecx = (DWORD)data;
    r.x.edi = (DWORD)index;
    int386(0x1a, &r, &r);
    if (r.h.ah != 0)
    {
        printf("Error : PCI write config word failed\n");
    }
}

void pci_config_write_dword(int index, DWORD data)
{
    union REGS r;

    memset(&r, 0, sizeof(r));
    r.x.eax = 0x0000B10D; // config write dword
    r.x.ebx = (DWORD)audio_pci.device_bus_number;
    r.x.ecx = (DWORD)data;
    r.x.edi = (DWORD)index;
    int386(0x1a, &r, &r);
    if (r.h.ah != 0)
    {
        printf("Error : PCI write config dword failed\n");
    }
}

BOOL pci_check_bios(void)
{
    union REGS r;

    memset(&r, 0, sizeof(r));
    r.x.eax = 0x0000B101; // PCI BIOS - installation check
    r.x.edi = 0x00000000;
    int386(0x1a, &r, &r);
    if (r.x.edx != 0x20494350)
        return FALSE; // ' ICP' identifier found ?
    return TRUE;
}

BOOL pci_find_device()
{
    union REGS r;

    memset(&r, 0, sizeof(r));
    r.x.eax = 0x0000B102;          // PCI BIOS - find PCI device
    r.x.ecx = audio_pci.device_id; // device ID
    r.x.edx = audio_pci.vender_id; // vender ID
    r.x.esi = 0x00000000;          // device index
    int386(0x1a, &r, &r);
    if (r.h.ah != 0)
        return FALSE;                     // device not found
    audio_pci.device_bus_number = r.w.bx; // save device & bus/funct number
    if (audio_pci.sub_vender_id != PCI_ANY_ID)
    {
        // check subsystem vender id
        if (pci_config_read_word(0x2C) != audio_pci.sub_vender_id)
            return FALSE;
    }
    if (audio_pci.sub_device_id != PCI_ANY_ID)
    {
        // check subsystem device id
        if (pci_config_read_word(0x2E) != audio_pci.sub_device_id)
            return FALSE;
    }
    return TRUE; // device found
}

void pci_enable_io_access()
{
    pci_config_write_word(0x04, pci_config_read_word(0x04) | BIT0);
}

void pci_enable_memory_access()
{
    pci_config_write_word(0x04, pci_config_read_word(0x04) | BIT1);
}

void pci_enable_busmaster()
{
    pci_config_write_word(0x04, pci_config_read_word(0x04) | BIT2);
}
