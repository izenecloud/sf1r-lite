INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/core)

SET(Boost_USE_STATIC_LIBS OFF)
FIND_PACKAGE(Boost ${Boost_FIND_VERSION}
  COMPONENTS unit_test_framework)

IF(Boost_FOUND AND Boost_UNIT_TEST_FRAMEWORK_FOUND)
  INCLUDE_DIRECTORIES(
    ${Boost_INCLUDE_DIRS}
  )
  ADD_EXECUTABLE(t_ranking_manager
    Runner.cpp
    t_TermFreqsOrPositionsVector.cpp
    t_RankQueryProperty.cpp
    )
  TARGET_LINK_LIBRARIES(t_ranking_manager ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY} sqlite3)
  SET_TARGET_PROPERTIES(t_ranking_manager PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${SF1RENGINE_ROOT}/testbin
    )
  ADD_TEST(ranking_manager "${SF1RENGINE_ROOT}/testbin/t_ranking_manager")
ENDIF(Boost_FOUND AND Boost_UNIT_TEST_FRAMEWORK_FOUND)
