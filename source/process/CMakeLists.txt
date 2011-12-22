INCLUDE_DIRECTORIES(
  ${CMAKE_SOURCE_DIR}
  ${CMAKE_SOURCE_DIR}/core/
  ${CMAKE_SOURCE_DIR}/bundles/
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/common

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
  ${xml2_INCLUDE_DIRS}
  ${sqlite3_INCLUDE_DIRS}
  )

SET(POSCharExtractor_SRC tools/POSCharExtractor.cpp)
SET(TGTrainerBridge_SRC tools/TGTrainerBridge.cpp)
SET(TGTrainer_SRC tools/TGTrainer.cpp)

FILE(GLOB_RECURSE common_SRC "common/*.cpp")
FILE(GLOB_RECURSE controller_SRC "controllers/*.cpp")
FILE(GLOB_RECURSE parsers_SRC "parsers/*.cpp")
FILE(GLOB_RECURSE renderers_SRC "renderers/*.cpp")
FILE(GLOB_RECURSE log_server_SRC "log-server/*.cpp")

FILE(GLOB cobra_process_SRC
  "Cobra*.cpp"
  ${common_SRC}
  ${controller_SRC}
  ${parsers_SRC}
  ${renderers_SRC}
  )

FILE(GLOB log_server_process_SRC
  "LogServer*.cpp"
  "common/ProcessOptions.cpp"  
  ${log_server_SRC}
  ${CMAKE_SOURCE_DIR}/core/log-manager/LogServerRequest.cpp
  ${CMAKE_SOURCE_DIR}/core/common/Keys.cpp
  )

ADD_DEFINITIONS("-fno-strict-aliasing")
ADD_EXECUTABLE(CobraProcess ${cobra_process_SRC})
ADD_EXECUTABLE(LogServerProcess ${log_server_process_SRC})

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
  ${xml2_LIBRARIES}
  ${Boost_LIBRARIES}
  ${TokyoCabinet_LIBRARIES}
  ${Glog_LIBRARIES}
  ${sqlite3_LIBRARIES}
  ${MYSQL_LIBRARIES}
  ${Thrift_LIBS}
)

TARGET_LINK_LIBRARIES(LogServerProcess
  ${izenelib_LIBRARIES}

  #external
  ${Boost_LIBRARIES}
  ${Glog_LIBRARIES}
)

INSTALL(TARGETS
    CobraProcess
    LogServerProcess
    RUNTIME DESTINATION bin
    COMPONENT sf1r_apps)
