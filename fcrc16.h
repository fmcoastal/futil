
#ifndef   _FCRC16_H
#define   _FCRC16_H
/*
 * crc code 
 *
 */
#define NEW_CRC16_SEED  ((uint32_t)0xffff)
#define NEW_CRC32_SEED  ((uint32_t)0xffffffff)

/** Reverses bit order. MSB -> LSB and LSB -> MSB. */
uint32_t reverse(uint32_t x); 


//   for starting new crc calc, make sure crc=0xffffffff
uint16_t crc16(uint8_t * message, uint32_t msgsize, uint16_t crc); 

//   for starting new crc calc, make sure crc=0xffffffff
uint32_t crc32(uint8_t * message, uint32_t msgsize, uint32_t crc); 


void  FCrcTest(void);


#endif




