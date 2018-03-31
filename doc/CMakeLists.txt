set (DOC "/usr/share/doc/slim")

###   https://cmake.org/cmake/help/v3.0/command/install.html?highlight=install%20files
install(FILES  ChangeLog_slim.txt  COPYING  OPTIONS_LIST.txt  DESTINATION ${DOC})
install(FILES  pam.sample THEMES  xinitrc.sample  DESTINATION ${DOC})
#install(FILES TODO DESTINATION ${DOC})

