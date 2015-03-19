#include "Enemy.h"
#include "Game.h"
#include "Player.h"


const float Enemy::Gravity(2.0f);
const int Enemy::JumpHeight(50);


Enemy::Enemy(SDL_Renderer* const renderer
	, Sprite* idleLeftSprite, Sprite* idleRightSprite
	, Sprite* walkLeftSprite, Sprite* walkRightSprite
	, Sprite* punchLeftSprite, Sprite* punchRightSprite
	, Sprite* hitLeftSprite, Sprite* hitRightSprite
	, Sprite* fallLeftSprite, Sprite* fallRightSprite
	, float posX, float posY
	, const Uint32 punchTimeout
	, float speed_
	, float patrolRange_
	, float patrolVecX_
	, float vision_
	, float minDistX
	, float minDistY
	)
	: GameObject(GT_Enemy, 50, Left, speed_)
	, idleLeft(idleLeftSprite)
	, idleRight(idleRightSprite)
	, walkLeft(walkLeftSprite)
	, walkRight(walkRightSprite)
	, punchLeft(punchLeftSprite)
	, punchRight(punchRightSprite)
	, hitLeft(hitLeftSprite)
	, hitRight(hitRightSprite)
	, fallLeft(fallLeftSprite)
	, fallRight(fallRightSprite)
	, current(walkLeft)
	, state(ES_Patrolling)
	, punchTimer(0)
	, idleTimer(0)
	, recoveryTimer(0)
	, hitCount(0)
	, jumpState(JS_Ground)
	, KnockDownHitCount(3)
	, patrolRange(patrolRange_)
	, patrolVecX(patrolVecX_)
	, vision(vision_)
	, PunchTimeOut(punchTimeout)
	, MinDistX(minDistX)
	, MinDistY(minDistY)
{
	position.x = posX, position.y = posY, position.w = (float)walkLeft->Position().w;
	position.h = (float)walkLeft->Position().h;
	AdjustZToGameDepth();

	punchLeft->FramePlayed.attach(this, &Enemy::OnPunchSprite);
	punchRight->FramePlayed.attach(this, &Enemy::OnPunchSprite);
}



void Enemy::Update()
{
	switch(state)
	{
	case ES_KnockedDown:
		OnKnockDown();
		return;

	//Recovery (when hit)
	case ES_Hit:
		if(SDL_GetTicks() > recoveryTimer)
			OnRecovery();
		break;

	case ES_Patrolling:
		OnPatrol();
		break;

	case ES_Chasing:
		OnChase();
		break;

	case ES_Attacking:
		OnPunch();
		break;

	case ES_Idle:
		OnIdle();
		break;
	}

	//Translate/animate
	Translate(xVel != 0 || yVel != 0 || state == ES_Attacking);
	
	//Propagate to the underlying currently active sprite
	//TODO: test code. Remove!!!
	if(state == ES_Attacking) {
		current->Position().x = position.x - (current->Position().w - idleLeft->Position().w);
		current->Position().y = position.y - (current->Position().h - idleLeft->Position().h);
	}
	else
	{
		current->Position().x = position.x;
		current->Position().y = position.y;
	}
	current->Update();
}


void Enemy::Draw(SDL_Renderer* const renderer) const
{
	current->Draw(renderer);
}


Enemy::~Enemy()
{
	punchLeft->FramePlayed.detach(this, &Enemy::OnPunchSprite);
	punchRight->FramePlayed.detach(this, &Enemy::OnPunchSprite);
	current = NULL;
	util::Delete(walkLeft);
	util::Delete(walkRight);
	util::Delete(punchLeft);
	util::Delete(punchRight);
	util::Delete(hitLeft);
	util::Delete(hitRight);
	util::Delete(fallLeft);
	util::Delete(fallRight);
	logPrintf("Enemy object released");
}


void Enemy::Walk(Directions dir)
{
	GameObject::SetDirection(dir);
	if (GetDirection() == Right) current = walkRight;
	else if(GetDirection() == Left) current = walkLeft;
}


void Enemy::Stop()
{
	xVel = yVel = 0;
	current = GetDirection() == Left? idleLeft: idleRight;
}


void Enemy::Translate()
{
	//Handle BG scrolling
	if(GAME.bg->IsScrolling())
	{
		position.x += GAME.bg->GetDirection() == Left?
			-GAME.bg->GetSpeed(): GAME.bg->GetSpeed();
	}

	//Translate
	position.x += xVel;
	position.y += yVel;

	//Z rules dont apply to jumping
	if(jumpState == JS_Ground)
	{
		position.y = position.y < GAME.MoveBounds.top()? GAME.MoveBounds.top(): position.y;
		AdjustZToGameDepth();
	}

	//logPrintf("Enemy Translate: Pos {%d, %d}", (int)position.x, (int)position.y);
}


void Enemy::Translate(bool anim)
{
	current->SetAnimation(anim);
	Translate();
}


void Enemy::OnPunchSprite(const Sprite* const sender, const Sprite::FramePlayedEventArgs* const e)
{
	if(e->FrameIndex == 1)
	{
		if(CollidedWith(GAME.player, 30))
		{
			MIXER.Play(Mixer::SE_PunchHit);
			GAME.player->OnEnemyAttack();
		}
		else
		{
			MIXER.Play(Mixer::SE_Punch);
		}
	}
}


void Enemy::OnPlayerAttack()
{
	if(state != ES_Attacking && state != ES_KnockedDown)
	{
		Stop();
		current = GetDirection() == Left? hitLeft: hitRight;
		state = ES_Hit;
		hitCount++;
		SetHealth(GetHealth() - 1);
	
		if(GetHealth() > 0 && hitCount < KnockDownHitCount){
			recoveryTimer = SDL_GetTicks() + 400;
		}
		else
		{
			hitCount = 0;
			current = GetDirection() == Left? fallLeft: fallRight;
			current->SetCurrentFrame(0);
			state = ES_KnockedDown;
			recoveryTimer = 0;
			Jump(8.0f, 10.0f);
		}
	}
}


