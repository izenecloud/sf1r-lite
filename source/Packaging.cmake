MESSAGE(STATUS "****** Starting to add libraries ******")
SET(CPACK_PACKAGE_VERSION_MAJOR 0 )
SET(CPACK_PACKAGE_VERSION_MINOR 6 )

####################################################
# Macro for getting version number out of a .so file
####
SET(THREE_PART_VERSION_REGEX ".+\\.so\\.[0-9]+\\.[0-9]+\\.[0-9]+")
SET(TWO_PART_VERSION_REGEX ".+\\.so\\.[0-9]+\\.[0-9]+")
SET(ONE_PART_VERSION_REGEX ".+\\.so\\.[0-9]+")
SET(NONE_PART_VERSION_REGEX ".+\\.so")

# Breaks up a string in the form n1.n2.n3 into three parts and stores
# them in major, minor, and patch.  version should be a value, not a
# variable, while major, minor and patch should be variables.
MACRO(THREE_PART_VERSION_TO_VARS version major minor patch)
    #MESSAGE(STATUS "THREE_PART_VERSION_TO_VARS >>>> ${version}")
    IF(${version} MATCHES ${THREE_PART_VERSION_REGEX})
        STRING(REGEX REPLACE ".+\\.so\\.([0-9]+)\\.[0-9]+\\.[0-9]+" "\\1"
            ${major} "${version}")
        STRING(REGEX REPLACE ".+\\.so\\.[0-9]+\\.([0-9]+)\\.[0-9]+" "\\1"
            ${minor} "${version}")
        STRING(REGEX REPLACE ".+\\.so\\.[0-9]+\\.[0-9]+\\.([0-9]+)" "\\1"
            ${patch} "${version}")
    ELSEIF(${version} MATCHES ${TWO_PART_VERSION_REGEX})
        STRING(REGEX REPLACE ".+\\.so\\.([0-9]+)\\.[0-9]+" "\\1"
            ${major} "${version}")
        STRING(REGEX REPLACE ".+\\.so\\.[0-9]+\\.([0-9]+)" "\\1"
            ${minor} "${version}")
        SET(${patch} "0")
    ELSEIF(${version} MATCHES ${ONE_PART_VERSION_REGEX})
        STRING(REGEX REPLACE ".+\\.so\\.([0-9]+)" "\\1"
            ${major} "${version}")
        SET(${minor} "0")
        SET(${patch} "0")
    ELSEIF(${version} MATCHES ${NONE_PART_VERSION_REGEX})
        SET(${major} "0")
        SET(${minor} "0")
        SET(${patch} "0")
    ELSE(${version} MATCHES ${THREE_PART_VERSION_REGEX})
        MESSAGE(STATUS "MACRO(THREE_PART_VERSION_TO_VARS ${version} ${major} ${minor} ${patch}")
        MESSAGE(FATAL_ERROR "Problem parsing version string, I can't parse it properly.")
    ENDIF(${version} MATCHES
        ${THREE_PART_VERSION_REGEX})
ENDMACRO(THREE_PART_VERSION_TO_VARS)

