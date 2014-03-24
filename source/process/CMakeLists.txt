INCLUDE_DIRECTORIES(
  ${CMAKE_SOURCE_DIR}
  ${CMAKE_SOURCE_DIR}/core/
  ${CMAKE_SOURCE_DIR}/bundles/
  ${CMAKE_CURRENT_SOURCE_DIR}

  ${idmlib_INCLUDE_DIRS}
  ${imllib_INCLUDE_DIRS}
  ${ilplib_INCLUDE_DIRS}
  ${wisekma_INCLUDE_DIRS}
  ${izenecma_INCLUDE_DIRS}
  ${izenejma_INCLUDE_DIRS}
  ${izenelib_INCLUDE_DIRS}
  ${Boost_INCLUDE_DIRS}
  ${TokyoCabinet_INCLUDE_DIRS}
  ${Glog_INCLUDE_DIRS}
  ${XML2_INCLUDE_DIR}
  ${SQLITE3_INCLUDE_DIR}
  ${ImageMagick_INCLUDE_DIRS}
  )

MESSAGE(STATUE "include path: ${INCLUDE_DIRECTORIES}")

FILE(GLOB_RECURSE common_SRC "common/*.cpp")
FILE(GLOB_RECURSE controller_SRC "controllers/*.cpp")
FILE(GLOB_RECURSE parsers_SRC "parsers/*.cpp")
FILE(GLOB_RECURSE renderers_SRC "renderers/*.cpp")
FILE(GLOB distribute_SRC "distribute/*.cpp")

FILE(GLOB cobra_process_SRC
  "Cobra*.cpp"
  ${common_SRC}
  ${controller_SRC}
  ${parsers_SRC}
  ${renderers_SRC}
  ${distribute_SRC}
  )

ADD_DEFINITIONS("-fno-strict-aliasing")
ADD_EXECUTABLE(CobraProcess ${cobra_process_SRC})

TARGET_LINK_LIBRARIES(CobraProcess
  sf1r_mining_manager

  sf1r_log_manager
  sf1r_document_manager
  sf1r_ranking_manager
  sf1r_search_manager
  sf1r_query_manager
  sf1r_la_manager
  sf1r_configuration_manager
  sf1r_index_manager
  sf1r_directory_manager
  sf1r_common
  sf1r_aggregator_manager
  sf1r_node_manager
  sf1r_net

  sf1r_bundle_index
  sf1r_bundle_mining

  ${idmlib_LIBRARIES}
  ${imllib_LIBRARIES}
  ${ilplib_LIBRARIES}
  ${wisekma_LIBRARIES}
  ${izenecma_LIBRARIES}
  ${izenejma_LIBRARIES}
  ${izenelib_LIBRARIES}

  #external
  ${XML2_LIBRARIES}
  ${Boost_LIBRARIES}
  ${TokyoCabinet_LIBRARIES}
  ${Glog_LIBRARIES}
  ${SQLITE3_LIBRARIES}
  ${MYSQL_LIBRARIES}
  ${LibCURL_LIBRARIES}
  ${ImageMagick_LIBRARIES}
  )

FIND_PACKAGE(izenelib REQUIRED COMPONENTS
  febird
  izene_util
  luxio
  leveldb
  json
  am
  compressor
  aggregator
  msgpack
  zookeeper
  compressor
  sf1r
  procmeminfo
  )


INSTALL(TARGETS
  CobraProcess
  RUNTIME DESTINATION bin
  COMPONENT sf1r_apps)
