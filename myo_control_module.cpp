#include <stdio.h>
#include <windows.h>
#include <winuser.h>
#include <process.h>

#include <myo/myo.hpp>

#include "module.h"
#include "control_module.h"

#include "myo_data_collector.h"
#include "myo_control_module.h"

const unsigned int COUNT_AXIS = 6;

#define DEFINE_ALL_AXIS \
	ADD_AXIS("fist", 1, 0)\
	ADD_AXIS("left_or_right", 1, -1)\
	ADD_AXIS("fingers_spread", 1, 0)\
	ADD_AXIS("double_tap", 1, 0)\
	ADD_AXIS("locked", 1, 0)\
	ADD_AXIS("arm_pitch_angle", 18, 0)

bool myo_working = false;
CRITICAL_SECTION myo_working_mutex;

unsigned int WINAPI waitTerminateSignal(void *arg) {
	HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);
	if (hStdin == INVALID_HANDLE_VALUE) {
		goto exit; //error
	}

	DWORD fdwSaveOldMode;
	if (!GetConsoleMode(hStdin, &fdwSaveOldMode)) {
		goto exit; //error
	}
	DWORD fdwMode = ENABLE_WINDOW_INPUT | ENABLE_MOUSE_INPUT;
	if (!SetConsoleMode(hStdin, fdwMode)) {
		goto exit; //error
	}

	INPUT_RECORD irInBuf[128];
	DWORD cNumRead;
	DWORD i;

	while (1) {
		if (!ReadConsoleInput(hStdin, irInBuf, 128, &cNumRead))  {
			goto exit; //error
		}

		for (i = 0; i < cNumRead; ++i) {
			if (irInBuf[i].EventType == KEY_EVENT) {
				WORD key_code = irInBuf[i].Event.KeyEvent.wVirtualKeyCode;
				if ((key_code == VK_ESCAPE) && (irInBuf[i].Event.KeyEvent.bKeyDown)) {
					goto exit; //exit
				}
			}
		}
	}

	exit:
	
	EnterCriticalSection(&myo_working_mutex);
	myo_working = false;
	LeaveCriticalSection(&myo_working_mutex);
	
	SetConsoleMode(hStdin, fdwSaveOldMode);
	return 0;
}

void MyoControlModule::execute(sendAxisState_t sendAxisState) {
	myo->vibrate(myo::Myo::VibrationType::vibrationShort);
	myo_data_collector->start(sendAxisState);

	myo_working = true;
	InitializeCriticalSection(&myo_working_mutex);

	unsigned int tid;
	HANDLE thread_handle = (HANDLE) _beginthreadex(NULL, 0, waitTerminateSignal, NULL, 0, &tid);

	EnterCriticalSection(&myo_working_mutex);
	while (myo_working) {
		LeaveCriticalSection(&myo_working_mutex);

        hub->run(1000/20);
        myo_data_collector->print();
		
		EnterCriticalSection(&myo_working_mutex);
    }
	LeaveCriticalSection(&myo_working_mutex);

	WaitForSingleObject(thread_handle, INFINITE);
	CloseHandle(thread_handle);
	DeleteCriticalSection(&myo_working_mutex);

	myo_data_collector->finish();
	myo->vibrate(myo::Myo::VibrationType::vibrationShort);
}

MyoControlModule::MyoControlModule() {
	robot_axis = new AxisData*[COUNT_AXIS];
	system_value axis_id = 0;
	DEFINE_ALL_AXIS
}

int MyoControlModule::init() {
	 try {
		hub = new myo::Hub("com.example.myo_control_module");
		
		colorPrintf(ConsoleColor(), "Attempting to find a Myo...\n");

		myo = hub->waitForMyo(10000);
		if (!myo) {
			throw std::runtime_error("Unable to find a Myo!");
		}
		colorPrintf(ConsoleColor(ConsoleColor::green), "Connected to a Myo armband!\n");

		myo_data_collector = new DataCollector();
		hub->addListener(myo_data_collector);

    } catch (const std::exception& e) {
        colorPrintf(ConsoleColor(ConsoleColor::red), "Error: %s\n", e.what());
        return 1;
    }
	return 0;
}

const char *MyoControlModule::getUID() {
	return "Myo control module v1.01b by m79lol";
}

void MyoControlModule::prepare(colorPrintfModule_t *colorPrintf_p, colorPrintfModuleVA_t *colorPrintfVA_p) {
	this->colorPrintf_p = colorPrintfVA_p;
}

void MyoControlModule::colorPrintf(ConsoleColor colors, const char *mask, ...) {
	va_list args;
    va_start(args, mask);
    (*colorPrintf_p)(this, colors, mask, args);
    va_end(args);
}

void MyoControlModule::final() {
	hub->removeListener(myo_data_collector);
	delete myo_data_collector;
	delete hub;
}

AxisData** MyoControlModule::getAxis(unsigned int *count_axis) {
	(*count_axis) = COUNT_AXIS;
	return robot_axis;
}

void MyoControlModule::destroy() {
	for (unsigned int j = 0; j < COUNT_AXIS; ++j) {
		delete robot_axis[j];
	}
	delete[] robot_axis;
	delete this;
}

void *MyoControlModule::writePC(unsigned int *buffer_length) {
	*buffer_length = 0;
	return NULL;
}

int MyoControlModule::startProgram(int uniq_index) {
	return 0;
}
int MyoControlModule::endProgram(int uniq_index) {
	return 0;
}

__declspec(dllexport) ControlModule* getControlModuleObject() {
	return new MyoControlModule();
}