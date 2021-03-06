#pragma once
#include "GameObject.h"
#include "Sprite.h"


class Roamer : public GameObject
{
public:
	Roamer(SDL_Renderer& renderer, Sprite::ptr walkLeftSprite, Sprite::ptr walkRightSprite
		, float posX, float posY, float roamMinX_, float roamMaxX_, bool backgroundObj);
	virtual void Update() override;
	virtual void Draw(SDL_Renderer& renderer) const override;
	virtual ~Roamer();
	virtual void SetDirection(Direction dir) override;
	void Stop();


private:
	void Translate(bool anim = true);


private:
	Sprite::ptr walkRight;
	Sprite::ptr walkLeft;
	Sprite* current;
	float roamMinX;
	float roamMaxX;
	const bool backgroundObject;
};
