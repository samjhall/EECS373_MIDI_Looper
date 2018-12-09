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
#include "drivers/mss_nvm/drivers/F2DSS_NVM/mss_nvm.h"
#include "drivers/mss_pdma/mss_pdma.h"
#include <stdint.h>
#include <stdio.h>


/***	GLOBAL	***/
#define BASE 0x40050000
#define DEFAULT_TIMER_CYCLE_COUNT 50000000 // defaults to a half-second timer
#define NUM_MEASURES 4
#define NOTES_PER_MEASURE 8
#define ACE_SAMPLE_SIZE 50 // number of consecutive readings of X/Y to get from touchscreen
#define DISTANCE_SENSOR_ADDRESS 0x40050100
#define FUNCTION_BUTTONS_ADDRESS 0x40050200
#define VGA_DISPLAY_ADDRESS 0x40050300

#define VGA_GREEN 0x2
#define VGA_YELLOW 0x6
#define VGA_RED 0x4

#define FREE_BUFFER         	0
#define UNDER_DMA_CONTROL   	1
#define DATA_READY          	2
#define SAMPLES_BUFFER_SIZE		128
#define NB_OF_SAMPLE_BUFFERS	2
#define	TOTAL_SAMPLE_SIZE		81920


UART_instance_t apb_uart;
ace_channel_handle_t adc_handler2; // IMU Y AXIS
ace_channel_handle_t adc_handler4; // X AXIS
ace_channel_handle_t adc_handler5; // Y AXIS

typedef enum{
	NORMAL = 0,
			DOUBLE,
			HALF,
			BACK,
			QUAD
} mymode_t;

mymode_t mymode;
uint8_t dac_irq;

uint32_t envm_idx;
uint32_t samples_buffer[NB_OF_SAMPLE_BUFFERS][SAMPLES_BUFFER_SIZE];
uint8_t buffer_status[NB_OF_SAMPLE_BUFFERS];
uint8_t recordVoice;
uint8_t playVoice;
uint8_t voiceRecorded;
uint32_t envm_idx_max;

void Global_init(); // calls every device's initializer

struct channel {
	uint8_t channelNumber;
	uint8_t programNumber;
	uint8_t lastPlayed;
	uint8_t data[NUM_MEASURES * NOTES_PER_MEASURE]; // eighth notes
	uint32_t attack[NUM_MEASURES * NOTES_PER_MEASURE];
};

struct Loop_Master { // the "Master" object for the project
	uint8_t count; // the current count of the "metronome"
	uint8_t shadow; // should always be what count used to be (if count == 5, shadow should be 4)
	uint8_t metronome_bound; // when the metronome reaches this bound, it will reset to zero
	
	uint8_t selectedChannel;
	uint8_t recordingMode; // should be FF if in recording mode, 0 else
	uint8_t touchscreenButtonPressed;
	
	uint8_t keypadBuffer[2];
	uint8_t channelsPlaying[16];
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
void charDisplayData(struct Loop_Master* loopIn, uint32_t distance);


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
void VGA_init();
void VGA_write(uint8_t button, uint8_t color);
uint32_t VGA_test();
// bits [7:4] - button number
// bits [2:0] - color

/*
parameter BLACK = 3'b000;
parameter BLUE = 3'b001;
parameter GREEN = 3'b010;
parameter CYAN = 3'b011;
parameter RED = 3'b100;
parameter MAGENTA = 3'b101;
parameter YELLOW = 3'b110;
parameter WHITE = 3'b111;
*/

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

/***	IMU 	***/
void IMU_init();
uint16_t readIMU();

/*** Microphone ***/
void Mic_init();
void free_samples(uint8_t full_flag);
void ace_pdma_rx_handler();
void process_samples();
void play_samples(mymode_t MODE);
void ACE_PC0_Flag0_IRQHandler();

#endif /* PROJECT_HELPERS_H_ */
