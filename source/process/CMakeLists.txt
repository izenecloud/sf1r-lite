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
  ${iise_INCLUDE_DIRS}
  ${Boost_INCLUDE_DIRS}
  ${TokyoCabinet_INCLUDE_DIRS}
  ${Glog_INCLUDE_DIRS}
  ${XML2_INCLUDE_DIR}
  ${SQLITE3_INCLUDE_DIR}
  ${ImageMagick_INCLUDE_DIRS}
  )

MESSAGE(STATUE "include path: ${INCLUDE_DIRECTORIES}")

SET(POSCharExtractor_SRC tools/POSCharExtractor.cpp)
SET(TGTrainerBridge_SRC tools/TGTrainerBridge.cpp)
SET(TGTrainer_SRC tools/TGTrainer.cpp)

FILE(GLOB_RECURSE common_SRC "common/*.cpp")
FILE(GLOB_RECURSE controller_SRC "controllers/*.cpp")
FILE(GLOB_RECURSE parsers_SRC "parsers/*.cpp")
FILE(GLOB_RECURSE renderers_SRC "renderers/*.cpp")
FILE(GLOB_RECURSE log_server_SRC "log-server/*.cpp")
FILE(GLOB_RECURSE img_dup_detector_SRC "ImgDupDetector/*.cpp")
FILE(GLOB distribute_SRC "distribute/*.cpp")

FILE(GLOB img_dup_process_SRC
  "ImgDup*.cpp"
  "common/ProcessOptions.cpp"
  "common/OnSignal.cpp"
  ${img_dup_detector_SRC}
  ${CMAKE_SOURCE_DIR}/core/b5m-manager/img_dup_detector.cc
  )

FILE(GLOB cobra_process_SRC
  "Cobra*.cpp"
  ${common_SRC}
  ${controller_SRC}
  ${parsers_SRC}
  ${renderers_SRC}
  ${distribute_SRC}
  )

FILE(GLOB distribute_tool_SRC
    "DistributeDataMigrateTool.cpp"
    ${CMAKE_SOURCE_DIR}/core/configuration-manager/ConfigurationTool.cpp
    ${CMAKE_SOURCE_DIR}/core/configuration-manager/PropertyConfig.cpp
    )

FILE(GLOB log_server_process_SRC
  "LogServer*.cpp"
  "common/ProcessOptions.cpp"
  "common/OnSignal.cpp"
  ${log_server_SRC}
  ${CMAKE_SOURCE_DIR}/core/log-manager/LogServerRequest.cpp
  ${CMAKE_SOURCE_DIR}/core/log-manager/LogServerRequestData.cpp
  ${CMAKE_SOURCE_DIR}/core/recommend-manager/item/SingleCollectionItemIdGenerator.cpp
  ${CMAKE_SOURCE_DIR}/core/recommend-manager/item/MultiCollectionItemIdGenerator.cpp
  ${CMAKE_SOURCE_DIR}/core/common/Keys.cpp
  ${CMAKE_SOURCE_DIR}/core/common/ScdWriter.cpp
  )
ADD_DEFINITIONS("-fno-strict-aliasing")
ADD_EXECUTABLE(CobraProcess ${cobra_process_SRC})
ADD_EXECUTABLE(LogServerProcess ${log_server_process_SRC})
ADD_EXECUTABLE(ImgDupDetector ${img_dup_process_SRC})
ADD_EXECUTABLE(DistributeDataMigrateTool ${distribute_tool_SRC})

TARGET_LINK_LIBRARIES(ImgDupDetector
  sf1r_product_manager
  sf1r_configuration_manager
  sf1r_mining_manager
  sf1r_license_manager
  sf1r_b5m_manager
  sf1r_log_manager
  sf1r_document_manager
  sf1r_ranking_manager
  sf1r_search_manager
  sf1r_query_manager
  sf1r_la_manager
  sf1r_index_manager
  sf1r_directory_manager
  sf1r_common
  sf1r_recommend_manager
  sf1r_aggregator_manager
  sf1r_node_manager

  sf1r_b5m_manager
  sf1r_image_manager
  sf1r_net

  sf1r_bundle_index
  sf1r_bundle_product
  sf1r_bundle_mining
  sf1r_bundle_recommend

  ${iise_LIBRARIES}
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
  ${Thrift_LIBS}
  ${LibCURL_LIBRARIES}
  ${ImageMagick_LIBRARIES}
  )


