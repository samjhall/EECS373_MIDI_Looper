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
		{0xBB},
		{0xFFFFFFFF},
};

// EMPTY CHANNEL
/*
struct channel channelN = {
		1, 36, 0, {-1, -1, -1, -1,		-1, -1, -1, -1,			-1, -1, -1, -1,			-1, -1, -1, -1,}};
 */

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

// This will be the main "driver" function as most of the work will be done between interrupts
void Timer1_IRQHandler() {
	readKeypad(Loop.keypadBuffer);
	Loop.selectedChannel = Loop.keypadBuffer[0];


	// RECORDING DEMO

	// DO NOT DELETE

	/*if((Loop.keypadBuffer[0] == '*') && (Loop.recordingMode == 0)) { // set recordingMode to "on" and restart the metronome
		Loop.recordingMode = ~Loop.recordingMode;
		Loop.count = 0;
		MSS_TIM1_clear_irq();
		return;
	}

	if(Loop.recordingMode != 0) {
		channels[Loop.selectedChannel]->data[Loop.count] = Loop.keypadBuffer[0];
	}*/

	// DO NOT DELETE ABOVE

	Cycle_channels(channels, &Loop);

	//uint8_t charBuffer[4] = {32, channels[Loop.selectedChannel]->data[Loop.count], Loop.recordingMode, Loop.count + 48};
	uint8_t charBuffer[2] = {32, Loop.selectedChannel};
	sendCharDisplay(charBuffer, sizeof(charBuffer));

	Update_metronome(&Loop);
	MSS_TIM1_clear_irq();
}

void test_library() {
	Global_init();

	allNotesOff();

	clearCharDisplay();

	programChange(channels[0], 127);
	//programChange(1, 36);
	//programChange(2, 42);
	//programChange(3, 81);
	//programChange(9, 37);

	Timer_set_and_start(25000000); // 1 second = 100 000 000

	while(1) {

	}
}


int main()
{

	test_library();
	while(1) { }
}
