/* SLiM - Simple Login Manager
   Copyright (C) 1997, 1998 Per Liden
   Copyright (C) 2004-06 Simone Rota <sip@varlock.com>
   Copyright (C) 2004-06 Johannes Winkelmann <jw@tks6.net>

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
*/

#include <sstream>
#include <poll.h>
#include "panel.h"
#include "capslock.h"
#include <iostream>   /*     skidoo  late late addition    */
// #i nclude <cstdio>  // late addition ~~  needed for logging?   // not helpful
#include "log.h"


using namespace std;

Panel::Panel(Display* dpy, int scr, Window root, Cfg* config, const string& themedir) {
    // Set display
    Dpy = dpy;
    Scr = scr;
    Root = root;
    cfg = config;

    session = "";

    // Init GC
    XGCValues gcv;
    unsigned long gcm = GCForeground|GCBackground|GCGraphicsExposures;
    gcv.foreground = GetColor("black");
    gcv.background = GetColor("white");
    gcv.graphics_exposures = False;
    TextGC = XCreateGC(Dpy, Root, gcm, &gcv);

    font = XftFontOpenName(Dpy, Scr, cfg->getOption("input_font").c_str());
    welcomefont = XftFontOpenName(Dpy, Scr, cfg->getOption("welcome_font").c_str());
    introfont = XftFontOpenName(Dpy, Scr, cfg->getOption("intro_font").c_str());
    enterfont = XftFontOpenName(Dpy, Scr, cfg->getOption("username_font").c_str());
    msgfont = XftFontOpenName(Dpy, Scr, cfg->getOption("msg_font").c_str());

    Visual* visual = DefaultVisual(Dpy, Scr);
    Colormap colormap = DefaultColormap(Dpy, Scr);
    // NOTE: using XftColorAllocValue() would be a better solution.
    XftColorAllocName(Dpy, visual, colormap, cfg->getOption("input_color").c_str(), &inputcolor);
    XftColorAllocName(Dpy, visual, colormap, cfg->getOption("input_shadow_color").c_str(), &inputshadowcolor);
    XftColorAllocName(Dpy, visual, colormap, cfg->getOption("welcome_color").c_str(), &welcomecolor);
    XftColorAllocName(Dpy, visual, colormap, cfg->getOption("welcome_shadow_color").c_str(), &welcomeshadowcolor);
    XftColorAllocName(Dpy, visual, colormap, cfg->getOption("username_color").c_str(), &entercolor);
    XftColorAllocName(Dpy, visual, colormap, cfg->getOption("username_shadow_color").c_str(), &entershadowcolor);
    XftColorAllocName(Dpy, visual, colormap, cfg->getOption("msg_color").c_str(), &msgcolor);
    XftColorAllocName(Dpy, visual, colormap, cfg->getOption("msg_shadow_color").c_str(), &msgshadowcolor);
    XftColorAllocName(Dpy, visual, colormap, cfg->getOption("intro_color").c_str(), &introcolor);
    XftColorAllocName(Dpy, visual, colormap, cfg->getOption("session_color").c_str(), &sessioncolor);
    XftColorAllocName(Dpy, visual, colormap, cfg->getOption("session_shadow_color").c_str(), &sessionshadowcolor);
    XftColorAllocName(Dpy, visual, colormap, cfg->getOption("background_color").c_str(), &backgroundcolor);

    // Load properties from config / theme
    input_name_x = cfg->getIntOption("input_name_x");
    input_name_y = cfg->getIntOption("input_name_y");
    input_pass_x = cfg->getIntOption("input_pass_x");
    input_pass_y = cfg->getIntOption("input_pass_y");
    inputShadowXOffset = cfg->getIntOption("input_shadow_xoffset");
    inputShadowYOffset = cfg->getIntOption("input_shadow_yoffset");

    if (input_pass_x < 0 || input_pass_y < 0){  // single inputbox mode
        input_pass_x = input_name_x;
        input_pass_y = input_name_y;
    }

    // Load  background image and panel image
    Image* bg = new Image();
    string bgstyle = cfg->getOption("background_style");
    if (bgstyle != "solidcolor") {                        //    && != "color"
        string bgimgsrc = themedir + "/background.png";
        bool bgsrcloaded = bg->Read(bgimgsrc.c_str());
        if (!bgsrcloaded) {
            bgimgsrc = themedir + "/background.jpg";  // yah
            bgsrcloaded = bg->Read(bgimgsrc.c_str());
            if (!bgsrcloaded){
                std::cout << "SLiM: could not load the specified background image for theme '"
                     << basename((char*)themedir.c_str()) << "'" << std::endl;
                //  skidoo   No! just fallback to solidcolor
                //exit(ERR_EXIT);
                bgstyle = "solidcolor";
            }
        }
    }

    string hexval = cfg->getOption("background_color");
    hexval = hexval.substr(1,6);
    if (hexval == ""){       //  skidoo intended to assert and provide visual feedback regarding error condition
        hexval = "ff0000";   //  but the value will NEVER be empty... and a forced color would probably be an annoyance
    }

    if (bgstyle == "stretch" || bgstyle == "stretched") {    // skidoo  late change (undocumented)
        bg->Resize(XWidthOfScreen(ScreenOfDisplay(Dpy, Scr)), XHeightOfScreen(ScreenOfDisplay(Dpy, Scr)));
    } else if (bgstyle == "tile" || bgstyle == "tiled") {
        bg->Tile(XWidthOfScreen(ScreenOfDisplay(Dpy, Scr)), XHeightOfScreen(ScreenOfDisplay(Dpy, Scr)));
    } else if (bgstyle == "center" || bgstyle == "centered") {
        bg->Center(  XWidthOfScreen(ScreenOfDisplay(Dpy, Scr)),
                     XHeightOfScreen(ScreenOfDisplay(Dpy, Scr)), hexval.c_str() );
    } else {          // solidcolor or error
        bg->Center(XWidthOfScreen(ScreenOfDisplay(Dpy, Scr)),
                   XHeightOfScreen(ScreenOfDisplay(Dpy, Scr)), hexval.c_str());
        //   ==================================== TEST:    DO NOTHING IN THIS CASE
    }

    string panelpng = themedir + "/panel.png";
    pimage = new Image;
    bool pimloaded = pimage->Read(panelpng.c_str());
    if (!pimloaded) {
        panelpng = themedir + "/panel.jpg";   // yah
        pimloaded = pimage->Read(panelpng.c_str());
        if (!pimloaded) {
            std::cout << "SLiM: could not load panel image for theme '"
                 << basename((char*)themedir.c_str()) << "'" << std::endl;
            //exit(ERR_EXIT);    // conditionally call Merge() instead
        }
    }

    string cfgX = cfg->getOption("input_panel_x");
    string cfgY = cfg->getOption("input_panel_y");
    if (pimloaded) {
        X = Cfg::absolutepos(cfgX, XWidthOfScreen(ScreenOfDisplay(Dpy, Scr)), pimage->Width());
        Y = Cfg::absolutepos(cfgY, XHeightOfScreen(ScreenOfDisplay(Dpy, Scr)), pimage->Height());
        // Merge image into background
        pimage->Merge(bg, X, Y);
        PanelPixmap = pimage->createPixmap(Dpy, Scr, Root);
    }

    delete bg;   //    pimage not deleted here
}

