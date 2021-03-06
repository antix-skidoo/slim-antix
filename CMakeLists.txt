cmake_minimum_required(VERSION 2.6.0 FATAL_ERROR)

set(PROJECT_NAME slim)
project(${PROJECT_NAME})

###Pretty colors
set(CMAKE_COLOR_MAKEFILE ON)
###Dont force verbose
set(CMAKE_VERBOSE_MAKEFILE ON)
###Include current dir
set(CMAKE_INCLUDE_CURRENT_DIR TRUE)

INCLUDE(CheckIncludeFile)
INCLUDE(CheckCCompilerFlag)
INCLUDE(CheckCXXCompilerFlag)
INCLUDE(CheckTypeSize)

set(SLIM_VERSION_MAJOR "1")
set(SLIM_VERSION_MINOR "4")
set(SLIM_VERSION_PATCH "5")
set(SLIM_VERSION "${SLIM_VERSION_MAJOR}.${SLIM_VERSION_MINOR}.${SLIM_VERSION_PATCH}")

set(CMAKE_INSTALL_PREFIX "/usr/local" CACHE PATH "Installation Directory")
set(PKGDATADIR "${CMAKE_INSTALL_PREFIX}/share/slim")
set(SYSCONFDIR "/etc")
set(MANDIR "${CMAKE_INSTALL_PREFIX}/share/man")

set(SLIM_DEFINITIONS)
if(${CMAKE_SYSTEM_NAME} MATCHES "FreeBSD" OR
		${CMAKE_SYSTEM_NAME} MATCHES "NetBSD" OR
		${CMAKE_SYSTEM_NAME} MATCHES "OpenBSD"
		)
	set(SLIM_DEFINITIONS ${SLIM_DEFINITIONS} "-DNEEDS_BASENAME")
else()
	set(SLIM_DEFINITIONS ${SLIM_DEFINITIONS} "-DHAVE_SHADOW")
endif()

set(SLIM_DEFINITIONS ${SLIM_DEFINITIONS} "-DPACKAGE=\"slim\"")
set(SLIM_DEFINITIONS ${SLIM_DEFINITIONS} "-DVERSION=\"${SLIM_VERSION}\"")
set(SLIM_DEFINITIONS ${SLIM_DEFINITIONS} "-DPKGDATADIR=\"${PKGDATADIR}\"")
set(SLIM_DEFINITIONS ${SLIM_DEFINITIONS} "-DSYSCONFDIR=\"${SYSCONFDIR}\"")

### project-specific CFLAGS or CPPFLAGS not needed. Defer to use of the system-specified flags.
#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -g -O2")
#set(CMAKE_CPP_FLAGS "${CMAKE_CPP_FLAGS} -Wall -g -O2")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -g -O2")

### source
set(slim_srcs
	main.cpp
	app.cpp
	capslock.cpp
	cfg.cpp
	image.cpp
	numlock.cpp
	panel.cpp
	switchuser.cpp
	util.cpp
	log.cpp
)

if(USE_PAM)
	set(slim_srcs ${slim_srcs} PAM.cpp)
endif(USE_PAM)
if(USE_CONSOLEKIT)
	set(slim_srcs ${slim_srcs} Ck.cpp)
endif(USE_CONSOLEKIT)

add_executable(${PROJECT_NAME} ${slim_srcs})

###Set the custom CMake module directory (where our include/lib finders reside)
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/modules")

find_package(X11 REQUIRED)
find_package(Freetype REQUIRED)
find_package(JPEG REQUIRED)
find_package(PNG REQUIRED)
#find_package(ZLIB REQUIRED)   # dpkg-shlibdeps warns "none of this library's symbols are used


### Fontconfig    # dpkg-shlibdeps sez "none of this library's symbols are used
set(FONTCONFIG_DIR ${CMAKE_MODULE_PATH})
find_package(FONTCONFIG REQUIRED)
if(FONTCONFIG_FOUND)
	message("\tFontConfig Found")
	target_link_libraries(${PROJECT_NAME} ${FONTCONFIG_LIBRARY})
	include_directories(${FONTCONFIG_INCLUDE_DIR})
endif(FONTCONFIG_FOUND)


