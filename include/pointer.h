#pragma once
#include "raylib.h"
#include "raymath.h"

namespace Pointer {

    // ── primary pointer ──
    inline Vector2 _pos       = {0, 0};
    inline bool    _down      = false;
    inline bool    _pressed   = false;
    inline bool    _released  = false;
    inline bool    _prevDown  = false;

    // ── touch cache ──
    inline bool    _usingTouch  = false;
    inline int     _cachedCount = 0;
    inline Vector2 _cachedPos[8] = {};

    // ── fresh-data detection ──
    inline Vector2 _prevFramePos[8] = {};
    inline int     _prevFrameCount  = 0;
    inline bool    _freshData       = false;

    inline void beginFrame()
    {
        _prevDown = _down;
        int tc        = GetTouchPointCount();
        Vector2 mouse = GetMousePosition();

        /* ── refresh cache, filtering contamination ── */
        if (tc > 0) {
            for (int i = 0; i < tc && i < 8; i++) {
                Vector2 raw = GetTouchPosition(i);

                if (raw.x < 0.0f || raw.y < 0.0f) continue;

                if (_usingTouch && i < _cachedCount) {
                    float jump      = Vector2Distance(raw, _cachedPos[i]);
                    float nearMouse = Vector2Distance(raw, mouse);

                    // raylib desktop fallback: (0,0) slot → GetMousePosition()
                    bool mouseContaminated = (jump > 30.0f && nearMouse < 10.0f);
                    // catch (0,0) or other wildly wrong positions
                    bool impossibleJump    = (jump > 300.0f);

                    if (mouseContaminated || impossibleJump)
                        continue;
                }

                _cachedPos[i] = raw;
            }
            _cachedCount = tc;
            _usingTouch  = true;
        }

        /* ── primary-pointer state machine ── */
        if (tc > 0) {
            _down = true;
            _pos  = _cachedPos[0];
        }
        else if (_usingTouch) {
            if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
                _down = true;
                _pos  = _cachedPos[0];
            } else {
                _down        = false;
                _pos         = _cachedPos[0];
                _usingTouch  = false;
                _cachedCount = 0;
            }
        }
        else {
            _down = IsMouseButtonDown(MOUSE_BUTTON_LEFT);
            _pos  = mouse;
        }

        _pressed  =  _down && !_prevDown;
        _released = !_down &&  _prevDown;

        /* ── detect whether cached positions actually changed ──
           If all positions are identical to last frame (within 0.5 px),
           no real finger movement occurred — gesture handlers should
           skip delta computation to prevent phantom zoom/rotate/pan. */
        _freshData = false;
        if (_usingTouch && _cachedCount > 0) {
            if (_cachedCount != _prevFrameCount) {
                _freshData = true;          // finger count changed
            } else {
                for (int i = 0; i < _cachedCount && i < 8; i++) {
                    if (Vector2Distance(_cachedPos[i], _prevFramePos[i]) > 0.5f) {
                        _freshData = true;  // at least one finger moved
                        break;
                    }
                }
            }
            _prevFrameCount = _cachedCount;
            for (int i = 0; i < _cachedCount && i < 8; i++)
                _prevFramePos[i] = _cachedPos[i];
        }
    }

    /* ── primary pointer ── */
    inline Vector2 position() { return _pos; }
    inline bool    pressed()  { return _pressed; }
    inline bool    down()     { return _down; }
    inline bool    released() { return _released; }

    /* ── fresh-data flag ── */
    inline bool    freshData(){ return _freshData; }

    /* ── multi-touch (filtered cache) ── */
    inline int touchCount() {
        int tc = GetTouchPointCount();
        if (tc > 0) return tc;
        if (_usingTouch) return _cachedCount;
        return 0;
    }

    inline Vector2 touchPosition(int index) {
        if (_usingTouch && index >= 0 && index < _cachedCount)
            return _cachedPos[index];
        int tc = GetTouchPointCount();
        if (tc > 0 && index >= 0 && index < tc)
            return GetTouchPosition(index);
        return {0, 0};
    }
}