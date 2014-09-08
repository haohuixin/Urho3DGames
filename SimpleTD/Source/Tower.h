#pragma once
#include "LogicComponent.h"
#include "Node.h"

// All Urho3D classes reside in namespace Urho3D
using namespace Urho3D;
#define COST_INCREASE 1.5f
#define BASE_PRIZE 10

class Tower : public LogicComponent
{
	OBJECT(Tower);
public:
	//-------------------------------------------------------------------------
	// Constructors & Destructors
	//-------------------------------------------------------------------------
	Tower(Context* context);
	~Tower();
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

	void SetInitialCost(int cost) { initialCost_ = cost; }
	void SetEnemies(Vector<WeakPtr<Node>> *enemies) { enemies_ = enemies; }
	Node* GetNearestEnemy();
	void Shoot();

	void  UpgradeRange();

	void  UpgradeDamage();

	void  UpgradeFirerate();

	int GetPrize(StringHash type);
protected:

	Vector<WeakPtr<Node>> *enemies_ = NULL;
	Node* target_ = NULL;

	float shootTimer_ = 1.0f;

	float range_ = 0.60f;
	float fireRate_ = 0.80f;
	int damage_ = 1;

	int rangeLevel_ = 1;
	int firerateLevel_ = 1;
	int damageLevel_ = 1;

	int rangePrize_ = BASE_PRIZE;
	int fireratePrize_ = BASE_PRIZE;
	int damagePrize_ = BASE_PRIZE;

	int initialCost_ = 0;

private:
};
