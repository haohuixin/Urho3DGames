#include "Tower.h"
#include "Context.h"
#include "Node.h"
#include "ResourceCache.h"
#include "SpriteSheet2D.h"
#include "Component.h"
#include "StaticSprite2D.h"
#include "Bullet.h"
#include "Scene.h"

Tower::Tower(Context* context) : LogicComponent(context)

{
	SetUpdateEventMask(USE_UPDATE);
}

Tower::~Tower()
{
}

void Tower::Start()
{
}

void Tower::RegisterObject(Context* context)
{
	context->RegisterFactory<Tower>();
}

void Tower::DelayedStart()
{
}

void Tower::Stop()
{
}

void Tower::Update(float timeStep)
{
	target_ = GetNearestEnemy();

	if (target_ == NULL)
	{
//		_indicator.visible = false;
	}
	else
	{
		shootTimer_ -= timeStep*fireRate_;
		if (shootTimer_ <=0.0f)
		{
			Shoot();
		}

	}
}

Node* Tower::GetNearestEnemy()
{


	for (int i = 0; i < enemies_->Size();i++)
	{
		WeakPtr<Node> enemy = enemies_->At(i);
		if (enemy.NotNull())
		{
			Vector2 dist = node_->GetPosition2D() - enemy->GetPosition2D();
			if (dist.Length() <= range_)
			{			
				return enemy.Get();
			}
			
		}
	}

	return NULL;
}

void Tower::Shoot()
{
	if (target_ == NULL)
		return;

	shootTimer_ = 1.0f;

	ResourceCache* cache = GetSubsystem<ResourceCache>();
	// create enemy
	SpriteSheet2D* 	spriteSheet = cache->GetResource<SpriteSheet2D>("Tilemaps/TileMapSprites.xml");
	if (!spriteSheet)
		return;

	SharedPtr<Node> bulletNode_(GetScene()->CreateChild("bullet"));
	bulletNode_->SetPosition2D(node_->GetPosition2D());
	StaticSprite2D* staticSprite = bulletNode_->CreateComponent<StaticSprite2D>();
	spriteSheet->GetSprite("Bullet")->SetHotSpot(Vector2(0.0f, 0.0f));
	staticSprite->SetSprite(spriteSheet->GetSprite("Bullet"));
	staticSprite->SetLayer(6 * 10);
	/// create Enemy component which controls the Enemy behavior
	Bullet* b = bulletNode_->CreateComponent<Bullet>();
	b->SetTarget(target_);
	b->SetDamage(damage_);

}

void Tower::UpgradeRange()
{
	range_ += 0.10f;
	rangeLevel_++;
	rangePrize_ = (int) rangePrize_ * COST_INCREASE;
}

void Tower::UpgradeDamage()
{
	damage_++;
	damageLevel_++;
	damagePrize_ = (int) damagePrize_ * COST_INCREASE;
}

void Tower::UpgradeFirerate()
{
	fireRate_ += 0.5;
	firerateLevel_++;
	fireratePrize_ =(int) fireratePrize_ * COST_INCREASE;
}
