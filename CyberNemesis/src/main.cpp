#include "core/Game.h"
#include <cstring>

// Argumento opcional:  --fullscreen (o -f) arranca a pantalla completa.
int main(int argc, char** argv) {
    Game game;
    for (int i = 1; i < argc; ++i)
        if (std::strcmp(argv[i], "--fullscreen") == 0 || std::strcmp(argv[i], "-f") == 0) game.SetStartFullscreen(true);
    game.Run();
    return 0;
}