Panel::~Panel() {
	Visual* visual = DefaultVisual(Dpy, Scr);
	Colormap colormap = DefaultColormap(Dpy, Scr);

	XftColorFree(Dpy, visual, colormap, &inputcolor);
	XftColorFree(Dpy, visual, colormap, &inputshadowcolor);
	XftColorFree(Dpy, visual, colormap, &welcomecolor);
	XftColorFree(Dpy, visual, colormap, &welcomeshadowcolor);
	XftColorFree(Dpy, visual, colormap, &entercolor);
	XftColorFree(Dpy, visual, colormap, &entershadowcolor);
	XftColorFree(Dpy, visual, colormap, &msgcolor);
	XftColorFree(Dpy, visual, colormap, &msgshadowcolor);
	XftColorFree(Dpy, visual, colormap, &introcolor);
	XftColorFree(Dpy, visual, colormap, &sessioncolor);
	XftColorFree(Dpy, visual, colormap, &sessionshadowcolor);
	XftColorFree(Dpy, visual, colormap, &backgroundcolor);

	XFreeGC(Dpy, TextGC);
	XftFontClose(Dpy, font);
	XftFontClose(Dpy, msgfont);
	XftFontClose(Dpy, introfont);
	XftFontClose(Dpy, welcomefont);
	XftFontClose(Dpy, enterfont);
    delete pimage;    //  howdy      hmm Pane lPixmap is never destroyed
}

