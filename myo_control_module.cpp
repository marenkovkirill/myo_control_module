#include <stdio.h>
#include <windows.h>
#include <winuser.h>
#include <process.h>

#include <myo/myo.hpp>

#include "../module_headers/module.h"
#include "../module_headers/control_module.h"
#include "../rcml_compiler/globals.h"

#include "myo_data_collector.h"
#include "myo_control_module.h"

const int COUNT_AXIS = 7;

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
				if (key_code == VK_ESCAPE) {
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
	regval axis_id = 0;
	DEFINE_ALL_AXIS
}

int MyoControlModule::init() {
	 try {
		hub = new myo::Hub("com.example.myo_control_module");
		
		std::cout << "Attempting to find a Myo..." << std::endl;

		myo = hub->waitForMyo(10000);
		if (!myo) {
			throw std::runtime_error("Unable to find a Myo!");
		}
		std::cout << "Connected to a Myo armband!" << std::endl << std::endl;

		myo_data_collector = new DataCollector();
		hub->addListener(myo_data_collector);

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
	return 0;
}

void MyoControlModule::final() {
	hub->removeListener(myo_data_collector);
	delete myo_data_collector;
	delete hub;
}

AxisData** MyoControlModule::getAxis(int *count_axis) {
	(*count_axis) = COUNT_AXIS;
	return robot_axis;
}

void MyoControlModule::destroy() {
	for (int j = 0; j < COUNT_AXIS; ++j) {
		delete robot_axis[j];
	}
	delete[] robot_axis;
	delete this;
}

__declspec(dllexport) ControlModule* getControlModuleObject() {
	return new MyoControlModule();
}