if(USE_PAM)
	message("\tPAM Enabled")
	find_package(PAM)
	if(PAM_FOUND)
		message("\tPAM Found")
		set(SLIM_DEFINITIONS ${SLIM_DEFINITIONS} "-DUSE_PAM")
		target_link_libraries(${PROJECT_NAME} ${PAM_LIBRARY})
		include_directories(${PAM_INCLUDE_DIR})
	else(PAM_FOUND)
		message("\tPAM Not Found")
	endif(PAM_FOUND)
else(USE_PAM)
	message("\tPAM disabled")
endif(USE_PAM)


if(USE_CONSOLEKIT)
	find_package(CkConnector)
	message("\tConsoleKit Enabled")
	if(CKCONNECTOR_FOUND)
		message("\tConsoleKit Found")
		# DBus check
		find_package(DBus REQUIRED)
		if(DBUS_FOUND)
			message("\tDBus Found")
			target_link_libraries(${PROJECT_NAME} ${DBUS_LIBRARIES})
			include_directories(${DBUS_ARCH_INCLUDE_DIR})
			include_directories(${DBUS_INCLUDE_DIR})
			set(SLIM_DEFINITIONS ${SLIM_DEFINITIONS} "-DUSE_CONSOLEKIT")
			target_link_libraries(${PROJECT_NAME} ${CKCONNECTOR_LIBRARIES})
			include_directories(${CKCONNECTOR_INCLUDE_DIR})
		else(DBUS_FOUND)
			message("\tDBus Not Found")
		endif(DBUS_FOUND)
	else(CKCONNECTOR_FOUND)
		message("\tConsoleKit Not Found")
		message("\tConsoleKit disabled")
	endif(CKCONNECTOR_FOUND)
else(USE_CONSOLEKIT)
	message("\tConsoleKit disabled")
endif(USE_CONSOLEKIT)

### system libraries
find_library(M_LIB m)
find_library(RT_LIB rt)
find_library(CRYPTO_LIB crypt)  # dpkg-shlibdeps sez "none of this library's symbols are used

add_definitions(${SLIM_DEFINITIONS})

###Set up include dirs with all found packages
include_directories(
	${X11_INCLUDE_DIR}
	${X11_Xft_INCLUDE_PATH}
	${X11_Xrender_INCLUDE_PATH}   # dpkg-shlibdeps misleadingly warns "none of this library's symbols are used

###     Using FREETYPE_INCLUDE_DIR_freetype2 directly
###       can cause issues with newer freetype packages (it does not contain all freetype include dirs)
#	${FREETYPE_INCLUDE_DIR_freetype2}
#	  ^---v
	${FREETYPE_INCLUDE_DIRS}   # dpkg-shlibdeps misleadingly warns "none of this library's symbols are used

	${X11_Xmu_INCLUDE_PATH}
#	${ZLIB_INCLUDE_DIR}   # dpkg-shlibdeps warns "none of this library's symbols are used
	${JPEG_INCLUDE_DIR}
	${PNG_INCLUDE_DIR}
)

set(X11_Xmu_LIB Xmu)
###Set up library with all found packages
target_link_libraries(${PROJECT_NAME}
	${M_LIB}
	${RT_LIB}
	${CRYPTO_LIB}    # dpkg-shlibdeps sez "none of this library's symbols are used
	${X11_X11_LIB}
	${X11_Xft_LIB}
	${X11_Xrender_LIB}       # dpkg-shlibdeps misleadingly warns "none of this library's symbols are used
	${X11_Xmu_LIB}
	${FREETYPE_LIBRARY}      # dpkg-shlibdeps misleadingly warns "none of this library's symbols are used
	${JPEG_LIBRARIES}
	${PNG_LIBRARIES}
	)

####### install
install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin )

install(FILES doc/slim.1 DESTINATION ${MANDIR}/man1/)

install(FILES slim.conf DESTINATION ${SYSCONFDIR})

###  TODO   this hard-coded dirpath may not exist on all target systems
###
###  TODO   check the inconsistent use of trailing slash
###         (vs none, as seen in the CMakeLists.txt residing in subdirs)
install(FILES README DESTINATION /usr/share/doc/slim/)

add_subdirectory(themes)
add_subdirectory(doc)