TARGET_LINK_LIBRARIES(CobraProcess
  sf1r_mining_manager
  sf1r_license_manager

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
  sf1r_recommend_manager
  sf1r_aggregator_manager
  sf1r_node_manager
  sf1r_product_manager
  sf1r_b5m_manager
  sf1r_image_manager
  sf1r_net

  sf1r_bundle_index
  sf1r_bundle_product
  sf1r_bundle_mining
  sf1r_bundle_recommend

  ${iise_LIBRARIES}
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
  ${Thrift_LIBS}
  ${LibCURL_LIBRARIES}
  ${ImageMagick_LIBRARIES}
  )

TARGET_LINK_LIBRARIES(DistributeDataMigrateTool
  sf1r_document_manager
  sf1r_common
  sf1r_license_manager

  ${ilplib_LIBRARIES}
  ${wisekma_LIBRARIES}
  ${izenelib_LIBRARIES}
  ${izenecma_LIBRARIES}
  ${izenejma_LIBRARIES}

  #external
  ${Boost_LIBRARIES}
  ${Glog_LIBRARIES}
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
  snappy
  sf1r
  procmeminfo
  )


TARGET_LINK_LIBRARIES(LogServerProcess
  sf1r_common
  ${izenelib_LIBRARIES}
  ${izenecma_LIBRARIES}

  #external
  ${Boost_LIBRARIES}
  ${Glog_LIBRARIES}
  ${TokyoCabinet_LIBRARIES}
  ${CMAKE_THREAD_LIBS_INIT}
  ${SYS_LIBS}
  )

IF(OpenCV_FOUND)

  IF(NOT $ENV{TBLIB_ROOT} STREQUAL "")
    MESSAGE("============ OpenCV and TFS client found, The ImageServerProcess will be builded. ==============")

    INCLUDE_DIRECTORIES(
      $ENV{TBLIB_ROOT}/include
      $ENV{TBLIB_ROOT}/include/tbsys
      $ENV{TBLIB_ROOT}/include/tbnet
      $ENV{HOME}/tfs_bin/include
      )

    FILE(GLOB image_server_process_SRC
      "ImageServerProcess/*.cc"
      "ImageServerProcess/*.cpp"
      "common/ProcessOptions.cpp"
      "common/OnSignal.cpp"
      )
    ADD_EXECUTABLE(ImageServerProcess ${image_server_process_SRC})

    TARGET_LINK_LIBRARIES(ImageServerProcess
      sf1r_common
      ${izenelib_LIBRARIES}
      sf1r_image_manager
      ${OpenCV_LIBS}

      #external
      ${Boost_LIBRARIES}
      ${Glog_LIBRARIES}
      ${TokyoCabinet_LIBRARIES}
      ${CMAKE_THREAD_LIBS_INIT}
      $ENV{TBLIB_ROOT}/lib/libtbnet.so
      $ENV{TBLIB_ROOT}/lib/libtbsys.so
      $ENV{HOME}/tfs_bin/lib/libtfsclient.so
      )

    INSTALL(TARGETS
      ImageServerProcess
      RUNTIME DESTINATION bin
      COMPONENT sf1r_apps)
  ENDIF()
ELSE(OpenCV_FOUND)
  MESSAGE("=============== OpenCV not found, the ImageServerProcess target will not be builded. =============")
ENDIF(OpenCV_FOUND)

INSTALL(TARGETS
  CobraProcess
  ImgDupDetector
  DistributeDataMigrateTool
  RUNTIME DESTINATION bin
  COMPONENT sf1r_apps)
