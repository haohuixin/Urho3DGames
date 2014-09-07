#pragma once
#include "LogicComponent.h"
namespace Urho3D
{

	/// Enemy died  
	EVENT(E_ENEMYDIED, EnemyDied)
	{
		PARAM(P_GAINMONEY, GainMoney);                // bool
		PARAM(P_NODE, NodePtr);                // Node Ptr
	}
}

// All Urho3D classes reside in namespace Urho3D
using namespace Urho3D;

class Enemy : public LogicComponent
{
	OBJECT(Enemy);
public:
	//-------------------------------------------------------------------------
	// Constructors & Destructors
	//-------------------------------------------------------------------------
	Enemy(Context* context);
	~Enemy();
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

	void SetMaxHealth(float maxHP);
	void SetHealth(float HP);
	void SetSpeed(float s);
	///Inflict damage
	void Hurt(float dmg);

	/// Called on enemys death.
	void Explode(bool gainMoney);

	void FollowPath(Vector<Vector2> *path);
	

protected:
	Vector<Vector2> *path_;
	Vector2 lastPathPoint_;
	int onPathIndex_;
	float speed_;
	float maxHealth_;
	float health_;
	bool followPath_;
	float movePrc_;
private:
};
