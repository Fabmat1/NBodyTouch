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
    // Normalize to 0..1 range across our temp span
    float t = std::clamp((tempK - 2000.0f) / (38000.0f), 0.0f, 1.0f);

    float r, g, b;

    if (t < 0.10f) {
        // 2000–5800 K  deep red → orange
        float f = t / 0.10f;
        r = 1.0f;
        g = 0.15f + 0.35f * f;
        b = 0.0f;
    } else if (t < 0.20f) {
        // 5800–9600 K  orange → yellow-white
        float f = (t - 0.10f) / 0.10f;
        r = 1.0f;
        g = 0.50f + 0.45f * f;
        b = 0.1f + 0.5f * f;
    } else if (t < 0.35f) {
        // 9600–15300 K  yellow-white → white
        float f = (t - 0.20f) / 0.15f;
        r = 1.0f - 0.05f * f;
        g = 0.95f + 0.05f * f;
        b = 0.60f + 0.40f * f;
    } else if (t < 0.55f) {
        // 15300–22900 K  white → blue-white
        float f = (t - 0.35f) / 0.20f;
        r = 0.95f - 0.30f * f;
        g = 1.0f  - 0.15f * f;
        b = 1.0f;
    } else {
        // 22900–40000 K  blue-white → blue
        float f = (t - 0.55f) / 0.45f;
        r = 0.65f - 0.25f * f;
        g = 0.85f - 0.25f * f;
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