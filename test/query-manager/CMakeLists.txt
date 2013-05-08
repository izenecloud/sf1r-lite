INCLUDE_DIRECTORIES(
    ${CMAKE_SOURCE_DIR}/core/
    ${ilplib_INCLUDE_DIRS}
    ${wisekma_INCLUDE_DIRS}
    ${izenecma_INCLUDE_DIRS}
    ${izenejma_INCLUDE_DIRS}
    ${izenelib_INCLUDE_DIRS}
    ${wiselib_INCLUDE_DIRS}
    ${idmlib_INCLUDE_DIRS}
    ${TokyoCabinet_INCLUDE_DIRS}
    ${Glog_INCLUDE_DIRS}
    )

SET(Boost_USE_STATIC_LIBS OFF)
FIND_PACKAGE(Boost ${Boost_FIND_VERSION}
  COMPONENTS unit_test_framework)

IF(Boost_FOUND AND Boost_UNIT_TEST_FRAMEWORK_FOUND)
  INCLUDE_DIRECTORIES(
    ${Boost_INCLUDE_DIRS}
  )
  ADD_EXECUTABLE(t_query_manager
    Runner.cpp
    test_def.cpp
    t_QueryParser.cpp
    t_SearchKeywordOperation.cpp
    )
  TARGET_LINK_LIBRARIES(t_query_manager
      ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}
      sf1r_configuration_manager
      sf1r_mining_manager
      sf1r_b5m_manager
      sf1r_product_manager
      sf1r_log_manager
      sf1r_license_manager
      sf1r_query_manager
      sf1r_la_manager
      sf1r_common
      sf1r_document_manager
      sf1r_directory_manager
      sf1r_node_manager
      sf1r_search_manager
      sf1r_ranking_manager
      sf1r_index_manager
      sf1r_image_manager
      sf1r_net
      ${ilplib_LIBRARIES}
      ${wisekma_LIBRARIES}
      ${izenecma_LIBRARIES}
      ${izenejma_LIBRARIES}
      ${izenelib_LIBRARIES}
      ${idmlib_LIBRARIES}
      ${imllib_LIBRARIES}
      ${XML2_LIBRARIES}
      ${iise_LIBRARIES}
      #external
      ${Boost_LIBRARIES}
      ${TokyoCabinet_LIBRARIES}
      ${Glog_LIBRARIES}
      ${SQLITE3_LIBRARIES}
      ${MYSQL_LIBRARIES}
      ${LibCURL_LIBRARIES}
      ${ImageMagick_LIBRARIES}
      )

ENDIF(Boost_FOUND AND Boost_UNIT_TEST_FRAMEWORK_FOUND)
