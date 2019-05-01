







this "tips" doc is comprised of excerpts from
	https://wiki.archlinux.org/index.php/Xorg
	and
	https://wiki.archlinux.org/index.php/ConsoleKit
	and
	https://wiki.archlinux.org/index.php/Xinit








========== ck-launch-session Configuration ==========

  To launch an X session with ConsoleKit, append the following to the exec statement in ~/.xinitrc e.g.:
        exec ck-launch-session dbus-launch --sh-syntax --exit-with-session openbox-session
  This starts Openbox with proper environment variables so it and its children are able to use ConsoleKit.

  Display managers like GDM, LXDM and SLiM start ConsoleKit automatically with each X session.

  Note:
    Do not nest ConsoleKit sessions by calling one from another, or you will break ConsoleKit.
    In particular, since SLiM reads ~/.xinitrc, you should make sure not to run ck-launch-session there.


  If you are not using a display manager, but starting your window manager via the startx command, or from inittab.
  If ConsoleKit is not working (i.e. ck-list-sessions command showing active = FALSE),
  you should start your window manager using the bash_profile method: Xinit#Autostart X at login. 

  If you are using a desktop environment suite (Mate, Xfce, LXDE...) rather than a standalone window manager...
  ...instead of SLiM, you might wish to consider using a different "login manager"; some of the alternative managers
  also provide features like session state management and power management. For instance, lxdm-consolekit,
  LightDM, mdm-display-manager, mate-session-manager-upower plus mate-power-manager-upower...





========== Running several applications from ~/.xinitrc ===========

  If several applications are to be executed from ~/.xinitrc, not all of these will have ConsoleKit environment variables set.
  In the following example, only children of Compiz will be able to properly use ConsoleKit, but children of xterm will not.
      ~/.xinitrc
          xterm &
          exec ck-launch-session compiz ccp
  Typically, this can be an issue when for example using Compiz standalone and some other application launchers, (gnome-do, kupfer, gmrun, xbindkeys, etc.)
  since children of the application launcher will not be able to use ConsoleKit. A dirty workaround is to have the entire session started by a second script,
  e.g. ~/.xstart. Do not forget dbus-launch, it is likely that you will need it too:
     ~/.xinitrc
          exec ck-launch-session dbus-launch ~/.xstart
     ~/.xstart
          xterm &
          thunar &
          compiz ccp
  Do not forget to make ~/.xstart executable:
      $ chmod +x ~/.xstart
  To see whether everything is started correctly:
      $ ck-list-sessions
  It should show at least one session





========== Rootless Xorg ============

  Xorg may run with standard user privileges with the help of systemd-logind(8)
  In order for the xserver to be able to run as a systemd-logind session controller it must be started inside a (pam) user-session.
  The requirements for this are:
      ~~ Starting X via xinit; display managers are not supported
      ~~ Kernel mode setting; implementations in proprietary display drivers fail auto-detection and require manually
         setting needs_root_rights = no in /etc/X11/Xwrapper.config.
  If you do not fit these requirements, re-enable root rights in /etc/X11/Xwrapper.config:
         needs_root_rights = yes
  See Also:   man Xorg.wrap(1)



============= TROUBLESHOOTING ==============
     General
     If a problem occurs, view the log stored in either /var/log/ or, for the rootless X default since v1.16, in ~/.local/share/xorg/

problem: X clients started with "su" fail

  If you are getting "Client is not authorized to connect to server", try adding the line:
         session        optional        pam_xauth.so
  to /etc/pam.d/su and /etc/pam.d/su-l. pam_xauth will then properly set environment variables and handle xauth keys. 

problem:    X failed to start: Keyboard initialization failed

  If the filesystem (specifically /tmp) is full, startx will fail. /var/log/Xorg.0.log will end with:
       (EE) Error compiling keymap (server-0)
       . . .
       (II) AIGLX: Suspending AIGLX clients for VT switch
  Make some free space on the relevant filesystem and X will start. 

problem: SocketCreateListener error

  If X terminates with error message "SocketCreateListener() failed", you may need to delete socket files in /tmp/.X11-unix.
  This may happen if you have previously run Xorg as root (e.g. to generate an xorg.conf).

problem: Invalid MIT-MAGIC-COOKIE-1 key when trying to run a program as root
  This error means that only the current user has access to the X server. The solution is to give access to root:
       $ xhost +si:localuser:root
  That line can also be used to give access to X to a different user than root. 

problem: Consolekit blocks active TTY
  Configure init to start ConsoleKit on an unused TTY, for example:
       /usr/bin/openvt -c 63 -f -- /usr/sbin/console-kit-daemon --no-daemon &
	See [ https://bugs.freedesktop.org/show_bug.cgi?id=29920 ] for details.

problem: Inactive session when launching X on same TTY
  Specify the keeptty flag to startx or xinit [5], for example:
       startx -- -keeptty
	See also Xorg#Broken redirection

==== Xorg broken redirection ===

problem: Broken redirection

  When user Xorg logs are stored in ~/.local/share/xorg/Xorg.log, they do not include the output from the X session.
  To re-enable redirection, start X with the -keeptty flag:
     exec startx -- -keeptty > ~/.xorg.log 2>&1
  Or copy /etc/X11/xinit/xserverrc to ~/.xserverrc, and append -keeptty

