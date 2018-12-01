/*
 * project_helpers.h
 *
 *  Created on: Nov 16, 2018
 *      Author: samjhall
 */

#ifndef PROJECT_HELPERS_H_
#define PROJECT_HELPERS_H_

#include "drivers/mss_uart/mss_uart.h"
#include "drivers/CoreUARTapb/core_uart_apb.h"
#include "drivers/mss_timer/mss_timer.h"
#include "drivers/mss_gpio/mss_gpio.h"
#include "drivers/mss_ace/mss_ace.h"
#include <stdint.h>
#include <stdio.h>


/***	GLOBAL	***/
#define BASE 0x40050000
#define DEFAULT_TIMER_CYCLE_COUNT 50000000 // defaults to a half-second timer
#define NUM_MEASURES 2
#define NOTES_PER_MEASURE 8
#define ACE_SAMPLE_SIZE 50 // number of consecutive readings of X/Y to get from touchscreen
#define DISTANCE_SENSOR_ADDRESS 0x40050100
#define FUNCTION_BUTTONS_ADDRESS 0x40050200

UART_instance_t apb_uart;
ace_channel_handle_t adc_handler4;//  X AXIS
ace_channel_handle_t adc_handler5; // Y AXIS

void Global_init(); // calls every device's initializer

struct channel {
	uint8_t channelNumber;
	uint8_t programNumber;
	uint8_t lastPlayed;
	uint8_t data[NUM_MEASURES * NOTES_PER_MEASURE]; // eighth notes
};

struct Loop_Master { // the "Master" object for the project
	uint8_t count; // the current count of the "metronome"
	uint8_t shadow; // should always be what count used to be (if count == 5, shadow should be 4)
	uint8_t metronome_bound; // when the metronome reaches this bound, it will reset to zero
	
	uint8_t selectedChannel;
	uint8_t recordingMode; // should be FF if in recording mode, 0 else
	uint8_t touchscreenButtonPressed;
	
	uint8_t keypadBuffer[2];
	uint32_t buttonsBuffer[1];
	uint32_t distanceBuffer[1];
	uint32_t touchscreenBuffer[14];
	//struct channel* channelPtrs[16];
	//uint8_t recordingBuffer[NUM_MEASURES * 8];
};

void Clear_channel(struct channel* channel);
void Fill_channel(struct channel* channel);
void Channel_init(struct channel* channelPtrs[16]);
void Update_metronome(struct Loop_Master* loopIn);
void Cycle_channels(struct channel* channelPtrs[NUM_MEASURES * 8], struct Loop_Master* loopIn);


/***	MIDI (UART1)	***/

/* MIDI board is controlled by the SmartFusion's UART1 at 31250 baud */

void MIDI_init(); // midi board on APB UART
void programChange(struct channel* ch, uint8_t program);
void noteOn(struct channel* ch, uint8_t pitch, uint8_t attack);
void noteOff(struct channel* ch, uint8_t pitch, uint8_t attack);
void allNotesOff(); // may need to be used to clear the controller
					// produces some audible noises but silences everything

// RESET GPIO 4
void Reset_init();
void reset();

					
/***	APB UART DEVICES	***/

/* The keypad and character display are controlled by an APB UART at 9600 baud */
/* Interface for the APB UART is provided in core_uart_apb.h */

void APB_UART_init(); // character display and keypad
size_t readKeypad(uint8_t* buffer);
uint8_t instrumentSelect(uint8_t* buffer);
void sendCharDisplay(uint8_t* buffer, uint8_t size);
void clearCharDisplay(); // clears the entire display


/***	DISTANCE SENSOR	***/
uint32_t readSensor(); // should read the raw data from the APB and convert it into centimeters

/***	TOUCHSCREEN		***/
void Touchscreen_init();
uint16_t getX();
uint16_t getY();
int checkPress(struct Loop_Master* loopIn);
void parseTouch(struct Loop_Master* loopIn);
		// should get X and Y and check for press
		// returns button that was pressed or FF if no press
void readTouch(struct Loop_Master* loopIn);

/***	GRAPHICS DISPLAY	***/


/***	TIMER	***/

/* The SmartFusion's Hardware Timer1 is used */

void Timer_init();
void Timer_set_and_start(uint32_t cycle_count);
/* USEFUL FUNCTIONS
 * MSS_TIM1_load_immediate( # of cycles );
 * MSS_TIM1_enable_irq();
 * MSS_TIM1_start();
 * MSS_TIM1_clear_irq();
 */


/***	BUTTONS 	***/

// PLAY/PAUSE GPIO 10
// CLEAR CHANNEL GPIO 11
// CLEAR ALL GPIO 12
// RECORD GPIO 13
//void Buttons_init();
uint32_t readButtons();


#endif /* PROJECT_HELPERS_H_ */
