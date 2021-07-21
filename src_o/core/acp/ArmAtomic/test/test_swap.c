/*
 * =====================================================================================
 *
 *       Filename:  test.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  07/03/18 13:26:16
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (), 
 *   Organization:  
 *
 * =====================================================================================
 */
#include <stdlib.h>
#include "acpByteOrder.h"
#include "acpTime.h"

int main(int argc, char **argv)
{
	unsigned short swapTest16 = 0xABCD, outTest16;
	unsigned int swapTest32 = 0x12345678, outTest32;
	unsigned long swapTest64 = 0x1234567890ABCDEF, outTest64;

	unsigned long time_ticks;

	outTest16 = acpByteSwap16(swapTest16);
	outTest32 = acpByteSwap32(swapTest32);
	outTest64 = acpByteSwap64(swapTest64);

	printf("16bit swap: %x => %x\n", swapTest16, outTest16);
	printf("32bit swap: %x => %x\n", swapTest32, outTest32);
	printf("64bit swap: %lx => %lx\n", swapTest64, outTest64);

	time_ticks = acpTimeTicks();
	printf("Time: %lx\n", time_ticks);

	return 0;
}
