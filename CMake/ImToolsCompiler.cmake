# Initialize CXXFLAGS.
set(CMAKE_CXX_FLAGS         "${CMAKE_CXX_FLAGS} -Wall -std=c++11")
set(CMAKE_CXX_FLAGS_DEBUG   "${CMAKE_CXX_FLAGS_DEBUG} -DIMTOOLS_DEBUG -O0 -g -pedantic")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -DNDEBUG")

# Compiler-specific C++11 activation
if ("${CMAKE_CXX_COMPILER_ID}" MATCHES "GNU")
  execute_process(
    COMMAND ${CMAKE_CXX_COMPILER} -dumpversion OUTPUT_VARIABLE GCC_VERSION)
  if (NOT (GCC_VERSION VERSION_GREATER 4.7 OR GCC_VERSION VERSION_EQUAL 4.7))
    message(FATAL_ERROR "${PROJECT_NAME} requires g++ 4.7 or greater.")
  endif ()
elseif (${CMAKE_CXX_COMPILER} MATCHES ".*clang.*")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
else ()
  message(FATAL_ERROR "Your C++ compiler does not support C++11.")
endif ()
