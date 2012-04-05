CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
GET_FILENAME_COMPONENT(SF1RENGINE_ROOT "${CMAKE_SOURCE_DIR}" PATH)

##################################################
# Find extra cmake modules dirs
#
# You can clone all modules from https://ssl.izenesoft.cn/projects/cmake
#
# Clone it and set environment variable EXTRA_CMAKE_MODULES_DIRS point to it.
#
SET(ENV_EXTRA_CMAKE_MODULES_DIRS $ENV{EXTRA_CMAKE_MODULES_DIRS})
IF(ENV_EXTRA_CMAKE_MODULES_DIRS)
  SET(EXTRA_CMAKE_MODULES_DIRS ${EXTRA_CMAKE_MODULES_DIRS} ${ENV_EXTRA_CMAKE_MODULES_DIRS})
ENDIF(ENV_EXTRA_CMAKE_MODULES_DIRS)
IF(NOT EXTRA_CMAKE_MODULES_DIRS)
  MESSAGE(FATAL_ERROR
    "Cannot find EXTRA_CMAKE_MODULES_DIRS.
1. Please clone it from https://ssl.izenesoft.cn/cmake.
2. Then set EXTRA_CMAKE_MODULES_DIRS through environment variable

       export EXTRA_CMAKE_MODULES_DIRS=/path/to/cmake

   You can put it in your .bashrc so it is loaded automatically.

   or cmake options

       cmake  -DEXTRA_CMAKE_MODULES_DIRS=/path/to/cmake (other options)
")
ENDIF(NOT EXTRA_CMAKE_MODULES_DIRS)
SET(CMAKE_MODULE_PATH ${EXTRA_CMAKE_MODULES_DIRS})

# force out-of-source build
IF (CMAKE_BINARY_DIR STREQUAL CMAKE_SOURCE_DIR)
  MESSAGE(FATAL_ERROR "In source build is not allowed")
ENDIF (CMAKE_BINARY_DIR STREQUAL CMAKE_SOURCE_DIR)

##################################################
# Project and some options
#####
PROJECT(sf1r-engine)
OPTION(VERBOSE "print detailed configure messages" ON)
OPTION(ENABLE_TEST "build test applications?" ON)
IF(ENABLE_TEST)
  ENABLE_TESTING()
ENDIF(ENABLE_TEST)

# Check the mode
IF( NOT MODE )
    MESSAGE( STATUS "Set Default Mode to MULTI")
    SET( MODE "MULTI")
ENDIF( NOT MODE )


STRING(TOUPPER ${MODE} UPPER_MODE)
IF( ${UPPER_MODE} STREQUAL "CN" )
    SET( USE_IZENECMA TRUE )
    SET( USE_WISEKMA FALSE )
    MESSAGE( STATUS "CN mode: uses iCMA.")
ELSEIF( ${UPPER_MODE} STREQUAL "KR" )
    SET( USE_IZENECMA FALSE )
    SET( USE_WISEKMA TRUE)
    MESSAGE( STATUS "KR mode: uses KMA")
ELSEIF( ${UPPER_MODE} STREQUAL "MULTI" )
    SET( USE_IZENECMA TRUE )
    SET( USE_IISE FALSE )
    SET( USE_WISEKMA TRUE)
    SET( USE_IZENEJMA TRUE)
    MESSAGE( STATUS "MULTI mode: uses iCMA ,iJMA and KMA." )
ELSE( ${UPPER_MODE} STREQUAL "CN" )
    MESSAGE( FATAL_ERROR "Invalid parameter mode: ${MODE}, should be one of CN, KR and MULTI")
ENDIF( ${UPPER_MODE} STREQUAL "CN" )

##################################################
# BUILD TYPE & OPTIONS
#
# Put this section just under PROJECT
#####

# License Check
IF (LICENSE_LEVEL)
    STRING(TOUPPER ${LICENSE_LEVEL} UPPER_LICENSE_LEVEL)
ENDIF (LICENSE_LEVEL)

IF (LICENSE_LEVEL)
    ADD_DEFINITIONS( -DEXIST_LICENSE )
ENDIF (LICENSE_LEVEL)

IF (UPPER_LICENSE_LEVEL STREQUAL "COBRA")
    SET(LICENSE_FLAGS "-DLICENSE_MAX_DOC=10000"   "-DLICENSE_MAX_SIZE=1"   CACHE STRING "Flags of cobra license." FORCE)
ELSEIF (UPPER_LICENSE_LEVEL STREQUAL "SILVER")
    SET(LICENSE_FLAGS "-DLICENSE_MAX_DOC=100000"  "-DLICENSE_MAX_SIZE=10"  CACHE STRING "Flags of silver license." FORCE)
ELSEIF (UPPER_LICENSE_LEVEL STREQUAL "GOLD")
    SET(LICENSE_FLAGS "-DLICENSE_MAX_DOC=1000000" "-DLICENSE_MAX_SIZE=100" CACHE STRING "Flags of gold license." FORCE)
ELSEIF (UPPER_LICENSE_LEVEL STREQUAL "PLATINUM")
    SET(LICENSE_FLAGS "-DLICENSE_MAX_DOC=500000000" "-DLICENSE_MAX_SIZE=50000" CACHE STRING "Flags of platinum license." FORCE)
ELSEIF (UPPER_LICENSE_LEVEL STREQUAL "MAX_SIZE_TEST")
    SET(LICENSE_FLAGS "-DLICENSE_MAX_DOC=500000000" "-DLICENSE_MAX_SIZE=1" CACHE STRING "Flags of max_size_test license." FORCE)
ELSEIF (UPPER_LICENSE_LEVEL STREQUAL "MAX_DOC_TEST")
    SET(LICENSE_FLAGS "-DLICENSE_MAX_DOC=1000" "-DLICENSE_MAX_SIZE=5000" CACHE STRING "Flags of max_doc_test license." FORCE)
ELSE (UPPER_LICENSE_LEVEL STREQUAL "COBRA")
    SET(UPPER_LICENSE_LEVEL "DEV-LICENSE")
    SET(LICENSE_FLAGS "-DLICENSE_MAX_DOC=500000000" "-DLICENSE_MAX_SIZE=50000" CACHE STRING "Flags of development license." FORCE)
ENDIF (UPPER_LICENSE_LEVEL STREQUAL "COBRA")
ADD_DEFINITIONS( ${LICENSE_FLAGS} )

# Use BUILD_FLAGS_HAS_BEEN_CUSTOMIZED as guard.
#
# The options are only customized in first run. User can edit cache to
# set their own options. If it's required to customized these in cmake
# command line options, please also add
# "-DBUILD_FLAGS_HAS_BEEN_CUSTOMIZED".
IF (CMAKE_COMPILER_IS_GNUCXX)
  INCLUDE(MacroSetBuildTypes)

  MACRO_SET_BUILD_TYPES(
    RelWithDebInfo
    "-O2 -g -Wno-deprecated -DNDEBUG"
    "Flags used by the compiler during Release with Debug Info builds."

    Release
    "-O3 -Wno-deprecated -DNDEBUG"
    "Flags used by the compiler during release builds"

    MinSizeRel
    "-Os -Wno-deprecated -DNDEBUG"
    "Flags used by the compiler during release minsize builds."

    Debug
    "-g -O0 -fno-reorder-blocks -fno-schedule-insns -fno-inline -Wno-deprecated"
    "Flags used by the compiler during debug builds."

    Profile
    "-g3 -fno-inline -ftest-coverage -fprofile-arcs -Wno-deprecated"
    "Profile build type"

    SF1DEBUG
    "-O3 -g -Wno-deprecated -DSF1_DEBUG"
    "Sf1 Debug build type"

    SF1TIMECHECK
    "-O3 -DNDEBUG -g -Wno-deprecated -DSF1_TIME_CHECK"
    "Sf1 time check build type"

    SF1PCHECK
    "-O3 -Wno-deprecated -DNDEBUG -DSF1_TIME_CHECK"
    "Sf1 performance check build type"

    COBRARELWITHDEBINFO
    "-O2 -g -Wno-deprecated -DNDEBUG"
    "Flags used by the compiler during Release with Debug Info builds for Cobra."

    COBRARELEASE
    "-O3 -Wno-deprecated -DNDEBUG"
    "Flags used by the compiler during release builds [CORBAR]"

    COBRADEBUG
    "-O0 -DDEBUG -g -fno-reorder-blocks -fno-schedule-insns -fno-inline -Wno-deprecated"
    "Flags used by the compiler during debug builds.[CORBAR]"

    COBRARESTRICTRELEASE
    "-O3 -g -Wno-deprecated -DNDEBUG -DCOBRA_RESTRICT "
    "Flags used by the compiler during debug builds.[CORBAR]"

    COBRARESTRICT
    "-O2 -g -Wno-deprecated -DCOBRA_RESTRICT "
    "Flags used by the compiler during debug builds.[CORBAR]"

    COBRASF1TIMECHECK
    "-O3 -DDEBUG -g  -Wno-deprecated -DSF1_TIME_CHECK "
    "Sf1 time check build type"
    )
ENDIF (CMAKE_COMPILER_IS_GNUCXX)

INCLUDE(MacroSetDefaultBuildType)
MACRO_SET_DEFAULT_BUILD_TYPE(RelWithDebInfo)

INCLUDE(SetCompilerWarningAll)
# Write to cache
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}" CACHE STRING
  "Flags used by the compiler during all build types." FORCE)
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}" CACHE STRING
  "Flags used by the compiler during all build types." FORCE)

