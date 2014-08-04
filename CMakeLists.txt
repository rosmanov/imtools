cmake_minimum_required (VERSION 2.6)
project (ImTools CXX C)

# -D IMTOOLS_DEBUG:STRING=OFF
option(IMTOOLS_DEBUG "Enable debugging support in ImTools" OFF)
# -D IMTOOLS_THREADS:STRING=ON
option(IMTOOLS_THREADS "Enable threading" ON)
# -D IMTOOLS_EXTRA:STRING=OFF
option(IMTOOLS_EXTRA "Enable extra tools" OFF)
# -D IMTOOLS_DEBUG_PROFILER:STRING=OFF
option(IMTOOLS_DEBUG_PROFILER "Enable profiling info in debug messages" OFF)

include_directories("${CMAKE_CURRENT_SOURCE_DIR}")
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/CMake" ${CMAKE_MODULE_PATH})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

include(ImToolsCompiler)

find_package(LibOpenCV REQUIRED)

if (IMTOOLS_DEBUG)
  set (CMAKE_BUILD_TYPE "Debug")
  if (IMTOOLS_DEBUG_PROFILE)
    add_definitions(-DIMTOOLS_DEBUG_PROFILER)
  endif (IMTOOLS_DEBUG_PROFILE)
else (IMTOOLS_DEBUG)
  set (CMAKE_BUILD_TYPE "Release")
endif (IMTOOLS_DEBUG)

if (IMTOOLS_THREADS)
  set(Boost_FOUND 0)
  include(FindBoost)
  find_package(Boost REQUIRED COMPONENTS system thread)
  if (NOT Boost_FOUND)
    message (FATAL_ERROR "Boost library with 'thread' component not found")
  endif (NOT Boost_FOUND)
  add_definitions(-DIMTOOLS_THREADS)
endif (IMTOOLS_THREADS)

set(CMAKE_REQUIRED_INCLUDES "${LIBOPENCV_INCLUDE_DIR} ${Boost_INCLUDE_DIRS}")
set(CMAKE_REQUIRED_LIBRARIES "${LIBOPENCV_CORE_LIB} ${LIBOPENCV_IMGPROC_LIB}
${LIBOPENCV_HIGHGUI_LIB}
")
set(LIBS ${LIBS} ${LIBOPENCV_LIBS} ${Boost_LIBRARIES})

set(common_src src/imtools.cxx src/exceptions.cxx src/log.cxx src/threads.cxx)

set(imtools_targets immerge)
if (IMTOOLS_EXTRA)
  set(imtools_targets ${imtools_targets} imdiff impatch imtpl imboundboxes)
endif (IMTOOLS_EXTRA)

include_directories(${LIBOPENCV_INCLUDE_DIR})

foreach(exec_item IN ITEMS ${imtools_targets})
  add_executable(${exec_item} src/${exec_item}.cxx ${common_src})
  target_link_libraries(${exec_item} ${LIBS})
endforeach()

install(TARGETS ${imtools_targets} DESTINATION "bin")
