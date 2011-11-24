INCLUDE_DIRECTORIES(
  ${Boost_INCLUDE_DIRS}
  )

SET(libs
  sf1r_common

  ${izenelib_LIBRARIES}

  # #external
  ${Boost_LIBRARIES}
  ${Glog_LIBRARIES}
  )

SET(Boost_USE_STATIC_LIBS OFF)
FIND_PACKAGE(Boost ${Boost_FIND_VERSION}
  COMPONENTS unit_test_framework serialization)

IF(Boost_FOUND AND Boost_UNIT_TEST_FRAMEWORK_FOUND AND Boost_SERIALIZATION_FOUND)

  ADD_EXECUTABLE(t_ScdParser
    Runner.cpp
    t_ScdParser.cpp
    )
  TARGET_LINK_LIBRARIES(t_ScdParser ${libs})

ENDIF()