void Enemy::Jump(float xAccel, float yAccel)
{
	jumpLocation.x = position.x;
	jumpLocation.y = position.y;
	xVel = GetDirection() == Right? -xAccel: xAccel;
	yVel = -yAccel;
	jumpState = JS_Jumped;
}


void Enemy::OnKnockDown()
{
	//Jump start..
	//Shoot up (yVel acceleration)...
	if(jumpState == JS_Jumped)
	{
		yVel += Gravity/(float)JumpHeight;
		if(position.y <= jumpLocation.y - JumpHeight) 
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
		}
		//Landed. On the ground now...
		else 
		{
			jumpState = JS_Ground;
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
					state = ES_Idle;
					recoveryTimer = 0;
				}
			}
		}
		//Enemy is dead.. 
		//TODO: Garbage collect
		else
		{
			state = ES_Dead;
			MIXER.Play(Mixer::SE_DragonRoar);
		}
	}

	Translate();
	current->Position().x = position.x;
	current->Position().y = position.y;
	current->Update();
}


void Enemy::OnPatrol()
{
	if(!GAME.player->IsDead())
	{
		float distanceToPlayer = util::GetDistance(GAME.player->Position(), position);
		//logPrintf("vision %f dist %f", vision, distanceToPlayer);
		if(distanceToPlayer <= vision) {
			state = ES_Chasing;
		}
	}

	//logPrintf("dist %f", patrolVecX)
	if(patrolVecX >= patrolRange) Walk(Left), patrolVecX = 0;
	else if(patrolVecX < -patrolRange) Walk(Right), patrolVecX = 0;
	if(GetDirection() == Left) xVel = -speed;
	else xVel = speed;
	patrolVecX += xVel;
}


void Enemy::OnChase()
{
	float distX = position.x - GAME.player->Position().x;
	float distY = position.y - GAME.player->Position().y;

	if(GAME.player->GetState() != Player::PS_Jumping)
	{
		if(distY > MinDistY) yVel = -speed;
		else if(distY < -MinDistY) yVel = speed;
	}
	else { yVel = 0.0f; }

	if(distX > MinDistX)
	{
		Walk(Left);
		xVel = -speed;
	}
	else if(distX < -MinDistX)
	{
		Walk(Right);
		xVel = speed;
	}

	//When close enough, attack
	if(SDL_abs((int)distX) <= (int)MinDistX 
		&& SDL_abs((int)distY) <= (int)MinDistY)
	{
		Stop();
		Attack();
		
		if(GAME.player->IsDead())
		{
			state = ES_Patrolling;
			current = GetDirection() == Right? walkRight: walkLeft;
		}
	}
}


void Enemy::OnRecovery()
{
	Stop();
	state = ES_Idle;
	recoveryTimer = 0;
	hitCount = 0;
}


void Enemy::OnPunch()
{
	current = GetDirection() == Left? punchLeft: punchRight;
	current->SetCurrentFrame(0);

	if(SDL_GetTicks() - punchTimer > PunchTimeOut)
	{
		state = ES_Idle;
		punchTimer = 0;
	}
}


void Enemy::Attack()
{
	state = ES_Attacking;
	punchTimer = SDL_GetTicks();
}


void Enemy::OnIdle()
{
	if(!idleTimer)
	{
		Stop();
		idleTimer = SDL_GetTicks() + WHEEL_OF_FORTUNE.Next(1000, 3000);
	}
	else
	{
		if(SDL_GetTicks() >= idleTimer)
		{
			state = ES_Chasing;
			idleTimer = 0;
		}
		else
		{
			//Face player all the time when on idle
			SetDirection(position.x > GAME.player->Position().x? Left: Right);
			Stop();
		}
	}
}






#pragma region Rock

const float Rock::Range(10.0f * (float)GAME.ClientWidth());


Rock::Rock(const string& file, SDL_Renderer* const renderer)
 : GameObject(GT_Enemy, 1, Left) 
 , texture(NULL)
{
	util::SDLSurfaceFromFile surf(file, true);
	texture = SDL_CreateTextureFromSurface(renderer, surf.surface);
	
	speed = 10.0f;
	position.x = Range;
	position.w = (float)surf.surface->w;
	position.h = (float)surf.surface->h;
	position.y = (float)GAME.player->Position().y + 20;
	AdjustZToGameDepth();
}


void Rock::Update()
{
	if(GetDirection() == Left)
	{
		SetAngle(GetAngle() - speed);
		xVel = -speed;
	}
	else
	{
		SetAngle(GetAngle() + speed);
		xVel = speed;
	}

	if(position.x >= Range)
	{
		position.y = (float)GAME.RandomYWithinMoveBounds((int)position.h);
		SetDirection(Left);
	}
	else if(position.x < -Range)
	{
		position.y = (float)GAME.RandomYWithinMoveBounds((int)position.h);
		SetDirection(Right);
	}

	position.x += xVel;

	AdjustZToGameDepth();
}


void Rock::Draw(SDL_Renderer* const renderer) const
{
	SDL_Rect nPos;
	util::Convert(position, nPos);
	SDL_RenderCopyEx(renderer, texture, NULL, &nPos, GetAngle(), NULL, SDL_FLIP_NONE);
}


Rock::~Rock()
{
	if(texture)
	{
		SDL_DestroyTexture(texture);
		texture = NULL;
	}

	logPrintf("Rock object released");
}

#pragma endregion
