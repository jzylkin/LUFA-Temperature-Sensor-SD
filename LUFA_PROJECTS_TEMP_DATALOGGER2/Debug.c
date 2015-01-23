/*
 * Debug.c
 *
 * Created: 1/22/2015 11:18:42 PM
 *  Author: Jack
 */ 
#include "TempDataLogger.h"

void Hang(uint8_t led_pulses){
	set_low(LED1);
	Delay_MS(500);
	set_high(LED1);
	Delay_MS(500);
	while(is_high(S1)){;}
}
