#include <stdio.h>
#include <windows.h>
#include <winuser.h>
#include <process.h>

#include <myo/myo.hpp>

#include "module.h"
#include "control_module.h"

#include <SimpleIni.h>

#include "myo_data_collector.h"
#include "myo_control_module.h"

const unsigned int COUNT_AXIS = 11;

#define ADD_AXIS(AXIS_NAME, AXIS_ID, UPPER_VALUE, LOWER_VALUE) \
  robot_axis[axis_id] = new AxisData;                          \
  robot_axis[axis_id]->axis_index = AXIS_ID;                   \
  robot_axis[axis_id]->upper_value = UPPER_VALUE;              \
  robot_axis[axis_id]->lower_value = LOWER_VALUE;              \
  robot_axis[axis_id]->name = AXIS_NAME;                       \
  axis_id++;

#define DEFINE_ALL_AXIS                                                       \
  ADD_AXIS("fist", MyoControlModule::Axis::fist, 1, 0)                        \
  ADD_AXIS("left_or_right", MyoControlModule::Axis::left_or_right, 1, -1)     \
  ADD_AXIS("fingers_spread", MyoControlModule::Axis::fingers_spread, 1, 0)    \
  ADD_AXIS("double_tap", MyoControlModule::Axis::double_tap, 1, 0)            \
  ADD_AXIS("locked", MyoControlModule::Axis::locked, 1, 0)                    \
  ADD_AXIS("fist_pitch_angle", MyoControlModule::Axis::fist_pitch_angle, 18,  \
           0)                                                                 \
  ADD_AXIS("fist_roll_angle", MyoControlModule::Axis::fist_roll_angle, 18, 0) \
  ADD_AXIS("fist_yaw_angle", MyoControlModule::Axis::fist_yaw_angle, 18, 0)   \
  ADD_AXIS("fingers_spread_pitch_angle",                                      \
           MyoControlModule::Axis::fingers_spread_pitch_angle, 18, 0)         \
  ADD_AXIS("fingers_spread_roll_angle",                                       \
           MyoControlModule::Axis::fingers_spread_roll_angle, 18, 0)          \
  ADD_AXIS("fingers_spread_yaw_angle",                                        \
           MyoControlModule::Axis::fingers_spread_yaw_angle, 18, 0)

#define UID "Myo control module v1.01b by m79lol"

bool myo_working = false;
CRITICAL_SECTION myo_working_mutex;
EXTERN_C IMAGE_DOS_HEADER __ImageBase;

unsigned int WINAPI waitTerminateSignal(void *arg) {
  HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);
  if (hStdin == INVALID_HANDLE_VALUE) {
    goto exit;  // error
  }

  DWORD fdwSaveOldMode;
  if (!GetConsoleMode(hStdin, &fdwSaveOldMode)) {
    goto exit;  // error
  }
  DWORD fdwMode = ENABLE_WINDOW_INPUT | ENABLE_MOUSE_INPUT;
  if (!SetConsoleMode(hStdin, fdwMode)) {
    goto exit;  // error
  }

  INPUT_RECORD irInBuf[128];
  DWORD cNumRead;
  DWORD i;

  while (1) {
    if (!ReadConsoleInput(hStdin, irInBuf, 128, &cNumRead)) {
      goto exit;  // error
    }

    for (i = 0; i < cNumRead; ++i) {
      if (irInBuf[i].EventType == KEY_EVENT) {
        WORD key_code = irInBuf[i].Event.KeyEvent.wVirtualKeyCode;
        if ((key_code == VK_ESCAPE) && (irInBuf[i].Event.KeyEvent.bKeyDown)) {
          goto exit;  // exit
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
  HANDLE thread_handle =
      (HANDLE)_beginthreadex(NULL, 0, waitTerminateSignal, NULL, 0, &tid);

  EnterCriticalSection(&myo_working_mutex);
  while (myo_working) {
    LeaveCriticalSection(&myo_working_mutex);

    hub->run(1000 / 20);
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
  mi = new ModuleInfo;
  mi->uid = UID;
  mi->mode = ModuleInfo::Modes::PROD;
  mi->version = BUILD_NUMBER;
  mi->digest = NULL;

  _isDebug = false;

  robot_axis = new AxisData *[COUNT_AXIS];
  system_value axis_id = 0;
  DEFINE_ALL_AXIS
}

int MyoControlModule::init() {
  WCHAR DllPath[MAX_PATH] = {0};
  GetModuleFileNameW((HINSTANCE)&__ImageBase, DllPath, (DWORD)MAX_PATH);

  WCHAR *tmp = wcsrchr(DllPath, L'\\');
  WCHAR wConfigPath[MAX_PATH] = {0};

  size_t path_len = tmp - DllPath;

  wcsncpy(wConfigPath, DllPath, path_len);
  wcscat(wConfigPath, L"\\config.ini");

  char ConfigPath[MAX_PATH] = {0};
  wcstombs(ConfigPath, wConfigPath, sizeof(ConfigPath));

  CSimpleIniA ini;
  ini.SetMultiKey(true);

  if (ini.LoadFile(ConfigPath) >= 0) {
    _isDebug = ini.GetBoolValue("main", "debug", false);
  }

  try {
    hub = new myo::Hub("com.example.myo_control_module");

    colorPrintf(ConsoleColor(), "Attempting to find a Myo...\n");

    myo = hub->waitForMyo(10000);
    if (!myo) {
      throw std::runtime_error("Unable to find a Myo!");
    }
    colorPrintf(ConsoleColor(ConsoleColor::green),
                "Connected to a Myo armband!\n");

    myo_data_collector = new DataCollector(this);
    hub->addListener(myo_data_collector);

  } catch (const std::exception &e) {
    colorPrintf(ConsoleColor(ConsoleColor::red), "Error: %s\n", e.what());
    return 1;
  }
  return 0;
}

const struct ModuleInfo &MyoControlModule::getModuleInfo() { return *mi; }

void MyoControlModule::prepare(colorPrintfModule_t *colorPrintf_p,
                               colorPrintfModuleVA_t *colorPrintfVA_p) {
  this->colorPrintf_p = colorPrintfVA_p;
}

void MyoControlModule::colorPrintf(ConsoleColor colors, const char *mask, ...) {
  va_list args;
  va_start(args, mask);
  (*colorPrintf_p)(this, colors, mask, args);
  va_end(args);
}

bool MyoControlModule::isDebug() { return _isDebug; }

void MyoControlModule::final() {
  hub->removeListener(myo_data_collector);
  delete myo_data_collector;
  delete hub;
}

AxisData **MyoControlModule::getAxis(unsigned int *count_axis) {
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

int MyoControlModule::startProgram(int uniq_index) { return 0; }
int MyoControlModule::endProgram(int uniq_index) { return 0; }

PREFIX_FUNC_DLL unsigned short getControlModuleApiVersion() {
  return CONTROL_MODULE_API_VERSION;
}
PREFIX_FUNC_DLL ControlModule *getControlModuleObject() {
  return new MyoControlModule();
}