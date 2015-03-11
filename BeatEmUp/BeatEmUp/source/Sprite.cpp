#include "Sprite.h"
#include "Util.h"



Sprite::Sprite(SDL_Surface* const spriteSheet, SDL_Renderer* const renderer, 
	int frameWidth, int frameHeight, int frameSpeed_, int stillFrame_)
	: GameObject(GT_Sprite)
	, sheet(NULL)
	, frameSpeed(frameSpeed_)
	, stillFrame(stillFrame_)
	, fromIndex(0)
	, toIndex(0)
	, currentFrame(0)
	, animationRunning(false)
	, loop(true)
{
	if(!spriteSheet || !renderer) return;

	position.x = 100, position.y = 400;
	position.w = (float)frameWidth, position.h = (float)frameHeight;
	framesPerRow = (int)SDL_floor((double)spriteSheet->w / frameWidth);
	rowCount = (int)SDL_floor((double)spriteSheet->h / frameHeight);
	frameCount = framesPerRow * rowCount;
	sheet = SDL_CreateTextureFromSurface(renderer, spriteSheet);

	fromIndex = 0;
	toIndex = frameCount - 1;
	logPrintf("spritesheet loaded (%d,%d) %d frames", spriteSheet->w, spriteSheet->h, frameCount);
}


void Sprite::PlayFrames(int fromFrame, int toFrame, bool loop_) 
{
	fromIndex = currentFrame = fromFrame; 
	toIndex = toFrame;
	SetLoop(loop_);
	SetAnimation(true);
}


void Sprite::Update()
{
	if (!animationRunning)
	{
		//currentFrame = stillFrame;
		return;
	}

	// update to the next frame if it is time
	if (counter == (frameSpeed - 1)) {
		currentFrame = (currentFrame + 1) % (toIndex + 1);
    if(currentFrame < fromIndex) currentFrame = fromIndex;

		//play sound effect if one exists for this frame
		std::map< int, Mixer::SoundEffect >::iterator it = frameSounds.find(currentFrame);
		if(it != frameSounds.end()) MIXER.Play(it->second);		
	}
	// update the counter
	counter = (counter + 1) % frameSpeed;
}


void Sprite::Draw(SDL_Renderer* const renderer) const
{
	int row = currentFrame / framesPerRow;
	int col = currentFrame % framesPerRow;
	SDL_Rect src = { (int)((float)col * position.w), (int)((float)row * position.h)
		, (int)position.w, (int)position.h };

	SDL_Rect nPos;
	util::Convert(position, nPos);
	SDL_RenderCopyEx(renderer, sheet, &src, &nPos, GetAngle(), NULL, SDL_FLIP_NONE);

	if(!loop && IsAnimationRunning() && currentFrame == toIndex)
	{
		const_cast<Sprite*>(this)->SetAnimation(false);
	}
}


Sprite::~Sprite()
{
	if(sheet)
	{
		SDL_DestroyTexture(sheet);
		sheet = NULL;
	}

	logPrintf("Sprite object released");
}

