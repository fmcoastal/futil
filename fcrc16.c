/*
* crc code 
*
*/
#include <stdio.h>
#include <string.h>

#define CAVIUM
#ifdef CAVIUM
#include "cvmx-config.h"
#include "cvmx.h"

#else

#include "ftype.h"
#include "fprintbuff.h"

#endif
#include "fcrc16.h"
#include "fbuildpacket.h"


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////



#define BYTE_SWAP(x) ( ((x << 8) & 0xff00) | (( x >> 8)  & 0x00ff)) 
//#define BYTE_SWAP(x) x 



/** Reverses bit order. MSB -> LSB and LSB -> MSB. */
uint32_t reverse(uint32_t x) 
{
    uint32_t ret = 0;
    int32_t  i;
    for ( i=0; i<32; ++i) 
    {
        if ((x & 0x1) != 0) 
        {
            ret |= (0x80000000 >> i);
        }
        else {}
        x = (x >> 1);
    }
    return ret;
}


uint16_t crc16(uint8_t * message, uint32_t msgsize, uint16_t crc) 
{
    uint32_t i, j; // byte counter, bit counter
    uint8_t byte;
    uint16_t poly = 0x1021;
    uint32_t ByteData;
    uint32_t CRCData;

    i = 0;
    for (i=0; i < msgsize; ++i) 
    {
        byte = message[i];       // Get next byte.
        for (j=0; j<= 7; ++j)    // Do eight times. 
        {  
            // Take Bits MSB to LSBs
            ByteData = ( byte & (0x80 >> j)) == 0 ? 0 : 1 ;     
            // Look at MSB of CRCs
            CRCData  = ( crc  & 0x8000     ) == 0 ? 0 : 1 ;        
            if ((CRCData ^ ByteData) > 0)
                crc = (crc << 1) ^ poly;
            else 
                crc = crc << 1;
        }
    }
    return crc;
}

uint32_t crc32(uint8_t * message, uint32_t msgsize, uint32_t crc) 
{
    uint32_t i, j; // byte counter, bit counter
    uint32_t byte;
    uint32_t poly = 0x04C11DB7;
    uint32_t ByteData;
    uint32_t CRCData;

    i = 0;
    for (i=0; i<msgsize; ++i) 
    {
        byte = message[i];       // Get next byte.
        byte = reverse(byte);    // 32-bit reversal.        *********** 

        for (j=0; j<= 7; ++j)   // Do eight times.
        {
            // Take Bits MSB to LSBs  ***  Reverse made this 32 bits
            ByteData = ( byte & (0x80000000 >> j)) == 0 ? 0 : 1 ;
            // Look at MSB of CRCs
            CRCData  = ( crc  &  0x80000000      ) == 0 ? 0 : 1 ;   
            if ((CRCData ^ ByteData) > 0) 
                crc = (crc << 1) ^ poly;
            else 
                crc = crc << 1;
        }
    }
    return reverse(~crc);
}



void FCrcTest(void)
{
    int     j;

    int     CalculationLength = 8;

    uint8_t  Data16[]={0xfe,0x54,0x31,0x32,0x33,0x34,0x35,0x36};
    uint8_t  Data32[]={0x7f,0x54,0x31,0x32,0x33,0x34,0x35,0x36};

    //    uint8_t  Data16[]={0x64,0x09,0x01,0x33,0x44,0xa6,0x35,0x36};
    //    uint8_t  Data32[]={0x64,0x09,0x01,0x33,0x44,0xa6,0x35,0x36};

    //unsigned int crc32(unsigned char* message, unsigned int msgsize, unsigned int crc)

    for ( j = 1 ; j <= CalculationLength ; j++)
        printf(" %d crc32 0x%08x\n",j,crc32(&(Data32[0]),j,0xffffffff));

    printf("\n");

    for ( j = 1 ; j <= CalculationLength ; j++)
        printf(" %d crc16 0x%04x \n",j,crc16(&(Data16[0]),j,0xffff));

}


