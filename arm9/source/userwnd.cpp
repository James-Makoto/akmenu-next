/*
    userwnd.cpp
    Copyright (C) 2007 Acekard, www.acekard.com
    Copyright (C) 2007-2009 somebody
    Copyright (C) 2009 yellow wood goblin

    SPDX-License-Identifier: GPL-3.0-or-later
*/

#include "userwnd.h"
#include <sys/stat.h>
#include "../../share/memtool.h"
#include "globalsettings.h"
#include "inifile.h"
#include "stringtool.h"
#include "systemfilenames.h"
#include "unicode.h"
#include "windowmanager.h"

using namespace akui;

cUserWindow::cUserWindow() : cWindow(NULL, "UserWindow") {
    _px = _py = 0;
    _tx = _ty = _tw = _th = 0;
    _ux = _uy = 0;
    _userTextColor = 0;
    _userNameColor = 0;
    _showUserName = false;
    _size = cSize(1, 1);
    _position = cPoint(0, 0);
    _engine = GE_SUB;

    init();
}

void cUserWindow::init() {
    CIniFile ini(SFN_USER_CUSTOM);
    std::string pictureFilename = ini.GetString("custom picture", "file", "");
    if (pictureFilename != "") {
        struct stat st;
        if (0 == stat(pictureFilename.c_str(), &st)) {
            if (st.st_size <= 1024 * 10)  // 64 x 64 15bit bmp
                _userPicture = createBMP15FromFile(pictureFilename);
        }
    }
    _px = ini.GetInt("custom picture", "x", 0);
    _py = ini.GetInt("custom picture", "y", 0);

    dbg_printf("%s valid %d x=%d y=%d", pictureFilename.c_str(), _userPicture.valid(), _px, _py);
    _userText = ini.GetString("custom text", "text", "");
    _userTextColor = ini.GetInt("custom text", "color", 0);
    _tx = ini.GetInt("custom text", "x", 0);
    _ty = ini.GetInt("custom text", "y", 0);
    _tw = ini.GetInt("custom text", "w", 0);
    _th = ini.GetInt("custom text", "h", 0);

    _ux = ini.GetInt("user name", "x", _ux);
    _uy = ini.GetInt("user name", "y", _uy);
    _userNameColor = ini.GetInt("user name", "color", 0xFFFF);
    _showUserName = ini.GetInt("user name", "show", 0);
    _showCustomText = ini.GetInt("custom text", "show", 0);
    _showCustomPic = ini.GetInt("custom picture", "show", 0);
    _userName = unicode_to_local_string((u16*)PersonalData->name, PersonalData->nameLen, NULL);
}

void cUserWindow::draw() {
    if (_showCustomPic && _userPicture.valid()) {
        gdi().maskBlt(_userPicture.buffer(), _px, _py, _userPicture.width(), _userPicture.height(),
                      _engine);
    }

    if (_showCustomText && _userText != "") {
        gdi().setPenColor(_userTextColor, _engine);
        gdi().textOutRect(_tx, _ty, _tw, _th, _userText.c_str(), _engine);
    }

    if (_showUserName && _userName != "") {
        gdi().setPenColor(_userNameColor, _engine);
        gdi().textOut(_ux, _uy, _userName.c_str(), _engine);
    }
}
