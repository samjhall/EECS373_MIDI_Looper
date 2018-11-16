/*
 * project_helpers.c
 *
 *  Created on: Nov 16, 2018
 *      Author: samjhall
 */

#include "drivers/mss_uart/mss_uart.h"
#include "drivers/CoreUARTapb/core_uart_apb.h"
#include "drivers/mss_timer/mss_timer.h"
#include "project_helpers.h"
#include <stdint.h>

/***	GLOBAL	***/
UART_instance_t apb_uart;

void Global_init() {
	MIDI_init();
	APB_UART_init();
	Timer_init();
}

/***	MIDI (UART1)	***/

void MIDI_init(){ // midi board on APB UART
	MSS_UART_init(&g_mss_uart1, 31250, MSS_UART_DATA_8_BITS | MSS_UART_NO_PARITY | MSS_UART_ONE_STOP_BIT );
}

void programChange(uint8_t channel, uint8_t program) {
	uint8_t buffer[2] = {0xC0 | channel, program};
	MSS_UART_polled_tx(&g_mss_uart1, buffer, 2);
}

void noteOn(uint8_t channel, uint8_t pitch, uint8_t attack) {
	uint8_t buffer[3] = {0x90 | channel, pitch, attack};
	MSS_UART_polled_tx(&g_mss_uart1, buffer, 3);
}

void noteOff(uint8_t channel, uint8_t pitch, uint8_t attack) {
	uint8_t buffer[3] = {0xC0 | channel, pitch, attack};
	MSS_UART_polled_tx(&g_mss_uart1, buffer, 3);
}

void allNotesOff() {
	uint8_t clearMIDI[1] = {123};
	MSS_UART_polled_tx(&g_mss_uart1, clearMIDI, 1);
}



/***	APB UART DEVICES	***/

void APB_UART_init(){ // character display and keypad
	UART_init(&apb_uart, BASE, 650, (DATA_8_BITS | NO_PARITY));
}

size_t readKeypad(uint8_t* buffer) {
	return UART_get_rx(&apb_uart, buffer, sizeof(buffer));
}

void sendCharDisplay(uint8_t* buffer, uint8_t size) {
	UART_send(&apb_uart, buffer, size);
}



/***	TIMER	***/

void Timer_init() {
	MSS_TIM1_init( MSS_TIMER_PERIODIC_MODE );
}

void Timer_set_and_start(uint32_t cycle_count) {
	MSS_TIM1_load_immediate(cycle_count);
	MSS_TIM1_enable_irq();
	MSS_TIM1_start();
}

// This should defined in main.c so it is easier to modify
//void Timer1_IRQHandler() {
//	MSS_TIM1_clear_irq();
//}

/* USEFUL FUNCTIONS
 * MSS_TIM1_load_immediate( # of cycles );
 * MSS_TIM1_enable_irq();
 * MSS_TIM1_start();
 * MSS_TIM1_clear_irq();
 */
