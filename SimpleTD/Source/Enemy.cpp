#include "Enemy.h"
#include "Context.h"
#include "Component.h"
#include "Node.h"

Enemy::Enemy(Context* context) : LogicComponent(context),
path_(NULL),
onPathIndex_(0),
maxHealth_(1.0f),
health_(1.0f),
followPath_(false),
movePrc_(0.0f),
speed_(1.0f)
{
	SetUpdateEventMask(USE_UPDATE);
}

Enemy::~Enemy()
{
}
void Enemy::RegisterObject(Context* context)
{
	context->RegisterFactory<Enemy>();
}
void Enemy::Start()
{
}
void Enemy::DelayedStart()
{

}

void Enemy::Stop()
{
}

void Enemy::Update(float timeStep)
{
	if (path_ && followPath_)
	{
	
			if (movePrc_ < 1.0f)movePrc_ += timeStep * speed_;
			if (movePrc_ > 1.0f)movePrc_ = 1.0f;

			//Goal reached
			if (movePrc_ == 1.0f)
			{
				if (onPathIndex_ + 1 >= path_->Size())
				{
					// end of path :-) 
					Explode(false);					
					return;
				}
				else  //Next Waypoint
				{
					lastPathPoint_ = path_->At(onPathIndex_);
					movePrc_ = 0.0f;
					onPathIndex_++;

				}
			}

			//Interpolate position between last and next path point
			node_->SetPosition2D(lastPathPoint_ * (1.0f - movePrc_) + path_->At(onPathIndex_) * movePrc_);

	}
}

void Enemy::SetMaxHealth(float maxHP)
{
	
	maxHealth_ = maxHP;
}

void Enemy::SetHealth(float HP)
{
	health_ = HP;
}



void Enemy::Hurt(float dmg)
{
	health_ -= dmg;

	if (health_ <= 0.0f) {
		Explode(true);
	}
}

void Enemy::Explode(bool gainMoney)
{

	
	{
		using namespace EnemyDied;

		VariantMap& eventData = GetEventDataMap();
		eventData[P_GAINMONEY] = gainMoney;
		eventData[P_NODE] = node_;

		SendEvent(E_ENEMYDIED, eventData);
	}

	node_->Remove();
}

void Enemy::FollowPath(Vector<Vector2> *path)
{
	if (path != NULL && path->Size()>0)
	{
		path_ = path;
		onPathIndex_ = 0;
		followPath_ = true;
		lastPathPoint_ = path_->At(0);
		movePrc_ = 1.0f;
		node_->SetPosition2D(lastPathPoint_);
	}
}

void Enemy::SetSpeed(float s)
{
	if (s<0.0f)
		return;
	
	speed_ = s;
}
