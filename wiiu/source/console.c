#include "console.h"
#include "font.h"

#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/iosupport.h>

#include <coreinit/debug.h>

#define NUM_LINES 21
#define LINE_LENGTH 500
static char consoleBuffer[NUM_LINES][LINE_LENGTH];
static size_t lineNum;
static int redraw = 0;

static ssize_t Console_Write(struct _reent* r, void* fd, const char* ptr, size_t len) {
    size_t length = len;
    if (length >= LINE_LENGTH) {
        length = LINE_LENGTH - 1;
    }

    if (lineNum == NUM_LINES) {
        for (int i = 0; i < NUM_LINES - 1; ++i) {
            memcpy(consoleBuffer[i], consoleBuffer[i + 1], LINE_LENGTH);
        }

        memcpy(consoleBuffer[lineNum - 1], ptr, length);
        consoleBuffer[lineNum - 1][length] = '\0';
    } else {
        memcpy(consoleBuffer[lineNum], ptr, length);
        consoleBuffer[lineNum][length] = '\0';
        ++lineNum;
    }

    redraw = 1;

    OSReport("%*.*s", len, len, ptr);
    return len;
}

static const devoptab_t devoptab_stdout = {
    .name = "stdout_console",
    .write_r = Console_Write,
};

void Console_Init(void)
{
    Font_Init();
    Font_SetSize(50);
    Font_SetColor(255, 255, 255, 255);

    lineNum = 0;

    devoptab_list[STD_OUT] = &devoptab_stdout;
    devoptab_list[STD_ERR] = &devoptab_stdout;
}

void Console_Shutdown(void)
{
    Font_Deinit();
}

void Console_Draw(void)
{
    if (!redraw) {
        return;
    }

    Font_Clear();

    for (int y = 1; y <= NUM_LINES; ++y) {
        Font_Print(0, y * 50, consoleBuffer[y - 1]);
    }

    redraw = 0;
}
