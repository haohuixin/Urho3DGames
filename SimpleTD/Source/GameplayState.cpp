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
#include "MessageBox.h"
#include "Tower.h"
#include "Bullet.h"
#include "UIElement.h"
#include "InputEvents.h"
#include "DecalSet.h"

#define WAVE_SPAWN_INTERVAL 3.0f  // seconds
#define ENEMY_SPAWN_INTERVAL 0.85f // seconds
#define ENEMY_SPEED 3.30f
#define PLAYER_LIFE 10
GameState::GameState(Context* context) : State(context),
wave_(0),
enemiesAlive_(0),
enemiesToSpawn_(0),
waveTimer_(5.0f),
enemyTimer_(0.0f),
money_(40),
lifes_(PLAYER_LIFE),
gameOver_(false)
{
	Enemy::RegisterObject(context);
	Bullet::RegisterObject(context);
	Tower::RegisterObject(context);
}

GameState::~GameState()
{
}

bool GameState::Begin()
{
	// Create the UI content
	CreateUI();
	// Create the scene content
	CreateScene();
	// Setup the viewport for displaying the scene
	SetupViewport();
	// Hook up to the frame update events
	SubscribeToEvents();

	ResetGame();
	GetSubsystem<Input>()->SetMouseVisible(true);
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

	plane_.Define(Vector3(0.0f, 0.0f, -10.0f), Vector3(0.0f, 0.0f, -1.0f));

	// find Terrain layer  and find the Events Object Layer to get spawn points and goals
	const TmxTileLayer2D* terrainlayer = NULL;
	const TmxObjectGroup2D* eventsLayer = NULL;
	for (int i = 0; i < tmxFile->GetNumLayers(); i++)
	{
		if (tmxFile->GetLayer(i)->GetName() == "Terrain")
		{
			terrainlayer = static_cast<const TmxTileLayer2D*> (tmxFile->GetLayer(i));
		}
		else if (tmxFile->GetLayer(i)->GetName() == "Events")
		{
			eventsLayer = static_cast<const TmxObjectGroup2D*> (tmxFile->GetLayer(i));
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
		}
		// retrieve Start and end points from events object layer		// 		SquareGrid::Location startPoint;
		// 		SquareGrid::Location goalPoint;
		Vector2 startPoint;
		Vector2 goalPoint;
		TileMapObject2D* tower = NULL;
		for (int i = 0; i < eventsLayer->GetNumObjects(); ++i)
		{
			TileMapObject2D* obj = eventsLayer->GetObject(i);
			if (obj->GetName() == "Goal")
			{
				Vector2 pixelPos = obj->GetPosition();
				pixelPos.y_ = (info.GetMapHeight() - pixelPos.y_) - obj->GetSize().y_;
				pixelPos /= PIXEL_SIZE;
				goalPoint.y_ = pixelPos.y_ / (info.tileHeight_ / PIXEL_SIZE);
				goalPoint.x_ = pixelPos.x_ / (info.tileWidth_ / PIXEL_SIZE);
				//	goalPoint = SquareGrid::Location{ gridPos.x_, gridPos.y_ };
			}
			else if (obj->GetName() == "SpawnPoint")
			{
				Vector2 pixelPos = obj->GetPosition();
				pixelPos.y_ = (info.GetMapHeight() - pixelPos.y_) - obj->GetSize().y_;
				pixelPos /= PIXEL_SIZE;
				startPoint.y_ = pixelPos.y_ / (info.tileHeight_ / PIXEL_SIZE);
				startPoint.x_ = pixelPos.x_ / (info.tileWidth_ / PIXEL_SIZE);
				//startPoint = SquareGrid::Location{ gridPos.x_, gridPos.y_ };
			}
			else if (obj->GetName() == "Tower")
			{
				// 				tower = eventsLayer->GetObject(i);
				// 				SharedPtr<Node> towerNode_(scene_->CreateChild("Tower"));
				// 				towerNode_->SetPosition2D(tower->GetPosition());
				// 				SharedPtr<Tower> t(towerNode_->CreateComponent<Tower>());
				// 				t->SetEnemies(&enemies_);
				// 
				// 				ResourceCache* cache = GetSubsystem<ResourceCache>();
				// 				// create enemy
				// 				SpriteSheet2D* 	spriteSheet = cache->GetResource<SpriteSheet2D>("Tilemaps/TileMapSprites.xml");
				// 				if (!spriteSheet)
				// 					return;
				// 
				// 				StaticSprite2D* staticSprite = towerNode_->CreateComponent<StaticSprite2D>();
				// 				spriteSheet->GetSprite("Tower")->SetHotSpot(Vector2(0.0f, 0.0f));
				// 				staticSprite->SetSprite(spriteSheet->GetSprite("Tower"));
				// 				staticSprite->SetLayer(5 * 10);
			}
		}

		// create path for the enemy to walk on.
		auto parents = breadth_first_search(grid, SquareGrid::Location{ int(startPoint.x_), int(startPoint.y_) }, SquareGrid::Location{ int(goalPoint.x_), int(goalPoint.y_) });
		vector<SquareGrid::Location> path = reconstruct_path(SquareGrid::Location{ int(startPoint.x_), int(startPoint.y_) }, SquareGrid::Location{ int(goalPoint.x_), int(goalPoint.y_) }, parents);

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

		waveInfo_->SetVisible(true);

		{	// temp tower for buildmode
		tempTowerNode_ = scene_->CreateChild("Tower");
		ResourceCache* cache = GetSubsystem<ResourceCache>();
		// create enemy
		SpriteSheet2D* 	spriteSheet = cache->GetResource<SpriteSheet2D>("Tilemaps/TileMapSprites.xml");
		if (!spriteSheet)
			return;

		StaticSprite2D* staticSprite = tempTowerNode_->CreateComponent<StaticSprite2D>();
		spriteSheet->GetSprite("Tower")->SetHotSpot(Vector2(0.0f, 0.0f));
		staticSprite->SetSprite(spriteSheet->GetSprite("Tower"));
		staticSprite->SetLayer(5 * 10);
		staticSprite->SetColor(Color::GRAY);
		tempTowerNode_->SetEnabled(false);
		//staticSprite->SetVisibility(false);
	}
	}
}

