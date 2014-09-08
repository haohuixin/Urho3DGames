#include "Bullet.h"
#include "Context.h"
#include "Component.h"
#include "Node.h"
#include "Enemy.h"

Bullet::Bullet(Context* context) : LogicComponent(context)
{
	SetUpdateEventMask(USE_UPDATE);
}

Bullet::~Bullet()
{
}

void Bullet::Start()
{
}

void Bullet::RegisterObject(Context* context)
{
	context->RegisterFactory<Bullet>();
}

void Bullet::DelayedStart()
{
}

void Bullet::Stop()
{
}

void Bullet::Update(float timeStep)
{
	if (!target_.Expired())
	{
		// follow target
		Vector2 pos = target_->GetPosition2D() - node_->GetPosition2D();
		if (pos.Length() < 0.05f)
		{
			// target was hit
			duration_ = 0;
			Enemy* e = target_->GetComponent<Enemy>();
			if (e)
				e->Hurt(damage_);
		}

		pos.Normalize();
		pos = node_->GetPosition2D() + pos *timeStep*speed_;
		node_->SetPosition2D(pos);
	
		// Disappear when duration expired
		if (duration_ >= 0)
		{
			duration_ -= timeStep;
			if (duration_ <= 0)
				node_->Remove();
		}
	}
	else
	{
		node_->Remove();
	}
}