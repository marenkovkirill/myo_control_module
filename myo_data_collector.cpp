#define _USE_MATH_DEFINES
#include <cmath>
#include <iostream>
#include <iomanip>
#include <stdexcept>
#include <string>
#include <algorithm>

#include <myo/myo.hpp>

#include "../module_headers/module.h"
#include "../module_headers/control_module.h"

#include "myo_data_collector.h"

void DataCollector::onUnpair(myo::Myo* myo, uint64_t timestamp) {
    roll_w = 0;
    pitch_w = 0;
    yaw_w = 0;
    onArm = false;
    isUnlocked = false;
}

void DataCollector::onOrientationData(myo::Myo* myo, uint64_t timestamp, const myo::Quaternion<float>& quat) {
    using std::atan2;
    using std::asin;
    using std::sqrt;
    using std::max;
    using std::min;

    float roll = atan2(2.0f * (quat.w() * quat.x() + quat.y() * quat.z()),
                        1.0f - 2.0f * (quat.x() * quat.x() + quat.y() * quat.y()));
    float pitch = asin(max(-1.0f, min(1.0f, 2.0f * (quat.w() * quat.y() - quat.z() * quat.x()))));
    float yaw = atan2(2.0f * (quat.w() * quat.z() + quat.x() * quat.y()),
                    1.0f - 2.0f * (quat.y() * quat.y() + quat.z() * quat.z()));

    roll_w = static_cast<int>((roll + (float)M_PI)/(M_PI * 2.0f) * 18);
    pitch_w = static_cast<int>((pitch + (float)M_PI/2.0f)/M_PI * 18);
    yaw_w = static_cast<int>((yaw + (float)M_PI)/(M_PI * 2.0f) * 18);
}

void DataCollector::onPose(myo::Myo* myo, uint64_t timestamp, myo::Pose pose) {
    currentPose = pose;
    if (pose != myo::Pose::unknown && pose != myo::Pose::rest) {
        myo->unlock(myo::Myo::unlockHold);
        myo->notifyUserAction();
    } else {
        myo->unlock(myo::Myo::unlockTimed);
    }
}

void DataCollector::onArmSync(myo::Myo* myo, uint64_t timestamp, myo::Arm arm, myo::XDirection xDirection, float rotation, myo::WarmupState warmupState) {
    onArm = true;
    whichArm = arm;
}

void DataCollector::onArmUnsync(myo::Myo* myo, uint64_t timestamp) {
    onArm = false;
}

void DataCollector::onUnlock(myo::Myo* myo, uint64_t timestamp) {
    isUnlocked = true;
}

void DataCollector::onLock(myo::Myo* myo, uint64_t timestamp) {
    isUnlocked = false;
}

void DataCollector::start(sendAxisState_t sendAxisState_addr) {
	this->sendAxisState_out = sendAxisState_addr;
}

void DataCollector::finish() {
	sendAxisState_out = NULL;
}

void DataCollector::print() {
    /*std::cout << '\n';

    std::cout << '[' << std::string(roll_w, '*') << std::string(18 - roll_w, ' ') << ']'
                  << '[' << std::string(pitch_w, '*') << std::string(18 - pitch_w, ' ') << ']'
                  << '[' << std::string(yaw_w, '*') << std::string(18 - yaw_w, ' ') << ']';

    if (onArm) {
        std::string poseString = currentPose.toString();
        std::cout << '[' << (isUnlocked ? "unlocked" : "locked  ") << ']'
                    << '[' << (whichArm == myo::armLeft ? "L" : "R") << ']'
                    << '[' << poseString << std::string(14 - poseString.size(), ' ') << ']';
    } else {
        std::cout << '[' << std::string(8, ' ') << ']' << "[?]" << '[' << std::string(14, ' ') << ']';
    }
	
	std::cout << '\n';

    std::cout << std::flush;*/

	if (sendAxisState_out) {
		if (onArm && isUnlocked) {
			(*sendAxisState_out)(this, 1, currentPose == myo::Pose::fist ? 1 : 0);

			if (
				(currentPose == myo::Pose::waveOut && whichArm == myo::armLeft)
				|| (currentPose == myo::Pose::waveIn && whichArm == myo::armRight)
			) {
				(*sendAxisState_out)(this, 2, -1);
			} else {
				if (
					(currentPose == myo::Pose::waveIn && whichArm == myo::armLeft)
					|| (currentPose == myo::Pose::waveOut && whichArm == myo::armRight)
				) {
					(*sendAxisState_out)(this, 2, 1);
				} else {
					(*sendAxisState_out)(this, 2, 0);
				}
			}

			(*sendAxisState_out)(this, 3, currentPose == myo::Pose::fingersSpread ? 1 : 0);
			(*sendAxisState_out)(this, 4, currentPose == myo::Pose::doubleTap ? 1 : 0);
			
			(*sendAxisState_out)(this, 5, 0);

			if (currentPose == myo::Pose::fist) {
				(*sendAxisState_out)(this, 6, pitch_w);
			}
		} else {
			(*sendAxisState_out)(this, 5, 1);
		}
	}
}