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

#include "GameplayState.h"
#include "Object.h"
#include "CoreEvents.h"
#include "ResourceCache.h"
#include "List.h"
#include "UI.h"

#include "Button.h"
#include "CheckBox.h"
#include "CoreEvents.h"
#include "Engine.h"
#include "Input.h"
#include "LineEdit.h"
#include "Text.h"
#include "UIEvents.h"
#include "Window.h"

#include "DebugNew.h"
#include "Texture2D.h"
#include "ResourceCache.h"
#include "XMLFile.h"
#include "Octree.h"
#include "StaticModel.h"
#include "Material.h"
#include "Model.h"
#include "Light.h"
#include "Vector3.h"
#include "Quaternion.h"
#include "Camera.h"
#include "Renderer.h"
#include "Viewport.h"
#include "Font.h"
#include "Graphics.h"
#include "TmxFile2D.h"
#include "TileMap2D.h"
#include "Drawable2D.h"
#include "Pathfinding.h"
#include "StaticSprite2D.h"
#include "SpriteSheet2D.h"
#include "Enemy.h"


GameState::GameState(Context* context) : State(context),
wave_(1)
{
	Enemy::RegisterObject(context);
}

GameState::~GameState()
{

}

bool GameState::Begin()
{
	// Create the scene content
	CreateScene();

	// Create the UI content
	CreateInstructions();

	// Setup the viewport for displaying the scene
	SetupViewport();

	// Hook up to the frame update events
	SubscribeToEvents();

	return State::Begin();
}

bool GameState::Initialize()
{
	return State::Initialize();
}

void GameState::CreateScene()
{
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

	ResourceCache* cache = GetSubsystem<ResourceCache>();
	// Get tmx file
	TmxFile2D* tmxFile = cache->GetResource<TmxFile2D>("Tilemaps/TestMap.tmx");
	if (!tmxFile)
		return;

	SharedPtr<Node> tileMapNode(scene_->CreateChild("TileMap"));
	tileMapNode->SetPosition(Vector3(0.0f, 0.0f, -1.0f));

	tileMap_ = tileMapNode->CreateComponent<TileMap2D>();
	// Set animation
	tileMap_->SetTmxFile(tmxFile);

	// Set camera's position
	const TileMapInfo2D& info = tileMap_->GetInfo();
	float x = info.GetMapWidth() * 0.5f;
	float y = info.GetMapHeight() * 0.5f;
	cameraNode_->SetPosition(Vector3(x, y, -10.0f));

	// find Terrain layer  and find the Events Object Layer to get spawn points and goals 
	const TmxTileLayer2D* terrainlayer = NULL;
	const TmxObjectGroup2D* eventsLayer = NULL;
	for (int i = 0; i < tmxFile->GetNumLayers(); i++)
	{
		if (tmxFile->GetLayer(i)->GetName() == "Terrain")
		{
			terrainlayer = static_cast<const TmxTileLayer2D* > (tmxFile->GetLayer(i));
		}
		else if (tmxFile->GetLayer(i)->GetName() == "Events")
		{
			eventsLayer = static_cast<const TmxObjectGroup2D* > (tmxFile->GetLayer(i));
		}
	}
	
	if (terrainlayer && eventsLayer)
	{
		ResourceCache* cache = GetSubsystem<ResourceCache>();
		
		// create Tile Grid from the Terrain layer. every existing tile is walkable, so set only wall tiles, which are not defined tiles.
		SquareGrid grid(terrainlayer->GetWidth(), terrainlayer->GetHeight());
		for (int x = 0; x < terrainlayer->GetWidth(); ++x) {
			for (int y = 0; y < terrainlayer->GetHeight(); ++y) {
				if (!terrainlayer->GetTile(x, y))
					grid.walls.insert(SquareGrid::Location{ x, y });
			}
		}		// retrieve Start and end points from events object layer		SquareGrid::Location startPoint;
		SquareGrid::Location goalPoint;
		for (int i = 0; i < eventsLayer->GetNumObjects(); ++i) 
		{
			TileMapObject2D* obj = eventsLayer->GetObject(i);
			if (obj->GetName() == "Goal")
			{
				Vector2 pixelPos = obj->GetPosition();
				pixelPos.y_ = (info.GetMapHeight() - pixelPos.y_) - obj->GetSize().y_;
				pixelPos /= PIXEL_SIZE;
				Vector2 gridPos;
				gridPos.y_ = pixelPos.y_ / (info.tileHeight_ / PIXEL_SIZE);
				gridPos.x_ = pixelPos.x_ / (info.tileWidth_ / PIXEL_SIZE);
				goalPoint = SquareGrid::Location{ gridPos.x_, gridPos.y_ };
			}
			else if (obj->GetName() == "SpawnPoint")
			{
				Vector2 pixelPos = obj->GetPosition();
				pixelPos.y_ = (info.GetMapHeight() - pixelPos.y_) - obj->GetSize().y_;
				pixelPos /= PIXEL_SIZE;
				Vector2 gridPos;
				gridPos.y_ = pixelPos.y_ / (info.tileHeight_ / PIXEL_SIZE);
				gridPos.x_ = pixelPos.x_ / (info.tileWidth_ / PIXEL_SIZE);
				startPoint = SquareGrid::Location{ gridPos.x_, gridPos.y_ };
			}
		}

		// create path for the enemy to walk on. 
		auto parents = breadth_first_search(grid, startPoint, goalPoint);
		vector<SquareGrid::Location> path = reconstruct_path(startPoint, goalPoint, parents);

		// Construct debug text 
//		UI* ui = GetSubsystem<UI>();
// 		Text* maptext = ui->GetRoot()->CreateChild<Text>();
// 		maptext->SetFont(cache->GetResource<Font>("Fonts/Anonymous Pro.ttf"), 15);
// 		maptext->SetHorizontalAlignment(HA_CENTER);
// 		maptext->SetVerticalAlignment(VA_CENTER);
//		maptext->SetText(draw_grid_to_String(grid, 2, nullptr, nullptr, &path).c_str());

		// create usable path for the enemy component.
		path_.Clear();
		for (int i = path.size() - 1; i >= 0; i--)
		{
			Vector2 pos(std::get<0>(path.at(i)), std::get<1>(path.at(i)));

			path_.Push(info.TileIndexToPosition(pos.x_, pos.y_));
		}
		
		// create enemy 
		SpriteSheet2D* 	spriteSheet = cache->GetResource<SpriteSheet2D>("Tilemaps/TileMapSprites.xml");
		if (!spriteSheet)
			return;
		Node* enemySpriteNode_ = scene_->CreateChild("Enemy");
		StaticSprite2D* staticSprite = enemySpriteNode_->CreateComponent<StaticSprite2D>();
		spriteSheet->GetSprite("Enemy")->SetHotSpot(Vector2(0.0f, 0.0f));
		staticSprite->SetSprite(spriteSheet->GetSprite("Enemy"));
		staticSprite->SetLayer(5*10);
		/// create Enemy component which controls the Enemy behavior
		Enemy* e = enemySpriteNode_->CreateComponent<Enemy>();
		e->FollowPath(&path_);
		e->SetSpeed(4.0f);		
	}
}

