#include "Game.h"
#include <sstream>
#include <algorithm>
#include "Mixer.h"


Game::Game() 
	: SDLApp(SCREEN_WIDTH
	, SCREEN_HEIGHT
	, "Nasir's Beat 'em Up Game")
	, MoveBounds(0.0f, 370.0f, (float)SCREEN_WIDTH, 120.0f)
	, bg(NULL)
	, tbFps(NULL), tbPlayerPos(NULL)
	, skaterboy(NULL)
	, player(NULL)
	, leftDown(false)
	, rightDown(false)
	, upDown(false)
	, downDown(false)
{
}



Game::~Game()
{
	logPrintf("Game object released");
}



bool Game::Init()
{
	if(!SDLApp::Init())
		return false;

	bg = new Background(clientWidth_, clientHeight_, renderer_);
	
	skaterboy = new Roamer(renderer_, 
		Sprite::FromFile("resources/skater_left.png", renderer_, 71, 90, 11, 0),
		Sprite::FromFile("resources/skater_right.png", renderer_, 71, 90, 11, 0), 
		-200, 400, -200, 1000);

	Roamer* knight1 = new Roamer(renderer_, 
		Sprite::FromFile("resources/knightwalk_left.png", renderer_, 128, 128, 4, 15),
		Sprite::FromFile("resources/knightwalk_right.png", renderer_, 128, 128, 4, 3), 
		5000, 450, -5000, 5000);


	player = new Player(renderer_);
	rock = new Rock("resources/rock.png", renderer_);

	tbFps = new TextBlock("FPS: 00.000000", 16, 0.0f, 0.0f, renderer_);	
	tbPlayerPos = new TextBlock("Pos {}", 16, 0.0f, tbFps->Position().bottom() + 1, renderer_);

	gameObjects.push_back(bg);
	gameObjects.push_back(tbFps);
	gameObjects.push_back(tbPlayerPos);

	gameObjects.push_back(skaterboy);
	gameObjects.push_back(player);
	gameObjects.push_back(rock);

	gameObjects.push_back(knight1);

	MIXER.Play(Mixer::ST_Track1);
	return true;
}



void Game::ProcessEvent(const SDL_Event& e)
{
	if(e.key.state == SDL_PRESSED && !e.key.repeat)
	{
		switch(e.key.keysym.sym)
		{
		case SDLK_F1:
			ToggleFullScreen();
			break;
		case SDLK_ESCAPE:
			quit_ = true;
			break;
			/* Player Keys */
		case SDLK_LEFT:
			leftDown = true;
			bg->SetScroll(Right);
			break;
		case SDLK_RIGHT:
			rightDown = true;
			bg->SetScroll(Left);
			break;
		case SDLK_UP:
			upDown = true;
			bg->SetScroll(player->GetDirection() == Right? Left : Right);
			break;
		case SDLK_DOWN:
			downDown = true;
			bg->SetScroll(player->GetDirection() == Right? Left : Right);
			break;
		case SDLK_SPACE:
			player->Jump(1, 20);
			break;
		}
	}
	else if(e.key.state == SDL_RELEASED && !e.key.repeat)
	{
		switch(e.key.keysym.sym)
		{
		case SDLK_LEFT:
			leftDown = false;
			Stop();
			break;
		case SDLK_RIGHT:
			rightDown = false;
			Stop();
			break;
		case SDLK_UP:
			upDown = false;
			Stop();
			break;
		case SDLK_DOWN:
			downDown = false;
			Stop();
			break;
		}
	}
}



void Game::Update()
{
	//Update movement vectors
	if (upDown) player->GoUp();
	if (downDown) player->GoDown();
	if (rightDown) player->GoRight();
	if (leftDown) player->GoLeft();
	bg->SetScroll(upDown || downDown || rightDown || leftDown);

	//text
	std::stringstream ss;
	ss << "FPS: " << Fps();
	tbFps->SetText(ss.str());
	ss.str("");
	ss.clear();
	ss << "Pos: {" << (int)player->Position().x 
		<< "," << (int)player->Position().y 
		<< "," << (int)player->Position().z 
		<< "}  Health: " << player->GetHealth();
	tbPlayerPos->SetText(ss.str());

	//Other game logic
	gameObjects.Update();
}



void Game::Render()
{
	SDL_RenderClear( renderer_ );
	//Sort by depth, then draw
	std::sort(gameObjects.begin(), gameObjects.end(), GameObjectSortByDepth());
	gameObjects.Draw( renderer_ );
	SDL_RenderPresent( renderer_ );
}



void Game::Stop()
{
	player->Stop();
	bg->Stop();
}

