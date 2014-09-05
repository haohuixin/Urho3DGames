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
#include "Application.h"
#include "Console.h"
#include "DebugHud.h"
#include "Engine.h"
#include "FileSystem.h"
#include "Graphics.h"
#include "InputEvents.h"
#include "Renderer.h"
#include "ResourceCache.h"
#include "Sprite.h"
#include "Texture2D.h"
#include "Timer.h"
#include "XMLFile.h"
#include "Camera.h"
#include "CoreEvents.h"
#include "Engine.h"
#include "Font.h"
#include "Graphics.h"
#include "Input.h"
#include "Material.h"
#include "Model.h"
#include "Octree.h"
#include "Renderer.h"
#include "ResourceCache.h"
#include "Scene.h"
#include "StaticModel.h"
#include "Text.h"
#include "UI.h"
#include "BorderImage.h"
#include "GraphicsEvents.h"


#include "SimpleTD.h"

#include "DebugNew.h"

#include "StateManager.h"
#include "MainMenuState.h"
#include "GameplayState.h"
#include "SplashState.h"

DEFINE_APPLICATION_MAIN(SimpleTD)

SimpleTD::SimpleTD(Context* context) :
Application(context)
{
}
void SimpleTD::Setup()
{
	// Modify engine startup parameters
	engineParameters_["WindowTitle"] = GetTypeName();
	engineParameters_["LogName"] = GetTypeName() + ".log";
	engineParameters_["FullScreen"] = false;
	engineParameters_["Headless"] = false;
	engineParameters_["WindowIcon"] = "Textures/UrhoIcon.png";
	engineParameters_["WindowTitle"] = "Simple Tower Defense";
	engineParameters_["ResourcePaths"] = "CoreData;Data;GameData;";
}


void SimpleTD::Start()
{
	// Create Game State Manager
	stateManager_ = new StateManager(context_);

	// Create console and debug HUD
	CreateConsoleAndDebugHud();

	// Main Menu
	State * mainMenuState = new MenuState(context_);
	mainMenuState->SetStateId("MainMenu");
	stateManager_->RegisterState(mainMenuState);

	// Game State
	State * gameState = new GameState(context_);
	gameState->SetStateId("GameState");
	stateManager_->RegisterState(gameState);


	SplashState * splashState = new SplashState(context_);
	splashState->SetStateId("SplashState");
	stateManager_->RegisterState(splashState);

	splashState->SetNextState("MainMenu");


	stateManager_->PushToStack("SplashState");

	// Subscribe key down event
	SubscribeToEvent(E_KEYDOWN, HANDLER(SimpleTD, HandleKeyDown));
}

void SimpleTD::CreateConsoleAndDebugHud()
{
	// Get default style
	ResourceCache* cache = GetSubsystem<ResourceCache>();
	XMLFile* xmlFile = cache->GetResource<XMLFile>("UI/DefaultStyle.xml");

	// Create console
	Console* console = engine_->CreateConsole();
	console->SetDefaultStyle(xmlFile);

	// Create debug HUD.
	DebugHud* debugHud = engine_->CreateDebugHud();
	debugHud->SetDefaultStyle(xmlFile);
}

void SimpleTD::HandleKeyDown(StringHash eventType, VariantMap& eventData)
{
	using namespace KeyDown;

	int key = eventData[P_KEY].GetInt();

	// Close console (if open) or exit when ESC is pressed
	if (key == KEY_ESC)
	{
		Console* console = GetSubsystem<Console>();
		if (console->IsVisible())
			console->SetVisible(false);
// 		else
// 			engine_->Exit();
	}

	// Toggle console with F1
	else if (key == KEY_F1)
		GetSubsystem<Console>()->Toggle();

	// Toggle debug HUD with F2
	else if (key == KEY_F2)
		GetSubsystem<DebugHud>()->ToggleAll();

	// Common rendering quality controls, only when UI has no focused element
	else if (!GetSubsystem<UI>()->GetFocusElement())
	{
		Renderer* renderer = GetSubsystem<Renderer>();

		// Texture quality
		if (key == '1')
		{
			int quality = renderer->GetTextureQuality();
			++quality;
			if (quality > QUALITY_HIGH)
				quality = QUALITY_LOW;
			renderer->SetTextureQuality(quality);
		}

		// Material quality
		else if (key == '2')
		{
			int quality = renderer->GetMaterialQuality();
			++quality;
			if (quality > QUALITY_HIGH)
				quality = QUALITY_LOW;
			renderer->SetMaterialQuality(quality);
		}

		// Specular lighting
		else if (key == '3')
			renderer->SetSpecularLighting(!renderer->GetSpecularLighting());

		// Shadow rendering
		else if (key == '4')
			renderer->SetDrawShadows(!renderer->GetDrawShadows());

		// Shadow map resolution
		else if (key == '5')
		{
			int shadowMapSize = renderer->GetShadowMapSize();
			shadowMapSize *= 2;
			if (shadowMapSize > 2048)
				shadowMapSize = 512;
			renderer->SetShadowMapSize(shadowMapSize);
		}

		// Shadow depth and filtering quality
		else if (key == '6')
		{
			int quality = renderer->GetShadowQuality();
			++quality;
			if (quality > SHADOWQUALITY_HIGH_24BIT)
				quality = SHADOWQUALITY_LOW_16BIT;
			renderer->SetShadowQuality(quality);
		}

		// Occlusion culling
		else if (key == '7')
		{
			bool occlusion = renderer->GetMaxOccluderTriangles() > 0;
			occlusion = !occlusion;
			renderer->SetMaxOccluderTriangles(occlusion ? 5000 : 0);
		}

		// Instancing
		else if (key == '8')
			renderer->SetDynamicInstancing(!renderer->GetDynamicInstancing());

		// Take screenshot
		else if (key == '9')
		{
			Graphics* graphics = GetSubsystem<Graphics>();
			Image screenshot(context_);
			graphics->TakeScreenShot(screenshot);
			// Here we save in the Data folder with date and time appended
			screenshot.SavePNG(GetSubsystem<FileSystem>()->GetProgramDir() + "Data/Screenshot_" +
				Time::GetTimeStamp().Replaced(':', '_').Replaced('.', '_').Replaced(' ', '_') + ".png");
		}
	}
}




