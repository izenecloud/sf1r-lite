INCLUDE_DIRECTORIES(
  ${CMAKE_SOURCE_DIR}/core
  ${CMAKE_SOURCE_DIR}/process/common
  ${CMAKE_SOURCE_DIR}/process/log-server
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
  )

# sequences is important for some linker
# if a dpendes b, a must precede b
SET(libs
  sf1r_log_manager
  sf1r_product_manager
  sf1r_search_manager
  sf1r_mining_manager
  sf1r_directory_manager
  sf1r_query_manager
  sf1r_license_manager
  sf1r_la_manager
  sf1r_configuration_manager
  sf1r_index_manager
  sf1r_document_manager
  sf1r_node_manager
  sf1r_common
  sf1r_ranking_manager

  ${idmlib_LIBRARIES}
  ${imllib_LIBRARIES}
  ${ilplib_LIBRARIES}
  ${iise_LIBRARIES}
  ${wisekma_LIBRARIES}
  ${izenecma_LIBRARIES}
  ${izenejma_LIBRARIES}
  ${izenelib_LIBRARIES}
  ${xml2_LIBRARIES}

  #external
  ${Boost_LIBRARIES}
  ${TokyoCabinet_LIBRARIES}
  ${Glog_LIBRARIES}
  ${sqlite3_LIBRARIES}
  ${MYSQL_LIBRARIES}
  ${LibCURL_LIBRARIES}
  ${ImageMagick_LIBRARIES}
  )
SET(Boost_USE_STATIC_LIBS OFF)
FIND_PACKAGE(Boost ${Boost_FIND_VERSION}
  COMPONENTS unit_test_framework)

IF(Boost_FOUND AND Boost_UNIT_TEST_FRAMEWORK_FOUND)
  INCLUDE_DIRECTORIES(
    ${Boost_INCLUDE_DIRS}
  )
  ADD_EXECUTABLE(t_log_manager
    Runner.cpp
    t_LogManager.cpp
    )
  TARGET_LINK_LIBRARIES(t_log_manager ${libs})
  SET_TARGET_PROPERTIES(t_log_manager PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${SF1RENGINE_ROOT}/testbin
    )
  ADD_TEST(log_manager "${SF1RENGINE_ROOT}/testbin/t_log_manager")

  ADD_EXECUTABLE(t_OrderLogger
    Runner.cpp
    t_OrderLogger.cpp
    )
  TARGET_LINK_LIBRARIES(t_OrderLogger ${libs})
  SET_TARGET_PROPERTIES(t_OrderLogger PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${SF1RENGINE_ROOT}/testbin
    )
  ADD_TEST(log_manager "${SF1RENGINE_ROOT}/testbin/t_OrderLogger")

  ADD_EXECUTABLE(t_ItemLogger
    Runner.cpp
    t_ItemLogger.cpp
    )
  TARGET_LINK_LIBRARIES(t_ItemLogger ${libs})
  SET_TARGET_PROPERTIES(t_ItemLogger PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${SF1RENGINE_ROOT}/testbin
    )
  ADD_TEST(log_manager "${SF1RENGINE_ROOT}/testbin/t_ItemLogger")

  ADD_EXECUTABLE(t_RpcServerConnection
    Runner.cpp
    t_RpcServerConnection.cpp
    RpcConnectionTestFixture.cpp
    RpcTestServer.cpp
    )
  TARGET_LINK_LIBRARIES(t_RpcServerConnection ${libs})
  SET_TARGET_PROPERTIES(t_RpcServerConnection PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${SF1RENGINE_ROOT}/testbin
    )
  ADD_TEST(log_manager "${SF1RENGINE_ROOT}/testbin/t_RpcServerConnection")
ENDIF(Boost_FOUND AND Boost_UNIT_TEST_FRAMEWORK_FOUND)

  ADD_EXECUTABLE(t_LogServerConnection
    t_LogServerConnection.cpp
    )
  TARGET_LINK_LIBRARIES(t_LogServerConnection ${libs})
  SET_TARGET_PROPERTIES(t_LogServerConnection PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${SF1RENGINE_ROOT}/testbin
    )

  ADD_EXECUTABLE(t_Drum
    t_Drum.cpp
    ${CMAKE_SOURCE_DIR}/process/log-server/LogServerCfg.cpp
    )
  TARGET_LINK_LIBRARIES(t_Drum ${libs})
  SET_TARGET_PROPERTIES(t_Drum PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${SF1RENGINE_ROOT}/testbin
    )
