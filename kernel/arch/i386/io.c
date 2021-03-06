#ifndef KERN_IO
#define KERN_IO
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <kernel/io.h>
#include <kernel/tty.h>
#define BADCHAR '\x1a'
void outb(uint16_t port, uint8_t val)
{
    asm volatile ( "outb %0, %1" : : "a"(val), "Nd"(port) );
    /* There's an outb %al, $imm8  encoding, for compile-time constant port numbers that fit in 8b.  (N constraint).
     * Wider immediate constants would be truncated at assemble-time (e.g. "i" constraint).
     * The  outb  %al, %dx  encoding is the only option for all other cases.
     * %1 expands to %dx because  port  is a uint16_t.  %w1 could be used if we had the port number a wider C type */
}
uint8_t inb(uint16_t port)
{
    uint8_t ret;
    asm volatile ( "inb %1, %0"
                   : "=a"(ret)
                   : "Nd"(port) );
    return ret;
}
void outl(uint16_t port, unsigned int val)
{
    asm volatile ( "outl %%eax, %1" : : "a"(val), "d"(port) );
    /* There's an outb %al, $imm8  encoding, for compile-time constant port numbers that fit in 8b.  (N constraint).
     * Wider immediate constants would be truncated at assemble-time (e.g. "i" constraint).
     * The  outb  %al, %dx  encoding is the only option for all other cases.
     * %1 expands to %dx because  port  is a uint16_t.  %w1 could be used if we had the port number a wider C type */
}
unsigned int inl(uint16_t port)
{
    unsigned int ret;
    asm volatile ( "inl %1, %%eax"
                   : "=a"(ret)
                   : "d"(port) );
    return ret;
}
char ask_cmos(char reg){outb(0x70,reg);return inb(0x71);}
//memcpy(scancode,s,sizeof(s));
//  \x1A\x32\x3334567890-=\x1A\x1Aqwertyuiop[]\n sdfghjkl     zxcvbnm,./ ";
//char* s(char v){char* b = {&v,'\0'};return b;}
Scancode getScancode()
{
  static char shift = 0;
  static char last = 0;
  //outb(0x60,0xF4);
  //outb(0x20, 0x20);
  char c=0;
  Scancode s = {.code = 3};
  while (1) {
    if(inb(0x60)!=c)
      {
	c=inb(0x60);
	if(c<0)//key not pressed!
	{
	  switch (c+128)
	  {
	  case SHIFT: shift = 0;break;
	  default:
	    if (c+128==last) last = 0; //Key was released!
	  }
	}
	if(c>0&&c!=last)
	{
	  switch (c)
	  {
	  case SHIFT: shift = 1; break;
     	  default:
	    last = c;
	    s.code = c;
	    s.Shift = shift;
	    return s;
	  }
	}  
      }
  }
}
void reboot()
{
	uint8_t good = 0x02;
	while (good & 0x02)
		good = inb(0x64);
	outb(0x64, 0xFE);
	halt();
}

const char scancode[] = {
	BADCHAR,BADCHAR,
	'1','2','3','4','5','6','7','8','9','0','-','=','\b',
	'\t','q','w','e','r','t','y','u','i','o','p','[',']','\n',
	BADCHAR,'a','s','d','f','g','h','j','k','l',';','\'',BADCHAR,BADCHAR,'#',
	'z','x','c','v','b','n','m',',','.','/',BADCHAR,BADCHAR,BADCHAR,' '
};
const char scancode_shift[] = {
  BADCHAR,BADCHAR,
  '!','"',156,'$','%','^','&','*','(',')','_','+',BADCHAR,
  BADCHAR,'Q','W','E','R','T','Y','U','I','O','P','{','}',BADCHAR,
  BADCHAR,'A','S','D','F','G','H','J','J','K','L',':',BADCHAR,BADCHAR,'@',
  'Z','X','C','V','B','N','M','<','>','?'
  
};
 uint16_t pciConfigReadWord (uint8_t bus, uint8_t slot,
                             uint8_t func, uint8_t offset)
 {
    uint32_t address;
    uint32_t lbus  = (uint32_t)bus;
    uint32_t lslot = (uint32_t)slot;
    uint32_t lfunc = (uint32_t)func;
    uint16_t tmp = 0;
 
    /* create configuration address as per Figure 1 */
    address = (uint32_t)((lbus << 16) | (lslot << 11) |
              (lfunc << 8) | (offset & 0xfc) | ((uint32_t)0x80000000));
 
    /* write out the address */
    outl (0xCF8, address);
    /* read in the data */
    /* (offset & 2) * 8) = 0 will choose the first word of the 32 bits register */
    tmp = (uint16_t)((inl (0xCFC) >> ((offset & 2) * 8)) & 0xffff);
    return (tmp);
 }
unsigned int pciConfigReadReg(uint8_t bus, uint8_t slot,
                            uint8_t func, uint8_t offset)
{
   uint32_t address;
   uint32_t lbus  = (uint32_t)bus;
   uint32_t lslot = (uint32_t)slot;
   uint32_t lfunc = (uint32_t)func;
   uint32_t tmp = 0;

   /* create configuration address as per Figure 1 */
   address = (uint32_t)((lbus << 16) | (lslot << 11) |
             (lfunc << 8) | (offset & 0xfc) | ((uint32_t)0x80000000));

   /* write out the address */
   outl (0xCF8, address);
   /* read in the data */
   /* (offset & 2) * 8) = 0 will choose the first word of the 32 bits register */
   tmp = (inl (0xCFC)>>(offset&2)*8);
   return (tmp);
}
unsigned char pciExist(unsigned short bus, unsigned short slot){return (unsigned short)pciConfigReadWord(bus,slot,0,0)!=0xFFFF;}
void pciEnumAll () {
    uint8_t bus;
    uint8_t device;
    unsigned short i = 0;
    for(bus = 0; bus < 255; bus++)
    {
        for(device = 0; device < 32; device++)
	{
	    int vendor = pciConfigReadWord(bus,device,0,0);
	    if(vendor!=0xFFFF)
	    {
		struct pci_device d =
		{
		    .bus=bus,
		    .device=device,
		    .class=(pciConfigReadWord(bus,device,0,0xB)),
		    .vendor = vendor
		};
		devices[i++] = d;
		unsigned char j = 1;
	        while(j<255)
	        {
		    vendor = pciConfigReadWord(bus,device,j,0);
		    if(vendor==0xFFFF){break;}
      		    struct pci_device d =
		    {
		        .bus=bus,
			.device=device,
			.class=(pciConfigReadWord(bus,device,j,0xB)),
			.vendor = vendor,
			.function = j
		    };
		    devices[i++] = d;
		    j++;
		    puts(hexdump(j));
		}
	    };
	}
    }
    n_devices=i;
}
#endif
