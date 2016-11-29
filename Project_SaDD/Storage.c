#include "storage.h"
#include <eeprom.h>
#include <sysctl.h>
#include <stdlib.h>

void EEPROM_init(){
  SysCtlPeripheralEnable(SYSCTL_PERIPH_EEPROM0);
  
  // Wait for the EEPROM module to be ready.
  while(!SysCtlPeripheralReady(SYSCTL_PERIPH_EEPROM0)){}
  
  uint32_t ui32EEPROMInit= EEPROMInit();

  if(ui32EEPROMInit != EEPROM_INIT_OK){
    while(1){}   
  }  
}

bool writeName(char *c, unsigned int address, int len){
  //Make sure constant 8 chars are stored
  if(len<8){
    for(int i=len;i<8;i++){
      c[i]=' ';
    }
  } 

  // Save a name as two unsigned 32bit ints
  for(int i=0;i<2;i++){
    uint32_t *num = malloc(4);
    *num=(uint32_t)c[4*i] << 24 |
     (uint32_t)c[4*i+1] << 16 |
     (uint32_t)c[4*i+2] << 8  |
     (uint32_t)c[4*i+3];
    EEPROMProgram(num, address, sizeof(num));
    address+=4;
  }
  return true;
}

bool writeRecord(uint32_t *t, unsigned int address, int len){
   if(address%4)
    return false;

  EEPROMProgram(t, address, len);
  return true;
}
uint32_t getRecord(unsigned int address, int len){
  if(address%4)
    return (char)0;

  uint32_t *pui32Read = malloc(4);
  EEPROMRead(pui32Read, address, sizeof(pui32Read));
  return *pui32Read;
}

void getName(char *c, unsigned int address, int len){
  uint32_t *data = malloc(8);
  EEPROMRead(data, address, 8);
  if(len<9)
    c=realloc(c,9);
  c[8]='\0';
  
  for(int i=0;i<4;i++){
    c[7-i]=(unsigned char)((data[1] >> 8*i) & 0xffu);
    c[3-i]=(unsigned char)((data[0] >> 8*i) & 0xffu);
  }
}

char * intToChar(uint32_t t){
  char *i =malloc(sizeof(char)*4);

/* least significant 8 bits: */
i[3] = (unsigned char)(t & 0xffu);
i[2] = (unsigned char)((t >> 8) & 0xffu);
i[1] = (unsigned char)((t >> 16) & 0xffu);
i[0] = (unsigned char)((t >> 24) & 0xffu);
return i;
}
