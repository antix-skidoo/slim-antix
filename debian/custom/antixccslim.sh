#!/bin/bash
# File Name: antixccslim.sh
# Purpose: provide a GUI wherein a user can set slim login background
###  modified for slim v1.4.2

# Check xserver is running and user is root
[[ $DISPLAY ]] || { echo "There is no xserver running. Exiting..." ; exit 1 ; }
[ "$UID" !="0" ] || { yad --image "error" --title "!" --text "root (sudo) authorization required \! \n\nCannot continue." ; exit 1 ; }

[[ -e /usr/local/bin/yad ]] || { echo "yad (provided by 'antix-goodies' package) missing; cannot proceed"; exit 1; }
[[ -e /usr/bin/gtkdialog ]] || { echo "/usr/bin/gtkdialog missing; cannot proceed"; exit 1; }

if [ ! -w /etc/slim.conf ]; then
    yad "error" --title "!" --button=OK --text "/etc/slim.conf is missing or not writable\!\n\nunable to proceed"
    exit 1
fi

if [ $(grep current_theme /etc/slim.conf | awk '{print $2}') != "antiX" ]; then
    yad "error" --button=OK --title "!" --text "this background setter utility\n\
         is effective only when the currently-selected SLiM theme\n\
         (specified in /etc/slim.conf) is:\n current_theme  antiX"
    exit 1
fi

export SLIMBACKGROUND='
<window title="choose SLiM login screen background" icon="gnome-control-center" window-position="1">

<vbox>
  <chooser>
    <height>560</height><width>680</width>
    <variable>BACKGROUND</variable>
  </chooser>

  <hbox>
    <button>
     <label>"View"</label>
	<input file>"/usr/share/icons/Faenza-Cupertino-mini/actions/32/gtk-zoom-out.png"</input>
        <action>feh -g 320x240 "$BACKGROUND" </action>
    </button>

    <button>
    <label>"Commit"</label>
	<input file>"/usr/share/icons/Faenza-Cupertino-mini/actions/32/gtk-yes.png"</input>
        <action>cp -bv "$BACKGROUND" /usr/share/slim/themes/antiX/background.jpg</action>
        <action>yad --text "Done!"</action>
	<action>EXIT:close</action>
    </button>

    <button>
    <label>"Cancel"</label>
	<input file>"/usr/share/icons/Faenza-Cupertino-mini/actions/32/gtk-cancel.png"</input>
	<action>EXIT:close</action>
    </button>
  </hbox>
</vbox>

</window>
'

gtkdialog --program=SLIMBACKGROUND
