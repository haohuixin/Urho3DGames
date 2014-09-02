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

#include "SimpleSnake.h"

#include "DebugNew.h"

#include "MessageBox.h"
#include "UIEvents.h"
#include "Button.h"
#include "Drawable2D.h"
#include "StaticSprite2D.h"

DEFINE_APPLICATION_MAIN(SimpleSnake)

SimpleSnake::SimpleSnake(Context* context) :
Application(context),
gameState_(GS_MAINMENU),
flashText_(300),
pause_(false)
{
}
void SimpleSnake::Setup()
{
	// Modify engine startup parameters
	engineParameters_["WindowTitle"] = GetTypeName();
	engineParameters_["LogName"] = GetTypeName() + ".log";
	engineParameters_["FullScreen"] = false;
	engineParameters_["Headless"] = false;
}

void SimpleSnake::Start()
{
	// show splash screen
	SplashScreen();

	// Create logo
	CreateLogo();

	// Set custom window Title & Icon
	SetWindowTitleAndIcon();

	// Create console and debug HUD
	CreateConsoleAndDebugHud();

	// Subscribe key down event
	SubscribeToEvent(E_KEYDOWN, HANDLER(SimpleSnake, HandleKeyDown));

	// Create the scene content
	CreateScene();

	//
	CreateGrid();

	// Create the UI content
	CreateInstructions();

	// Setup the viewport for displaying the scene
	SetupViewport();

	// Hook up to the frame update events
	SubscribeToEvents();
	flashText_.Reset();
}
void SimpleSnake::SetLogoVisible(bool enable)
{
	if (logoSprite_)
		logoSprite_->SetVisible(enable);
}
void SimpleSnake::CreateLogo()
{
	// Get logo texture
	ResourceCache* cache = GetSubsystem<ResourceCache>();
	Texture2D* logoTexture = cache->GetResource<Texture2D>("Textures/LogoLarge.png");
	if (!logoTexture)
		return;

	// Create logo sprite and add to the UI layout
	UI* ui = GetSubsystem<UI>();
	logoSprite_ = ui->GetRoot()->CreateChild<Sprite>();

	// Set logo sprite texture
	logoSprite_->SetTexture(logoTexture);

	int textureWidth = logoTexture->GetWidth();
	int textureHeight = logoTexture->GetHeight();

	// Set logo sprite scale
	logoSprite_->SetScale(256.0f / textureWidth);

	// Set logo sprite size
	logoSprite_->SetSize(textureWidth, textureHeight);

	// Set logo sprite hot spot
	logoSprite_->SetHotSpot(0, textureHeight);

	// Set logo sprite alignment
	logoSprite_->SetAlignment(HA_LEFT, VA_BOTTOM);

	// Make logo not fully opaque to show the scene underneath
	logoSprite_->SetOpacity(0.75f);

	// Set a low priority for the logo so that other UI elements can be drawn on top
	logoSprite_->SetPriority(-100);
}

void SimpleSnake::SetWindowTitleAndIcon()
{
	ResourceCache* cache = GetSubsystem<ResourceCache>();
	Graphics* graphics = GetSubsystem<Graphics>();
	Image* icon = cache->GetResource<Image>("Textures/UrhoIcon.png");
	graphics->SetWindowIcon(icon);
	graphics->SetWindowTitle("Simple Snake");
}

void SimpleSnake::CreateConsoleAndDebugHud()
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

void SimpleSnake::CreateScene()
{
	ResourceCache* cache = GetSubsystem<ResourceCache>();

	scene_ = new Scene(context_);
	scene_->CreateComponent<Octree>();

	// Create camera node
	cameraNode_ = scene_->CreateChild("Camera");
	// Set camera's position
	cameraNode_->SetPosition(Vector3(0.0f, 0.0f, -10.0f));

	Camera* camera = cameraNode_->CreateComponent<Camera>();
	camera->SetOrthographic(true);

	Graphics* graphics = GetSubsystem<Graphics>();
	camera->SetOrthoSize((float)graphics->GetHeight() * PIXEL_SIZE);
	// Get sprite
	Sprite2D* sprite = cache->GetResource<Sprite2D>("Urho2D/Aster.png");
	if (!sprite)
		return;

	float halfWidth = graphics->GetWidth() * 0.5f * PIXEL_SIZE;
	float halfHeight = graphics->GetHeight() * 0.5f * PIXEL_SIZE;

	for (unsigned i = 0; i < 24; ++i)
	{
		SharedPtr<Node> spriteNode(scene_->CreateChild("StaticSprite2D"));
		spriteNode->SetPosition(Vector3(Random(-halfWidth, halfWidth), Random(-halfHeight, halfHeight), 0.0f));

		StaticSprite2D* staticSprite = spriteNode->CreateComponent<StaticSprite2D>();
		// Set random color
		staticSprite->SetColor(Color(Random(1.0f), Random(1.0f), Random(1.0f), 1.0f));
		// Set blend mode
		staticSprite->SetBlendMode(BLEND_ALPHA);
		// Set sprite
		staticSprite->SetSprite(sprite);
	}


}

