cmake_minimum_required (VERSION 2.6)
project (ImTools CXX C)

# -D IMTOOLS_DEBUG:STRING=OFF
option(IMTOOLS_DEBUG "Enable debugging support in ImTools" OFF)
# -D IMTOOLS_THREADS:STRING=ON
option(IMTOOLS_THREADS "Enable threading" ON)
# -D IMTOOLS_EXTRA:STRING=OFF
option(IMTOOLS_EXTRA "Enable extra tools" OFF)

include_directories("${CMAKE_CURRENT_SOURCE_DIR}")
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/CMake" ${CMAKE_MODULE_PATH})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

include(ImToolsCompiler)

find_package(LibOpenCV REQUIRED)

if (IMTOOLS_DEBUG)
  set (CMAKE_BUILD_TYPE "Debug")
else (IMTOOLS_DEBUG)
  set (CMAKE_BUILD_TYPE "Release")
endif (IMTOOLS_DEBUG)
message (STATUS "Build type: ${CMAKE_BUILD_TYPE}")

set(THREADS_LIBRARIES)
if (IMTOOLS_THREADS)
  set(CMAKE_THREAD_PREFER_PTHREAD)
  find_package(Threads)

  if(NOT CMAKE_USE_PTHREADS_INIT)
    message(FATAL_ERROR "pthreads not found")
  endif(NOT CMAKE_USE_PTHREADS_INIT)

  add_definitions(-DIMTOOLS_THREADS)
  set(THREADS_LIBRARIES ${CMAKE_THREAD_LIBS_INIT})
endif (IMTOOLS_THREADS)

set(CMAKE_REQUIRED_INCLUDES "${LIBOPENCV_INCLUDE_DIR}")
set(CMAKE_REQUIRED_LIBRARIES "${LIBOPENCV_CORE_LIB} ${LIBOPENCV_IMGPROC_LIB}
${LIBOPENCV_HIGHGUI_LIB}")

set(LIBS ${LIBS} ${LIBOPENCV_LIBS} ${THREADS_LIBRARIES})

set(common_src src/imtools.cxx src/exceptions.cxx src/log.cxx)

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
