/* SLiM - Simple Login Manager
   Copyright (C) 2004-06 Simone Rota <sip@varlock.com>
   Copyright (C) 2004-06 Johannes Winkelmann <jw@tks6.net>
   Copyright (C) 2012    Nobuhiro Iwamatsu <iwamatsu@nigauri.org>

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
*/

#include "capslock.h"
#include <string.h>

CapsLock::CapsLock() {
}

int CapsLock::xkb_init(Display* dpy) {
    int xkb_opcode, xkb_event, xkb_error;
    int xkb_lmaj = XkbMajorVersion;
    int xkb_lmin = XkbMinorVersion;

    return XkbLibraryVersion( &xkb_lmaj, &xkb_lmin )
        && XkbQueryExtension( dpy, &xkb_opcode, &xkb_event, &xkb_error,
                   &xkb_lmaj, &xkb_lmin );
}

unsigned int CapsLock::xkb_mask_modifier( XkbDescPtr xkb, const char *name ) {
    int i;
    if( !xkb || !xkb->names )
        return 0;

    for( i = 0; i < XkbNumVirtualMods; i++ ) {
        char* modStr = XGetAtomName( xkb->dpy, xkb->names->vmods[i] );
        if( modStr != NULL && strcmp(name, modStr) == 0 ) {
            unsigned int mask;
            XkbVirtualModsToReal( xkb, 1 << i, &mask );
            return mask;
        }
    }
    return 0;
}


bool CapsLock::checkCapslock(Display *dpy) {
    unsigned int mask;

    if( !xkb_init(dpy) )
        return 0;

    unsigned n;
    XkbGetIndicatorState(dpy, XkbUseCoreKbd, &n);
    return (n & 0x01) == 1;
}

//     v---- the following, modified from numlock.cpp, are currently unused

unsigned int CapsLock::xkb_capslock_mask(Display* dpy) {
    XkbDescPtr xkb;

    xkb = XkbGetKeyboard( dpy, XkbAllComponentsMask, XkbUseCoreKbd );
    if( xkb != NULL ) {
        unsigned int mask = xkb_mask_modifier( xkb, "CapsLock" );
        XkbFreeKeyboard( xkb, 0, True );
        return mask;
    }
    return 0;
}

void CapsLock::control_capslock(Display *dpy, bool flag) {
    unsigned int mask;

    if( !xkb_init(dpy) )
        return;

    mask = xkb_capslock_mask(dpy);
    if( mask == 0 )
        return;

    if( flag == true )
        XkbLockModifiers ( dpy, XkbUseCoreKbd, mask, mask);
    else
        XkbLockModifiers ( dpy, XkbUseCoreKbd, mask, 0);
}

void CapsLock::setOn(Display *dpy) {
    control_capslock(dpy, true);
}

void CapsLock::setOff(Display *dpy) {
    control_capslock(dpy, false);
}
