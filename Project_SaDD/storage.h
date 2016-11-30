#ifndef STORAGE_H
#define STORAGE_H

#ifdef __cplusplus
extern "C"
{
#endif


#include <inttypes.h>
#include <stdbool.h>
#define NAME_SIZE = 2;
#define RECORDS_STORED =10;
#define BUTTON_NAME_ADDR =0;
#define SHAKE_NAME_ADDR =4*RECORDS_STORED;

void EEPROM_init();
bool writeName(char *c, unsigned int address, int len);
void getName(char *c, unsigned int address, int len);
bool writeRecord(const uint32_t *t, unsigned int address, int len);
uint32_t getRecord(unsigned int address, int len);
char * intToChar(uint32_t t);

#ifdef __cplusplus
}
#endif

#endif