void Panel::OpenPanel(const string& themedir) {
    string zsrcimg = themedir + "/panel.png";
    //zpimage = new Image;
    Image* zpimage = new Image();
    bool zpimloaded = zpimage->Read(zsrcimg.c_str());
    if (!zpimloaded) {
        zsrcimg = themedir + "/panel.jpg";
        zpimloaded = zpimage->Read(zsrcimg.c_str());
    }

    //std::cout << zsrcimg << " ~~~~~  " << themedir << std::endl;

    if (!zpimloaded) {
        Win = XCreateSimpleWindow(Dpy, Root, X, Y, 480, 260,
                              0, GetColor("white"), GetColor("white"));
    } else {
        Win = XCreateSimpleWindow(Dpy, Root, X, Y, zpimage->Width(), zpimage->Height(),
                              0, GetColor("white"), GetColor("white"));
        XSetWindowBackgroundPixmap(Dpy, Win, PanelPixmap);
    }

    // Events
    XSelectInput(Dpy, Win, ExposureMask | KeyPressMask);

    // Show window
    XMapWindow(Dpy, Win);
    XMoveWindow(Dpy, Win, X, Y);

    // Grab keyboard
    XGrabKeyboard(Dpy, Win, False, GrabModeAsync, GrabModeAsync, CurrentTime);

    XFlush(Dpy);
    delete zpimage;
}

void Panel::ClosePanel() {
    XUngrabKeyboard(Dpy, CurrentTime);
    XUnmapWindow(Dpy, Win);      //  conditionally, it may not have been mapped
    XDestroyWindow(Dpy, Win);
    XFlush(Dpy);
}

void Panel::ClearPanel() {
    session = "";
    Reset();
    XClearWindow(Dpy, Root);
    XClearWindow(Dpy, Win);
    Cursor(SHOW);
    ShowText();
    XFlush(Dpy);
}


// skidoo    currently passing a hardcoded value.
// Consider adding a cfg option to specify timeout duration
//
//    TODO   just set conditional text and call  Panel::Message(text)
//                ahhhhhhh  timeout
void Panel::WarnCapslock(int timeout) {
    string message;
    XGlyphInfo extents;

    if (CapsLock::checkCapslock(Dpy)) {
        message = cfg->getOption("password_feedback_capslock");
    } else {
        message = cfg->getOption("password_feedback_msg");
    }

    XftDraw *draw = XftDrawCreate(Dpy, Win,
        DefaultVisual(Dpy, Scr), DefaultColormap(Dpy, Scr));
        XftTextExtentsUtf8(Dpy, msgfont, reinterpret_cast<const XftChar8*>(message.c_str()),
                  message.length(), &extents);

    int shadowXOffset = cfg->getIntOption("msg_shadow_xoffset");
    int shadowYOffset = cfg->getIntOption("msg_shadow_yoffset");

    OnExpose();
    SlimDrawString8(draw, &msgcolor, msgfont, int(30), int(30), message,
               &msgshadowcolor, shadowXOffset, shadowYOffset);

    XFlush(Dpy);
    sleep(timeout);
    ResetPasswd();
    OnExpose();
    SlimDrawString8(draw, &msgcolor, msgfont, int(30), int(30), message,
               &msgshadowcolor, shadowXOffset, shadowYOffset);
    XSync(Dpy, True);
    XftDrawDestroy(draw);
}


