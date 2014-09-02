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
	void HandleSplash(StringHash eventType, VariantMap& eventData);

	//////////////////////////////////////////////////////////////////////////
	// create sample
    /// Construct the scene content.
    void CreateScene();
	/// 
	void CreateGrid();
    /// Construct an instruction text to the UI.
    void CreateInstructions();
    /// Set up a viewport for displaying the scene.
    void SetupViewport();
    /// Read input and moves the camera.
    void MoveCamera(float timeStep);
    /// Subscribe to application-wide logic update events.
    void SubscribeToEvents();
    /// Handle the logic update event.
    void HandleUpdate(StringHash eventType, VariantMap& eventData);
	/// Handle the quit message 
	void HandleQuitMessageAck(StringHash eventType, VariantMap& eventData);
	/// Start Gameplay
	void StartGame();
	/// Exit Game 
	void Quit();
    /// Scene.
    SharedPtr<Scene> scene_;
    /// Camera scene node.
    SharedPtr<Node> cameraNode_;

	SharedPtr<Text> titleText_;

	SharedPtr<Text> startText_;

	GameSate gameState_;
	ExpirationTimer flashText_;
	SharedPtr<Urho3D::MessageBox> messageBox_;
	bool pause_;
	SharedPtr<Node> gridNode_;
	SharedPtr<CustomGeometry> grid_;

	bool showGrid = true;
	bool grid2DMode = false;
	int gridSize = 16;
	int gridSubdivisions = 3;
	float gridScale = 2.0f;
	Color gridColor = Color(0.1f, 0.1f, 0.1f);
	Color gridSubdivisionColor = Color(0.05f, 0.05f, 0.05f);
	Color gridXColor = Color(0.1f, 0.1f, 0.1f);
	Color gridYColor = Color(0.1f, 0.1f, 0.1f);
	Color gridZColor = Color(0.1f, 0.1f, 0.1f);
};
