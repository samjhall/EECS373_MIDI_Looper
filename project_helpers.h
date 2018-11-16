/*
 * project_helpers.h
 *
 *  Created on: Nov 16, 2018
 *      Author: samjhall
 */

#ifndef PROJECT_HELPERS_H_
#define PROJECT_HELPERS_H_


/***	GLOBAL	***/
#define BASE 0x40050000
#define DEFAULT_TIMER_CYCLE_COUNT 50000000 // defaults to a half-second timer
#define NUM_MEASURES 2
#define NOTES_PER_MEASURE 8
UART_instance_t apb_uart;

void Global_init();

struct channel {
	uint8_t channelNumber;
	uint8_t programNumber;
	uint8_t data[NUM_MEASURES * 8]; // eighth notes
};

struct Loop_Master { // the "Master" object for the project
	uint8_t count; // the current count of the "metronome"
	uint8_t shadow; // should always be what count used to be (if count == 5, shadow should be 4)
	uint8_t metronome_bound; // when the metronome reaches this bound, it will reset to zero
	
	uint8_t keypadBuffer[1];
	uint32_t distanceBuffer[1];
};


/***	MIDI (UART1)	***/

void MIDI_init(); // midi board on APB UART
void programChange(uint8_t channel, uint8_t program);
void noteOn(uint8_t channel, uint8_t pitch, uint8_t attack);
void noteOff(uint8_t channel, uint8_t pitch, uint8_t attack);
void allNotesOff(); // may need to be used to clear the controller
					// produces some audible noises but silences everything


/***	APB UART DEVICES	***/

void APB_UART_init(); // character display and keypad
size_t readKeypad(uint8_t* buffer);
void sendCharDisplay(uint8_t* buffer, uint8_t size);



/***	TIMER	***/

void Timer_init();
void Timer_set_and_start(uint32_t cycle_count);
//void Timer1_IRQHandler();
/* USEFUL FUNCTIONS
 * MSS_TIM1_load_immediate( # of cycles );
 * MSS_TIM1_enable_irq();
 * MSS_TIM1_start();
 * MSS_TIM1_clear_irq();
 */



#endif /* PROJECT_HELPERS_H_ */