void Panel::Message(const string& text) {
    string cfgX, cfgY;
    XGlyphInfo extents;

    XftDraw *draw = XftDrawCreate(Dpy, Root, DefaultVisual(Dpy, Scr), DefaultColormap(Dpy, Scr));
    XftTextExtentsUtf8(Dpy, msgfont, reinterpret_cast<const XftChar8*>(text.c_str()),
               text.length(), &extents);
    cfgX = cfg->getOption("msg_x");   // 30 if not specified in themefile
    cfgY = cfg->getOption("msg_y");
    int shadowXOffset = cfg->getIntOption("msg_shadow_xoffset");
    int shadowYOffset = cfg->getIntOption("msg_shadow_yoffset");
    int msg_x = Cfg::absolutepos(cfgX, XWidthOfScreen(ScreenOfDisplay(Dpy, Scr)), extents.width);
    int msg_y = Cfg::absolutepos(cfgY, XHeightOfScreen(ScreenOfDisplay(Dpy, Scr)), extents.height);

    SlimDrawString8 (draw, &msgcolor, msgfont, msg_x, msg_y, text,
               &msgshadowcolor, shadowXOffset, shadowYOffset);
    XFlush(Dpy);
    XftDrawDestroy(draw);
}

void Panel::Error(const string& text, const string& themedir) {
    ClosePanel();
    Message(text);
    sleep(ERROR_DURATION);   // 3, per const.h
    OpenPanel(themedir);
    ClearPanel();
}


unsigned long Panel::GetColor(const char* colorname) {
    XColor color;
    XWindowAttributes attributes;

    XGetWindowAttributes(Dpy, Root, &attributes);
    color.pixel = 0;

    if(!XParseColor(Dpy, attributes.colormap, colorname, &color))
        logStream << "SLiM: can't parse color " << colorname << endl;
    else if(!XAllocColor(Dpy, attributes.colormap, &color))
        logStream << "SLiM: can't allocate color " << colorname << endl;

    return color.pixel;
}

void Panel::Cursor(int visible) {
    if(cfg->getOption("input_hidecursor") == "true") {
        return;
    }

    const char* text = NULL;
    int xx = 0, yy = 0, y2 = 0, cheight = 0;
    const char* txth = "Wj";  // used to calc cursor height

    switch(field) {
        case Get_Passwd:
            text = HiddenPasswdBuffer.c_str();
            xx = input_pass_x;
            yy = input_pass_y;
            break;

        case Get_Name:
            text = NameBuffer.c_str();
            xx = input_name_x;
            yy = input_name_y;
            break;
    }

    XGlyphInfo extents;
    XftTextExtentsUtf8(Dpy, font, (XftChar8*)txth, strlen(txth), &extents);
    cheight = extents.height;
    y2 = yy - extents.y + extents.height;
    XftTextExtentsUtf8(Dpy, font, (XftChar8*)text, strlen(text), &extents);
    if(cfg->getOption("input_center_text") == "true") {
        xx += extents.width / (double) 2;       // spec double is a late change
    } else {
        xx += extents.width;
    }

    if(visible == SHOW) {
        XSetForeground(Dpy, TextGC, GetColor(cfg->getOption("input_color").c_str()));
        XDrawLine(Dpy, Win, TextGC, xx+1, yy-cheight, xx+1, y2);
    } else {
        XClearArea(Dpy, Win, xx+1, yy-cheight, 1, y2-(yy-cheight)+1, false);
    }
}

void Panel::EventHandler(const Panel::FieldType& curfield) {
    XEvent event;
    field=curfield;
    bool loop = true;
    OnExpose();

    struct pollfd x11_pfd = {0};
    x11_pfd.fd = ConnectionNumber(Dpy);
    x11_pfd.events = POLLIN;

    while(loop) {
        if(XPending(Dpy) || poll(&x11_pfd, 1, -1) > 0) {
            while(XPending(Dpy)) {
                XNextEvent(Dpy, &event);
                switch(event.type) {
                    case Expose:
                        OnExpose();
                        break;

                    case KeyPress:
                        loop=OnKeyPress(event);
                        break;
                }
            }
        }
    }

    return;
}

