#ifndef MYO_DATA_COLLECTOR_H
#define	MYO_DATA_COLLECTOR_H

class DataCollector : public myo::DeviceListener {
	public:
		bool onArm;
		myo::Arm whichArm;

		bool isUnlocked;

		int roll_w, pitch_w, yaw_w;
		myo::Pose currentPose;

		sendAxisState_t sendAxisState_out;

		DataCollector() : onArm(false), isUnlocked(false), roll_w(0), pitch_w(0), yaw_w(0), currentPose(), sendAxisState_out(NULL) {}
		
		void onUnpair(myo::Myo* myo, uint64_t timestamp);
		void onOrientationData(myo::Myo* myo, uint64_t timestamp, const myo::Quaternion<float>& quat);
		void onPose(myo::Myo* myo, uint64_t timestamp, myo::Pose pose);
		void onArmSync(myo::Myo* myo, uint64_t timestamp, myo::Arm arm, myo::XDirection xDirection);
		void onArmUnsync(myo::Myo* myo, uint64_t timestamp);
		void onUnlock(myo::Myo* myo, uint64_t timestamp);
		void onLock(myo::Myo* myo, uint64_t timestamp);

		void start(sendAxisState_t sendAxisState_addr);
		void finish();
		void print();
};

#endif	/* MYO_DATA_COLLECTOR_H */