# build shared libs by default
OPTION(BUILD_SHARED_LIBS "Build shared libraries?" ON)

##################################################
# Check Dependencies
####
INCLUDE(SetPkgConfigPath)
INCLUDE(ConfigureChecks.cmake)

##################################################
# Set options for packaging
#####
STRING(TOLOWER ${CMAKE_BUILD_TYPE} buildtype)
INCLUDE(Packaging.cmake)

##################################################
# Sub Directories
#####

# library dir
LINK_DIRECTORIES(
  ${izenelib_LIBRARY_DIRS}
  ${ilplib_LIBRARY_DIRS}
  ${idmlib_LIBRARY_DIRS}
)

# set output directory
SET(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${SF1RENGINE_ROOT}/lib")
SET(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${SF1RENGINE_ROOT}/lib")
SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${SF1RENGINE_ROOT}/bin")

ADD_SUBDIRECTORY(core)
ADD_SUBDIRECTORY(bundles)
ADD_SUBDIRECTORY(process)
ADD_SUBDIRECTORY(tools)

IF(ENABLE_TEST)
  ADD_SUBDIRECTORY("${SF1RENGINE_ROOT}/test" test)
ENDIF(ENABLE_TEST)


##################################################
# Config
#####
SET(LOCAL_HOST_USER_NAME $ENV{USER})
IF(NOT LOCAL_HOST_USER_NAME)
  SET(LOCAL_HOST_USER_NAME $ENV{LOGNAME})
ENDIF(NOT LOCAL_HOST_USER_NAME)
IF(NOT LOCAL_HOST_USER_NAME)
  SET(LOCAL_HOST_USER_NAME $ENV{USERNAME})
ENDIF(NOT LOCAL_HOST_USER_NAME)

CONFIGURE_FILE(
  "${SF1RENGINE_ROOT}/bin/config/sf1config.xml.in"
  "${SF1RENGINE_ROOT}/bin/config/sf1config.xml"
  ESCAPE_QUOTES @ONLY)

CONFIGURE_FILE( "${SF1RENGINE_ROOT}/bin/config/example.xml.in"
  "${SF1RENGINE_ROOT}/bin/config/example.xml"
  ESCAPE_QUOTES @ONLY)

CONFIGURE_FILE(
  "${SF1RENGINE_ROOT}/source/core/config.h.in"
  "${SF1RENGINE_ROOT}/source/core/config.h"
  ESCAPE_QUOTES @ONLY)

CONFIGURE_FILE(
  "${CMAKE_SOURCE_DIR}/driver.doxygen.in"
  "${CMAKE_BINARY_DIR}/driver.doxygen"
  ESCAPE_QUOTES @ONLY
  )

##################################################
# Miscellaneous
#####

IF (DOXYGEN_FOUND)
  ADD_CUSTOM_TARGET(driver-docs ${DOXYGEN_EXECUTABLE} "driver.doxygen")
ENDIF (DOXYGEN_FOUND)

##################################################
# Messages
#####
IF(VERBOSE)

  MESSAGE(STATUS "========================================")
  MESSAGE(STATUS "CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}")
  STRING(TOUPPER "${CMAKE_BUILD_TYPE}" CMAKE_BUILD_TYPE_UPPER)
  MESSAGE(STATUS "  corresponding flags: ${CMAKE_CXX_FLAGS_${CMAKE_BUILD_TYPE_UPPER}}")
  MESSAGE(STATUS "CMAKE_CXX_FLAGS: ${CMAKE_CXX_FLAGS}")
  IF(${buildtype} STREQUAL "cobrarestrict"  OR ${buildtype} STREQUAL "cobrarestrictrelease" )
      MESSAGE(STATUS "LICENSE LEVEL = ${UPPER_LICENSE_LEVEL}")
      MESSAGE(STATUS "LICENSE FLAGS = ${LICENSE_FLAGS}")
  ELSE(${buildtype} STREQUAL "cobrarestrict"  OR ${buildtype} STREQUAL "cobrarestrictrelease" )
      MESSAGE(STATUS "LICENSE LEVEL = NO LICENSE RESTRICTION")
  ENDIF(${buildtype} STREQUAL "cobrarestrict"  OR ${buildtype} STREQUAL "cobrarestrictrelease" )
  MESSAGE(STATUS "----------------------------------------")

  IF(izenelib_FOUND)
    MESSAGE(STATUS "izenelib:")
    MESSAGE(STATUS "  header: ${izenelib_INCLUDE_DIRS}")
    MESSAGE(STATUS "  lib: ${izenelib_LIBRARIES}")
  ELSE(izenelib_FOUND)
    MESSAGE(STATUS "izenelib: not found")
  ENDIF(izenelib_FOUND)

  IF(ilplib_FOUND)
    MESSAGE(STATUS "ilpla library:")
    MESSAGE(STATUS "  header: ${ilplib_INCLUDE_DIRS}")
    MESSAGE(STATUS "  lib: ${ilplib_LIBRARIES}")
  ELSE(ilplib_FOUND)
    MESSAGE(STATUS "ilpla library: not found")
  ENDIF(ilplib_FOUND)

  IF( USE_IISE )
    IF(iise_FOUND)
      MESSAGE(STATUS "iise:")
      MESSAGE(STATUS "  header: ${iise_INCLUDE_DIRS}")
      MESSAGE(STATUS "  lib: ${iise_LIBRARIES}")
    ELSE(iise_FOUND)
      MESSAGE(STATUS "IISE: not found")
    ENDIF(iise_FOUND)
  ENDIF( USE_IISE )

  IF( USE_WISEKMA )
    IF(wisekma_FOUND)
      MESSAGE(STATUS "wisekma:")
      MESSAGE(STATUS "  header: ${wisekma_INCLUDE_DIRS}")
      MESSAGE(STATUS "  lib: ${wisekma_LIBRARIES}")
      MESSAGE(STATUS "  knowledge: ${wisekma_KNOWLEDGE}")
    ELSE(wisekma_FOUND)
      MESSAGE(STATUS "WISEKMA: not found")
    ENDIF(wisekma_FOUND)
  ENDIF( USE_WISEKMA )

  IF( USE_IZENECMA )
    IF(izenecma_FOUND)
      MESSAGE(STATUS "izenecma:")
      MESSAGE(STATUS "  header: ${izenecma_INCLUDE_DIRS}")
      MESSAGE(STATUS "  lib: ${izenecma_LIBRARIES}")
      MESSAGE(STATUS "  knowledge: ${izenecma_KNOWLEDGE}")
    ELSE(izenecma_FOUND)
      MESSAGE(STATUS "IZENECMA: not found")
    ENDIF(izenecma_FOUND)
  ENDIF( USE_IZENECMA )

  IF( USE_IZENEJMA )
    IF(izenejma_FOUND)
      MESSAGE(STATUS "izenejma:")
      MESSAGE(STATUS "  header: ${izenejma_INCLUDE_DIRS}")
      MESSAGE(STATUS "  lib: ${izenejma_LIBRARIES}")
      MESSAGE(STATUS "  knowledge: ${izenejma_KNOWLEDGE}")
    ELSE(izenejma_FOUND)
      MESSAGE(STATUS "IZENEJMA: not found")
    ENDIF(izenejma_FOUND)
  ENDIF( USE_IZENEJMA )


  IF(ilplib_FOUND)
    MESSAGE(STATUS "ilplib:")
    MESSAGE(STATUS "  header: ${ilplib_INCLUDE_DIRS}")
    MESSAGE(STATUS "  lib: ${ilplib_LIBRARIES}")
  ELSE(ilplib_FOUND)
    MESSAGE(STATUS "ilplib: not found")
  ENDIF(ilplib_FOUND)

  IF(idmlib_FOUND)
    MESSAGE(STATUS "idmlib:")
    MESSAGE(STATUS "  header: ${idmlib_INCLUDE_DIRS}")
    MESSAGE(STATUS "  lib: ${idmlib_LIBRARIES}")
  ELSE(idmlib_FOUND)
    MESSAGE(STATUS "idmlib: not found")
  ENDIF(idmlib_FOUND)

  IF(imllib_FOUND)
    MESSAGE(STATUS "imllib:")
    MESSAGE(STATUS "  header: ${imllib_INCLUDE_DIRS}")
    MESSAGE(STATUS "  lib: ${imllib_LIBRARIES}")
  ELSE(imllib_FOUND)
    MESSAGE(STATUS "imllib: not found")
  ENDIF(imllib_FOUND)

  IF(xml2_FOUND)
    MESSAGE(STATUS "XML2:")
    MESSAGE(STATUS " header: ${xml2_INCLUDE_DIRS}")
    MESSAGE(STATUS " libraries: ${xml2_LIBRARIES}")
  ELSE(xml2_FOUND)
    MESSAGE(STATUS "XML2: not found")
  ENDIF(xml2_FOUND)

  IF(sqlite3_FOUND)
    MESSAGE(STATUS "SQLITE3:")
    MESSAGE(STATUS " header: ${sqlite3_INCLUDE_DIRS}")
    MESSAGE(STATUS " libraries: ${sqlite3_LIBRARIES}")
  ELSE(sqlite3_FOUND)
    MESSAGE(STATUS "SQLITE3: not found")
  ENDIF(sqlite3_FOUND)

  IF(MYSQL_FOUND)
    MESSAGE(STATUS "MySQL:")
    MESSAGE(STATUS " header: ${MYSQL_INCLUDE_DIR}")
    MESSAGE(STATUS " libraries: ${MYSQL_LIBRARIES}")
  ELSE(MYSQL_FOUND)
    MESSAGE(STATUS "MySQL: not found")
  ENDIF(MYSQL_FOUND)

  IF(Boost_FOUND)
    MESSAGE(STATUS "BOOST:")
    MESSAGE(STATUS "  header: ${Boost_INCLUDE_DIR}")
    MESSAGE(STATUS "  version: ${Boost_MAJOR_VERSION}.${Boost_MINOR_VERSION}.${Boost_SUBMINOR_VERSION}")
  ELSE(Boost_FOUND)
    MESSAGE(STATUS "BOOST: not found")
  ENDIF(Boost_FOUND)

  IF(TokyoCabinet_FOUND)
    MESSAGE(STATUS "TokyoCabine:")
    MESSAGE(STATUS "  header: ${TokyoCabinet_INCLUDE_DIRS}")
    MESSAGE(STATUS "  version: ${TokyoCabinet_VERSION}")
  ELSE(TokyoCabinet_FOUND)
    MESSAGE(STATUS "TokyoCabinet: not found")
  ENDIF(TokyoCabinet_FOUND)

  IF(Glog_FOUND)
    MESSAGE(STATUS "Google-glog:")
    MESSAGE(STATUS "  header: ${Glog_INCLUDE_DIRS}")
  ELSE(Glog_FOUND)
    MESSAGE(STATUS "Google-glog: not found")
  ENDIF(Glog_FOUND)

  IF(Thrift_FOUND)
    MESSAGE(STATUS "Apache Thrift:")
    MESSAGE(STATUS "  header: ${Thrift_INCLUDE_DIR}")
  ELSE(Thrift_FOUND)
    MESSAGE(STATUS "Apache Thrift: not found")
  ENDIF(Thrift_FOUND)

  IF(Threads_FOUND)
    MESSAGE(STATUS "Threads Library: ${CMAKE_THREAD_LIBS_INIT}")
  ELSE(Threads_FOUND)
    MESSAGE(STATUS "Threads Library: none")
  ENDIF(Threads_FOUND)

  IF(LibCURL_FOUND)
    MESSAGE(STATUS "libcurl:")
    MESSAGE(STATUS "  header: ${LibCURL_INCLUDE_DIRS}")
    MESSAGE(STATUS "  lib: ${LibCURL_LIBRARIES}")
  ELSE(LibCURL_FOUND)
    MESSAGE(STATUS "libcurl: not found!")
  ENDIF(LibCURL_FOUND)

  MESSAGE(STATUS "========================================")

ENDIF(VERBOSE)
