#include "star.h"
#include <cmath>
#include <algorithm>

void stellarModel(float mass, float age,
                  float &outRadius, float &outLuminosity, float &outTempK)
{
    float msLifetime = 10.0f * powf(mass, -2.5f);

    if (age < msLifetime) {
        outLuminosity = powf(mass, 3.5f);
        outRadius     = powf(mass, 0.8f);
        outTempK      = 5778.0f * powf(outLuminosity, 0.25f) / sqrtf(outRadius);
    } else if (age < msLifetime * 1.3f) {
        float frac    = (age - msLifetime) / (msLifetime * 0.3f);
        outLuminosity = powf(mass, 3.5f) * (1.0f + 99.0f * frac);
        outRadius     = powf(mass, 0.8f) * (1.0f + 49.0f * frac);
        outTempK      = 3500.0f + (1.0f - frac) * 1500.0f;
    } else {
        float wdAge   = age - msLifetime * 1.3f;
        float cooling = expf(-wdAge * 0.3f);
        outRadius     = 0.01f * mass;
        outLuminosity = 0.001f * cooling;
        outTempK      = 4000.0f + 36000.0f * cooling;
    }

    outRadius     = std::clamp(outRadius, 0.01f, 60.0f);
    outLuminosity = std::clamp(outLuminosity, 0.0001f, 100000.0f);
    outTempK      = std::clamp(outTempK, 2000.0f, 40000.0f);
}

// Attempt a perceptually correct blackbody → sRGB mapping
// that keeps cool stars red/orange (not magenta).
Color temperatureToColor(float tempK) {
    float r, g, b;

    if (tempK < 3500.0f) {
        // M-type: deep red → red-orange
        float f = (tempK - 2000.0f) / 1500.0f;
        f = std::clamp(f, 0.0f, 1.0f);
        r = 1.0f;
        g = 0.35f + 0.25f * f;
        b = 0.10f + 0.10f * f;
    } else if (tempK < 5000.0f) {
        // K-type: orange → orange-yellow
        float f = (tempK - 3500.0f) / 1500.0f;
        r = 1.0f;
        g = 0.60f + 0.25f * f;
        b = 0.20f + 0.40f * f;
    } else if (tempK < 6000.0f) {
        // G-type (Sun): yellow → pale yellow-white
        float f = (tempK - 5000.0f) / 1000.0f;
        r = 1.0f;
        g = 0.85f + 0.10f * f;
        b = 0.60f + 0.30f * f;
    } else if (tempK < 7500.0f) {
        // F-type: pale yellow → white
        float f = (tempK - 6000.0f) / 1500.0f;
        r = 1.0f - 0.02f * f;
        g = 0.95f + 0.03f * f;
        b = 0.90f + 0.10f * f;
    } else if (tempK < 10000.0f) {
        // A-type: white → blue-white
        float f = (tempK - 7500.0f) / 2500.0f;
        r = 0.98f - 0.15f * f;
        g = 0.98f - 0.05f * f;
        b = 1.0f;
    } else if (tempK < 20000.0f) {
        // B-type: blue-white → light blue
        float f = (tempK - 10000.0f) / 10000.0f;
        r = 0.83f - 0.25f * f;
        g = 0.93f - 0.20f * f;
        b = 1.0f;
    } else {
        // O-type: light blue → deeper blue
        float f = (tempK - 20000.0f) / 30000.0f;
        f = std::clamp(f, 0.0f, 1.0f);
        r = 0.58f - 0.10f * f;
        g = 0.73f - 0.15f * f;
        b = 1.0f;
    }

    return Color{
        (unsigned char)(std::clamp(r, 0.0f, 1.0f) * 255),
        (unsigned char)(std::clamp(g, 0.0f, 1.0f) * 255),
        (unsigned char)(std::clamp(b, 0.0f, 1.0f) * 255),
        255
    };
}

void Star::updateStellarProperties() {
    stellarModel(mass, age, radius, luminosity, tempK);
    color = temperatureToColor(tempK);
}

void Star::pushTrail() {
    trail.push_back(pos);
    if ((int)trail.size() > MAX_TRAIL)
        trail.erase(trail.begin());
}