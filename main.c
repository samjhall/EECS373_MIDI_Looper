#include "drivers/mss_uart/mss_uart.h"
#include "drivers/CoreUARTapb/core_uart_apb.h"
#include "drivers/mss_timer/mss_timer.h"
#include "project_helpers.h"

//(PADDR >= 0x40050000 && PADDR <= 0x400500FF

// baud val (written to BASE + 0x008) = (clock / (16 * baud rate)) - 1
// for 100 MHz at 9600 baud, this is 650 in decimal

//#define BASE 0x40050000
//UART_instance_t apb_uart;

void timer_test() {
	MSS_TIM1_init( MSS_TIMER_PERIODIC_MODE );

	MSS_TIM1_load_immediate( 50000000 );

	MSS_TIM1_enable_irq();

	MSS_TIM1_start();

	uint8_t programChange[2] = {0xC0, 0x64};
	uint8_t readBuf[1] = {0x51};
	while(1) {
		//uint32_t timer_temp = MSS_TIM1_get_current_value();

		uint8_t temp = UART_get_rx(&apb_uart, readBuf, sizeof(readBuf));
		if(temp != 0) {
			programChange[1] = readBuf[0];
			MSS_UART_polled_tx(&g_mss_uart1, programChange, sizeof(programChange));
		}
	}
}

/*
uint8_t tmpArray[4] = {0x31, 0x33, 0x35, 0x36};
uint8_t tmpBuf[3] = {0x90, 0x64, 0x40};
uint8_t charDisplayBuffer[1] = {0x64};
uint8_t counter = 0;

void Timer1_IRQHandler() {
	tmpBuf[1] = tmpArray[counter];
	charDisplayBuffer[0] = tmpArray[counter];
	MSS_UART_polled_tx(&g_mss_uart1, tmpBuf, sizeof(tmpBuf));
	UART_send(&apb_uart, charDisplayBuffer, sizeof(charDisplayBuffer));
	MSS_TIM1_clear_irq();
	if(counter < (4-1)) { // array size - 1
		++counter;
	}
	else {
		counter = 0;
	}
}*/

uint8_t METRONOME_BOUND = NUM_MEASURES * 8;
uint8_t METRONOME = 1; // start at one for the shadow
uint8_t METRONOME_SHADOW = 0; // used for noteOff
uint8_t readBuf[1] = {0xBB};
struct channel channel0 = {
		0, // channel number
		28, // program number
		{60, 62, 64, 65,
				67, 69, 71, 72,
				74, 76, 77, 79,
				81, 83, 84, 84
		}
};
struct channel channel1 = {
		1,
		36,
		{36, 0, 36, 0,
				36, 0, 36, 0,
				36, 0, 36, 0,
				36, 0, 36, 0,
		}
};

void Timer1_IRQHandler() {
	readKeypad(readBuf);
	noteOff(0, channel0.data[METRONOME_SHADOW], 0);
	noteOn(0, channel0.data[METRONOME], 50);

	//noteOff(1, channel1.data[METRONOME_SHADOW], 0);
	noteOn(1, channel1.data[METRONOME], 50);

	METRONOME_SHADOW = METRONOME;
	uint8_t charBuffer[2] = {32, METRONOME + 48};
	sendCharDisplay(charBuffer, 2);

	++METRONOME;
	if(METRONOME >= METRONOME_BOUND) {
		METRONOME = 0;
	}
	MSS_TIM1_clear_irq();
}

void test_library() {
	Global_init();

	allNotesOff();

	Timer_set_and_start(25000000); // half-second or 120 BPM
	programChange(0, 49);
	while(1) {

	}
}


int main()
{

	test_library();

}
