#include <stdio.h>
#include <p18f2680.h>
#include <delays.h>
#include <usart.h>
#include "can18xx8.h"



#define XTAL 10000000
#define led PORTCbits.RC0

#pragma config OSC = HS
#pragma config FCMEN = OFF
#pragma config IESO = OFF
#pragma config PWRT = OFF

#pragma config WDT = OFF

#pragma config MCLRE = ON

#pragma config LPT1OSC = OFF
#pragma config PBADEN = OFF
#pragma config DEBUG = OFF
#pragma config XINST = OFF
#pragma config BBSIZ = 1024
#pragma config LVP = OFF



/////VARIABLES GLOBALES ////

int packetPos = 0;
int packetSize;
unsigned long packetId;
unsigned char packetData[8];

/////VARIABLES GLOBALES ////
void high_isr(void);
void low_isr(void);


/////*INTERRUPTIONS*/////

#pragma code high_vector=0x08

void high_interrupt(void) {
    _asm GOTO high_isr _endasm
}
#pragma code low_vector=0x18

void low_interrupt(void) {
    _asm GOTO low_isr _endasm
}
#pragma code

#pragma interrupt high_isr

void high_isr(void) {
    if (PIE3bits.RXB0IE && PIR3bits.RXB0IF || PIE3bits.RXB1IE && PIR3bits.RXB1IF) { // CAN reception

        char data[8];
        unsigned long id;
        int len;
        enum CAN_RX_MSG_FLAGS flag;
        int i;

        while (CANIsRxReady()) { // Read all available messages.
            if(!CANReceiveMessage(&id, data, &len, &flag))
                break;
            
            led ^= 1;

            // [ FD ] [ size | 0 | id10..8 ] [ id7..0] [ M1 ] [ M2 ] ? [ M8 ] [ BF ]
            while (BusyUSART());
            WriteUSART(0xFD);
            while (BusyUSART());
            WriteUSART(len << 4 | id >> 8);
            while (BusyUSART());
            WriteUSART(id & 0xFF);
            while (BusyUSART());
            for(i = 1; i <= len && i <= 8; i++) {
                while (BusyUSART());
                WriteUSART(data[i]);
            }
            while (BusyUSART());
            WriteUSART(0xBF);

        }

       PIR3bits.ERRIF = 0; // TODO
        if (PIE3bits.RXB0IE)
            PIR3bits.RXB0IF = 0;
        if (PIE3bits.RXB1IE)
            PIR3bits.RXB1IF = 0;
    }

    if (PIE1bits.RCIE && PIR1bits.RCIF) { // UART reception.
        unsigned char c = ReadUSART();
        PIR1bits.RCIF = 0;
        
        if(packetPos == 0 && c == 0xFD) { // (si c'est pas 0xFD on ignore)
            // [ 0xFD ] Packet initialization.
            packetPos = 1;
        }
        else if(packetPos == 1 && (c & 0b00001000) == 0) { // Le cinqui�me bit est � 0.
            // [ size3..0 | 0 | id10..8 ]
            packetSize = c >> 4;
            packetId = (c & 0b00000111) << 8;
            
            if(packetSize <= 8)
                packetPos = 2;
            else
                packetPos = 0; // Discard packet.
        }
        else if(packetPos == 2) {
            // [ id7..0 ]
            packetId |= c;
            packetPos = 3;
        }
        else if(3 <= packetPos && packetPos < 3 + packetSize) {
            // Message byte (from 0 to 8).
            packetData[packetPos - 3] = c;
            packetPos++;
        }
        else if(packetPos == 3 + packetSize && c == 0xBF) {
            // [ 0xBF ] Packet end.
            CANSendMessage(packetId, packetData, packetSize, CAN_TX_PRIORITY_0 & CAN_TX_STD_FRAME & CAN_TX_NO_RTR_FRAME);
            packetPos = 0;
        }
        else // Malformed packet, ignored.
            packetPos = 0;
    }
}

#pragma interrupt low_isr

void low_isr(void) {


}

void OpenCAN(unsigned long mask0, unsigned long filter00, unsigned long filter01, unsigned long mask1, unsigned long filter10, unsigned long filter11, unsigned long filter12, unsigned long filter13) {
    CANInitialize(1, 2, 6, 3, 2, CAN_CONFIG_VALID_STD_MSG);

    /*Config interupt CAN- Buffeur 0*/
    IPR3bits.RXB0IP = 1; // priorit� haute
    PIE3bits.RXB0IE = 1; // autorise int sur buff1
    PIR3bits.RXB0IF = 0; // mise � 0 du flag

    /*Config interupt CAN- Buffeur 1*/
    IPR3bits.RXB1IP = 1; // priorit� haute
    PIE3bits.RXB1IE = 1; // autorise int sur buff1
    PIR3bits.RXB1IF = 0; // mise � 0 du flag

    //config des mask et filtres
    // Set CAN module into configuration mode
    CANSetOperationMode(CAN_OP_MODE_CONFIG);
    // Set Buffer 1 Mask value
    CANSetMask(CAN_MASK_B1, mask0, CAN_CONFIG_STD_MSG);
    // Set Buffer 2 Mask value
    CANSetMask(CAN_MASK_B2, mask1, CAN_CONFIG_STD_MSG);
    // Set Buffer 1 Filter values
    CANSetFilter(CAN_FILTER_B1_F1, filter00, CAN_CONFIG_STD_MSG);
    CANSetFilter(CAN_FILTER_B1_F2, filter01, CAN_CONFIG_STD_MSG);
    CANSetFilter(CAN_FILTER_B2_F1, filter10, CAN_CONFIG_STD_MSG);
    CANSetFilter(CAN_FILTER_B2_F2, filter11, CAN_CONFIG_STD_MSG);
    CANSetFilter(CAN_FILTER_B2_F3, filter12, CAN_CONFIG_STD_MSG);
    CANSetFilter(CAN_FILTER_B2_F4, filter13, CAN_CONFIG_STD_MSG);

    // Set CAN module into Normal mode
    CANSetOperationMode(CAN_OP_MODE_NORMAL);
}

////////*PROGRAMME PRINCIPAL*////////

void main(void) {
    //initialisations
    ADCON1 = 0x0F;
    ADCON0 = 0b00000000;
    WDTCON = 0;

    /* Direction des ports I in, O out*/
    TRISA = 0b11111111;
    TRISB = 0b01111011; //canTX en sortie
    TRISC = 0b11111110;

    /* Etat des sorties */
    PORTA = 0b11111111;
    PORTB = 0b11111111;
    PORTC = 0b11111111;

    /* USART */
    OpenUSART(USART_TX_INT_OFF & USART_RX_INT_ON & USART_ASYNCH_MODE
            & USART_EIGHT_BIT & USART_CONT_RX & USART_BRGH_HIGH, 77);
    IPR1bits.RCIP = 1; // priorit� haute comme le CAN pour pas qu'il s'interrompent l'un l'autre

    OpenCAN(0, 0, 0, 0, 0, 0, 0, 0);

    /*Config interupt General*/
    INTCONbits.PEIE = 1;
    INTCONbits.GIE = 1;

    while (1) {
       Delay10KTCYx(5);
    
       CANSendMessage(77, NULL, 0, CAN_TX_PRIORITY_0 & CAN_TX_STD_FRAME & CAN_TX_NO_RTR_FRAME);
    }
}