void Panel::OnExpose(void) {
    XftDraw *draw = XftDrawCreate(Dpy, Win, DefaultVisual(Dpy, Scr), DefaultColormap(Dpy, Scr));
    XClearWindow(Dpy, Win);

    if (input_pass_x != input_name_x || input_pass_y != input_name_y){
        SlimDrawString8 (draw, &inputcolor, font, input_name_x, input_name_y,
                         NameBuffer,
                         &inputshadowcolor,
                         inputShadowXOffset, inputShadowYOffset);
        SlimDrawString8 (draw, &inputcolor, font, input_pass_x, input_pass_y,
                         HiddenPasswdBuffer,
                         &inputshadowcolor,
                         inputShadowXOffset, inputShadowYOffset);
    } else {  //single input mode
        switch(field) {
            case Get_Passwd:
                SlimDrawString8 (draw, &inputcolor, font,
                                 input_pass_x, input_pass_y,
                                 HiddenPasswdBuffer,
                                 &inputshadowcolor,
                                 inputShadowXOffset, inputShadowYOffset);
                break;
            case Get_Name:
                SlimDrawString8 (draw, &inputcolor, font,
                                 input_name_x, input_name_y,
                                 NameBuffer,
                                 &inputshadowcolor,
                                 inputShadowXOffset, inputShadowYOffset);
                break;
        }
    }

    XftDrawDestroy (draw);
    Cursor(SHOW);
    ShowText();
}

bool Panel::OnKeyPress(XEvent& event) {
    char ascii;
    KeySym keysym;
    XComposeStatus compstatus;
    int xx = 0;
    int yy = 0;
    string text;
    string formerString = "";
    string ssmessage;

    XLookupString(&event.xkey, &ascii, 1, &keysym, &compstatus);
    switch(keysym){
        case XK_F1:
            SwitchSession();
            return true;

        case XK_F11:
            system(cfg->getOption("screenshot_cmd").c_str());  // howdy    not sanitized
            ssmessage = cfg->getOption("screenshot_feedback_msg");
            Message(ssmessage);  // remains onscreen indefinitely ~~ consider using Panel::Error(string,path) instead
            return true;

        case XK_Return:
        case XK_KP_Enter:
            if (field==Get_Name){
                // Don't allow an empty username
                if (NameBuffer.empty()) return true;

                if (NameBuffer==CONSOLE_STR){
                    action = Console;
                } else if (NameBuffer==HALT_STR){
                    action = Halt;
                } else if (NameBuffer==IMA_B_ROOOT_STR){
                    action = Exit;  /* Halt; */    // Hmmm... Panel::Reset()
                } else if (NameBuffer==REBOOT_STR){
                    action = Reboot;
                } else if (NameBuffer==SUSPEND_STR){
                    action = Suspend;
                } else if (NameBuffer==EXIT_STR){
                    action = Exit;
                } else{
                    action = Login;
                }
            }
            return false;
        default:
            break;
    }

    Cursor(HIDE);
    switch(keysym){
        case XK_Delete:
        case XK_BackSpace:
            switch(field) {
                case GET_NAME:
                    if (! NameBuffer.empty()){
                        formerString=NameBuffer;
                        NameBuffer.erase(--NameBuffer.end());
                    }
                    break;
                case GET_PASSWD:
                    if (! PasswdBuffer.empty()){
                        formerString=HiddenPasswdBuffer;
                        PasswdBuffer.erase(--PasswdBuffer.end());
                        HiddenPasswdBuffer.erase(--HiddenPasswdBuffer.end());
                    }
                    break;
            }
            break;

        case XK_w:
        case XK_u:
            if (reinterpret_cast<XKeyEvent&>(event).state & ControlMask) {
                switch(field) {
                    case Get_Passwd:
                        formerString = HiddenPasswdBuffer;
                        HiddenPasswdBuffer.clear();
                        PasswdBuffer.clear();
                        break;

                    case Get_Name:
                        formerString = NameBuffer;
                        NameBuffer.clear();
                        break;
                }
                break;
            }
            // Deliberate fall-through

        default:
            if (isprint(ascii) && (keysym < XK_Shift_L || keysym > XK_Hyper_R)){
                switch(field) {
                    case GET_NAME:
                        formerString=NameBuffer;
                        if (NameBuffer.length() < INPUT_MAXLENGTH_NAME-1){
                            NameBuffer.append(&ascii,1);
                        }
                        break;
                    case GET_PASSWD:
                        formerString=HiddenPasswdBuffer;
                        if (PasswdBuffer.length() < INPUT_MAXLENGTH_PASSWD-1){
                            PasswdBuffer.append(&ascii,1);
                            HiddenPasswdBuffer.append("*");
                        }
                    break;
                }
            }
            break;
    }

    XGlyphInfo extents;
    XftDraw *draw = XftDrawCreate(Dpy, Win, DefaultVisual(Dpy, Scr), DefaultColormap(Dpy, Scr));

    switch(field) {
        case Get_Name:
            text = NameBuffer;
            xx = input_name_x;
            yy = input_name_y;
            break;

        case Get_Passwd:
            text = HiddenPasswdBuffer;
            xx = input_pass_x;
            yy = input_pass_y;
            break;
    }

    if (!formerString.empty()){
        const char* txth = "Wj"; // get proper maximum height ?
        XftTextExtentsUtf8(Dpy, font, reinterpret_cast<const XftChar8*>(txth), strlen(txth), &extents);
        int maxHeight = extents.height;

        XftTextExtentsUtf8(Dpy, font, reinterpret_cast<const XftChar8*>(formerString.c_str()),
                        formerString.length(), &extents);
        int maxLength = extents.width;
        int centeringOffset = 0;
   	if(cfg->getOption("input_center_text") == "true"){
		centeringOffset = maxLength / (double) 2;   // spec double is a late change
	}
        XClearArea(Dpy, Win, xx-3-centeringOffset, yy-maxHeight-3, maxLength+6, maxHeight+6, false);
    }

    if (!text.empty()) {
        int centeringOffset = 0;

	if(cfg->getOption("input_center_text") == "true"){
		XftTextExtentsUtf8(Dpy, font, reinterpret_cast<const XftChar8*>(text.c_str()), text.length(), &extents);
        int maxLength = extents.width;
 		centeringOffset = maxLength / (double) 2;      // spec double is a late change
	}
	SlimDrawString8 (draw, &inputcolor, font, xx-centeringOffset, yy,
                         text,
                         &inputshadowcolor,
                         inputShadowXOffset, inputShadowYOffset);
    }

    XftDrawDestroy (draw);
    Cursor(SHOW);
    return true;
}

