#include "App.h"

#include <windows.h>

int WINAPI wWinMain(HINSTANCE instanceHandle, HINSTANCE, PWSTR, int showCommand) {
    App app;
    return app.Run(instanceHandle, showCommand);
}
