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
        Device            :  PIC16F18855
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

// 1ms割り込み処理
uint8_t sInterval;
void isr_TMR0(void){
	sInterval ++;
}

#define STX		0x02
#define ETX		0x03
#define DLE		0x10

#define POWER_OFF_VALUE	10000

uint8_t revDat[32],revLength;
bool isReceived(void){
	return (revLength) ?1 :0 ;
}

void _receive( void ){
	static bool isDle = false;
	static uint8_t pos = 0;
	uint8_t data = RC1REG;

	if( isDle ){
		isDle = false;
		switch( data ){
		case STX:
			pos = 0;
			break;
		case ETX:
			revLength = pos;
			pos = 0;
			break;
		case DLE:
			revDat[pos++] = data;
			break;
		}
	}else if( data == DLE ){
		isDle = true;
	}else{
		revDat[pos++] = data;
	}
}


enum {AC_NONE=0, AC_50Hz, AC_60Hz};
uint8_t gAC = AC_NONE;
uint16_t _phaseOutBaseValue = POWER_OFF_VALUE;

uint16_t GetPhaseValue( void ){
	uint16_t val;
	switch( gAC ){
	case AC_50Hz:	val = (uint16_t)(_phaseOutBaseValue *1.2f);	break;
	case AC_60Hz:	val = _phaseOutBaseValue;					break;
	default:		val = POWER_OFF_VALUE;						break;
	}
	return val;
}

// ゼロクロス時間計測
#define CHECK_ZC_COUNT		10
void isr_CCP1_CAP(uint16_t val){
	const uint16_t Threshold = 9300;
	static uint16_t exVal;
	uint16_t time;
	static int8_t Count=0;

	time = val - exVal;
	exVal = val;
			
	if( gAC == AC_NONE ){
		// 50Hz 60Hz判定処理
		if( time > Threshold ){		// 50Hzの可能性
			if( Count > CHECK_ZC_COUNT ){ Count ++; }
			else{
				gAC = AC_50Hz;
				TMR3_StartTimer();
			}
		}else{
			if( Count > -CHECK_ZC_COUNT ){ Count --; }
			else{
				gAC = AC_60Hz;
				TMR3_StartTimer();
			}
		}
	}else{
		static uint8_t cnt=0;
		if( cnt <60 ) cnt ++;
		else{
			cnt = 0;
			oPOW_Toggle();
		}

		static uint8_t StartMaxTime = 35;
		if( _phaseOutBaseValue == POWER_OFF_VALUE ) StartMaxTime = 35;
		else if( StartMaxTime ) StartMaxTime --;
		
		// 判定完了後
		if( ((_phaseOutBaseValue!=POWER_OFF_VALUE) && StartMaxTime)  || GetPhaseValue() == 0 ){
			LATBbits.LATB5 = 1;
		}else{
			LATBbits.LATB5 = 0;
			TMR3_WriteTimer(0);
			CCP3_SetCompareCount(GetPhaseValue());
		}
	}
}

/*
                         Main application
 */
void main(void){
    // initialize the device
    SYSTEM_Initialize();

    // When using interrupts, you need to set the Global and Peripheral Interrupt Enable bits
    // Use the following macros to:
	TMR0_SetInterruptHandler(isr_TMR0);
	CCP1_SetCallBack(isr_CCP1_CAP);
	EUSART_SetRxInterruptHandler(_receive);
    // Enable the Global Interrupts
	INTERRUPT_GlobalInterruptEnable();

    // Enable the Peripheral Interrupts
	INTERRUPT_PeripheralInterruptEnable();

    // Disable the Global Interrupts
    //INTERRUPT_GlobalInterruptDisable();

    // Disable the Peripheral Interrupts
    //INTERRUPT_PeripheralInterruptDisable();

	oPOW_SetLow();
	oCOOLFAN_SetHigh();
	
	oI2C_EN_SetLow();
	LATBbits.LATB5 = 0;
		
    while (1)
    {
        // Add your application code
		if( sInterval ){
			sInterval --;
		}
		
		if( isReceived() ){
			uint16_t tmp = ((uint16_t)revDat[1]<<8 | revDat[0]);
			if( tmp < 100 ) tmp = POWER_OFF_VALUE;
			else if( tmp > 8100 ) tmp = 0;
			else tmp = 8333-tmp;
			_phaseOutBaseValue = tmp;
			revLength = 0;
		}
		
    }
}
/**
 End of File
*/