// Draw welcome and "enter username" message
void Panel::ShowText(){
    string cfgX, cfgY;
    XGlyphInfo extents;

    bool singleInputMode =
    input_name_x == input_pass_x &&
    input_name_y == input_pass_y;

    XftDraw *draw = XftDrawCreate(Dpy, Win, DefaultVisual(Dpy, Scr), DefaultColormap(Dpy, Scr));

    // Read (and substitute vars in) the welcome message
    welcome_message = cfg->getWelcomeMessage();    //   "Press F1 to toggle sessions"
    //intro_message = cfg->getOption("intro_msg");     // currently unused

    XftTextExtentsUtf8(Dpy, welcomefont, (XftChar8*)welcome_message.c_str(),
                    strlen(welcome_message.c_str()), &extents);
    cfgX = cfg->getOption("welcome_x");
    cfgY = cfg->getOption("welcome_y");
    int shadowXOffset = cfg->getIntOption("welcome_shadow_xoffset");
    int shadowYOffset = cfg->getIntOption("welcome_shadow_yoffset");

    welcome_x = Cfg::absolutepos(cfgX, pimage->Width(), extents.width);
    welcome_y = Cfg::absolutepos(cfgY, pimage->Height(), extents.height);
    if (welcome_x >= 0 && welcome_y >= 0) {
        SlimDrawString8 (draw, &welcomecolor, welcomefont,
                         welcome_x, welcome_y,
                         welcome_message,
                         &welcomeshadowcolor, shadowXOffset, shadowYOffset);
    }

    /* Enter username-password message */
    string msg;
    if (!singleInputMode|| field == Get_Passwd ) {
        msg = cfg->getOption("password_msg");
        XftTextExtentsUtf8(Dpy, enterfont, (XftChar8*)msg.c_str(), strlen(msg.c_str()), &extents);
        cfgX = cfg->getOption("password_x");
        cfgY = cfg->getOption("password_y");
        int shadowXOffset = cfg->getIntOption("username_shadow_xoffset");
        int shadowYOffset = cfg->getIntOption("username_shadow_yoffset");       // minus to end of line is a late change
        password_x = Cfg::absolutepos(cfgX, pimage->Width(), extents.width) - extents.width / (double) 2;
        password_y = Cfg::absolutepos(cfgY, pimage->Height(), extents.height);
        if (password_x >= 0 && password_y >= 0){
            SlimDrawString8 (draw, &entercolor, enterfont, password_x, password_y,
                             msg, &entershadowcolor, shadowXOffset, shadowYOffset);
        }
    }
    if (!singleInputMode|| field == Get_Name ) {
        msg = cfg->getOption("username_msg");
        XftTextExtentsUtf8(Dpy, enterfont, (XftChar8*)msg.c_str(), strlen(msg.c_str()), &extents);
        cfgX = cfg->getOption("username_x");
        cfgY = cfg->getOption("username_y");
        int shadowXOffset = cfg->getIntOption("username_shadow_xoffset");
        int shadowYOffset = cfg->getIntOption("username_shadow_yoffset");      // minus to end of line is a late change
        username_x = Cfg::absolutepos(cfgX, pimage->Width(), extents.width) - extents.width / (double) 2;
        username_y = Cfg::absolutepos(cfgY, pimage->Height(), extents.height);
        if (username_x >= 0 && username_y >= 0){
            SlimDrawString8 (draw, &entercolor, enterfont, username_x, username_y,
                             msg, &entershadowcolor, shadowXOffset, shadowYOffset);
        }
    }
    XftDrawDestroy(draw);
}

