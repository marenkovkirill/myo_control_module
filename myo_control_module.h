#ifndef MYO_CONTROL_MODULE_H
#define	MYO_CONTROL_MODULE_H

class MyoControlModule : public ControlModule {
	myo::Hub* hub;
	myo::Myo* myo;
	DataCollector *myo_data_collector;
	AxisData **robot_axis;
	colorPrintfVA_t *colorPrintf_p;

	public:
		MyoControlModule();
		const char *getUID();
		void prepare(colorPrintf_t *colorPrintf_p, colorPrintfVA_t *colorPrintfVA_p);
		int init();
		AxisData** getAxis(unsigned int *count_axis);
		void execute(sendAxisState_t sendAxisState);
		void final();
		void destroy();
		~MyoControlModule() {}

		void colorPrintf(ConsoleColor colors, const char *mask, ...);
};

#define ADD_AXIS(AXIS_NAME, UPPER_VALUE, LOWER_VALUE) \
	robot_axis[axis_id] = new AxisData; \
	robot_axis[axis_id]->axis_index = axis_id + 1; \
	robot_axis[axis_id]->upper_value = UPPER_VALUE; \
	robot_axis[axis_id]->lower_value = LOWER_VALUE; \
	robot_axis[axis_id]->name = AXIS_NAME; \
	axis_id++;

#endif	/* MYO_CONTROL_MODULE_H */