
#pragma once
#include "LogicComponent.h"







// All Urho3D classes reside in namespace Urho3D
using namespace Urho3D;


class Bullet :  public LogicComponent
{
	OBJECT(Bullet);
public:
    //-------------------------------------------------------------------------
    // Constructors & Destructors
    //-------------------------------------------------------------------------
	Bullet(Context* context);
	~Bullet();
	/// Register object factory.
	static void RegisterObject(Context* context);

	/// Called when the component is added to a scene node. Other components may not yet exist.
	virtual void Start();
	/// Called before the first update. At this point all other components of the node should exist. Will also be called if update events are not wanted; in that case the event is immediately unsubscribed afterward.
	virtual void DelayedStart();
	/// Called when the component is detached from a scene node, usually on destruction.
	virtual void Stop();
	/// Called on scene update, variable timestep.
	virtual void Update(float timeStep);


	void SetDamage(int dmg) { damage_ = dmg; }
	void SetTarget(WeakPtr<Node> tar) { target_ = tar; }
	void SetSpeed(float speed) { speed_ = speed; }
	void SetDuration(float d) { duration_ = d; }

protected:
	float speed_ = 1.0f;
	int damage_ = 1;
	WeakPtr<Node> target_;
	float duration_ = 1.30f;
private:

	
};

