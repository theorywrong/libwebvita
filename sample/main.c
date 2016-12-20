#include <stdlib.h>
#include <stdio.h>

#include <psp2/ctrl.h>
#include <psp2/io/stat.h>
#include <psp2/io/fcntl.h>
#include <psp2/kernel/sysmem.h>
#include <psp2/kernel/processmgr.h>
#include <string.h>

#include <webvita.h>

#include "graphics.h"

SceCtrlData pad;
uint32_t old_buttons, current_buttons, pressed_buttons;

void readPad() {
    memset(&pad, 0, sizeof(SceCtrlData));
    sceCtrlPeekBufferPositive(0, &pad, 1);

    current_buttons = pad.buttons;
    pressed_buttons = current_buttons & ~old_buttons;

    if (old_buttons != current_buttons) {
        old_buttons = current_buttons;
    }
}

void press(int buttons) {
    readPad();
    while (pressed_buttons != buttons) {
        readPad();
    }
}

Response test_call(Request* req) {
    char* html = "{'path': '%s'}\x00";

    char* data = malloc(strlen(html) + strlen(default_path) + 1);
    sprintf(data, html, default_path);

    Response res;
    res.statut_code = HTTP_OK;
    res.mime = getMime("json");
    res.data = data;
    res.data_size = strlen(data);

    return res;
}

int main() {
	psvDebugScreenInit();
	psvDebugScreenPrintf("WebServer by TheoryWrong\n");
    psvDebugScreenPrintf("Press X to close server.\n");

    initWebServer();
    addCall("/test", test_call);
    set404error("<h1>Holy sh*t !</h1><p>i doesn't find %s on this server</p>");

    launchWebServer();

    press(SCE_CTRL_CROSS);

    stopWebServer();
    psvDebugScreenPrintf("Waiting close ...\n");

    while (!isServerStop()) {}

    psvDebugScreenPrintf("Closed !\n");
    psvDebugScreenPrintf("Press X to Exit this app\n");

    press(SCE_CTRL_CROSS);

    sceKernelExitProcess(0);
	return 0;
}