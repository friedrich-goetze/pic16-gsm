#ifndef __USB_USART__
#define __USB_USART__

#include <xc.h>
#define USB_USART_FORWARDMODE

void USB_USART_WriteNHex(uint8_t* buf, size_t bufSize);
void USB_USART_WriteHex(uint8_t bum);
void USB_USART_PutToOutBuf(uint8_t ch);
void USB_USART_WriteToOutBuf(uint8_t *buffer, uint8_t len);
void USB_USART_WriteStrToOutBuf(const char* string);

void USB_USART_SetCommandParser(void(*pParserFunc)(char*));

void USB_USART_Update(void);

#endif