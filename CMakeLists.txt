cmake_minimum_required (VERSION 2.6)
project (ImTools CXX)

# -D IMTOOLS_DEBUG:STRING=ON
option(IMTOOLS_DEBUG "Enable debugging support in ImTools" ON)

include_directories("${CMAKE_CURRENT_SOURCE_DIR}")
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/CMake" ${CMAKE_MODULE_PATH})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

find_package(LibOpenCV REQUIRED)

if (IMTOOLS_DEBUG)
  message (STATUS "Build type: Debug")
  set (CMAKE_BUILD_TYPE "Debug")
  add_definitions(-DIMTOOLS_DEBUG)
endif (IMTOOLS_DEBUG)

set(CMAKE_REQUIRED_INCLUDES "${LIBOPENCV_INCLUDE_DIR}")
set(CMAKE_REQUIRED_LIBRARIES "${LIBOPENCV_CORE_LIB} ${LIBOPENCV_IMGPROC_LIB}
${LIBOPENCV_HIGHGUI_LIB}")

set(LIBS ${LIBS} ${LIBOPENCV_LIBS})

set(common_src src/imtools.cxx)

include_directories(${LIBOPENCV_INCLUDE_DIR})

foreach(exec_item IN ITEMS imdiff impatch imtpl immerge)
  add_executable(${exec_item} src/${exec_item}.cxx ${common_src})
  target_link_libraries(${exec_item} ${LIBS})
endforeach()
