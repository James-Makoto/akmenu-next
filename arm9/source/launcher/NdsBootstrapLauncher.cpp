/*
    Copyright (C) 2024 lifehackerhansol

    SPDX-License-Identifier: GPL-3.0-or-later
*/

#include <sys/stat.h>
#include <unistd.h>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include <fat.h>

#include <nds/ndstypes.h>

#include "../cheatwnd.h"
#include "../dsrom.h"
#include "../flags.h"
#include "../inifile.h"
#include "../mainlist.h"
#include "../systemfilenames.h"
#include "../language.h"
#include "../ui/msgbox.h"
#include "../ui/progresswnd.h"
#include "ILauncher.h"
#include "NdsBootstrapLauncher.h"
#include "nds_loader_arm9.h"

bool NdsBootstrapLauncher::prepareCheats() {
    u32 gameCode, crc32;

    if (cCheatWnd::romData(mRomPath, gameCode, crc32)) {
        FILE* cheatDb = fopen(SFN_CHEATS, "rb");
        if (!cheatDb) goto cheat_failed;
        long cheatOffset;
        size_t cheatSize;
        if (cCheatWnd::searchCheatData(cheatDb, gameCode, crc32, cheatOffset, cheatSize)) {
            cCheatWnd chtwnd((256) / 2, (192) / 2, 100, 100, NULL, mRomPath);

            chtwnd.parse(mRomPath);
            chtwnd.writeCheatsToFile("/_nds/nds-bootstrap/cheatData.bin");
            FILE* cheatData = fopen("/_nds/nds-bootstrap/cheatData.bin", "rb");
            if (cheatData) {
                u32 check[2];
                fread(check, 1, 8, cheatData);
                fclose(cheatData);
                // TODO: Delete file, if above 0x8000 bytes
                if (check[1] == 0xCF000000) goto cheat_failed;
            }
        } else {
            fclose(cheatDb);
            goto cheat_failed;
        }
        fclose(cheatDb);
    }

    return true;

cheat_failed:
    // Remove cheat bin if exists
    if (access("/_nds/nds-bootstrap/cheatData.bin", F_OK) == 0) {
        remove("/_nds/nds-bootstrap/cheatData.bin");
    }

    return false;
}

bool NdsBootstrapLauncher::prepareIni() {
    CIniFile ini;
    tDSiHeader header;
    char sfnSrl[62];
	char sfnPub[62];
    char sfnPrv[62];
    _romInfo.MayBeDSRom(mRomPath);
    bool dsiWare = _romInfo.isDSiWare();
    
    ini.SetString("NDS-BOOTSTRAP", "NDS_PATH", mRomPath);
    // check for DSiWare
    if(dsiWare){
        //TODO create pub & prv savwe
        #ifdef __DSIMODE__
        /*fatGetAliasPath("sd:/", mRomPath, sfnSrl);
        fatGetAliasPath("sd:/", pubPath, sfnPub);
        fatGetAliasPath("sd:/", prvPath, sfnPrv);
        ini.SetString("NDS-BOOTSTRAP", "APP_PATH", sfnSrl);
        ini.SetString("NDS-BOOTSTRAP", "SAV_PATH", sfnPub);
        ini.SetString("NDS-BOOTSTRAP", "PRV_PATH", sfnPrv);*/
        #else
        //TODO flashcart
        #endif
    }
    else{
        ini.SetString("NDS-BOOTSTRAP", "SAV_PATH", mSavePath);
    }

    ini.SaveIniFile("/_nds/nds-bootstrap.ini");

    return true;
}

bool NdsBootstrapLauncher::launchRom(std::string romPath, std::string savePath, u32 flags,
                                     u32 cheatOffset, u32 cheatSize) {
    const char ndsBootstrapPath[] = SD_ROOT_0 "/_nds/nds-bootstrap-release.nds";
    const char ndsBootstrapPathNightly[] = SD_ROOT_0 "/_nds/nds-bootstrap-nightly.nds";
    const char ndsBootstrapCheck[] = SD_ROOT_0 "/_nds/pagefile.sys";
    bool useNightly = false;

    //has the user used nds-bootstrap before?
    if(access(ndsBootstrapCheck, F_OK) != 0){
        akui::messageBox(NULL, LANG("nds bootstrap", "firsttimetitle"), LANG("nds bootstrap", "firsttime"), MB_OK);
    }

    progressWnd().setTipText("Initializing nds-bootstrap...");
    progressWnd().show();
    progressWnd().setPercent(0);

    //Check which nds-bootstrap version has been selected
    if(gs().nightly){
        if(access(ndsBootstrapPathNightly, F_OK) != 0){
            progressWnd().hide();
            printLoaderNotFound(ndsBootstrapPathNightly);
            return false;
        }
        else{
            useNightly = true;
        }
    }
    else{
        if(access(ndsBootstrapPath, F_OK) != 0){
            progressWnd().hide();
            printLoaderNotFound(ndsBootstrapPath);
            return false;
        }
        else{
            useNightly = false;
        }
    }

    std::vector<const char*> argv;

    mRomPath = romPath;
    mSavePath = savePath;
    mFlags = flags;

    // Create the nds-bootstrap directory if it doesn't exist
    if (access("/_nds/nds-bootstrap/", F_OK) != 0) {
        mkdir("/_nds/nds-bootstrap/", 0777);
    }
    progressWnd().setPercent(25);

    // Setup argv to launch nds-bootstrap                             
    if(!useNightly){
        argv.push_back(ndsBootstrapPath);
    }
    else{
        argv.push_back(ndsBootstrapPathNightly);
    }

    progressWnd().setTipText("Loading usrcheat.dat...");
    progressWnd().setPercent(50);
    // Prepare cheat codes if enabled
    if (flags & PATCH_CHEATS) {
        if (!prepareCheats()) {
            return false;
        }
    }
    progressWnd().setTipText("Initializing nds-bootstrap...");
    progressWnd().setPercent(75);

    // Setup nds-bootstrap INI parameters
    if (!prepareIni()) return false;
    progressWnd().setPercent(100);

    // Launch
    eRunNdsRetCode rc = runNdsFile(argv[0], argv.size(), &argv[0]);
    if (rc == RUN_NDS_OK) return true;

    return false;
}
