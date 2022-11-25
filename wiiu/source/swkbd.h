#pragma once

#include <vpad/input.h>

#ifdef __cplusplus
extern "C" {
#endif

int swkbdInit(void);

void swkbdExit(void);

void swkbdShow(void);

int swkbdIsOpened(void);

const char* swkbdProc(VPADStatus* vpad);

void swkbdDrawTV(void);

void swkbdDrawDRC(void);

#ifdef __cplusplus
}
#endif