void SimpleSnake::CreateInstructions()
{
	ResourceCache* cache = GetSubsystem<ResourceCache>();
	UI* ui = GetSubsystem<UI>();
	XMLFile* style = cache->GetResource<XMLFile>("UI/DefaultStyle.xml");

	// Set the loaded style as default style
	ui->GetRoot()->SetDefaultStyle(style);
	SharedPtr<Cursor> cursor(new Cursor(context_));
	cursor->SetStyleAuto(style);
	ui->SetCursor(cursor);
	ui->GetCursor()->SetVisible(false);

	titleText_ = ui->GetRoot()->CreateChild<Text>();
	titleText_->SetText("Simple Snake");
	titleText_->SetFont(cache->GetResource<Font>("Fonts/Anonymous Pro.ttf"), 55);
	titleText_->SetHorizontalAlignment(HA_CENTER);
	titleText_->SetVerticalAlignment(VA_CENTER);
	titleText_->SetPosition(0, -ui->GetRoot()->GetHeight() / 4);
	titleText_->SetTextEffect(TE_STROKE);

	// Construct new Text object, set string to display and font to use
	startText_ = ui->GetRoot()->CreateChild<Text>();
	startText_->SetText("Click to Start Game");
	startText_->SetFont(cache->GetResource<Font>("Fonts/Anonymous Pro.ttf"), 15);

	// Position the text relative to the screen center
	startText_->SetHorizontalAlignment(HA_CENTER);
	startText_->SetVerticalAlignment(VA_CENTER);
	startText_->SetPosition(0, ui->GetRoot()->GetHeight() / 4);
}

void SimpleSnake::SetupViewport()
{
	Renderer* renderer = GetSubsystem<Renderer>();

	// Set up a viewport to the Renderer subsystem so that the 3D scene can be seen. We need to define the scene and the camera
	// at minimum. Additionally we could configure the viewport screen size and the rendering path (eg. forward / deferred) to
	// use, but now we just use full screen and default render path configured in the engine command line options
	SharedPtr<Viewport> viewport(new Viewport(context_, scene_, cameraNode_->GetComponent<Camera>()));
	renderer->SetViewport(0, viewport);
}

void SimpleSnake::MoveCamera(float timeStep)
{
	// Do not move if the UI has a focused element (the console)
	if (GetSubsystem<UI>()->GetFocusElement())
		return;

	Input* input = GetSubsystem<Input>();

	// Movement speed as world units per second
	const float MOVE_SPEED = 20.0f;
	// Mouse sensitivity as degrees per pixel
	const float MOUSE_SENSITIVITY = 0.1f;

	// Read WASD keys and move the camera scene node to the corresponding direction if they are pressed
	// Use the TranslateRelative() function to move relative to the node's orientation. Alternatively we could
	// multiply the desired direction with the node's orientation quaternion, and use just Translate()
	if (input->GetKeyDown('W'))
		cameraNode_->Translate(Vector3::FORWARD * MOVE_SPEED * timeStep);
	if (input->GetKeyDown('S'))
		cameraNode_->Translate(Vector3::BACK * MOVE_SPEED * timeStep);
	if (input->GetKeyDown('A'))
		cameraNode_->Translate(Vector3::LEFT * MOVE_SPEED * timeStep);
	if (input->GetKeyDown('D'))
		cameraNode_->Translate(Vector3::RIGHT * MOVE_SPEED * timeStep);
}

void SimpleSnake::SubscribeToEvents()
{
	// Subscribe HandleUpdate() function for processing update events
	SubscribeToEvent(E_UPDATE, HANDLER(SimpleSnake, HandleUpdate));
}

void SimpleSnake::HandleUpdate(StringHash eventType, VariantMap& eventData)
{
	using namespace Update;

	// Take the frame time step, which is stored as a float
	float timeStep = eventData[P_TIMESTEP].GetFloat();

	switch (gameState_)
	{
	case GS_MAINMENU:
	{
						if (flashText_.Expired() && startText_->IsVisible())
						{
							startText_->SetVisible(false);
							flashText_.Reset();
						}
						else if (flashText_.Expired() && !startText_->IsVisible())
						{
							startText_->SetVisible(true);
							flashText_.Reset();
						}
	}
		break;
	case GS_GAMEPLAY:
		break;
	case GS_GAMEOVER:
		break;
	case GS_QUIT:
		break;
	default:
		break;
	}
}

