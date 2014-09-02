#pragma once

#include "Timer.h"







// All Urho3D classes reside in namespace Urho3D
using namespace Urho3D;

//TODO: Animation speed is defined through characters velocity
class ExpirationTimer
{

public:
	ExpirationTimer(unsigned expirationTime,bool startActive = false);
	~ExpirationTimer ();


	void Reset();
	bool Active();
	bool Expired();
	unsigned GetCurrentTime();
private:
	Timer timer_;
	unsigned expirationTime_;
};

