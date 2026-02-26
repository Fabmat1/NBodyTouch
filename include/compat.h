#pragma once
#include "raylib.h"

// DrawRectangleRoundedLinesEx was added in raylib 5.0
// Older versions only have DrawRectangleRoundedLines (no thickness param)
#if !defined(RAYLIB_VERSION_MAJOR) || RAYLIB_VERSION_MAJOR < 5
inline void DrawRectangleRoundedLinesEx(Rectangle rec, float roundness,
                                         int segments, float lineThick,
                                         Color color) {
    (void)lineThick;  // ignore thickness, not supported in 4.x
    DrawRectangleRoundedLines(rec, roundness, segments, color);
}

inline Ray GetScreenToWorldRay(Vector2 pos, Camera cam) {
    return GetMouseRay(pos, cam);
}
#endif