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
  sf1r_product_manager
  sf1r_node_manager
  sf1r_common

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
  ADD_EXECUTABLE(t_split_ustr
    Runner.cpp
    t_split_ustr.cpp
  )
  TARGET_LINK_LIBRARIES(t_split_ustr ${libs})
  SET_TARGET_PROPERTIES(t_split_ustr PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${SF1RENGINE_ROOT}/testbin
    )
  ADD_TEST(group "${SF1RENGINE_ROOT}/testbin/t_split_ustr")

  ADD_EXECUTABLE(t_group_manager
    Runner.cpp
    t_group_manager.cc
    NumericPropertyTableBuilderStub.cpp
    GroupManagerTestFixture.cpp
  )
  TARGET_LINK_LIBRARIES(t_group_manager ${libs})
  SET_TARGET_PROPERTIES(t_group_manager PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${SF1RENGINE_ROOT}/testbin
    )
  ADD_TEST(group "${SF1RENGINE_ROOT}/testbin/t_group_manager")

  ADD_EXECUTABLE(t_attr_manager
    Runner.cpp
    t_attr_manager.cc
  )
  TARGET_LINK_LIBRARIES(t_attr_manager ${libs})
  SET_TARGET_PROPERTIES(t_attr_manager PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${SF1RENGINE_ROOT}/testbin
    )
  ADD_TEST(group "${SF1RENGINE_ROOT}/testbin/t_attr_manager")

  ADD_EXECUTABLE(t_GroupLabelLogger
    Runner.cpp
    t_GroupLabelLogger.cpp
  )
  TARGET_LINK_LIBRARIES(t_GroupLabelLogger ${libs})
  SET_TARGET_PROPERTIES(t_GroupLabelLogger PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${SF1RENGINE_ROOT}/testbin
    )
  ADD_TEST(group "${SF1RENGINE_ROOT}/testbin/t_GroupLabelLogger")

  ADD_EXECUTABLE(t_ec_manager
    Runner.cpp
    t_ec_manager.cc
  )
  TARGET_LINK_LIBRARIES(t_ec_manager ${libs})
  SET_TARGET_PROPERTIES(t_ec_manager PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${SF1RENGINE_ROOT}/testbin
    )
  ADD_TEST(group "${SF1RENGINE_ROOT}/testbin/t_ec_manager")

ENDIF(Boost_FOUND AND Boost_UNIT_TEST_FRAMEWORK_FOUND)

INCLUDE_DIRECTORIES(
${Boost_INCLUDE_DIRS}
)
ADD_EXECUTABLE(dd_group_gen
dd_group_gen.cc
)
TARGET_LINK_LIBRARIES(dd_group_gen ${libs})
SET_TARGET_PROPERTIES(dd_group_gen PROPERTIES
RUNTIME_OUTPUT_DIRECTORY ${SF1RENGINE_ROOT}/testbin
)