string Panel::getSession() {
    return session;
}

// choose next available session type
void Panel::SwitchSession() {
    session = cfg->nextSession(session);
    if (session.size() > 0) {
        ShowSession();
    }
}

// Display session type on the screen
void Panel::ShowSession() {
	string msg_x, msg_y;
    XClearWindow(Dpy, Root);
    string currsession = cfg->getOption("session_msg") + " " + session;
    XGlyphInfo extents;

	sessionfont = XftFontOpenName(Dpy, Scr, cfg->getOption("session_font").c_str());

	XftDraw *draw = XftDrawCreate(Dpy, Root, DefaultVisual(Dpy, Scr), DefaultColormap(Dpy, Scr));
    XftTextExtentsUtf8(Dpy, sessionfont, reinterpret_cast<const XftChar8*>(currsession.c_str()),
                    currsession.length(), &extents);
    msg_x = cfg->getOption("session_x");
    msg_y = cfg->getOption("session_y");
    int x = Cfg::absolutepos(msg_x, XWidthOfScreen(ScreenOfDisplay(Dpy, Scr)), extents.width);
    int y = Cfg::absolutepos(msg_y, XHeightOfScreen(ScreenOfDisplay(Dpy, Scr)), extents.height);
    int shadowXOffset = cfg->getIntOption("session_shadow_xoffset");
    int shadowYOffset = cfg->getIntOption("session_shadow_yoffset");

    SlimDrawString8(draw, &sessioncolor, sessionfont, x, y,
                    currsession,
                    &sessionshadowcolor,
                    shadowXOffset, shadowYOffset);
    XFlush(Dpy);
    XftDrawDestroy(draw);
}


void Panel::SlimDrawString8(XftDraw *d, XftColor *color, XftFont *font,
                            int x, int y, const string& str,
                            XftColor* shadowColor, int xOffset, int yOffset)
{
    int calc_x = 0;
    int calc_y = 0;

	if (xOffset && yOffset) {
		XftDrawStringUtf8(d, shadowColor, font, x + xOffset + calc_x, y + yOffset + calc_y,
			reinterpret_cast<const FcChar8*>(str.c_str()), str.length());
	}
    XftDrawStringUtf8(d, color, font, x, y, reinterpret_cast<const FcChar8*>(str.c_str()), str.length());
}

Panel::ActionType Panel::getAction(void) const{
    return action;
}

void Panel::Reset(void){
    ResetName();
    ResetPasswd();
}

void Panel::ResetName(void){
    NameBuffer.clear();
}

void Panel::ResetPasswd(void){
    PasswdBuffer.clear();
    HiddenPasswdBuffer.clear();
}

void Panel::SetName(const string& name){
    NameBuffer=name;
    action = Login;
}

const string& Panel::GetName(void) const{
    return NameBuffer;
}

const string& Panel::GetPasswd(void) const{
    return PasswdBuffer;
}
