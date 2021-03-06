/*
* Programme carte suiveur petit robot
* Eurobot 2012
* Compiler : Microchip C18
* �C : 18f2680
* Jan.21 2011
*    ____________      _           _
*   |___  /| ___ \    | |         | |
*      / / | |_/ /___ | |__   ___ | |_
*     / /  |    // _ \| '_ \ / _ \| __|
*    / /   | |\ \ (_) | |_) | (_) | |_
*   /_/    |_| \_\___/|____/ \___/'\__|
*			      7robot.fr
*/

#include <stdio.h>
#include <stdlib.h>
#include <timers.h>
#include <p18f2680.h>


/////*CONFIGURATION*/////
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

/////*CONSTANTES*/////
#define XTAL    20000000
#define led     PORTAbits.RA4


/////*PROTOTYPES*/////
void high_isr(void);
void low_isr(void);
void DelayMS(int delay);

/////*VARIABLES GLOBALES*/////
int i=0;

/////*INTERRUPTIONS*/////

#pragma code high_vector=0x08
void high_interrupt(void)
{
     _asm GOTO high_isr _endasm
}
#pragma code low_vector=0x18
void low_interrupt(void)
{
     _asm GOTO low_isr _endasm
}
#pragma code

#pragma interrupt high_isr
void high_isr(void)
{


}

#pragma interrupt low_isr
void low_isr(void)
{

}


/////*PROGRAMME PRINCIPAL*/////
void main (void)
{
    /* Initialisations. */
    ADCON1 = 0x0F ;
    ADCON0 = 0b00000000;
    WDTCON = 0 ;

    /* Configurations. */
    TRISA   = 0b11101111 ;
    TRISB   = 0xFF ;
    TRISC   = 0xFF ;

    /* Signal de d�marrage du programme. */
    led = 0;
    for(i=0;i<20;i++)
    {
        led=led^1;
        DelayMS(50);
    }
    led = 0;

    INTCONbits.GIE = 1; /* Autorise interruptions. */

    DelayMS(1000);


    /* Boucle principale. */
     while(1)
    {

    }
}

///// D�finition des fonctions du programme. /////
void DelayMS(int delay)
{
    /*Attente en ms, sur cette carte c'est utile, et vu que le Quart est soud�,
     il y a peu de raisons pour que �a change...*/
    int cp = 0;
    for(cp=0; cp<delay; cp++)
    {
        Delay1KTCYx(5);
    }
}

