#include "Player.h"
#include "Util.h"
#include "Game.h"
#include "Enemy.h"
#include "Mixer.h"

const float Player::Gravity(2.0f);
const int Player::JumpHeight(50);



Player::Player(SDL_Renderer* const renderer)
	: GameObject(GT_Player, 10, Right)	
	, stanceRight(Sprite::FromFile("resources/baddude_stanceright.png", renderer, 67, 108, 10, 0))
	, stanceLeft(Sprite::FromFile("resources/baddude_stanceleft.png", renderer, 67, 108, 10, 0))
	, walkRight(Sprite::FromFile("resources/baddude_walkright.png", renderer, 60, 116, 5, 7))
	, walkLeft(Sprite::FromFile("resources/baddude_walkleft.png", renderer, 60, 116, 5, 7))
	, punchRight(Sprite::FromFile("resources/baddude_punchright.png", renderer, 94, 130, 5, 0, 0xFF, 0xFF, 0xFF))
	, punchLeft(Sprite::FromFile("resources/baddude_punchleft.png", renderer, 94, 130, 5, 0, 0xFF, 0xFF, 0xFF))
	, current(NULL)
	, jumpState(JS_Ground)
	, pState(PS_Stance)
	, punchTimeout(0)
{
	position.x  = 100.0f , position.w = 76.0f, position.h = 120.0f;
	position.y = (float)GAME.MidSectionY((int)position.h);
	position.z = position.y - GAME.MoveBounds.top();
	//test gladiator walker//////////////////////////////////////////////////////////////////////////////////
	//walkRight = Sprite::FromFile("resources/walkright.png", renderer, 76, 120, 5, 1, 0xFF, 0x40, 0x40);
	//walkLeft = Sprite::FromFile("resources/walkleft.png", renderer, 76, 120, 5, 1, 0xFF, 0x40, 0x40);
	/////////////////////////////////////////////////////////////////////////////////////////////////////////
	punchRight->AddSoundEffect(1, Mixer::SE_Punch);
	punchRight->AddSoundEffect(4, Mixer::SE_Punch);
	punchRight->AddSoundEffect(8, Mixer::SE_Punch);
	punchLeft->AddSoundEffect(1, Mixer::SE_Punch);
	punchLeft->AddSoundEffect(4, Mixer::SE_Punch);
	punchLeft->AddSoundEffect(8, Mixer::SE_Punch);	
	SetDirection(Right);
	Stop();
}



void Player::Update()
{
	//punching
	if(pState == PS_Punching)
	{
		if(SDL_GetTicks() > punchTimeout) {
			Stop(); //sets pState to PS_Stance
			punchTimeout = 0;
		}
		else if(current->GetCurrentFrame() == 1
			|| current->GetCurrentFrame() == 4
			|| current->GetCurrentFrame() == 8)
		{
			if(CollidedWith(GAME.andore))
			{
				GAME.andore->OnPlayerAttack();
			}
		}
	}

	//Jump rotation...
	if(jumpState == JS_Jumped || jumpState == JS_Landing) {
		SetAngle(GetAngle() + (GetDirection()==Right? 13: -13));
	}
	else {
		SetAngle(0);
		if(pState == PS_Jumping) {
			Stop(); //jump complete set pState to PS_Stance
		}
	}

	//Jump start..
	//Shoot up (yVel acceleration)...
	if(jumpState == JS_Jumped)
	{
		yVel += Gravity/(float)JumpHeight;
		if(position.y > jumpLocation.y - JumpHeight) 
			Translate(false);
		else 
			jumpState = JS_Landing;
	}

	//Landing (in the air)..
	else if(jumpState == JS_Landing)
	{
		//Not landed yet..
		if(position.y < jumpLocation.y)
		{
			yVel += Gravity;
			xVel += GetDirection() == Right? 0.15f: -0.15f;
			Translate(false);
		}
		//On the ground now...
		else 
		{
			jumpState = JS_Ground;
			xVel = 0, yVel = 0;
			position.y = jumpLocation.y;
			Translate(false);
		}
	}

	//Propagate to the underlying currently active sprite
	current->Position().x = position.x;
	current->Position().y = position.y;
	current->Update();

	//Collision detection
	if(CollidedWith(GAME.rock))
	{
		//got hit by rock!
		if(GAME.rock->GetDirection() == Right) 
			SetDirection(Left);
		else 
			SetDirection(Right);

		Jump(0, 30);
		SetHealth(GetHealth() - 1);
		MIXER.Play(Mixer::SE_Grunt);
	}
}


