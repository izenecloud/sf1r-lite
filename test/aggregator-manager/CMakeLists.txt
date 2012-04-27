INCLUDE_DIRECTORIES(
  ${CMAKE_SOURCE_DIR}/core
  ${Boost_INCLUDE_DIRS}
  ${Glog_INCLUDE_DIRS}
  )

# sequences is important for some linker
# if a dpendes b, a must precede b
SET(libs
  ${Boost_LIBRARIES}
  ${Glog_LIBRARIES}
  )

SET(Boost_USE_STATIC_LIBS OFF)
FIND_PACKAGE(Boost ${Boost_FIND_VERSION}
  COMPONENTS unit_test_framework)

IF(Boost_FOUND AND Boost_UNIT_TEST_FRAMEWORK_FOUND)
  INCLUDE_DIRECTORIES(
    ${Boost_INCLUDE_DIRS}
  )

  ADD_EXECUTABLE(t_SequenceMerger
    Runner.cpp
    t_SequenceMerger.cpp
  )
  TARGET_LINK_LIBRARIES(t_SequenceMerger ${libs})
  SET_TARGET_PROPERTIES(t_SequenceMerger PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${SF1RENGINE_ROOT}/testbin
    )
  ADD_TEST(aggregator-manager "${SF1RENGINE_ROOT}/testbin/t_SequenceMerger")

ENDIF(Boost_FOUND AND Boost_UNIT_TEST_FRAMEWORK_FOUND)
