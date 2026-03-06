#include <cmath>
#include <algorithm>

#include "renderer.h"
#include "assets.h"
#include "rlgl.h"

static Texture2D skyFaces[6]   = {};
static bool      skyFacesLoaded = false;

static const char *SKY_FACES_REL[6] = {
    "assets/skybox/sky_pos_x.png",
    "assets/skybox/sky_neg_x.png",
    "assets/skybox/sky_pos_y.png",
    "assets/skybox/sky_neg_y.png",
    "assets/skybox/sky_pos_z.png",
    "assets/skybox/sky_neg_z.png",
};

static Texture2D glowTex = {0};

static void generateGlowTexture() {
    const int size = 256;
    Image img = GenImageColor(size, size, BLANK);

    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            float dx = (x - size * 0.5f + 0.5f) / (size * 0.5f);
            float dy = (y - size * 0.5f + 0.5f) / (size * 0.5f);
            float d  = sqrtf(dx * dx + dy * dy);

            if (d >= 1.0f) {
                ImageDrawPixel(&img, x, y, BLANK);
                continue;
            }

            float core = expf(-d * d * 20.0f);
            float glow = expf(-d * d * 5.0f) * 0.6f;
            float halo = expf(-d * d * 1.8f) * 0.25f;

            float a = core + glow + halo;

            float edgeFade = 1.0f - d * d;
            edgeFade = std::clamp(edgeFade, 0.0f, 1.0f);
            a *= edgeFade;
            a = std::clamp(a, 0.0f, 1.0f);

            unsigned char alpha = (unsigned char)(a * 255);
            ImageDrawPixel(&img, x, y, Color{255, 255, 255, alpha});
        }
    }

    glowTex = LoadTextureFromImage(img);
    SetTextureFilter(glowTex, TEXTURE_FILTER_BILINEAR);
    UnloadImage(img);
}

void Renderer::init(int w, int h) {
    screenW = w;
    screenH = h;

    camera = {};
    camera.position   = {0.0f, 20.0f, 40.0f};
    camera.target     = {0.0f, 0.0f, 0.0f};
    camera.up         = {0.0f, 1.0f, 0.0f};
    camera.fovy       = 60.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    loadSkybox();
    generateGlowTexture();
}

void Renderer::loadSkybox() {
    skyboxLoaded = true;
    for (int i = 0; i < 6; i++) {
        const char *path = AssetPath(SKY_FACES_REL[i]);
        if (!FileExists(path)) {
            TraceLog(LOG_WARNING, "Skybox face missing: %s", path);
            skyboxLoaded = false;
            return;
        }
        skyFaces[i] = LoadTexture(path);
        SetTextureFilter(skyFaces[i], TEXTURE_FILTER_BILINEAR);
        SetTextureWrap(skyFaces[i], TEXTURE_WRAP_CLAMP);
        if (skyFaces[i].id == 0) {
            skyboxLoaded = false;
            return;
        }
    }
    skyFacesLoaded = true;
    TraceLog(LOG_INFO, "Skybox loaded (%dx%d per face)", skyFaces[0].width, skyFaces[0].height);
}

static void drawSkyFace(unsigned int texId,
                         Vector3 v0, Vector3 v1, Vector3 v2, Vector3 v3) {
    rlSetTexture(texId);
    rlBegin(RL_QUADS);
        rlColor4ub(255, 255, 255, 255);
        rlTexCoord2f(0, 1); rlVertex3f(v0.x, v0.y, v0.z);
        rlTexCoord2f(1, 1); rlVertex3f(v1.x, v1.y, v1.z);
        rlTexCoord2f(1, 0); rlVertex3f(v2.x, v2.y, v2.z);
        rlTexCoord2f(0, 0); rlVertex3f(v3.x, v3.y, v3.z);
    rlEnd();
}

void Renderer::drawSkybox() const {
    if (!skyboxLoaded || !skyFacesLoaded) return;

    float S = 500.0f;
    float x = camera.position.x, y = camera.position.y, z = camera.position.z;

    rlDrawRenderBatchActive();
    rlDisableBackfaceCulling();
    rlDisableDepthMask();
    rlDisableDepthTest();

    drawSkyFace(skyFaces[0].id,
        {x+S,y-S,z+S},{x+S,y-S,z-S},{x+S,y+S,z-S},{x+S,y+S,z+S});
    drawSkyFace(skyFaces[1].id,
        {x-S,y-S,z-S},{x-S,y-S,z+S},{x-S,y+S,z+S},{x-S,y+S,z-S});
    drawSkyFace(skyFaces[2].id,
        {x-S,y+S,z+S},{x+S,y+S,z+S},{x+S,y+S,z-S},{x-S,y+S,z-S});
    drawSkyFace(skyFaces[3].id,
        {x-S,y-S,z-S},{x+S,y-S,z-S},{x+S,y-S,z+S},{x-S,y-S,z+S});
    drawSkyFace(skyFaces[4].id,
        {x-S,y-S,z+S},{x+S,y-S,z+S},{x+S,y+S,z+S},{x-S,y+S,z+S});
    drawSkyFace(skyFaces[5].id,
        {x+S,y-S,z-S},{x-S,y-S,z-S},{x-S,y+S,z-S},{x+S,y+S,z-S});

    rlSetTexture(0);
    rlDrawRenderBatchActive();
    rlEnableDepthTest();
    rlEnableDepthMask();
    rlEnableBackfaceCulling();
}

