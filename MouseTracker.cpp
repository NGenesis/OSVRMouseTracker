#include <osvr/ClientKit/Context.h>
#include <osvr/ClientKit/Interface.h>
#include <osvr/ClientKit/InterfaceStateC.h>

#include <windows.h>
#include <iostream>
#include <chrono>

// OSVR Helper Functions
namespace osvrMathHelpers
{
	// Extract Pitch Angle From Quaternion
	double osvrQuatGetPitch(const OSVR_Quaternion *q)
	{
		return asin(2.0f * (osvrQuatGetX(q) * osvrQuatGetY(q) - osvrQuatGetW(q) * osvrQuatGetZ(q)));

	}

	// Extract Yaw Angle From Quaternion
	double osvrQuatGetYaw(const OSVR_Quaternion *q)
	{
		return atan2(2.0f * osvrQuatGetX(q) * osvrQuatGetZ(q) + 2.0f * osvrQuatGetY(q) * osvrQuatGetW(q), 1.0f - 2.0f * (osvrQuatGetZ(q) * osvrQuatGetZ(q) + osvrQuatGetY(q) * osvrQuatGetY(q)));

	}

	// Extract Roll Angle From Quaternion
	double osvrQuatGetRoll(const OSVR_Quaternion *q)
	{
		return atan2(2.0f * osvrQuatGetX(q) * osvrQuatGetW(q) + 2.0f * osvrQuatGetZ(q) * osvrQuatGetY(q), 1.0f - 2.0f * (osvrQuatGetY(q) * osvrQuatGetY(q) + osvrQuatGetW(q) * osvrQuatGetW(q)));

	}

}

int main()
{
	osvr::clientkit::ClientContext context("com.osvr.MouseTracker");

	// HMD Interface: /me/head
	osvr::clientkit::Interface hmd = context.getInterface("/me/head");

	bool isInitState = false, isActive = false, isKeyDown = false, isKeyAction = false;
	OSVR_AngularVelocityState angularVelocityState, prevAngularVelocityState;
	std::chrono::high_resolution_clock::time_point stateTime, prevStateTime = std::chrono::steady_clock::now();

	while (true)
	{
		stateTime = std::chrono::steady_clock::now();
		auto elaspedTime = std::chrono::duration_cast<std::chrono::milliseconds>(stateTime - prevStateTime).count();

		// Update HMD Tracker State Every 10ms
		if (elaspedTime >= 10)
		{
			// Update HMD Tracker State
			context.update();

			// Acquire Current HMD Orientation
			OSVR_TimeValue timestamp;
			OSVR_ReturnCode ret = osvrGetAngularVelocityState(hmd.get(), &timestamp, &angularVelocityState);

			// Toggle Mouse Tracking Using Shift Key
			isKeyDown = (GetAsyncKeyState(VK_SHIFT) >= 0);
			if (isKeyDown && !isKeyAction) isKeyAction = true;
			if (!isKeyDown && isKeyAction)
			{
				isActive = !isActive;
				isKeyAction = false;

			}

			if (ret == OSVR_RETURN_SUCCESS)
			{
				if (isInitState && isActive)
				{
					const double SCALING_FACTOR = 1000000.0f;
					const double SENSITIVITY = 5.0f;

					// Calculate X-Axis Rotation Delta From HMD Pitch
					double xAngle = osvrMathHelpers::osvrQuatGetPitch(&angularVelocityState.incrementalRotation);
					int xDeltaAngle = (xAngle * (180 / 3.141592654)) * SCALING_FACTOR;

					// Calculate Y-Axis Rotation Delta From HMD Yaw
					double yAngle = osvrMathHelpers::osvrQuatGetYaw(&angularVelocityState.incrementalRotation);
					int yDeltaAngle = (yAngle * (180 / 3.141592654)) * SCALING_FACTOR;

					double xDeltaPos = (static_cast<double>(xDeltaAngle) * elaspedTime / SCALING_FACTOR) * SENSITIVITY;
					double yDeltaPos = (static_cast<double>(yDeltaAngle) * elaspedTime / SCALING_FACTOR) * SENSITIVITY;

					INPUT input;
					input.type = INPUT_MOUSE;
					input.mi.mouseData = 0;
					input.mi.dx = xDeltaPos;
					input.mi.dy = yDeltaPos;
					input.mi.dwFlags = MOUSEEVENTF_MOVE;
					SendInput(1, &input, sizeof(input));

					prevAngularVelocityState = angularVelocityState;
					prevStateTime = stateTime;

				}
				else
				{
					isInitState = true;
					prevAngularVelocityState = angularVelocityState;
					prevStateTime = stateTime;

				}

			}

		}

	}

	return 0;

}
