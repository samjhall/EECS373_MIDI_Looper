//#include "drivers/mss_uart/mss_uart.h"
//#include "drivers/CoreUARTapb/core_uart_apb.h"
//#include "drivers/mss_timer/mss_timer.h"
//#include "drivers/mss_ace/mss_ace.h"
#include "project_helpers.h"


// baud val (written to BASE + 0x008) = (clock / (16 * baud rate)) - 1
// for 100 MHz at 9600 baud, this is 650 in decimal



struct Loop_Master Loop = {
		1,
		0,
		NUM_MEASURES * NOTES_PER_MEASURE,
		0,
		0,
		-1,
		{0x00},
		{0xBB, 0xBB},
		{0xFFFFFFFF},
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
};

// EMPTY CHANNEL

struct channel EMPTY_CHANNEL = {
		1, 36, 0, {-1, -1, -1, -1,		-1, -1, -1, -1,			-1, -1, -1, -1,			-1, -1, -1, -1,}};


struct channel channel0 = {
		0, // channel number
		28, // program number
		0, // lastPlayed
			   //{60, 62, 64, 65,		67, 69, 71, 72,		74, 76, 77, 79,		81, 83, 84, 84}};
			   {60, -1, -1, -1,		-1, -1, -1, -1,			-1, -1, -1, -1,			-1, -1, -1, -1,}};

struct channel channel1 = {
		1, 36, 0, {-1, 61, -1, -1,		-1, -1, -1, -1,			-1, -1, -1, -1,			-1, -1, -1, -1,}};

struct channel channel2 = {
		2, 111, 0, {-1, -1, 62, -1,		-1, -1, -1, -1,			-1, -1, -1, -1,			-1, -1, -1, -1,}};

struct channel channel3 = {
		3, 36, 0, {-1, -1, -1, 63,		-1, -1, -1, -1,			-1, -1, -1, -1,			-1, -1, -1, -1,}};

struct channel channel4 = {
		4, 2, 0, {-1, -1, 48, -1,		48, -1, -1, -1,			-1, -1, 48, -1,			48, -1, -1, -1,}};

struct channel channel5 = {
		5, 105, 0, {60, -1, -1, -1,		60, -1, -1, -1,		60, -1, -1, -1,		60, -1, -1, -1,}};

struct channel channel6 = {
		6, 105, 0, {-1, 60, -1, -1,		-1, 60, -1, -1,		-1, 60, -1, -1,		-1, 60, -1, -1,}};

struct channel channel7 = {
		7, 105, 0, {-1, -1, 60, -1,		-1, -1, 60, -1,		-1, -1, 60, -1,		-1, -1, 60, -1,}};

struct channel channel8 = {
		8, 105, 0, {-1, -1, -1, 60,		-1, -1, -1, 60,		-1, -1, -1, 60,		-1, -1, -1,  60,}};

struct channel channel9 = {
		9, 36, 0, {36, 42, 36, 42,		36, 42, 36, 42,		36, 42, 36, 42,		36, 42, 36, 42,}};

struct channel channel10 = {
		1, 36, 0, {-1, -1, -1, -1,		-1, -1, -1, -1,			-1, -1, -1, -1,			-1, -1, -1, -1,}};

struct channel channel11 = {
		1, 36, 0, {-1, -1, -1, -1,		-1, -1, -1, -1,			-1, -1, -1, -1,			-1, -1, -1, -1,}};

struct channel channel12 = {
		1, 36, 0, {-1, -1, -1, -1,		-1, -1, -1, -1,			-1, -1, -1, -1,			-1, -1, -1, -1,}};

struct channel channel13 = {
		1, 36, 0, {-1, -1, -1, -1,		-1, -1, -1, -1,			-1, -1, -1, -1,			-1, -1, -1, -1,}};

struct channel channel14 = {
		1, 36, 0, {-1, -1, -1, -1,		-1, -1, -1, -1,			-1, -1, -1, -1,			-1, -1, -1, -1,}};

struct channel channel15 = {
		1, 36, 0, {-1, -1, -1, -1,		-1, -1, -1, -1,			-1, -1, -1, -1,			-1, -1, -1, -1,}};


struct channel* channels[16] = {&channel0, &channel1, &channel2, &channel3,
								&channel4, &channel5, &channel6, &channel7,
								&channel8, &channel9, &channel10, &channel11,
								&channel12, &channel13, &channel14, &channel15,};

uint8_t GLOBAL_PAUSE_FLAG = 0;