void Player::Draw(SDL_Renderer* const renderer) const
{
	current->Draw(renderer);
}


Player::~Player()
{
	util::Delete(walkRight);
	util::Delete(walkLeft);
	util::Delete(stanceRight);
	util::Delete(stanceLeft);
	util::Delete(punchRight);
	util::Delete(punchLeft);
	logPrintf("Player object released");
}


void Player::SetDirection(Directions dir)
{
	GameObject::SetDirection(dir);
	if (GetDirection() == Right) current = walkRight;
	else if(GetDirection() == Left) current = walkLeft;
}


void Player::SetAngle(double theta)
{
	GameObject::SetAngle(theta);
	current->SetAngle(theta);
}


void Player::Jump(float xForce, float yForce)
{
	//Can only jump whilst on the ground
	if(jumpState != JS_Ground) return;

	jumpLocation.x = position.x;
	jumpLocation.y = position.y;
	current = GetDirection() == Right? stanceRight: stanceLeft;
	xVel = GetDirection() == Right? xForce: -xForce;
	yVel = -yForce;
	jumpState = JS_Jumped;
	pState = PS_Jumping;
}


void Player::Punch()
{
	if(pState == PS_Punching)
	{
		punchTimeout += 250;
	}
	else
	{
		current = GetDirection()==Right? punchRight: punchLeft;
		current->SetAnimation(true);
		current->SetCurrentFrame(0);
		punchTimeout = SDL_GetTicks() + 250;
		pState = PS_Punching;
	}
}


void Player::Stop()
{
	position.x -= xVel;
	position.y -= yVel;
	xVel = yVel = 0;
	current = GetDirection() == Right? stanceRight: stanceLeft;
	current->SetAnimation(true);
	pState = PS_Stance;
}


void Player::GoUp()
{
	if(jumpState != JS_Ground || pState == PS_Punching) return;

	current = GetDirection() == Right? walkRight: walkLeft;

	if (position.y >= GAME.MoveBounds.y)
		yVel = -(speed/2.0f);
	else 
		yVel = 0;
	
	Translate();
	pState = PS_Walking;
}


void Player::GoDown()
{
	if(jumpState != JS_Ground || pState == PS_Punching) return;
	
	current = GetDirection() == Right? walkRight: walkLeft;
	
	if (position.y <= GAME.MoveBounds.bottom()) 
		yVel = (speed/2.0f);
	else 
		yVel = 0;
	
	Translate();        
	pState = PS_Walking;
}


void Player::GoRight()
{
	if(jumpState != JS_Ground || pState == PS_Punching) return;

	current = GetDirection() == Right? walkRight: walkLeft;

	if (position.x <= GAME.MoveBounds.right() - position.w) 
		xVel = speed;
	else 
		xVel = 0;

	SetDirection(Right);
	Translate();
	pState = PS_Walking;
}


void Player::GoLeft()
{
	if(jumpState != JS_Ground || pState == PS_Punching) return;
	
	current = GetDirection() == Right? walkRight: walkLeft;

	if (position.x >= GAME.MoveBounds.x) 
		xVel = -speed;
	else 
		xVel = 0;

	SetDirection(Left);
	Translate();
	pState = PS_Walking;
}


void Player::Translate(bool anim)
{
	current->SetAnimation(anim);
	position.x += xVel;
	position.y += yVel;

	//Jumping doesn't change z order
	if(jumpState == JS_Ground)
	{
		 AdjustZToGameDepth();
	}
	//logPrintf("Translate: Pos {%d, %d}", position.x, position.y); 
}
