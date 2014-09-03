//
// Copyright (c) 2008-2014 the Urho3D project.
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
#include "Application.h"
#undef MessageBox
#include "ExpirationTimer.h"
#include "MessageBox.h"
#include "CustomGeometry.h"
#include "Vector2.h"


namespace Urho3D
{

class Node;
class Scene;
class Sprite;
class BorderImage;

}
enum GameSate
{
	GS_MAINMENU,
	GS_GAMEPLAY,
	GS_GAMEOVER,
	GS_QUIT
};

enum MoveDirection
{
	MD_LEFT,
	MD_RIGHT,
	MD_UP,
	MD_DOWN
};

// All Urho3D classes reside in namespace Urho3D
using namespace Urho3D;

class SimpleSnake : public Application
{
	OBJECT(SimpleSnake);

public:
    /// Construct.
	SimpleSnake(Context* context);

	/// Setup before engine initialization. Modifies the engine parameters.
	virtual void Setup();
	/// Setup after engine initialization. Creates the logo, console & debug HUD.
	virtual void Start();

	/// Control logo visibility.
	void SetLogoVisible(bool enable);
protected:
	/// Logo sprite.
	SharedPtr<Sprite> logoSprite_;

private:
	//////////////////////////////////////////////////////////////////////////
	// create Application
	/// Create logo.
	void CreateLogo();
	/// Set custom window Title & Icon
	void SetWindowTitleAndIcon();
	/// Create console and debug HUD.
	void CreateConsoleAndDebugHud();
	/// Handle key down event to process key controls common to all samples.
	void HandleKeyDown(StringHash eventType, VariantMap& eventData);

	//////////////////////////////////////////////////////////////////////////
	// splash screen
	void SplashScreen();
	/// 
	void HandleSplash(StringHash eventType, VariantMap& eventData);

	//////////////////////////////////////////////////////////////////////////
	// Setup scene
	/// Set up a viewport for displaying the scene.
	void SetupViewport();
	/// Subscribe to application-wide logic update events.
	void SubscribeToEvents();

	// Create game scene 
    void CreateScene();
	/// Create the background grid
	void CreateGrid();
    /// Create all HUD elements.
    void CreateUI();
	/// 
	void AddSegment();
	/// 
	void RandomizeFruitPosition();
	/// 
	bool CheckOverlap(float x, float y);
	/// 
	void FlashStartText();
	/// 
	void CollectFruit();

	// Start game state functions
	/// Start Gameplay
	void StartGame();
	/// Exit Game 
	void Quit();
	/// 
	void GameOver();

	// Handle scene events
    /// Handle the logic update event.
    void HandleUpdate(StringHash eventType, VariantMap& eventData);
	/// Handle the quit message 
	void HandleQuitMessageAck(StringHash eventType, VariantMap& eventData);
	/// 
	void UpdateGameplay(float timeStep);
	///
	void MoveSnake();
	///
	bool ApproximatelyEqual(Vector2 a, Vector2 b, float epsilon);

	//////////////////////////////////////////////////////////////////////////
    // Scene
    SharedPtr<Scene> scene_;
    SharedPtr<Node> cameraNode_;
	SharedPtr<Node> gameNode_;

	// Quit Message Box 
	SharedPtr<Urho3D::MessageBox> messageBox_;
	
	// Background Grid 
	SharedPtr<Node> gridNode_;
	SharedPtr<CustomGeometry> grid_;
	IntVector2 gridSize = IntVector2(32,32);
	int gridTileSize = 16;
	Color gridBorderColor = Color(0.31f, 0.31f, 0.31f);
	Color gridColor = Color(0.15f, 0.15f, 0.15f);

	// snake 
	SharedPtr<Node> snakeHead_;
	SharedPtr<Node> fruit_;
	Vector<SharedPtr<Node>> snakeBody_;
	MoveDirection headDirection = MD_LEFT;
	ExpirationTimer moveTimer_;
	
	// Player Attributes
	int score_ = 0;
	bool snakeAlive_;

	// HUD 
	SharedPtr<Text> scoreText_;
	SharedPtr<Text> titleText_;
	SharedPtr<Text> startText_;
	ExpirationTimer flashText_;

	// Game State
	GameSate gameState_;
};
