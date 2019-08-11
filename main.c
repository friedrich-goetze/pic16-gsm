/**
  Generated Main Source File

  Company:
    Microchip Technology Inc.

  File Name:
    main.c

  Summary:
    This is the main file generated using PIC10 / PIC12 / PIC16 / PIC18 MCUs

  Description:
    This header file provides implementations for driver APIs for all modules selected in the GUI.
    Generation Information :
        Product Revision  :  PIC10 / PIC12 / PIC16 / PIC18 MCUs - 1.77
        Device            :  PIC16F1459
        Driver Version    :  2.00
*/

/*
    (c) 2018 Microchip Technology Inc. and its subsidiaries. 
    
    Subject to your compliance with these terms, you may use Microchip software and any 
    derivatives exclusively with Microchip products. It is your responsibility to comply with third party 
    license terms applicable to your use of third party software (including open source software) that 
    may accompany Microchip software.
    
    THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS". NO WARRANTIES, WHETHER 
    EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE, INCLUDING ANY 
    IMPLIED WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY, AND FITNESS 
    FOR A PARTICULAR PURPOSE.
    
    IN NO EVENT WILL MICROCHIP BE LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE, 
    INCIDENTAL OR CONSEQUENTIAL LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND 
    WHATSOEVER RELATED TO THE SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP 
    HAS BEEN ADVISED OF THE POSSIBILITY OR THE DAMAGES ARE FORESEEABLE. TO 
    THE FULLEST EXTENT ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL 
    CLAIMS IN ANY WAY RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT 
    OF FEES, IF ANY, THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR THIS 
    SOFTWARE.
*/

#include "mcc_generated_files/mcc.h"
#include "usb_usart.h"
#include "memory2.h"

#include <stdio.h>
#include <stdlib.h>

void parseUSBCommand(char* pStr);

typedef struct {
    unsigned    isSet               :1;
    unsigned    smsAlert            :1;
    unsigned    smsAlarmActive      :1;
    unsigned    smsAllowStatus      :1;
    unsigned    leadingZeros        :4;
    uint64_t    telNumber;
} UserConfigEntry;

//#define USB_AUTO_ATTACH
//void MCC_USB_CDC_DemoTasks(void);
/*
                         Main application
 */
void main(void)
{
    // initialize the device
    CDCSetLineCoding(115200, NUM_STOP_BITS_1, PARITY_NONE, 8);
    SYSTEM_Initialize();

    // When using interrupts, you need to set the Global and Peripheral Interrupt Enable bits
    // Use the following macros to:

    // Enable the Global Interrupts
    INTERRUPT_GlobalInterruptEnable();

    // Enable the Peripheral Interrupts
    INTERRUPT_PeripheralInterruptEnable();

    // Disable the Global Interrupts
    //INTERRUPT_GlobalInterruptDisable();

    // Disable the Peripheral Interrupts
    //INTERRUPT_PeripheralInterruptDisable();
    
    USB_USART_SetCommandParser(parseUSBCommand);

#ifdef USB_AUTO_ATTACH
    USBDeviceAttach();
#endif
    
    while (1)
    {
#ifdef USB_POLLING
        USBDeviceTasks();
#endif
#ifndef USB_AUTO_ATTACH
        // Sense is Vusb is present, use this info to 
        // attach or detach this device
        if(USBGetDeviceState() == DETACHED_STATE && VUSB_SENSE_IO_PORT) {
            USBDeviceAttach();
        }
        else if(USBGetDeviceState() > DEFAULT_STATE && !VUSB_SENSE_IO_PORT) {
            USBDeviceDetach();
        }
#endif
        if(USBGetDeviceState() == CONFIGURED_STATE && !USBIsDeviceSuspended()) {
            USB_USART_Update();
            CDCTxService();
        }
    }
}

#define MAX_ARGS 3
char *args[MAX_ARGS];
char str[8];

void parseUSBCommand(char* pStr) {
    uint8_t i;
    int8_t si;
    char *p;
    uint8_t nArgs = 0;
    uint8_t idx;
    uint8_t len;
    
    /*
     * Parse string for words
     */
    p = pStr;
    while(nArgs < MAX_ARGS) {
        args[nArgs] = p;
        if(*p == 0) {
            break;
        }
        
        while(*p != 0 && *p != ' ') {
            p++;
        }
        
        // escape arg
        if(*p != 0) {
            *p = 0;
            // goto next word
            p++;
        }
        nArgs++;
    }
    
    USB_USART_WriteStrToOutBuf("nW=0x");
    USB_USART_WriteHex(nArgs);
    USB_USART_PutToOutBuf('\n');
    for(i = 0; i < nArgs; i++) {
        USB_USART_PutToOutBuf('\'');
        USB_USART_WriteStrToOutBuf(args[i]);
        USB_USART_PutToOutBuf('\'');
        USB_USART_PutToOutBuf('\n');
    }
    
    if(nArgs == 0) {
        return;
    }
    
    /*
     * Execute commands
     */
    if(nArgs > 1 && strcmp("flash", args[0] + 1) == 0) {
        uint8_t isWrite = (*args[0] == 'w') ? 1 : 0;
        if(nArgs < 2 + isWrite) {
            USB_USART_WriteStrToOutBuf("req more args\n");
        }
        idx = (uint8_t)(*args[1]) - '0';
        if(idx >= USER_FLASH_BLOCK_COUNT) {
            USB_USART_WriteStrToOutBuf("illegal 1st arg\n");
            return;
        }
        
        if(isWrite) {
            len = strlen(args[2]);
            if(len == 0) {
                USB_USART_WriteStrToOutBuf("no data\n");
                return;
            }
            
            memset(USER_FLASH_Get(), 0, USER_FLASH_BLOCK_SIZE);
            memcpy(USER_FLASH_Get(), args[2], len);
            si = USER_FLASH_WriteBlock(idx);
            USB_USART_PutToOutBuf(si == 0 ? '0' : 'E');
            USB_USART_PutToOutBuf('\n');
        } else if(*args[0] == 'r') {
            si = USER_FLASH_ReadBlock(idx);
            if(si != 0) {
                USB_USART_PutToOutBuf(si == 0 ? '0' : 'E');
                USB_USART_PutToOutBuf('\n');
                return;
            }
            
            // print first 32 bytes
            uint8_t ch;
            for(i = 0; i < 32; i++) {
                ch = USER_FLASH_Get()[i];
                if(ch < 0x20 || ch > 0x7D) {
                    ch = '.';   
                }
                USB_USART_PutToOutBuf(ch);
            }
            USB_USART_PutToOutBuf('\n');
        }
    } else {
        USB_USART_WriteStrToOutBuf("help: rflash [idx], wflash [idx] [data]\n");
    }
}
/**
 End of File
*/