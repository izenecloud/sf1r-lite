INCLUDE_DIRECTORIES(
  ${CMAKE_SOURCE_DIR}/core/include
  ${izenelib_INCLUDE_DIRS}
  ${Boost_INCLUDE_DIRS}
  )

SET(libs

  sf1r_query_manager
  sf1r_log_manager
  sf1r_la_manager
  sf1r_configuration_manager
  sf1r_common

  ${ilplib_LIBRARIES}
  ${wisekma_LIBRARIES}
  ${izenecma_LIBRARIES}
  ${izenelib_LIBRARIES}

  # #external
  ${Boost_LIBRARIES}
  ${Glog_LIBRARIES}
  ${sqlite3_LIBRARIES}
  ${xml2_LIBRARIES}
  )

SET(Boost_USE_STATIC_LIBS OFF)
FIND_PACKAGE(Boost ${Boost_FIND_VERSION}
  COMPONENTS unit_test_framework serialization)

IF(Boost_FOUND AND Boost_UNIT_TEST_FRAMEWORK_FOUND AND Boost_SERIALIZATION_FOUND)

  ADD_EXECUTABLE(t_configuration_manager
    Runner.cpp
    t_BrokerAgentConfig.cpp
    t_CollectionMeta.cpp
    t_CollectionPath.cpp
    t_PropertyConfig.cpp
    t_ConfigurationTool.cpp
    t_LAConfigUnit.cpp
    t_Acl.cpp
    )
  TARGET_LINK_LIBRARIES(t_configuration_manager ${libs})
  ADD_TEST(configuration_manager "${SF1RENGINE_ROOT}/testbin/t_configuration_manager")

ENDIF()
