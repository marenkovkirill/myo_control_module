#ifndef MYO_CONTROL_MODULE_H
#define MYO_CONTROL_MODULE_H

class MyoControlModule : public ControlModule {
  myo::Hub *hub;
  myo::Myo *myo;
  DataCollector *myo_data_collector;
  AxisData **robot_axis;
  colorPrintfModuleVA_t *colorPrintf_p;
  ModuleInfo *mi;

 public:
	 enum Axis : system_value {
		 fist = 1,
		 left_or_right = 2,
		 fingers_spread = 3,
		 double_tap = 4,
		 locked = 5,
		 fist_pitch_angle = 6,
		 fist_roll_angle = 7,
		 fist_yaw_angle = 8,
		 fingers_spread_pitch_angle = 9,
		 fingers_spread_roll_angle = 10,
		 fingers_spread_yaw_angle = 11
	 };


  MyoControlModule();
  const struct ModuleInfo& getModuleInfo();
  void prepare(colorPrintfModule_t *colorPrintf_p,
               colorPrintfModuleVA_t *colorPrintfVA_p);

  AxisData **getAxis(unsigned int *count_axis);
  void *writePC(unsigned int *buffer_length);

  int init();
  void execute(sendAxisState_t sendAxisState);
  void final();

  void readPC(void *buffer, unsigned int buffer_length){};

  int startProgram(int uniq_index);
  int endProgram(int uniq_index);

  void destroy();
  ~MyoControlModule() {}

  void colorPrintf(ConsoleColor colors, const char *mask, ...);
};

#endif /* MYO_CONTROL_MODULE_H */