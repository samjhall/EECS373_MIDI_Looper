#include "drivers/mss_uart/mss_uart.h"
#include "drivers/CoreUARTapb/core_uart_apb.h"
#include "drivers/mss_timer/mss_timer.h"
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
		{0xFFFFFFFF}
};

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

struct channel* channels[2] = {&channel0, &channel1};

// This will be the main "driver" function as most of the work will be done between interrupts
void Timer1_IRQHandler() {
	readKeypad(Loop.keypadBuffer);
	if((Loop.keypadBuffer[0] == '*') && (Loop.recordingMode == 0)) { // set recordingMode to "on" and restart the metronome
		Loop.recordingMode = ~Loop.recordingMode;
		Loop.count = 0;
		MSS_TIM1_clear_irq();
		return;
	}

	if(Loop.recordingMode != 0) {
		channels[Loop.selectedChannel]->data[Loop.count] = Loop.keypadBuffer[0];
		//Record(&Loop_Master);
	}
	noteOff(0, channels[Loop.selectedChannel]->data[Loop.shadow], 0);
	noteOn(0, channels[Loop.selectedChannel]->data[Loop.count], 50);

	//noteOff(1, channel1.data[METRONOME_SHADOW], 0);
	noteOn(1, channel1.data[Loop.count], 50);


	uint8_t charBuffer[4] = {32, channels[Loop.selectedChannel]->data[Loop.count], Loop.recordingMode, Loop.count + 48};
	sendCharDisplay(charBuffer, 4);


	Loop.shadow = Loop.count;
	++Loop.count;
	if(Loop.count >= Loop.metronome_bound) {
		if(Loop.recordingMode != 0) {
			Loop.recordingMode = ~Loop.recordingMode;
		}

		clearCharDisplay();
		Loop.count = 0;
	}
	MSS_TIM1_clear_irq();
}

void test_library() {
	Global_init();

	allNotesOff();

	clearCharDisplay();

	Timer_set_and_start(25000000); // 1 second = 100 000 000
	programChange(0, 49);
	while(1) {

	}
}


int main()
{

	test_library();
	while(1) { }
}