void GameState::CreateUI()
{
	ResourceCache* cache = GetSubsystem<ResourceCache>();
	UI* ui = GetSubsystem<UI>();
	// Load XML file containing default UI style sheet
	XMLFile* style = cache->GetResource<XMLFile>("UI/DefaultStyle.xml");

	// Set the loaded style as default style
	ui->GetRoot()->SetDefaultStyle(style);

	// Construct new Text object, set string to display and font to use
	waveInfo_ = ui->GetRoot()->CreateChild<Text>();
	waveInfo_->SetText("Wave ");
	waveInfo_->SetFont(cache->GetResource<Font>("Fonts/Anonymous Pro.ttf"), 15);
	waveInfo_->SetPosition(15, 15);
	waveInfo_->SetVisible(false);
	waveInfo_->SetColor(Color::WHITE);
	waveInfo_->SetEffectColor(Color::BLACK);
	waveInfo_->SetTextEffect(TE_STROKE);

	enemyInfo_ = ui->GetRoot()->CreateChild<Text>();
	enemyInfo_->SetText("Enemies left: ");
	enemyInfo_->SetFont(cache->GetResource<Font>("Fonts/Anonymous Pro.ttf"), 12);
	enemyInfo_->SetPosition(15, 38);
	enemyInfo_->SetVisible(false);
	enemyInfo_->SetColor(Color::RED);
	enemyInfo_->SetEffectColor(Color::WHITE);
	enemyInfo_->SetTextEffect(TE_STROKE);

	playerInfo_ = ui->GetRoot()->CreateChild<Text>();
	playerInfo_->SetFont(cache->GetResource<Font>("Fonts/Anonymous Pro.ttf"), 15);
	playerInfo_->SetPosition(0, 35);
	playerInfo_->SetVisible(true);
	playerInfo_->SetColor(Color::WHITE);
	playerInfo_->SetEffectColor(Color::BLACK);
	playerInfo_->SetTextEffect(TE_STROKE);
	playerInfo_->SetAlignment(HA_CENTER, VA_TOP);
	String str;
	str.AppendWithFormat("Lifes %i Money %i", lifes_, money_);
	playerInfo_->SetText(str);

	// Create Upgrade Window container
	menuBar_ = ui->GetRoot()->CreateChild<UIElement>();
	menuBar_->SetMinSize(0, 24);
	menuBar_->SetPosition(0, -50);
	menuBar_->SetAlignment(HA_CENTER, VA_BOTTOM);
	menuBar_->SetLayoutMode(LM_HORIZONTAL);
	menuBar_->SetVisible(false);

	// Create a Buy Tower Button
	buyTowerButton_ = ui->GetRoot()->CreateChild<Button>();;
	buyTowerButton_->SetName("Buy");
	buyTowerButton_->SetFixedSize(100, 30);
	buyTowerButton_->SetPosition(0, -50);
	buyTowerButton_->SetStyleAuto();
	buyTowerButton_->SetAlignment(HA_CENTER, VA_BOTTOM);
	buytowerText_ = buyTowerButton_->CreateChild< Text>();
	buytowerText_->SetName("BuyTower");
	str.Clear();
	str.AppendWithFormat("Buy %i", towerPrice_);
	buytowerText_->SetText(str);
	buytowerText_->SetFont(cache->GetResource<Font>("Fonts/Anonymous Pro.ttf"), 12);
	buytowerText_->SetAlignment(HA_CENTER, VA_CENTER);
	// Subscribe to buttonClose release (following a 'press') events
	SubscribeToEvent(buyTowerButton_, E_RELEASED, HANDLER(GameState, HandleBuyPressed));

	// Create a Cancel Button. to abort buy state or sell state
	canelButton_ = ui->GetRoot()->CreateChild<Button>();;
	canelButton_->SetName("Cancel");
	canelButton_->SetFixedSize(100, 30);
	canelButton_->SetPosition(0, -50);
	canelButton_->SetStyleAuto();
	canelButton_->SetAlignment(HA_CENTER, VA_BOTTOM);
	Text* cancelText_ = canelButton_->CreateChild< Text>();
	cancelText_->SetName("Cancel");
	cancelText_->SetText("Cancel");
	cancelText_->SetFont(cache->GetResource<Font>("Fonts/Anonymous Pro.ttf"), 12);
	cancelText_->SetAlignment(HA_CENTER, VA_CENTER);
	cancelText_->SetColor(Color::RED);
	cancelText_->SetEffectColor(Color::WHITE);
	cancelText_->SetTextEffect(TE_STROKE);
	canelButton_->SetVisible(false);
	// Subscribe to buttonClose release (following a 'press') events
	SubscribeToEvent(canelButton_, E_RELEASED, HANDLER(GameState, HandleCancelPressed));

	// Upgrade menu
	Button* backButton = menuBar_->CreateChild<Button>();;
	backButton->SetName("Back");
	backButton->SetFixedSize(80, 30);
	backButton->SetStyleAuto();
	backButton->SetAlignment(HA_RIGHT, VA_BOTTOM);
	Text* backText_ = backButton->CreateChild< Text>();
	backText_->SetName("BackText");
	backText_->SetText("Back");
	backText_->SetFont(cache->GetResource<Font>("Fonts/Anonymous Pro.ttf"), 12);
	backText_->SetAlignment(HA_CENTER, VA_CENTER);
	backText_->SetColor(Color::BLACK);
	backText_->SetEffectColor(Color::WHITE);
	backText_->SetTextEffect(TE_STROKE);
	// Subscribe to buttonClose release (following a 'press') events
	SubscribeToEvent(backButton, E_RELEASED, HANDLER(GameState, HandleBackPressed));

	Button* uRangeButton = menuBar_->CreateChild<Button>();;
	uRangeButton->SetName("uRange");
	uRangeButton->SetFixedSize(150, 30);
	uRangeButton->SetStyleAuto();
	uRangeButton->SetAlignment(HA_RIGHT, VA_BOTTOM);
	uRangeText_ = uRangeButton->CreateChild< Text>();
	uRangeText_->SetName("upgradeRange");
	uRangeText_->SetText("Upgrade Range");
	uRangeText_->SetFont(cache->GetResource<Font>("Fonts/Anonymous Pro.ttf"), 12);
	uRangeText_->SetAlignment(HA_CENTER, VA_CENTER);
	// Subscribe to buttonClose release (following a 'press') events
	SubscribeToEvent(uRangeButton, E_RELEASED, HANDLER(GameState, HandleUpgradePressed));

	Button* uDmgButton = menuBar_->CreateChild<Button>();;
	uDmgButton->SetName("uDmg");
	uDmgButton->SetFixedSize(150, 30);
	uDmgButton->SetStyleAuto();
	uDmgButton->SetAlignment(HA_RIGHT, VA_BOTTOM);
	uDmgText_ = uDmgButton->CreateChild< Text>();
	uDmgText_->SetName("upgradeDmg");
	uDmgText_->SetText("Upgrade Dmg");
	uDmgText_->SetFont(cache->GetResource<Font>("Fonts/Anonymous Pro.ttf"), 12);
	uDmgText_->SetAlignment(HA_CENTER, VA_CENTER);
	// Subscribe to buttonClose release (following a 'press') events
	SubscribeToEvent(uDmgButton, E_RELEASED, HANDLER(GameState, HandleUpgradePressed));

	Button* uFireRateButton = menuBar_->CreateChild<Button>();;
	uFireRateButton->SetName("uFireRate");
	uFireRateButton->SetFixedSize(150, 30);
	uFireRateButton->SetStyleAuto();
	uFireRateButton->SetAlignment(HA_RIGHT, VA_BOTTOM);
	uFireRateText_ = uFireRateButton->CreateChild< Text>();
	uFireRateText_->SetName("upgradeFireRate");
	uFireRateText_->SetText("Upgrade FireRate");
	uFireRateText_->SetFont(cache->GetResource<Font>("Fonts/Anonymous Pro.ttf"), 12);
	uFireRateText_->SetAlignment(HA_CENTER, VA_CENTER);
	// Subscribe to buttonClose release (following a 'press') events
	SubscribeToEvent(uFireRateButton, E_RELEASED, HANDLER(GameState, HandleUpgradePressed));

	Button* sellButton = menuBar_->CreateChild<Button>();;
	sellButton->SetName("Sell");
	sellButton->SetFixedSize(100, 30);
	sellButton->SetStyleAuto();
	sellButton->SetAlignment(HA_RIGHT, VA_BOTTOM);
	sellText_ = sellButton->CreateChild< Text>();
	sellText_->SetName("Sell");
	sellText_->SetText("Sell");
	sellText_->SetFont(cache->GetResource<Font>("Fonts/Anonymous Pro.ttf"), 12);
	sellText_->SetAlignment(HA_CENTER, VA_CENTER);
	sellText_->SetColor(Color::RED);
	sellText_->SetEffectColor(Color::WHITE);
	sellText_->SetTextEffect(TE_STROKE);
	// Subscribe to buttonClose release (following a 'press') events
	SubscribeToEvent(sellButton, E_RELEASED, HANDLER(GameState, HandleSellPressed));

	
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



void GameState::SubscribeToEvents()
{
	// Subscribe HandleUpdate() function for processing update events
	SubscribeToEvent(E_UPDATE, HANDLER(GameState, HandleUpdate));

	SubscribeToEvent(E_ENEMYDIED, HANDLER(GameState, HandleEnemyDied));

	SubscribeToEvent(E_MOUSEBUTTONUP, HANDLER(GameState, HandleMouseButtonUpPressed));
}

void GameState::HandleUpdate(StringHash eventType, VariantMap& eventData)
{
	using namespace Update;

	// Take the frame time step, which is stored as a float
	float timeStep = eventData[P_TIMESTEP].GetFloat();

	if (gameOver_)
	{
		return;
	}

	Input* input = GetSubsystem<Input>();
	if (input->GetKeyPress(KEY_ESC))
		stateManager_->PopStack();

	// Control wave spawning, enemy spawning,
	if (enemiesAlive_ == 0)
	{
		waveTimer_ -= timeStep;

		if (waveTimer_ <= 0.0f)
		{
			SpawnWave();
		}
		else
		{
			String str;
			char name[6];
			std::sprintf(name, "%2.2f", waveTimer_);
			str.AppendWithFormat("Wave %i  Next Wave in %s ", wave_, name);
			waveInfo_->SetText(str);
		}
	}
	else if (enemiesToSpawn_ > 0)
	{
		enemyTimer_ -= timeStep;
		if (enemyTimer_ <= 0.0f)
		{
			SpawnEnemy();
		}
	}

	if (buildingMode_)
	{
		Vector3 hitPos;
		if (RaycastWithPlane(hitPos))
		{
			int x = 0, y = 0;
			bool b = tileMap_->GetInfo().PositionToTileIndex(x, y, Vector2(hitPos.x_, hitPos.y_));

			if (b)
			{

				
				tempTowerNode_->SetPosition2D(tileMap_->GetInfo().TileIndexToPosition(x, y));

			}
		}
	}

}

void GameState::End()
{
	scene_.Reset();
	cameraNode_.Reset();
	tileMap_.Reset();
	path_.Clear();
	enemies_.Clear();
	if (GetSubsystem<UI>())
	{
		GetSubsystem<UI>()->GetRoot()->RemoveChild(waveInfo_);
		GetSubsystem<UI>()->GetRoot()->RemoveChild(enemyInfo_);
		GetSubsystem<UI>()->GetRoot()->RemoveChild(playerInfo_);
		GetSubsystem<UI>()->GetRoot()->RemoveChild(menuBar_);
		GetSubsystem<UI>()->GetRoot()->RemoveChild(buyTowerButton_);
	}
	UnsubscribeFromAllEvents();
	GetSubsystem<Input>()->SetMouseVisible(false);
	State::End();
}

void GameState::HandleEnemyDied(StringHash eventType, VariantMap& eventData)
{
	using namespace EnemyDied;

	// play sound

	// spawn particles

	if (eventData[P_GAINMONEY].GetBool())
	{
		// update player money
		money_ += (wave_ < 5) ? 2 : 1;;
	}
	else
	{
		// update player life
		lifes_--;
		if (lifes_ <= 0 && !gameOver_)
		{
			GameOver();
		}
	}
	Node* n = (Node*)eventData[P_NODE].GetPtr();
	enemies_.Remove(WeakPtr<Node>(n));

	enemiesAlive_--;

	String str;
	str.AppendWithFormat("Enemies alive %i ", enemiesAlive_);
	enemyInfo_->SetText(str);

	if (enemiesAlive_ == 0)
		enemyInfo_->SetVisible(false);

	str.Clear();
	str.AppendWithFormat("Lifes %i Money %i", lifes_, money_);
	playerInfo_->SetText(str);
}

void GameState::SpawnWave()
{
	wave_++;
	waveTimer_ = WAVE_SPAWN_INTERVAL;

	enemiesAlive_ = 5 + wave_;
	enemiesToSpawn_ = enemiesAlive_;
	enemyTimer_ = 0.0f;

	String str;
	str.AppendWithFormat("Wave %i ", wave_);
	waveInfo_->SetText(str);

	str.Clear();
	str.AppendWithFormat("Enemies alive %i ", enemiesAlive_);
	enemyInfo_->SetText(str);
	enemyInfo_->SetVisible(true);
}

void GameState::SpawnEnemy()
{
	enemiesToSpawn_--;
	enemyTimer_ = ENEMY_SPAWN_INTERVAL;

	ResourceCache* cache = GetSubsystem<ResourceCache>();
	// create enemy
	SpriteSheet2D* 	spriteSheet = cache->GetResource<SpriteSheet2D>("Tilemaps/TileMapSprites.xml");
	if (!spriteSheet)
		return;
	SharedPtr<Node> enemySpriteNode_(scene_->CreateChild("Enemy"));
	StaticSprite2D* staticSprite = enemySpriteNode_->CreateComponent<StaticSprite2D>();
	spriteSheet->GetSprite("Enemy")->SetHotSpot(Vector2(0.0f, 0.0f));
	staticSprite->SetSprite(spriteSheet->GetSprite("Enemy"));
	staticSprite->SetLayer(5 * 10);
	/// create Enemy component which controls the Enemy behavior
	Enemy* e = enemySpriteNode_->CreateComponent<Enemy>();
	e->FollowPath(&path_);
	e->SetSpeed(ENEMY_SPEED + wave_*0.3f);
	e->SetMaxHealth((wave_ / 3) + 1.0f);
	e->SetHealth((wave_ / 3) + 1.0f);

	enemies_.Push(WeakPtr<Node>(enemySpriteNode_));
}

void GameState::GameOver()
{
	gameOver_ = true;
	SharedPtr<Urho3D::MessageBox> messageBox(new Urho3D::MessageBox(context_, "Restart Game ?", "Game Over ! :-/ "));

	if (messageBox->GetWindow() != NULL)
	{
		Button* cancelButton = (Button*)messageBox->GetWindow()->GetChild("CancelButton", true);
		cancelButton->SetVisible(true);
		cancelButton->SetFocus(true);
		SubscribeToEvent(messageBox, E_MESSAGEACK, HANDLER(GameState, HandleQuitMessageAck));
	}
	if (GetSubsystem<Input>())
		GetSubsystem<Input>()->SetMouseVisible(true);
	messageBox->AddRef();
}

void GameState::ResetGame()
{
	wave_ = 0;
	enemiesAlive_ = 0;
	enemiesToSpawn_ = 0;
	waveTimer_ = 5.0f;
	enemyTimer_ = 0.0f;
	money_ = 40;
	lifes_ = PLAYER_LIFE;
	gameOver_ = false;
	towerPrice_ = 8;
	buildingMode_ = false;
	upgradeMode_ = false;

	enemies_.Clear();
	towers_.Clear();
	String str;
	str.AppendWithFormat("Lifes %i Money %i", lifes_, money_);
	playerInfo_->SetText(str);
}

void GameState::HandleQuitMessageAck(StringHash eventType, VariantMap& eventData)
{
	using namespace MessageACK;

	bool ok_ = eventData[P_OK].GetBool();

	if (GetSubsystem<Input>())
		GetSubsystem<Input>()->SetMouseVisible(false);

	if (ok_)
		stateManager_->SetActiveState("GameState");
	else
		stateManager_->PopStack();
}

void GameState::SetBuyMenu(bool visible)
{
	buyTowerButton_->SetVisible(visible);
}

void GameState::SetUpgradeMenu(bool visible)
{
	menuBar_->SetVisible(visible);
}

void GameState::HandleBuyPressed(StringHash eventType, VariantMap& eventData)
{
	if (towerPrice_ > money_) {
		return;
	}
	buildingMode_ = true;
	
	canelButton_->SetVisible(true);
	tempTowerNode_->SetEnabled(true);
	SetBuyMenu(false);
}

void GameState::HandleBackPressed(StringHash eventType, VariantMap& eventData)
{
	HandleCancelPressed(eventType, eventData);
}

void GameState::HandleSellPressed(StringHash eventType, VariantMap& eventData)
{
	canelButton_->SetVisible(true);
	SetUpgradeMenu(false);
	SetBuyMenu(false);
}

void GameState::HandleUpgradePressed(StringHash eventType, VariantMap& eventData)
{
	if (selectedTower_.Expired())
		return;
	using namespace Released;
	Button* clicked = static_cast<Button*>(eventData[P_ELEMENT].GetPtr());
	if (clicked)
	{
		if (clicked->GetName() == "uRange")
		{
			int prize = selectedTower_->GetPrize("Range");
			if (money_ >= prize) {
				money_ -= prize;
				selectedTower_->UpgradeRange();
				UpdateUpgradeLabels();
			}
			
		}
		else if (clicked->GetName() == "uDmg")
		{
			int prize = selectedTower_->GetPrize("Damage");
			if (money_ >= prize) {
				money_ -= prize;
				selectedTower_->UpgradeDamage();
				UpdateUpgradeLabels();
			}
			
		}
		else if (clicked->GetName() == "uFireRate")
		{
			int prize = selectedTower_->GetPrize("FireRate");
			if (money_ >= prize) {
				money_ -= prize;
				selectedTower_->UpgradeFirerate();
				UpdateUpgradeLabels();
			}
			
		}
	}
}

void GameState::HandleCancelPressed(StringHash eventType, VariantMap& eventData)
{
	if (buildingMode_)
	{
		buildingMode_ = false;
		canelButton_->SetVisible(false);
		tempTowerNode_->SetEnabled(false);
		SetUpgradeMenu(false);
		SetBuyMenu(true);
	}
	if (upgradeMode_)
	{
		upgradeMode_ = false;
		canelButton_->SetVisible(false);
		SetUpgradeMenu(false);
		SetBuyMenu(true);

		if (!selectedTower_.Expired())
		{
			StaticSprite2D* staticSprite = selectedTower_->GetComponent<StaticSprite2D>();
			staticSprite->SetColor(Color::WHITE);
		}
	
	}


}

void GameState::PlaceTower()
{
	Vector3 hitPos;
	if (RaycastWithPlane(hitPos))
	{
		int x=0, y=0;
		bool b = tileMap_->GetInfo().PositionToTileIndex(x, y, Vector2(hitPos.x_, hitPos.y_));

		if (b)
		{
			
			WeakPtr<Node> temp = towers_[MakePair(x, y)];
			if (!temp.Expired())
				return;

			SharedPtr<Node> towerNode_(scene_->CreateChild("Tower"));
			towerNode_->SetPosition2D(tileMap_->GetInfo().TileIndexToPosition(x,y));
			SharedPtr<Tower> t(towerNode_->CreateComponent<Tower>());
			t->SetEnemies(&enemies_);

			ResourceCache* cache = GetSubsystem<ResourceCache>();
			// create enemy
			SpriteSheet2D* 	spriteSheet = cache->GetResource<SpriteSheet2D>("Tilemaps/TileMapSprites.xml");
			if (!spriteSheet)
				return;

			StaticSprite2D* staticSprite = towerNode_->CreateComponent<StaticSprite2D>();
			spriteSheet->GetSprite("Tower")->SetHotSpot(Vector2(0.0f, 0.0f));
			staticSprite->SetSprite(spriteSheet->GetSprite("Tower"));
			staticSprite->SetLayer(5 * 10);
			
			towers_[MakePair(x,y)]=towerNode_;
			money_ -= towerPrice_;
			towerPrice_ += int(towerPrice_ * 0.3f);
			String str;
			str.AppendWithFormat("Buy %i", towerPrice_);
			buytowerText_->SetText(str);

			str.Clear();
			str.AppendWithFormat("Lifes %i Money %i", lifes_, money_);
			playerInfo_->SetText(str);

			HandleCancelPressed(StringHash(), VariantMap());
		}
	}
}

void GameState::HandleMouseButtonUpPressed(StringHash eventType, VariantMap& eventData)
{
	using namespace MouseButtonUp;

	int button = eventData[P_BUTTON].GetInt();


		if (MOUSEB_LEFT == button)
		{
			if (buildingMode_)
			{
				if (towerPrice_ > money_) {
					return;
				}
				PlaceTower();
			}
				
			else
				ClickedOnTower();
		}
		else 
		{
			HandleCancelPressed(eventType, eventData);
		}
	
}

bool GameState::Raycast(float maxDistance, Vector3& hitPos, Drawable*& hitDrawable)
{
	hitDrawable = 0;
	UI* ui = GetSubsystem<UI>();

	Cursor* cursor = ui->GetCursor();
	IntVector2 pos;
	bool visible = false;

	// Prefer software cursor then OS-specific cursor
	if (cursor && cursor->IsVisible())
	{
		pos = cursor->GetPosition();
		visible = true;
	}
	else
	{
		Input* input = GetSubsystem<Input>();
		pos = input->GetMousePosition();
		visible = input->IsMouseVisible();
		if (!visible && cursor)
			pos = cursor->GetPosition();
	}

	// Check the cursor is visible and there is no UI element in front of the cursor
	if (!visible || ui->GetElementAt(pos, true))
		return false;
	Graphics* graphics = GetSubsystem<Graphics>();
	Camera* camera = cameraNode_->GetComponent<Camera>();
	Ray cameraRay = camera->GetScreenRay((float)pos.x_ / graphics->GetWidth(), (float)pos.y_ / graphics->GetHeight());
	// Pick only geometry objects, not eg. zones or lights, only get the first (closest) hit
	PODVector<RayQueryResult> results;
	RayOctreeQuery query(results, cameraRay, RAY_TRIANGLE, maxDistance, DRAWABLE_GEOMETRY);
	scene_->GetComponent<Octree>()->RaycastSingle(query);
	if (results.Size())
	{
		RayQueryResult& result = results[0];
		hitPos = result.position_;
		hitDrawable = result.drawable_;
		return true;
	}
	return false;
}

bool GameState::RaycastWithPlane(Vector3& hitPos)
{
	
	UI* ui = GetSubsystem<UI>();

	Cursor* cursor = ui->GetCursor();
	IntVector2 pos;
	bool visible = false;

	// Prefer software cursor then OS-specific cursor
	if (cursor && cursor->IsVisible())
	{
		pos = cursor->GetPosition();
		visible = true;
	}
	else
	{
		Input* input = GetSubsystem<Input>();
		pos = input->GetMousePosition();
		visible = input->IsMouseVisible();
		if (!visible && cursor)
			pos = cursor->GetPosition();
	}

	// Check the cursor is visible and there is no UI element in front of the cursor
	if (!visible || ui->GetElementAt(pos, true))
		return false;


	Graphics* graphics = GetSubsystem<Graphics>();
	Camera* camera = cameraNode_->GetComponent<Camera>();
	Ray cameraRay = camera->GetScreenRay((float)pos.x_ / graphics->GetWidth(), (float)pos.y_ / graphics->GetHeight());

	float distance = cameraRay.HitDistance(plane_);
	if (distance == M_INFINITY)
	{
		return false;
	}
	hitPos = cameraRay.origin_ + distance * cameraRay.direction_;
	return true;
}

void GameState::ClickedOnTower()
{
	Vector3 hitPos;
	if (RaycastWithPlane(hitPos))
	{
		int x = 0, y = 0;
		bool b = tileMap_->GetInfo().PositionToTileIndex(x, y, Vector2(hitPos.x_, hitPos.y_));

		if (b)
		{

			WeakPtr<Node> temp = towers_[MakePair(x, y)];
			if (temp.Expired())
			{
				HandleCancelPressed(StringHash(), VariantMap());
				return;
			}
				
			selectedTower_ = temp->GetComponent<Tower>();
			upgradeMode_ = true;
			canelButton_->SetVisible(false);

			StaticSprite2D* staticSprite = selectedTower_->GetComponent<StaticSprite2D>();	
			staticSprite->SetColor(Color::YELLOW);

			UpdateUpgradeLabels();
			SetUpgradeMenu(true);
			SetBuyMenu(false);
		}
	}
}

void GameState::UpdateUpgradeLabels()
{
	String str;
	str.AppendWithFormat("Lifes %i Money %i", lifes_, money_);
	playerInfo_->SetText(str);

	str.Clear();
	str.AppendWithFormat("Range %i", selectedTower_->GetPrize("Range"));
	uRangeText_->SetText(str);

	str.Clear();
	str.AppendWithFormat("Dmg %i", selectedTower_->GetPrize("Damage"));
	uDmgText_->SetText(str);

	str.Clear();
	str.AppendWithFormat("FireRate %i", selectedTower_->GetPrize("FireRate"));
	uFireRateText_->SetText(str);
}
