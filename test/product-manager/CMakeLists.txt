INCLUDE_DIRECTORIES(
  ${CMAKE_SOURCE_DIR}/core
  ${izenelib_INCLUDE_DIRS}
  ${ilplib_INCLUDE_DIRS}
  ${idmlib_INCLUDE_DIRS}
  ${imllib_INCLUDE_DIRS}
  ${wisekma_INCLUDE_DIRS}
  ${izenecma_INCLUDE_DIRS}
  ${izenejma_INCLUDE_DIRS}
  ${Boost_INCLUDE_DIRS}
  ${TokyoCabinet_INCLUDE_DIRS}
  ${Glog_INCLUDE_DIRS}
  )

# sequences is important for some linker
# if a dpendes b, a must precede b
SET(libs
  sf1r_node_manager
  sf1r_query_manager
  sf1r_search_manager
  sf1r_ranking_manager
  sf1r_document_manager
  sf1r_mining_manager
  sf1r_directory_manager

  sf1r_log_manager
  sf1r_license_manager
  sf1r_la_manager
  sf1r_configuration_manager
  sf1r_index_manager
  sf1r_common
  sf1r_product_manager

  ${ilplib_LIBRARIES}
  ${idmlib_LIBRARIES}
  ${imllib_LIBRARIES}
  ${wisekma_LIBRARIES}
  ${izenecma_LIBRARIES}
  ${izenejma_LIBRARIES}
  ${iise_LIBRARIES}
  ${izenelib_LIBRARIES}

  #external
  ${Boost_LIBRARIES}
  ${TokyoCabinet_LIBRARIES}
  ${Glog_LIBRARIES}
  ${xml2_LIBRARIES}
  ${sqlite3_LIBRARIES}
  ${MYSQL_LIBRARIES}
  )
SET(Boost_USE_STATIC_LIBS OFF)
FIND_PACKAGE(Boost ${Boost_FIND_VERSION}
  COMPONENTS unit_test_framework)

IF(Boost_FOUND AND Boost_UNIT_TEST_FRAMEWORK_FOUND)
  INCLUDE_DIRECTORIES(
    ${Boost_INCLUDE_DIRS}
  )

  ADD_EXECUTABLE(t_product_price
    Runner.cpp
    t_product_price.cc
  )
  TARGET_LINK_LIBRARIES(t_product_price ${libs})
  SET_TARGET_PROPERTIES(t_product_price PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${SF1RENGINE_ROOT}/testbin
    )
  ADD_TEST(product "${SF1RENGINE_ROOT}/testbin/t_product_price")

  ADD_EXECUTABLE(t_data_source
    Runner.cpp
    t_data_source.cc
  )
  TARGET_LINK_LIBRARIES(t_data_source ${libs})
  SET_TARGET_PROPERTIES(t_data_source PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${SF1RENGINE_ROOT}/testbin
    )
  ADD_TEST(product "${SF1RENGINE_ROOT}/testbin/t_data_source")

  ADD_EXECUTABLE(t_product_manager
    Runner.cpp
    t_product_manager.cc
  )
  TARGET_LINK_LIBRARIES(t_product_manager ${libs})
  SET_TARGET_PROPERTIES(t_product_manager PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${SF1RENGINE_ROOT}/testbin
    )
  ADD_TEST(product "${SF1RENGINE_ROOT}/testbin/t_product_manager")



ENDIF(Boost_FOUND AND Boost_UNIT_TEST_FRAMEWORK_FOUND)
