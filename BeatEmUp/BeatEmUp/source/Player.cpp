#include "Player.h"
#include "Util.h"
#include "Game.h"
#include "Enemy.h"
#include "Mixer.h"

const float Player::Gravity(2.0f);
const int Player::JumpHeight(50);



Player::Player(SDL_Renderer* const renderer)
	: GameObject(GT_Player, 10, Right)	
	, idleRight(Sprite::FromFile("resources/baddude_stanceright.png", renderer, 67, 108, 10, 0))
	, idleLeft(Sprite::FromFile("resources/baddude_stanceleft.png", renderer, 67, 108, 10, 0))
	, walkRight(Sprite::FromFile("resources/baddude_walkright.png", renderer, 60, 116, 5, 7))
	, walkLeft(Sprite::FromFile("resources/baddude_walkleft.png", renderer, 60, 116, 5, 7))
	, punchRight(Sprite::FromFile("resources/baddude_punchright.png", renderer, 94, 130, 6, 0, false, 0xFF, 0xFF, 0xFF))
	, punchLeft(Sprite::FromFile("resources/baddude_punchleft.png", renderer, 94, 130, 6, 0, false, 0xFF, 0xFF, 0xFF))
	,	kickLeft(Sprite::FromFile("resources/baddude_kickleft.png", renderer, 95, 120, 10, 0))
	,	kickRight(Sprite::FromFile("resources/baddude_kickright.png", renderer, 95, 120, 10, 0))
	,	hitLeft(Sprite::FromFile("resources/baddude_hitleft.png", renderer, 70, 108, 5, 0)) 
	,	hitRight(Sprite::FromFile("resources/baddude_hitright.png", renderer, 70, 108, 5, 0))
	,	fallLeft(Sprite::FromFile("resources/baddude_fallleft.png", renderer, 133, 121, 1, 0))
	,	fallRight(Sprite::FromFile("resources/baddude_fallright.png", renderer, 133, 121, 1, 0))
	, current(NULL)
	, jumpState(JS_Ground)
	, pState(PS_Idle)
	, punchTimeout(0)
	, kickTimeout(0)
	, hitCount(0)
	, recoveryTimer(0)
	, KnockDownHitCount(3)
{
	position.x  = 100.0f , position.w = 76.0f, position.h = 120.0f;
	position.y = (float)GAME.MidSectionY((int)position.h);
	position.z = position.y - GAME.MoveBounds.top();
	//test gladiator walker//////////////////////////////////////////////////////////////////////////////////
	//walkRight = Sprite::FromFile("resources/walkright.png", renderer, 76, 120, 5, 1, 0xFF, 0x40, 0x40);
	//walkLeft = Sprite::FromFile("resources/walkleft.png", renderer, 76, 120, 5, 1, 0xFF, 0x40, 0x40);
	/////////////////////////////////////////////////////////////////////////////////////////////////////////
	punchRight->FramePlayed.attach(this, &Player::OnPunchSprite);
	punchLeft->FramePlayed.attach(this, &Player::OnPunchSprite);
	kickRight->FramePlayed.attach(this, &Player::OnKickSprite);
	kickLeft->FramePlayed.attach(this, &Player::OnKickSprite);
	SetDirection(Right);
	Stop();
}


Player::~Player()
{
	punchLeft->FramePlayed.detach(this, &Player::OnPunchSprite);
	punchRight->FramePlayed.detach(this, &Player::OnPunchSprite);
	kickRight->FramePlayed.detach(this, &Player::OnKickSprite);
	kickLeft->FramePlayed.detach(this, &Player::OnKickSprite);

	current = NULL;
	util::Delete(walkRight);
	util::Delete(walkLeft);
	util::Delete(idleRight);
	util::Delete(idleLeft);
	util::Delete(punchRight);
	util::Delete(punchLeft);
	util::Delete(hitLeft);
	util::Delete(hitRight);
	util::Delete(fallLeft);
	util::Delete(fallRight);
	util::Delete(kickLeft);
	util::Delete(kickRight);
	logPrintf("Player object released");
}


