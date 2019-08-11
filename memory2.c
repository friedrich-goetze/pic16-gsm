#include "memory2.h"
#include "mcc_generated_files/usb/usb_device.h"

const uint16_t user_flash[USER_FLASH_SIZE_WORDS] __at(_USER_FLASH_ADDR) = {0};

uint16_t pFlashBuf[USER_FLASH_SIZE_WORDS];
uint8_t* pUserFlashWnd = ((uint8_t*)pFlashBuf) + _USER_FLASH_BLOCK_OFFSET;

uint16_t IndexToBlockAddr(uint8_t idx) {
    if (idx >= USER_FLASH_BLOCK_COUNT) {
        return 0;
    }
    return _USER_FLASH_ADDR + (idx * _USER_FLASH_BLOCK_SIZE_WORDS);
}

uint8_t* USER_FLASH_Get(void) {
    return (uint8_t*) pUserFlashWnd;
}

int8_t USER_FLASH_ReadBlock(uint8_t index) {
    uint16_t blockAddr = IndexToBlockAddr(index);
    if(blockAddr == 0) {
        return -1;
    }
    
    uint16_t wordPair[2];
    uint8_t *pWordPair = (uint8_t*)wordPair;
    uint8_t *pByteTripple = (uint8_t*)pFlashBuf;
    
    for(uint8_t i = 0; i < (_USER_FLASH_BLOCK_SIZE_WORDS >> 1); i++) {
        wordPair[0] = FLASH_ReadWord(blockAddr++);
        wordPair[1] = FLASH_ReadWord(blockAddr++);
        
        pByteTripple[0] = (pWordPair[0] >> 4) | (pWordPair[1] << 4);
        pByteTripple[1] = (pWordPair[0] << 4) | (pWordPair[3] & 0xF);
        pByteTripple[2] = pWordPair[2];
        
        pByteTripple += 3;
    }
    
    return 0;
}

int8_t USER_FLASH_WriteBlock(uint8_t index) {
    uint16_t blockAddr = IndexToBlockAddr(index);
    if(blockAddr == 0) {
        return -1;
    }
    
    uint8_t tripple[3];
    
    uint8_t *pDstWordPair = (uint8_t*)(pFlashBuf + _USER_FLASH_BLOCK_SIZE_WORDS - 2);
    uint8_t *pSrcByteTripple = (uint8_t*)pFlashBuf + USER_FLASH_BLOCK_SIZE + _USER_FLASH_BLOCK_OFFSET - 3;

    for(uint8_t i = 0; i < (_USER_FLASH_BLOCK_SIZE_WORDS >> 1); i++) {
        // read tripple
        memcpy(tripple, pSrcByteTripple, 3);
        
        pDstWordPair[0] = (tripple[0] << 4) | (tripple[1] >> 4);
        pDstWordPair[1] = tripple[0] >> 4;
        pDstWordPair[2] = tripple[2];
        pDstWordPair[3] = tripple[1] & 0xF;

        pSrcByteTripple -= 3;
        pDstWordPair -= 4;
    }

    if(FLASH_WriteBlock(blockAddr, pFlashBuf) != 0) {
        return -1;
    }

    // TODO RM
    return 0;
    // return USER_FLASH_ReadBlock(index);
}