##################################################
# Macro for copying third party libraries (boost, tokyo cabinet, glog)
####
MACRO(INSTALL_RELATED_LIBRARIES)
FOREACH(thirdlib ${ARGV})
    #MESSAGE(STATUS ">>>>>> ${thirdlib}")
    IF( ${thirdlib} MATCHES ".+\\.so" OR ${thirdlib} MATCHES ".+\\.so\\..+" )

        get_filename_component(libfilename ${thirdlib} NAME)
        get_filename_component(libpath ${thirdlib} PATH)

        # add the library files that exists
        SET(filestocopy ${thirdlib})

        EXECUTE_PROCESS( COMMAND "readlink"
            ${thirdlib} OUTPUT_VARIABLE oriname )

        IF(oriname)
            STRING(STRIP ${oriname} oriname)

            #MESSAGE(STATUS "NAME: ${libfilename}")
            #MESSAGE(STATUS "ORINAME: ${oriname}")
            #MESSAGE(STATUS "PATH: ${libpath}")

            THREE_PART_VERSION_TO_VARS(${oriname} major_vers minor_vers patch_vers)
            #MESSAGE("version = ${major_vers}%${minor_vers}%${patch_vers}")

            #MESSAGE(STATUS "EXISTS: ${thirdlib}.${major_vers}")
            IF(EXISTS "${thirdlib}.${major_vers}")
                LIST(APPEND filestocopy "${thirdlib}.${major_vers}")
            ENDIF(EXISTS "${thirdlib}.${major_vers}")

            #MESSAGE(STATUS "EXISTS: ${libpath}/${oriname}" )
            IF(EXISTS "${libpath}/${oriname}" )
                LIST(APPEND filestocopy "${libpath}/${oriname}")
            ENDIF(EXISTS "${libpath}/${oriname}" )
        ENDIF(oriname)

        #MESSAGE(STATUS "@@@@ ${filestocopy}")

        INSTALL(PROGRAMS ${filestocopy}
            DESTINATION lib-thirdparty
            COMPONENT  sf1r_packings
            )
    ENDIF( ${thirdlib} MATCHES ".+\\.so" OR ${thirdlib} MATCHES ".+\\.so\\..+" )
ENDFOREACH(thirdlib)
ENDMACRO(INSTALL_RELATED_LIBRARIES)


##################################################
# Installing third party library files
####
SET(ENV_ONLY_PACKAGE_SF1 $ENV{ONLY_PACKAGE_SF1})
IF(NOT ENV_ONLY_PACKAGE_SF1)
#  INSTALL_RELATED_LIBRARIES(${Boost_LIBRARIES})
  INSTALL_RELATED_LIBRARIES(${izenelib_LIBRARIES})
  INSTALL_RELATED_LIBRARIES(${ilplib_LIBRARIES})
  INSTALL_RELATED_LIBRARIES(${imllib_LIBRARIES})
  INSTALL_RELATED_LIBRARIES(${idmlib_LIBRARIES})
  INSTALL_RELATED_LIBRARIES(${Curl_LIBRARIES})

  IF( USE_IZENECMA )
    INSTALL_RELATED_LIBRARIES(${izenecma_LIBRARIES})
  ENDIF( USE_IZENECMA )
  IF( USE_IZENEJMA )
    INSTALL_RELATED_LIBRARIES(${izenejma_LIBRARIES})
  ENDIF( USE_IZENEJMA )
  IF( USE_IISE )
    INSTALL_RELATED_LIBRARIES(${iise_LIBRARIES})
  ENDIF( USE_IISE )
  IF( USE_WISEKMA )
    INSTALL_RELATED_LIBRARIES(${wisekma_LIBRARIES})
  ENDIF( USE_WISEKMA )

  INSTALL_RELATED_LIBRARIES(${TokyoCabinet_LIBRARIES})
  INSTALL_RELATED_LIBRARIES(${Glog_LIBRARIES})
  INSTALL_RELATED_LIBRARIES(${MYSQL_LIBRARIES})
  INSTALL_RELATED_LIBRARIES("/usr/lib/libboost_date_time.so.1.47.0")
  INSTALL_RELATED_LIBRARIES("/usr/lib/libboost_filesystem.so.1.47.0")
  INSTALL_RELATED_LIBRARIES("/usr/lib/libboost_iostreams.so.1.47.0")
  INSTALL_RELATED_LIBRARIES("/usr/lib/libboost_program_options.so.1.47.0")
  INSTALL_RELATED_LIBRARIES("/usr/lib/libboost_regex.so.1.47.0")
  INSTALL_RELATED_LIBRARIES("/usr/lib/libboost_serialization.so.1.47.0")
  INSTALL_RELATED_LIBRARIES("/usr/lib/libboost_system.so.1.47.0")
  INSTALL_RELATED_LIBRARIES("/usr/lib/libboost_thread.so.1.47.0")
  INSTALL_RELATED_LIBRARIES("/usr/lib/libboost_unit_test_framework.so.1.47.0")
  INSTALL_RELATED_LIBRARIES("/usr/local/lib/libthrift.so")
  INSTALL_RELATED_LIBRARIES("/usr/lib/libmysqlclient_r.so.16")
  INSTALL_RELATED_LIBRARIES("/usr/lib/libmysqlclient.so.16")
  INSTALL_RELATED_LIBRARIES("/usr/lib/libicuuc.so")
  INSTALL_RELATED_LIBRARIES("/usr/lib/libicui18n.so")
  INSTALL_RELATED_LIBRARIES("/usr/lib/libicudata.so")

  IF( USE_WISEKMA )
    INSTALL( DIRECTORY ${wisekma_KNOWLEDGE}
      DESTINATION "."
      COMPONENT sf1r_packings
      )
  ENDIF( USE_WISEKMA )

  IF( USE_IZENECMA )
    INSTALL( DIRECTORY ${izenecma_KNOWLEDGE}
      DESTINATION "."
      COMPONENT sf1r_packings
      )
  ENDIF( USE_IZENECMA )

  IF( USE_IZENEJMA )
    INSTALL( DIRECTORY ${izenejma_KNOWLEDGE}
      DESTINATION "."
      COMPONENT sf1r_packings
      )
  ENDIF( USE_IZENEJMA )

  INSTALL( DIRECTORY ${ilplib_LANGUAGEIDENTIFIER_DB}
    DESTINATION "."
    COMPONENT sf1r_packings
    )

  INSTALL( DIRECTORY "${CMAKE_SOURCE_DIR}/../bin/config"
    DESTINATION "."
    COMPONENT sf1r_packings
    REGEX ".xml.in" EXCLUDE
    PATTERN "example.xml" EXCLUDE
    )
  INSTALL( PROGRAMS
    ${CMAKE_SOURCE_DIR}/../bin/sf1r-engine
    #    ${CMAKE_SOURCE_DIR}/../bin/clean.query
    DESTINATION "bin"
    COMPONENT sf1r_packings
    )
  INSTALL( DIRECTORY "${CMAKE_SOURCE_DIR}/../instances"
    DESTINATION "."
    COMPONENT sf1r_packings
    )
  INSTALL( DIRECTORY "${CMAKE_SOURCE_DIR}/../package/resource"
    DESTINATION "."
    COMPONENT sf1r_packings
    REGEX ".gitignore" EXCLUDE
    )

