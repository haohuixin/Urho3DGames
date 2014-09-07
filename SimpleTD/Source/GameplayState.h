//
// Copyright (c) 2008-2013 the Urho3D project.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

#pragma once
#include "Object.h"
#include "Context.h"
#include "Ptr.h"
#include "List.h"
#include "HashMap.h"
#include "StateManager.h"
#include "UIElement.h"
#include "Sprite.h"
#include "Node.h"
#include "Scene.h"
#include "Text.h"
#include "Vector.h"
#include "ExpirationTimer.h"
#include "TileMap2D.h"


// All Urho3D classes reside in namespace Urho3D
using namespace Urho3D;


//-----------------------------------------------------------------------------
// State
//-----------------------------------------------------------------------------
class GameState : public State
{
	// Enable type information.
	OBJECT(GameState);


public:

	//-------------------------------------------------------------------------
	// Constructor & Destructor
	//-------------------------------------------------------------------------
	GameState(Context* context);
	~GameState();

	//-------------------------------------------------------------------------
	// Public Virtual Methods
	//-------------------------------------------------------------------------
	// State Activation
	/// Init is called directly after state is registered 
	virtual bool	Initialize();
	virtual bool	Begin();
	/// after end : We are no longer attached to anything (simply being stored in
	///				the registered state list maintained by the manager).
	virtual void	End();

	//-------------------------------------------------------------------------
	// Public Methods
	//-------------------------------------------------------------------------

private:
	//-------------------------------------------------------------------------
	// Private Methods
	//-------------------------------------------------------------------------
	//////////////////////////////////////////////////////////////////////////
	/// Construct the scene content.
	void CreateScene();
	/// Construct Game UI.
	void CreateUI();
	/// Set up a viewport for displaying the scene.
	void SetupViewport();
	/// Read input and moves the camera.
	void MoveCamera(float timeStep);
	/// Subscribe to application-wide logic update events.
	void SubscribeToEvents();
	/// Handle the logic update event.
	void HandleUpdate(StringHash eventType, VariantMap& eventData);

	// Handle Gameplay
	void ResetGame();
	void GameOver();
	void HandleQuitMessageAck(StringHash eventType, VariantMap& eventData);

	// Enemy handling function
	void HandleEnemyDied(StringHash eventType, VariantMap& eventData);
	void SpawnEnemy();

	// Wave Handling Function
	void SpawnWave();

	SharedPtr<Scene> scene_;

	SharedPtr<Node> cameraNode_;

	// Tile Map 
	SharedPtr<TileMap2D> tileMap_;

	// Pathfinding
	Vector<Vector2> path_;

	// Wave
	int wave_;
	SharedPtr<Text> waveInfo_;
	float waveTimer_;
	SharedPtr<Text> enemyInfo_;
	int enemiesAlive_;
	int enemiesToSpawn_;
	float enemyTimer_;

	Vector<WeakPtr<Node>> enemies_;

	// Player
	int money_;
	int lifes_;
	SharedPtr<Text> playerInfo_;

	bool gameOver_;

protected:
	//-------------------------------------------------------------------------
	// Protected Variables
	//-------------------------------------------------------------------------

};