##################################################
# Headers
#####
INCLUDE(CheckIncludeFile)

# int types
CHECK_INCLUDE_FILE(inttypes.h HAVE_INTTYPES_H)
CHECK_INCLUDE_FILE(stdint.h HAVE_STDINT_H)
CHECK_INCLUDE_FILE(sys/types.h HAVE_SYS_TYPES_H)
CHECK_INCLUDE_FILE(sys/stat.h HAVE_SYS_STAT_H)
CHECK_INCLUDE_FILE(stddef.h HAVE_STDDEF_H)

# signal.h
CHECK_INCLUDE_FILE(signal.h HAVE_SIGNAL_H)

# ext hash
CHECK_INCLUDE_FILE(ext/hash_map HAVE_EXT_HASH_MAP)

##################################################
# Our Proprietary Libraries
#####

FIND_PACKAGE(izenelib REQUIRED COMPONENTS
  izene_util
  index_manager
  mock_index_manager
  febird
  udt3
  izene_log
  bigint
  procmeminfo
  ticpp
  luxio
  leveldb
  jemalloc
  json
  ticpp
  am
  compressor
  aggregator
  distribute
  msgpack
  zookeeper
  udt
  snappy
  cassandra
  sf1r
  NUMA
  )

FIND_PACKAGE(ilplib REQUIRED)
FIND_PACKAGE(idmlib REQUIRED)
FIND_PACKAGE(imllib REQUIRED)

IF( USE_WISEKMA )
  FIND_PACKAGE(wisekma REQUIRED)
  ADD_DEFINITIONS( -DUSE_WISEKMA=TRUE )
ELSE( USE_WISEKMA )
  SET( wisekma_LIBRARIES "" )
  SET( wisekma_INCLUDE_DIRS "" )
  SET( wisekma_LIBRARY_DIRS "" )
ENDIF( USE_WISEKMA )

IF( USE_IZENECMA )
  FIND_PACKAGE(izenecma REQUIRED)
  ADD_DEFINITIONS( -DUSE_IZENECMA=TRUE )
ELSE( USE_IZENECMA )
  SET( izenecma_INCLUDE_DIRS "" )
  SET( izenecma_LIBRARIES "" )
  SET( izenecma_LIBRARY_DIRS "" )
ENDIF( USE_IZENECMA )

IF( USE_IZENEJMA )
  FIND_PACKAGE(izenejma REQUIRED)
  ADD_DEFINITIONS( -DUSE_IZENEJMA=TRUE )
ELSE( USE_IZENEJMA )
  SET( izenejma_INCLUDE_DIRS "" )
  SET( izenejma_LIBRARIES "" )
  SET( izenejma_LIBRARY_DIRS "" )
ENDIF( USE_IZENEJMA )

##################################################
# Other Libraries
#####

FIND_PACKAGE(xml2 REQUIRED)
FIND_PACKAGE(Threads REQUIRED)

SET(Boost_ADDITIONAL_VERSIONS 1.47)
FIND_PACKAGE(Boost 1.47 REQUIRED
  COMPONENTS
  system
  program_options
  thread
  regex
  date_time
  serialization
  filesystem
  unit_test_framework
  iostreams
  )

FIND_PACKAGE(TokyoCabinet 1.4.29 REQUIRED)
FIND_PACKAGE(Glog REQUIRED)
FIND_PACKAGE(sqlite3 REQUIRED)
FIND_PACKAGE(MySQL REQUIRED)
FIND_PACKAGE(Thrift REQUIRED)
FIND_PACKAGE(LibCURL REQUIRED)
FIND_PACKAGE(ImageMagick COMPONENTS Magick++ MagickWand MagickCore REQUIRED)
FIND_PACKAGE(JPEG REQUIRED)
FIND_PACKAGE(PNG REQUIRED)
##################################################
# Driver Docs
#####
GET_FILENAME_COMPONENT(SF1R_PARENT_DIR "${SF1RENGINE_ROOT}" PATH)
SET(SF1R_DRIVER_DOCS_ROOT "${SF1R_PARENT_DIR}/sf1-driver-docs/")

##################################################
# Doxygen
#####
FIND_PACKAGE(Doxygen)
IF(DOXYGEN_DOT_EXECUTABLE)
  OPTION(USE_DOT "use dot in doxygen?" FLASE)
ENDIF(DOXYGEN_DOT_EXECUTABLE)

SET(USE_DOT_YESNO NO)
IF(USE_DOT)
  SET(USE_DOT_YESNO YES)
ENDIF(USE_DOT)

FIND_PACKAGE( OpenCV )