void GameState::CreateInstructions()
{
	ResourceCache* cache = GetSubsystem<ResourceCache>();
	UI* ui = GetSubsystem<UI>();

	// Construct new Text object, set string to display and font to use
// 	instructionText = ui->GetRoot()->CreateChild<Text>();
// 	instructionText->SetText("Use WASD keys to move, use PageUp PageDown keys to zoom.");
// 	instructionText->SetFont(cache->GetResource<Font>("Fonts/Anonymous Pro.ttf"), 15);
// 
// 	// Position the text relative to the screen center
// 	instructionText->SetHorizontalAlignment(HA_CENTER);
// 	instructionText->SetVerticalAlignment(VA_CENTER);
// 	instructionText->SetPosition(0, ui->GetRoot()->GetHeight() / 4);
}

void GameState::SetupViewport()
{
	Renderer* renderer = GetSubsystem<Renderer>();

	// Set up a viewport to the Renderer subsystem so that the 3D scene can be seen. We need to define the scene and the camera
	// at minimum. Additionally we could configure the viewport screen size and the rendering path (eg. forward / deferred) to
	// use, but now we just use full screen and default render path configured in the engine command line options
	SharedPtr<Viewport> viewport(new Viewport(context_, scene_, cameraNode_->GetComponent<Camera>()));
	renderer->SetViewport(0, viewport);
}

void GameState::MoveCamera(float timeStep)
{
	// Do not move if the UI has a focused element (the console)
	if (GetSubsystem<UI>()->GetFocusElement())
		return;

	Input* input = GetSubsystem<Input>();

	// Movement speed as world units per second
	const float MOVE_SPEED = 4.0f;

	// Read WASD keys and move the camera scene node to the corresponding direction if they are pressed
	if (input->GetKeyDown('W'))
		cameraNode_->Translate(Vector3::UP * MOVE_SPEED * timeStep);
	if (input->GetKeyDown('S'))
		cameraNode_->Translate(Vector3::DOWN * MOVE_SPEED * timeStep);
	if (input->GetKeyDown('A'))
		cameraNode_->Translate(Vector3::LEFT * MOVE_SPEED * timeStep);
	if (input->GetKeyDown('D'))
		cameraNode_->Translate(Vector3::RIGHT * MOVE_SPEED * timeStep);

	if (input->GetKeyDown(KEY_PAGEUP))
	{
		Camera* camera = cameraNode_->GetComponent<Camera>();
		camera->SetZoom(camera->GetZoom() * 1.01f);
	}

	if (input->GetKeyDown(KEY_PAGEDOWN))
	{
		Camera* camera = cameraNode_->GetComponent<Camera>();
		camera->SetZoom(camera->GetZoom() * 0.99f);
	}
}

void GameState::SubscribeToEvents()
{
	// Subscribe HandleUpdate() function for processing update events
	SubscribeToEvent(E_UPDATE, HANDLER(GameState, HandleUpdate));
}

void GameState::HandleUpdate(StringHash eventType, VariantMap& eventData)
{
	using namespace Update;

	// Take the frame time step, which is stored as a float
	float timeStep = eventData[P_TIMESTEP].GetFloat();

	// Move the camera, scale movement with time step
	MoveCamera(timeStep);

	Input* input = GetSubsystem<Input>();
	if (input->GetKeyPress(KEY_ESC))
		stateManager_->PopStack();
}

void GameState::End()
{
	 scene_.Reset();	
	 cameraNode_.Reset();
// 	 if (GetSubsystem<UI>())
// 		 GetSubsystem<UI>()->GetRoot()->RemoveChild(instructionText);
	 UnsubscribeFromEvent(E_UPDATE);
	 path_.Clear();
	State::End();
}


