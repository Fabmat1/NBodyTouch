#pragma once
#include "raylib.h"
#include "raymath.h"
#include <vector>

constexpr float G_CONST   = 40.0f;
constexpr int   MAX_TRAIL  = 200;
constexpr int   MAX_STARS  = 64;

#ifdef PLATFORM_ANDROID
constexpr int SCREEN_W = 1920;  // won't be used, overridden at runtime
constexpr int SCREEN_H = 1080;
constexpr float UI_SCALE_BOOST = 2.2f;
#else
constexpr int SCREEN_W = 1920;
constexpr int SCREEN_H = 1080;
constexpr float UI_SCALE_BOOST = 1.0f;
#endif

struct Star {
    Vector3 pos;
    Vector3 vel;
    float   mass;
    float   age;

    float   radius;
    float   luminosity;
    float   tempK;
    Color   color;

    std::vector<Vector3> trail;
    bool active = false;

    void updateStellarProperties();
    void pushTrail();
};

Color temperatureToColor(float tempK);
void  stellarModel(float mass, float age,
                   float &outRadius, float &outLuminosity,
                   float &outTempK);