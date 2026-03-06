#include "assets.h"
#include <cstring>
#include <cstdio>

static char g_root[1024] = "";
static char g_pathBuf[2048] = "";

void SetAssetRoot(const char *root) {
    if (!root) { g_root[0] = '\0'; return; }
    snprintf(g_root, sizeof(g_root), "%s", root);
    size_t len = strlen(g_root);
    while (len > 0 && (g_root[len-1] == '/' || g_root[len-1] == '\\'))
        g_root[--len] = '\0';
}

const char *AssetPath(const char *relative) {
    if (g_root[0] == '\0') return relative;
    snprintf(g_pathBuf, sizeof(g_pathBuf), "%s/%s", g_root, relative);
    return g_pathBuf;
}