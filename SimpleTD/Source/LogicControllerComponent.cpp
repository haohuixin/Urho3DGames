



#include "RandomWanderController.h"
#include "Context.h"





RandomWanderController::RandomWanderController(Context* context) : LogicComponent(context)
{
    // Initialize variables to sensible defaults


}


RandomWanderController::~RandomWanderController()
{
}

void RandomWanderController::Start()
{
	
//	SubscribeToEvent(node_, E_NODECOLLISION, HANDLER(RandomWanderController, HandleNodeCollision));
}



void RandomWanderController::RegisterObject( Context* context )
{
	context->RegisterFactory<RandomWanderController>();
}

void RandomWanderController::DelayedStart()
{

}

void RandomWanderController::Stop()
{

}

void RandomWanderController::Update(float timeStep)
{

}

void RandomWanderController::PostUpdate(float timeStep)
{

}

void RandomWanderController::FixedUpdate(float timeStep)
{

}

void RandomWanderController::FixedPostUpdate(float timeStep)
{

}