static void drawBillboard(Camera3D cam, Vector3 pos, float size, Color tint) {
    if (glowTex.id == 0) return;

    Vector3 forward = Vector3Normalize(Vector3Subtract(cam.position, pos));
    Vector3 right   = Vector3Normalize(Vector3CrossProduct(cam.up, forward));
    Vector3 up      = Vector3CrossProduct(forward, right);

    float half = size * 0.5f;
    Vector3 r = Vector3Scale(right, half);
    Vector3 u = Vector3Scale(up, half);

    Vector3 v0 = Vector3Subtract(Vector3Subtract(pos, r), u);
    Vector3 v1 = Vector3Subtract(Vector3Add(pos, r), u);
    Vector3 v2 = Vector3Add(Vector3Add(pos, r), u);
    Vector3 v3 = Vector3Add(Vector3Subtract(pos, r), u);

    rlSetTexture(glowTex.id);
    rlBegin(RL_QUADS);
        rlColor4ub(tint.r, tint.g, tint.b, tint.a);
        rlTexCoord2f(0, 1); rlVertex3f(v0.x, v0.y, v0.z);
        rlTexCoord2f(1, 1); rlVertex3f(v1.x, v1.y, v1.z);
        rlTexCoord2f(1, 0); rlVertex3f(v2.x, v2.y, v2.z);
        rlTexCoord2f(0, 0); rlVertex3f(v3.x, v3.y, v3.z);
    rlEnd();
}

void Renderer::drawStar(const Star &s) const {
    if (!s.active) return;

    float displayR = 0.3f + 0.25f * logf(1.0f + s.radius);
    float glowSize = displayR * 6.0f;

    Color core = s.color;
    core.a = 255;
    drawBillboard(camera, s.pos, displayR * 2.0f, core);

    Color mid = s.color;
    mid.a = 100;
    drawBillboard(camera, s.pos, glowSize, mid);

    Color outer = s.color;
    outer.a = 30;
    drawBillboard(camera, s.pos, glowSize * 2.0f, outer);
}

void Renderer::drawTrail(const Star &s) const {
    if (!s.active || s.trail.size() < 2) return;

    for (size_t i = 1; i < s.trail.size(); i++) {
        float alpha = (float)i / (float)s.trail.size();
        Color c = s.color;
        c.a = (unsigned char)(alpha * 140);
        DrawLine3D(s.trail[i - 1], s.trail[i], c);
    }
}

void Renderer::drawVelocityArrow(Vector3 from, Vector3 to) const {
    DrawLine3D(from, to, YELLOW);
    DrawSphere(to, 0.2f, YELLOW);
}

void Renderer::drawScene(const Simulation &sim) {
    BeginMode3D(camera);

        drawSkybox();

        rlDrawRenderBatchActive();
        rlDisableDepthMask();
        BeginBlendMode(BLEND_ADDITIVE);

        for (int i = 0; i < sim.starCount; i++)
            drawTrail(sim.stars[i]);

        for (int i = 0; i < sim.starCount; i++)
            drawStar(sim.stars[i]);

        rlDrawRenderBatchActive();
        EndBlendMode();
        rlEnableDepthMask();

    EndMode3D();
}

void Renderer::zoomCamera(float delta) {
    Vector3 dir = Vector3Subtract(camera.target, camera.position);
    float dist = Vector3Length(dir);
    if (dist < 2.0f && delta > 0.0f) return;
    dir = Vector3Scale(Vector3Normalize(dir), delta);
    camera.position = Vector3Add(camera.position, dir);
}

void Renderer::shutdown() {
    if (skyFacesLoaded) {
        for (int i = 0; i < 6; i++) UnloadTexture(skyFaces[i]);
        skyFacesLoaded = false;
    }
    if (glowTex.id != 0) UnloadTexture(glowTex);
}