void SimpleSnake::HandleKeyDown(StringHash eventType, VariantMap& eventData)
{
	using namespace KeyDown;

	int key = eventData[P_KEY].GetInt();

	// Close console (if open) or exit when ESC is pressed
	if (key == KEY_ESC)
	{
		Console* console = GetSubsystem<Console>();
		if (console->IsVisible())
			console->SetVisible(false);
		else
			Quit();
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

void SimpleSnake::SplashScreen()
{
	ResourceCache* cache = GetSubsystem<ResourceCache>();
	UI* ui = GetSubsystem<UI>();
	BorderImage* splashUI = new BorderImage(context_);
	splashUI->SetName("Splash");
	Texture2D* texture = cache->GetResource<Texture2D>("Textures/LogoLarge.png");
	splashUI->SetTexture(texture); // Set texture
	splashUI->SetSize(texture->GetWidth(), texture->GetHeight());
	splashUI->SetAlignment(HA_CENTER, VA_CENTER);
	ui->GetRoot()->AddChild(splashUI);
	GetSubsystem<Engine>()->RunFrame(); // Render Splash immediately
	SubscribeToEvent(E_ENDRENDERING, HANDLER(SimpleSnake, HandleSplash)); // Keep visible until rendering of the scene
}

void SimpleSnake::HandleSplash(StringHash eventType, VariantMap& eventData)
{
	// Remove splash screen when scene fully rendered
	UIElement* splashUI = GetSubsystem<UI>()->GetRoot()->GetChild("Splash", true);
	if (splashUI)
	{
		UnsubscribeFromEvent(E_ENDRENDERING);
		splashUI->Remove();
	}
}

void SimpleSnake::Quit()
{

	GetSubsystem<UI>()->GetCursor()->SetVisible(true);

	messageBox_ = new Urho3D::MessageBox(context_, "Do you really want to exit the Game ?", "Quit Game ?");
	if (messageBox_->GetWindow() != NULL)
	{
		Button* cancelButton = (Button*)messageBox_->GetWindow()->GetChild("CancelButton", true);
		cancelButton->SetVisible(true);
		cancelButton->SetFocus(true);
		SubscribeToEvent(messageBox_, E_MESSAGEACK, HANDLER(SimpleSnake, HandleQuitMessageAck));
	}

	if (scene_.NotNull())
		scene_->SetUpdateEnabled(false);

	gameState_ = GS_QUIT;
}

void SimpleSnake::HandleQuitMessageAck(StringHash eventType, VariantMap& eventData)
{
	using namespace MessageACK;

	bool ok_ = eventData[P_OK].GetBool();
	

	GetSubsystem<UI>()->GetCursor()->SetVisible(false);

	if (scene_.NotNull())
		scene_->SetUpdateEnabled(true);
	gameState_ = GS_MAINMENU;
	if (ok_)
	{
		engine_->Exit();
	}
	
}

void SimpleSnake::StartGame()
{
	gameState_ = GS_GAMEPLAY;
	startText_->SetVisible(false);
	titleText_->SetVisible(false);


}

void SimpleSnake::CreateGrid()
{
	ResourceCache* cache = GetSubsystem<ResourceCache>();
	gridNode_ = scene_->CreateChild("Grid");
	gridNode_->SetScale ( Vector3(gridScale, gridScale, gridScale));
	grid_ = gridNode_->CreateComponent<CustomGeometry>();
	grid_->SetNumGeometries( 1);
	grid_->SetMaterial(cache->GetResource<Material>("Materials/VColUnlit.xml"));

	
	int size = int(floor(gridSize / 2) * 2);
	float halfSizeScaled = size / 2;
	float scale = 1.0;
	int subdivisionSize = int(pow(2.0f, float(gridSubdivisions)));
	if (subdivisionSize > 0)
	{
		size *= subdivisionSize;
		scale /= subdivisionSize;
	}

	int halfSize = size / 2;

	grid_->BeginGeometry(0, LINE_LIST);

	float lineOffset = -halfSizeScaled;
	for (int i = 0; i <= size; ++i)
	{
		bool lineCenter = i == halfSize;
		bool lineSubdiv = !Equals((i% subdivisionSize), 0.0);

		{
			grid_->DefineVertex(Vector3(lineOffset, halfSizeScaled, 0.0));
			grid_->DefineColor(lineCenter ? gridYColor : (lineSubdiv ? gridSubdivisionColor : gridColor));
			grid_->DefineVertex(Vector3(lineOffset, -halfSizeScaled, 0.0));
			grid_->DefineColor(lineCenter ? gridYColor : (lineSubdiv ? gridSubdivisionColor : gridColor));
			grid_->DefineVertex(Vector3(-halfSizeScaled, lineOffset, 0.0));
			grid_->DefineColor(lineCenter ? gridXColor : (lineSubdiv ? gridSubdivisionColor : gridColor));
			grid_->DefineVertex(Vector3(halfSizeScaled, lineOffset, 0.0));
			grid_->DefineColor(lineCenter ? gridXColor : (lineSubdiv ? gridSubdivisionColor : gridColor));
		}
		lineOffset += scale;
	}
	grid_->Commit();
}