void Player::OnPunchSprite(const Sprite* const sender, const Sprite::FramePlayedEventArgs* const e)
{
	if(e->FrameIndex == 1 || e->FrameIndex == 4 || e->FrameIndex == 8)
	{
		if(GAME.andore->IsAttackable() && CollidedWith(GAME.andore) 
			&& GetDirection() != GAME.andore->GetDirection())
		{
			MIXER.Play(Mixer::SE_PunchHit);
			GAME.andore->OnHit();
		}
		if(GAME.andore2->IsAttackable() && CollidedWith(GAME.andore2) 
			&& GetDirection() != GAME.andore2->GetDirection())
		{
			MIXER.Play(Mixer::SE_PunchHit);
			GAME.andore2->OnHit();
		}
		if(GAME.joker->IsAttackable() && CollidedWith(GAME.joker) 
			&& GetDirection() != GAME.joker->GetDirection())
		{
			MIXER.Play(Mixer::SE_PunchHit);
			GAME.joker->OnHit();
		}
		else
		{
			MIXER.Play(Mixer::SE_Punch);
		}
	}
}


void Player::OnKickSprite(const Sprite* const sender, const Sprite::FramePlayedEventArgs* const e)
{
	if(e->FrameIndex == 1)
	{
		if(GAME.andore->IsAttackable() && CollidedWith(GAME.andore, 15) 
			&& GetDirection() != GAME.andore->GetDirection())
		{
			MIXER.Play(Mixer::SE_Kick);
			GAME.andore->OnHit();
		}
		if(GAME.andore2->IsAttackable() && CollidedWith(GAME.andore2, 15) 
			&& GetDirection() != GAME.andore2->GetDirection())
		{
			MIXER.Play(Mixer::SE_Kick);
			GAME.andore2->OnHit();
		}
		if(GAME.joker->IsAttackable() && CollidedWith(GAME.joker, 15) 
			&& GetDirection() != GAME.joker->GetDirection())
		{
			MIXER.Play(Mixer::SE_Kick);
			GAME.joker->OnHit();
		}
		else
		{
			MIXER.Play(Mixer::SE_Punch);
		}
	}
}


void Player::KnockedDown()
{
	hitCount = 0;
	current = GetDirection() == Left? fallLeft: fallRight;
	current->SetCurrentFrame(0);
	pState = PS_KnockedDown;
	recoveryTimer = 0;
	Jump(8.0f, 10.0f);
}


void Player::OnHit(Uint8 damage)
{
	if(pState != PS_KnockedDown && pState != PS_Dead)
	{
		Stop();
		current = GetDirection() == Left? hitLeft: hitRight;
		pState = PS_Hit;
		hitCount += damage;
		SetHealth(GetHealth() - damage);

		if(GetHealth() > 0 && hitCount < KnockDownHitCount)
		{
			recoveryTimer = SDL_GetTicks() + 300;
		}
		else
		{
			KnockedDown();
		}
	}
}


void Player::OnKnockDown()
{
	//Jump start..
	//Shoot up (yVel acceleration)...
	if(jumpState == JS_Jumped)
	{
		yVel += Gravity/(float)JumpHeight;
		if(position.y <= jumpLocation.y - JumpHeight) 
			jumpState = JS_Landing;

		if( (position.right() + xVel >= GAME.MoveBounds.right() - position.w)
			|| (position.left() + xVel <= GAME.MoveBounds.x))
				xVel = 0;
	}
	//Landing (in the air)..
	else if(jumpState == JS_Landing)
	{
		//Not landed yet..
		if(position.y < jumpLocation.y)
		{
			yVel += Gravity;
			if( (position.right() + (xVel + 0.15f) >= GAME.MoveBounds.right() - position.w)
				|| (position.left() + (xVel - 0.15f)  <= GAME.MoveBounds.x))
					xVel = 0;
			else
				xVel += GetDirection() == Right? 0.15f: -0.15f;
		}
		//Landed. On the ground now...
		else 
		{
			jumpState = JS_Ground;
			SetAngle(0.0);
			xVel = 0, yVel = 0;
			position.y = jumpLocation.y;
			current->SetCurrentFrame(1);
			MIXER.Play(Mixer::SE_Thud);
		}
	}
	//On-the-ground logic...
	else if(jumpState == JS_Ground)
	{
		//Getting up...
		if(GetHealth() > 0)
		{
			if(current->GetCurrentFrame() == 1)
			{
				if(recoveryTimer == 0) {
					recoveryTimer = SDL_GetTicks() + 2000;
				} else if (SDL_GetTicks() > recoveryTimer) {
					current->SetCurrentFrame(2);
					recoveryTimer = SDL_GetTicks() + 500;
				}
			}
			//Half up...
			else if(current->GetCurrentFrame() == 2)
			{
				//full up.. go to idle..
				if(SDL_GetTicks() > recoveryTimer) {
					Stop();
					recoveryTimer = 0;
				}
			}
		}
		//Player is dead..
		else
		{
			pState = PS_Dead;
			MIXER.Play(Mixer::SE_Grunt);
		}
	}

	Translate(false);
	current->Position().x = position.x;
	current->Position().y = position.y;
	current->Update();
}


