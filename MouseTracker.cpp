#include <osvr/ClientKit/Context.h>
#include <osvr/ClientKit/Interface.h>
#include <osvr/ClientKit/InterfaceStateC.h>

#include <windows.h>
#include <iostream>
#include <chrono>

// OSVR Helper Functions
namespace osvrMouseTracker
{
	double osvrQuatGetPitch(const OSVR_Quaternion *p)
	{
		return asin(2.0f * (osvrQuatGetX(p) * osvrQuatGetY(p) - osvrQuatGetW(p) * osvrQuatGetZ(p)));

	}

	double osvrQuatGetYaw(const OSVR_Quaternion *p)
	{
		return atan2(2.0f * osvrQuatGetX(p) * osvrQuatGetZ(p) + 2.0f * osvrQuatGetY(p) * osvrQuatGetW(p), 1.0f - 2.0f * (osvrQuatGetZ(p) * osvrQuatGetZ(p) + osvrQuatGetY(p) * osvrQuatGetY(p)));

	}

	double osvrQuatGetRoll(const OSVR_Quaternion *p)
	{
		return atan2(2.0f * osvrQuatGetX(p) * osvrQuatGetW(p) + 2.0f * osvrQuatGetZ(p) * osvrQuatGetY(p), 1.0f - 2.0f * (osvrQuatGetY(p) * osvrQuatGetY(p) + osvrQuatGetW(p) * osvrQuatGetW(p)));

	}

}

int main()
{
	osvr::clientkit::ClientContext context("com.osvr.MouseTracker");

	// HMD Interface: /me/head
	osvr::clientkit::Interface hmd = context.getInterface("/me/head");

	bool isInitState = false, isActive = false, isKeyDown = false, isKeyAction = false;
	OSVR_PoseState state, prevState;
	std::chrono::high_resolution_clock::time_point stateTime, prevStateTime;

	while (true)
	{
		// Update HMD Tracker State
		context.update();

		// Acquire Current HMD Orientation
		OSVR_TimeValue timestamp;
		OSVR_ReturnCode ret = osvrGetPoseState(hmd.get(), &timestamp, &state);
		stateTime = std::chrono::steady_clock::now();

		if (ret == OSVR_RETURN_SUCCESS)
		{
			// Toggle Mouse Tracking Using Shift Key
			isKeyDown = (GetAsyncKeyState(VK_SHIFT) >= 0);
			if (isKeyDown && !isKeyAction) isKeyAction = true;
			if (!isKeyDown && isKeyAction)
			{
				isActive = !isActive;
				isKeyAction = false;

			}

			if (isInitState && isActive)
			{
				// Calculate X-Axis Rotation From HMD Yaw
				double xAngle = osvrMouseTracker::osvrQuatGetYaw(&state.rotation);
				double xAnglePrev = osvrMouseTracker::osvrQuatGetYaw(&prevState.rotation);

				// Calculate Y-Axis Rotation From HMD Pitch
				double yAngle = osvrMouseTracker::osvrQuatGetPitch(&state.rotation);
				double yAnglePrev = osvrMouseTracker::osvrQuatGetPitch(&prevState.rotation);

				const double SCALING_FACTOR = 1000000.0f;
				const double SENSITIVITY = 20.0f;

				// Calculate X-Axis Rotation Delta
				int xCurAngle = (xAngle * (180 / 3.141592654)) * SCALING_FACTOR;
				int xPrevAngle = (xAnglePrev * (180 / 3.141592654)) * SCALING_FACTOR;
				if (xCurAngle < 0) xCurAngle += (360 * SCALING_FACTOR);
				if (xPrevAngle < 0) xPrevAngle += (360 * SCALING_FACTOR);
				int xDeltaAngle = (xCurAngle - xPrevAngle); // 360 (Turning Left) > 180 > 0 (Turning Right)

				// Calculate Y-Axis Rotation Delta
				int yCurAngle = (yAngle * (180 / 3.141592654)) * SCALING_FACTOR;
				int yPrevAngle = (yAnglePrev * (180 / 3.141592654)) * SCALING_FACTOR;
				int yDeltaAngle = (yCurAngle - yPrevAngle); // -90 (Facing Up) > 0 > 90 (Facing Down)

				auto elaspedTime = std::chrono::duration_cast<std::chrono::milliseconds>(stateTime - prevStateTime).count();

				if (elaspedTime >= 10)
				{
					double xDeltaPos = (static_cast<double>(xDeltaAngle) * elaspedTime / SCALING_FACTOR);
					double yDeltaPos = (static_cast<double>(yDeltaAngle) * elaspedTime / SCALING_FACTOR);
					std::cout << elaspedTime << "ms Elapsed, X: " << xCurAngle / SCALING_FACTOR << ", " << xDeltaPos << ", Y: " << yCurAngle / SCALING_FACTOR << ", " << yDeltaPos << std::endl;
					INPUT input;
					input.type = INPUT_MOUSE;
					input.mi.mouseData = 0;
					input.mi.dx = -xDeltaPos;
					input.mi.dy = yDeltaPos;
					//input.mi.dx = -(static_cast<float>(xDeltaAngle) / SCALING_FACTOR) * SENSITIVITY;// *(65536.0f / GetSystemMetrics(SM_CXSCREEN));
					//input.mi.dy = -(static_cast<float>(yDeltaAngle) / SCALING_FACTOR) * SENSITIVITY;// *(65536.0f / GetSystemMetrics(SM_CYSCREEN));
					input.mi.dwFlags = MOUSEEVENTF_MOVE;// | MOUSEEVENTF_ABSOLUTE;
					SendInput(1, &input, sizeof(input));

					//POINT mousepos;
					//GetCursorPos(&mousepos);
					//SetCursorPos(mousepos.x - static_cast<int>(static_cast<float>(xDeltaAngle) / SCALING_FACTOR) * SENSITIVITY, mousepos.y - static_cast<int>(static_cast<float>(yDeltaAngle) / SCALING_FACTOR) * SENSITIVITY);

					prevState = state;
					prevStateTime = stateTime;

				}

			}
			else
			{
				isInitState = true;
				prevState = state;
				prevStateTime = stateTime;

			}

		}

	}

	return 0;

}
