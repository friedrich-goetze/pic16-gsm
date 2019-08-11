#ifndef __MEMORY_2__
#define __MEMORY_2__

#include <xc.h>
#include <stdint.h>
#include "mcc_generated_files/memory.h"

#if ERASE_FLASH_BLOCKSIZE != WRITE_FLASH_BLOCKSIZE
#error "Your pic is not supported."
#endif
#define _FLASH_WORD_WIDTH 12

#define USER_FLASH_SIZE_WORDS 64
#define _USER_FLASH_SIZE ((USER_FLASH_SIZE_WORDS * _FLASH_WORD_WIDTH) / 8)
#define _USER_FLASH_ADDR (END_FLASH - USER_FLASH_SIZE_WORDS)
#define _USER_FLASH_BLOCK_SIZE_WORDS ERASE_FLASH_BLOCKSIZE
// Blocksize from memory.h is in words. Each Word is 14 bits long
#define _USER_FLASH_BLOCK_OFFSET 4
// 44
#define USER_FLASH_BLOCK_SIZE (((_USER_FLASH_BLOCK_SIZE_WORDS * _FLASH_WORD_WIDTH) / 8) - _USER_FLASH_BLOCK_OFFSET)
#define _USER_FLASH_BLOCK_COUNT (USER_FLASH_SIZE_WORDS / _USER_FLASH_BLOCK_SIZE_WORDS)

#ifndef _USER_FLASH_ADDR
#error "Please define where user flash memory starts"
#elif (_USER_FLASH_ADDR & ((END_FLASH-1) ^ (ERASE_FLASH_BLOCKSIZE-1))) != _USER_FLASH_ADDR
#error "USER_FLASH_ADDR doesn't address a valid block-address"
#endif

uint8_t* USER_FLASH_Get(void);

int8_t USER_FLASH_ReadBlock(uint8_t index);

int8_t USER_FLASH_WriteBlock(uint8_t index);

#endif