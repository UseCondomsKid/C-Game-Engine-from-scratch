#pragma once


typedef struct ANIMATION
{
	char* Name;
	uint16_t Index;
	uint16_t Frames;
	float Speed;
	uint16_t Width;
	uint16_t Height;

} ANIMATION;



ANIMATION CreateAnimation(_In_ char* AnimationName, _In_ uint16_t Width, _In_ uint16_t Height, _In_ uint16_t Index, _In_ uint16_t Frames, _In_ float Speed);

GAMEBITMAP AnimateSprite(ANIMATION Animation, ENTITY Entity, GAMEBITMAP* SpriteSheet);