ENDIF(NOT ENV_ONLY_PACKAGE_SF1)

##################################################
# Settings for CPack
####
SET(CPACK_PACKAGE_NAME "SF-1 Revolution x86_64 GNU/Linux" )

IF( USE_MF_LIGHT )

SET(CPACK_PACKAGE_VENDOR "IZENEsoft" )
SET(CPACK_PACKAGE_VERSION "beta")
SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY "SF-1 Revolution Cobra Trial" )
SET(CPACK_PACKAGE_FILE_NAME "SF-1R-${CPACK_PACKAGE_VERSION}-Cobra-x86_64-Linux-GCC4.1.2" )
#SET(CPACK_PACKAGE_FILE_NAME "SF-1R-${CPACK_PACKAGE_VERSION}-Cobra-Trial-x86_64-Linux-GCC4.1.2" )
SET(CPACK_PACKAGE_FILE_NAME "sf1r" )

ELSE( USE_MF_LIGHT )

SET(CPACK_PACKAGE_VENDOR "Wisenut" )
SET(CPACK_PACKAGE_VERSION "alpha")
SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY "SF-1 Revolution QC candidate" )
SET(CPACK_PACKAGE_FILE_NAME "SF-1R-${CPACK_PACKAGE_VERSION}-x86_64-Linux-GCC4.1.2" )
SET(CPACK_PACKAGE_FILE_NAME "sf1r-engine" )

ENDIF( USE_MF_LIGHT )

SET(CPACK_GENERATOR "TGZ")
SET(CPACK_SOURCE_GENERATOR "TGZ")

IF(ENV_ONLY_PACKAGE_SF1)
  SET(CPACK_COMPONENTS_ALL sf1r_libraries sf1r_apps)
ELSE(ENV_ONLY_PACKAGE_SF1)
  SET(CPACK_COMPONENTS_ALL
    sf1r_libraries
    sf1r_apps
    sf1r_packings
    )
ENDIF(ENV_ONLY_PACKAGE_SF1)

INCLUDE(UseCPack)
