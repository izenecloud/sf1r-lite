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

FILE(GLOB_RECURSE cobra_SRC "common/*.cpp" "controllers/*.cpp" "parsers/*.cpp" "renders/*.cpp" "*.cpp")
ADD_EXECUTABLE(CobraProcess ${cobra_SRC})

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

  sf1r_bundle_index 
  sf1r_bundle_querylog
  sf1r_bundle_mining

  ${iise_LIBRARIES}
  ${idmlib_LIBRARIES}
  ${imllib_LIBRARIES}
  ${ilplib_LIBRARIES}
  ${wisekma_LIBRARIES}
  ${izenecma_LIBRARIES}
  ${izenelib_LIBRARIES}
  ${xml2_LIBRARIES}

  #external
  ${Boost_LIBRARIES}
  ${TokyoCabinet_LIBRARIES}
  ${Glog_LIBRARIES}
  ${sqlite3_LIBRARIES}
)


INSTALL(TARGETS 
    CobraProcess 
    RUNTIME DESTINATION bin
    COMPONENT sf1r_apps)    

