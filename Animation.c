#pragma warning(push, 3)

#include<stdio.h>

#include<windows.h>

#include<psapi.h>

#include<emmintrin.h>

#pragma warning(pop)

#include<stdint.h>

#include "Main.h"

#include "Animation.h"

#pragma comment(lib, "Winmm.lib")



ANIMATION CreateAnimation(_In_ char* AnimationName, _In_ uint16_t Width, _In_ uint16_t Height, _In_ uint16_t Index, _In_ uint16_t Frames, _In_ float Speed)
{ 
    ANIMATION Animation = { 0 };

    Animation.Name = AnimationName;
    Animation.Index = Index;
    Animation.Frames = Frames;
    Animation.Speed = Speed;
    Animation.Width = Width;
    Animation.Height = Height;

    return Animation;
}



void AnimateSprite(ANIMATION Animation, _ENTITY Entity, GAMEBITMAP* SpriteSheet)
{
    
    uint16_t Index = Animation.Index;
    uint16_t Frames = Animation.Frames;
    float Speed = Animation.Speed;
    uint16_t Width = Animation.Width;
    uint16_t Height = Animation.Height;


    static float AnimationTime;
    AnimationTime += Speed;

    uint16_t SpriteWidth = (uint16_t)SpriteSheet->BitMapInfo.bmiHeader.biWidth / Frames;
    uint16_t SpriteHeight = ((uint16_t)SpriteSheet->BitMapInfo.bmiHeader.biHeight / Entity.NumberOfAnimations) + (Index * Height);


    uint16_t BytesPerSprite = (SpriteWidth * SpriteHeight * (SpriteSheet->BitMapInfo.bmiHeader.biBitCount / 8));


    GAMEBITMAP Sprite = { 0 };
    Sprite.BitMapInfo.bmiHeader.biBitCount = GAME_BPP;
    Sprite.BitMapInfo.bmiHeader.biHeight = SpriteHeight;
    Sprite.BitMapInfo.bmiHeader.biWidth = SpriteWidth;
    Sprite.BitMapInfo.bmiHeader.biPlanes = 1;
    Sprite.BitMapInfo.bmiHeader.biCompression = BI_RGB;
    Sprite.Memory = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, ((size_t)BytesPerSprite * ((size_t)SpriteWidth * (size_t)SpriteHeight)));

    uint16_t StartingSpriteWidth = SpriteWidth * ((int)AnimationTime % Frames);

    uint16_t StartingSpriteSheetPixel = (SpriteSheet->BitMapInfo.bmiHeader.biWidth * (SpriteSheet->BitMapInfo.bmiHeader.biHeight - (SpriteHeight - Height))) - (SpriteSheet->BitMapInfo.bmiHeader.biWidth + StartingSpriteWidth);


    int SpriteSheetOffset = 0;
    int SpriteBitmapOffset = 0;
    PIXEL32 SpritePixel = { 0 };


     for (int YPixel = 0; YPixel < SpriteHeight; YPixel++)
     {
        for (int XPixel = 0; XPixel < SpriteWidth; XPixel++)
        {
            SpriteSheetOffset = StartingSpriteSheetPixel + XPixel - (SpriteSheet->BitMapInfo.bmiHeader.biWidth * YPixel);

            SpriteBitmapOffset = ((Sprite.BitMapInfo.bmiHeader.biWidth * Sprite.BitMapInfo.bmiHeader.biHeight) - \
                Sprite.BitMapInfo.bmiHeader.biWidth) + XPixel - ((Sprite.BitMapInfo.bmiHeader.biWidth) * YPixel);

            memcpy_s(&SpritePixel, sizeof(PIXEL32), (PIXEL32*)SpriteSheet->Memory + SpriteSheetOffset, sizeof(PIXEL32));

            memcpy_s((PIXEL32*)Sprite.Memory + SpriteBitmapOffset, sizeof(PIXEL32), &SpritePixel, sizeof(PIXEL32));
        }
     }

     Blit32BppBitmapToBuffer(&Sprite, Entity.ScreenPosX, Entity.ScreenPosY);

     if (Sprite.Memory)
     {
         HeapFree(GetProcessHeap(), 0, Sprite.Memory);
     }
}
