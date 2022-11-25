#include "swkbd.h"

#include <stdlib.h>
#include <nn/swkbd.h>

extern "C" {

static nn::swkbd::CreateArg createArg;
static FSClient* fsClient;
static char textBuffer[0x200];

int swkbdInit(void)
{
    // Create FSClient for swkbd
    fsClient = (FSClient*) malloc(sizeof(FSClient));
    FSAddClient(fsClient, FS_ERROR_FLAG_NONE);

    // Create swkbd
    createArg.regionType = nn::swkbd::RegionType::Europe;
    createArg.workMemory = malloc(nn::swkbd::GetWorkMemorySize(0));
    createArg.fsClient = fsClient;
    if (!nn::swkbd::Create(createArg)) {
        return -1;
    }

    return 0;
}

void swkbdExit(void)
{
   nn::swkbd::Destroy();
   free(createArg.workMemory);

   FSDelClient(fsClient, FS_ERROR_FLAG_NONE);
   free(fsClient);
}

void swkbdShow(void)
{
    nn::swkbd::AppearArg appearArg;
    appearArg.keyboardArg.configArg.languageType = nn::swkbd::LanguageType::English;
    nn::swkbd::AppearInputForm(appearArg);
}

int swkbdIsOpened(void)
{
    return nn::swkbd::GetStateInputForm() != nn::swkbd::State::Hidden;
}

const char* swkbdProc(VPADStatus* vpad)
{
    nn::swkbd::ControllerInfo controllerInfo;
    controllerInfo.vpad = vpad;
    controllerInfo.kpad[0] = nullptr;
    controllerInfo.kpad[1] = nullptr;
    controllerInfo.kpad[2] = nullptr;
    controllerInfo.kpad[3] = nullptr;
    nn::swkbd::Calc(controllerInfo);

    if (nn::swkbd::IsNeedCalcSubThreadFont()) {
        nn::swkbd::CalcSubThreadFont();
    }

    if (nn::swkbd::IsNeedCalcSubThreadPredict()) {
        nn::swkbd::CalcSubThreadPredict();
    }

    if (nn::swkbd::IsDecideOkButton(nullptr)) {
        nn::swkbd::DisappearInputForm();

        const char16_t* str = nn::swkbd::GetInputFormString();
        if (!str) {
            return nullptr;
        }

        for (size_t i = 0; i < sizeof(textBuffer); i++) {
            if (!str[i]) {
                textBuffer[i] = '\0';
                break;
            }

            if (str[i] > 0x7F) {
                textBuffer[i] = '?';
            } else {
                textBuffer[i] = str[i];
            }
        }

        return textBuffer;
    }

    if (nn::swkbd::IsDecideCancelButton(nullptr)) {
        nn::swkbd::DisappearInputForm();
    }

    return nullptr;
}

void swkbdDrawTV(void)
{
    if (swkbdIsOpened()) {
        nn::swkbd::DrawTV();
    }
}

void swkbdDrawDRC(void)
{
    if (swkbdIsOpened()) {
        nn::swkbd::DrawDRC();
    }
}

}