// This will be the main "driver" function as most of the work will be done between interrupts
void Timer1_IRQHandler() {
	// check the four function buttons
	// check for pause
	if(Loop.buttonsBuffer[0] & 0x01) {
		GLOBAL_PAUSE_FLAG = ~GLOBAL_PAUSE_FLAG;
	}

	if(Loop.buttonsBuffer[0] & 0x02) {
		int i = 0;
		while(i < NUM_MEASURES * 8) {
			channels[Loop.selectedChannel]->data[i] = EMPTY_CHANNEL.data[i];
			++i;
		}
		printf("CHANNEL %d CLEARED\n\r", Loop.selectedChannel);
	}
	if(Loop.buttonsBuffer[0] & 0x04) {
		int i = 0;
		int j = 0;
		while (i < 16) {
			while(j < NUM_MEASURES * 8) {
				(channels[i]->data)[j] = EMPTY_CHANNEL.data[0];
				++j;
			}
			++i;
			j = 0;
		}
		allNotesOff();
		printf("ALL CHANNELS CLEARED\n\r");
	}

	if(GLOBAL_PAUSE_FLAG != 0) { // user should be able to clear when paused, but not record
		allNotesOff();
		MSS_TIM1_clear_irq();
		return;
	}

	if((Loop.buttonsBuffer[0] & 0x08) && (Loop.recordingMode == 0)) { // set recordingMode to "on" and restart the metronome
		Loop.recordingMode = ~Loop.recordingMode;
		Loop.count = 0;
		MSS_TIM1_clear_irq();
		return;
	}


	// check for a touchscreen press
	if(Loop.touchscreenButtonPressed != 255) {
		Loop.selectedChannel = Loop.touchscreenButtonPressed;
		printf("Button Pressed: %d\n\r", Loop.touchscreenButtonPressed);
	}


	// instrument selection from the keypad
	Loop.keypadBuffer[1] = Loop.keypadBuffer[0];
	readKeypad(Loop.keypadBuffer);
	uint8_t program = instrumentSelect(Loop.keypadBuffer);
	if((program != channels[Loop.selectedChannel]->programNumber)
			&& (program != 255)
			&& (Loop.keypadBuffer[0] != Loop.keypadBuffer[1])) {
		printf("CHANGE TO %d\n\r", program);
		programChange(channels[Loop.selectedChannel], program);
		channels[Loop.selectedChannel]->programNumber = program;
	}



	if(Loop.recordingMode != 0) {
		//channels[Loop.selectedChannel]->data[Loop.count] = Loop.keypadBuffer[0];
		channels[Loop.selectedChannel]->data[Loop.count] = readSensor() + 24;
	}



	Cycle_channels(channels, &Loop);

	//uint8_t charBuffer[4] = {32, channels[Loop.selectedChannel]->data[Loop.count], Loop.recordingMode, Loop.count + 48};
	uint8_t charBuffer[4] = {32, Loop.keypadBuffer[0], 32, Loop.selectedChannel+48};
	sendCharDisplay(charBuffer, sizeof(charBuffer));

	Update_metronome(&Loop);
	MSS_TIM1_clear_irq();
}

void test_library() {
	Global_init();

	//allNotesOff();
	reset();

	clearCharDisplay();

	programChange(channels[0], 127);

	Timer_set_and_start(25000000); // 1 second = 100 000 000

	while(1) {
		parseTouch(&Loop);
		Loop.buttonsBuffer[0] = readButtons();
		int x =Loop.touchscreenBuffer[0];
		int y =Loop.touchscreenBuffer[1];
		int xSection = -1;
		int ySection = -1;
		if(x > 2600){
			xSection = 0;
		}
		else if(x < 2500 && x > 1900){
			xSection = 1;
		}
		else if(x < 1700 && x > 1100){
			xSection = 2;
		}
		else if(x < 1000){
			xSection = 3;
		}

		if(y > 2450){
			ySection = 0;
		}
		else if(y < 2400 && y > 1750){
			ySection = 1;
		}
		else if(y < 1650 && y > 1100){
			ySection = 2;
		}
		else if(y < 1000){
			ySection = 3;
		}

		//Read Which Section
		if(checkPress(&Loop) && xSection != -1 && ySection!= -1){

			//printf("X = %d  and Y = %d and final Section = %d\n\r", xSection, ySection, xSection + ySection*4);
			Loop.touchscreenButtonPressed = xSection + ySection*4;
		}
		else{
			//printf("No Touch\n\r");
			Loop.touchscreenButtonPressed = -1;
		}


		/*** MICROPHONE ***/
		ace_channel_handle_t adc_handler2 = ACE_get_channel_handle((const uint8_t *)"ADCDirectInput_2");
		uint16_t adc_data2 = ACE_get_ppe_sample(adc_handler2);
		//printf("Here it is: %d\n\r", (int)adc_data2);


		//printf("\tDistance (cm): %d\n\r", (int)readSensor());

	}
}


int main()
{
	printf("test\n"); // test UART0
	test_library();
	while(1) { }
}