void Player::HandleJump()
{
	//Jump rotation...
	if(jumpState == JS_Jumped || jumpState == JS_Landing) {
		SetAngle(GetAngle() + (GetDirection()==Right? 13: -13));
	}
	else {
		SetAngle(0);
		if(pState == PS_Jumping) {
			Stop(); //jump complete set pState to PS_Idle
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
}


void Player::Update()
{
	//Dead...
	if(pState == PS_Dead) return;

	//Knocked down.. get up or die...
	if(pState == PS_KnockedDown)
	{
		OnKnockDown();
		return;
	}

	//Recovery (when hit)
	if(pState == PS_Hit && SDL_GetTicks() > recoveryTimer)
	{
		Stop();
		recoveryTimer = 0;
		hitCount = 0;
	}

	//punching
	if(pState == PS_Punching)
	{
		if(SDL_GetTicks() > punchTimeout) {
			Stop(); //sets pState to PS_Idle
			punchTimeout = 0;
		}
	}

	//kicking
	if(pState == PS_Kicking)
	{
		if(SDL_GetTicks() > kickTimeout) {
			Stop(); //sets pState to PS_Idle
			kickTimeout = 0;
		}
	}

	//jumping
	HandleJump();

	//Propagate to the underlying currently active sprite
	current->Position().x = position.x;
	current->Position().y = position.y;
	current->Update();
}


void Player::Draw(SDL_Renderer* const renderer) const
{
	current->Draw(renderer);
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


void Player::Jump()
{
	//Can only jump whilst on the ground
	if(jumpState != JS_Ground) return;
	
	current = GetDirection() == Right? idleRight: idleLeft;
	pState = PS_Jumping;
	Jump(1, 20);
}


void Player::Jump(float xAccel, float yAccel)
{
	jumpLocation.x = position.x;
	jumpLocation.y = position.y;
	xVel = GetDirection() == Right? -xAccel: xAccel;
	yVel = -yAccel;
	jumpState = JS_Jumped;
}


void Player::Punch()
{
	if(CantAttack()) return;

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


void Player::Kick()
{
	if(CantAttack()) return;

	current = GetDirection()==Right? kickRight: kickLeft;
	current->SetAnimation(true);
	kickTimeout = SDL_GetTicks() + 250;
	pState = PS_Kicking;
}


void Player::Stop()
{
	if(pState == PS_Dead){
		logPrintf("Player dead");
		return;
	}

	position.x -= xVel;
	position.y -= yVel;
	xVel = yVel = 0;
	current = GetDirection() == Right? idleRight: idleLeft;
	current->SetAnimation(true);
	pState = PS_Idle;
}


bool Player::CantAttack() const
{
	return ( 
		pState == PS_Dead ||
		pState == PS_KnockedDown
	);
}


bool Player::CantMove() const
{
	return (jumpState != JS_Ground || 
		pState == PS_Punching ||
		pState == PS_Kicking ||
		pState == PS_Hit ||
		pState == PS_Jumping ||
		pState == PS_Dead ||
		pState == PS_KnockedDown
	);
}


void Player::GoUp()
{
	if(CantMove()) return;

	current = GetDirection() == Right? walkRight: walkLeft;

	if (position.y >= GAME.MoveBounds.y)
		yVel = -(speed/2.0f);
	else 
		yVel = 0;
	
	Translate(true);
	pState = PS_Walking;
}


void Player::GoDown()
{
	if(CantMove()) return;
	
	current = GetDirection() == Right? walkRight: walkLeft;
	
	if (position.y <= GAME.MoveBounds.bottom()) 
		yVel = (speed/2.0f);
	else 
		yVel = 0;
	
	Translate(true);        
	pState = PS_Walking;
}


void Player::GoRight()
{
	if(CantMove()) return;

	current = GetDirection() == Right? walkRight: walkLeft;

	if (position.x <= GAME.MoveBounds.right() - position.w) 
		xVel = speed;
	else 
		xVel = 0;

	SetDirection(Right);
	Translate(true);
	pState = PS_Walking;
}


void Player::GoLeft()
{
	if(CantMove()) return;
	
	current = GetDirection() == Right? walkRight: walkLeft;

	if (position.x >= GAME.MoveBounds.x) 
		xVel = -speed;
	else 
		xVel = 0;

	SetDirection(Left);
	Translate(true);
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
