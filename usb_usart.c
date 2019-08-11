#include <stdint.h>
#include <string.h>

#include "usb_usart.h"
#include "mcc_generated_files/usb/usb_device_cdc.h"
#include "mcc_generated_files/eusart.h"

#define ENABLE_FORWARD_MODE_CHAR '^'
#define OUTBUF_SIZE 64
#define OUTBUF_LAST_INDEX (OUTBUF_SIZE - 1)

uint8_t inBuf[32];
uint8_t inBufPos = 0;

#ifdef USB_USART_FORWARDMODE
uint8_t isForwardModeEnabled = 0;
#endif

uint8_t outBuf[OUTBUF_SIZE];
uint8_t outWriteHead = 0;
uint8_t outWriteStart = 0;
uint8_t wasLastCR = 0;

void(*pParserFunc)(char*) = NULL;

void OutBufService(void);

void USB_USART_SetCommandParser(void(*func)(char*)) {
    pParserFunc = func;
}

void USB_USART_Update() {
    uint8_t nRead,
            inBufEnd,
            iBuf,
            newIBuf,
            ch,
            realignBuf;

    CDCTxService();
    OutBufService();

#ifdef USB_USART_FORWARDMODE
    if (isForwardModeEnabled) {
        while (EUSART_is_rx_ready()) {
            ch = EUSART_Read();
            USB_USART_WriteToOutBuf(&ch, 1);
        }
    }
#endif

    if (inBufPos >= sizeof (inBuf)) {
        // can't handle input, so overwrite 
        // current content of inBuf
        USB_USART_WriteStrToOutBuf("--<");
        inBufPos = 0;
    }

    nRead = getsUSBUSART(inBuf + inBufPos, sizeof (inBuf) - inBufPos);

    inBufEnd = inBufPos + nRead;
    for (iBuf = inBufPos; iBuf < inBufEnd; iBuf++) {
        realignBuf = 0;
        ch = inBuf[iBuf];

#ifdef USB_USART_FORWARDMODE  
        if (ch == ENABLE_FORWARD_MODE_CHAR) {
            // toggle forward mode
            isForwardModeEnabled = (isForwardModeEnabled) ? 0 : 1;
            if (isForwardModeEnabled) {
                USB_USART_WriteStrToOutBuf("\nFWD_MODE_ENABLED\n");
            } else {
                USB_USART_WriteStrToOutBuf("\nFWD_MODE_DISABLED\n");
            }
            realignBuf = 1;
        } else if (isForwardModeEnabled) {
            if (EUSART_is_tx_ready()) {
                EUSART_Write(ch);
            }
        } else {
#endif
            // Echo entered char
            USB_USART_PutToOutBuf(ch);

            if (ch == '\n') {
                for (newIBuf = iBuf; newIBuf != 0xFF; newIBuf--) {
                    if (inBuf[newIBuf] < 0x1B) {
                        inBuf[newIBuf] = 0;
                    } else {
                        break;
                    }
                }
                if (pParserFunc != NULL) {
                    pParserFunc((char*) inBuf);
                }
                realignBuf = 1;
            }
#ifdef USB_USART_FORWARDMODE
        }
#endif

        if (realignBuf) {
            newIBuf = 0;
            iBuf++; // char after current one
            while (iBuf < inBufEnd) {
                inBuf[newIBuf++] = inBuf[iBuf++];
            }

            inBufEnd = newIBuf;
            inBufPos = 0;
            iBuf = 0xFF; // when incremented in for-loop, will be 0x00
        }
    } // end for (each new char in inBuf)

#ifdef USB_USART_FORWARDMODE
    if (isForwardModeEnabled) {
        // use fresh buffer next time
        inBufPos = 0;
    } else {
        // continue current buffer next time
        inBufPos = inBufEnd;
    }
#else
    inBufPos = inBufEnd;
#endif
    
}

void OutBufService() {
    if (!USBUSARTIsTxTrfReady() || outWriteHead == outWriteStart) {
        return;
    }

    if (outWriteHead > outWriteStart) {
        putUSBUSART(outBuf + outWriteStart, outWriteHead - outWriteStart);
        outWriteStart = outWriteHead;
    } else {
        putUSBUSART(outBuf + outWriteStart, OUTBUF_SIZE - outWriteStart);
        outWriteStart = 0;
    }
}

void _PutToOutBuf(uint8_t ch) {
    uint8_t newWriteHead = (outWriteHead == OUTBUF_LAST_INDEX) ? 0 : outWriteHead + 1;

    if (newWriteHead == outWriteStart || (wasLastCR != 0 && ch == '\r')) {
        return;
    }

    outBuf[outWriteHead] = ch;
    outWriteHead = newWriteHead;

    wasLastCR = (ch == '\r') ? 1 : 0;
}

void USB_USART_WriteNHex(uint8_t* buf, size_t bufSize) {
    while (bufSize--) {
        USB_USART_WriteHex(*buf);
        buf++;
    }
}

void USB_USART_WriteHex(uint8_t num) {
    uint8_t i, h;
    for (i = 0; i < 2; i++) {
        h = (i == 0)
                ? (num >> 4)
                : (num & 0xF);
        _PutToOutBuf((h < 0xA) ? '0' + h : ('A' - 0xA) + h);
    }
}

void USB_USART_PutToOutBuf(uint8_t ch) {
    if (ch == '\n') {
        _PutToOutBuf('\r');
        _PutToOutBuf('\n');
    } else {
        _PutToOutBuf(ch);
    }
}

void USB_USART_WriteToOutBuf(uint8_t *buffer, uint8_t len) {
    for (; len > 0; len--, buffer++) {
        USB_USART_PutToOutBuf(*buffer);
    }
}

void USB_USART_WriteStrToOutBuf(const char* string) {
    uint8_t len = strlen(string);
    for (uint8_t i = 0; i < len; i++) {
        USB_USART_PutToOutBuf(string[i]);
    }
}
