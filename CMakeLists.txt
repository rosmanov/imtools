cmake_minimum_required (VERSION 2.6)
project (ImTools CXX C)

include(CMakeDependentOption)

# -D IMTOOLS_DEBUG:STRING=OFF
option(IMTOOLS_DEBUG "Enable debugging support in ImTools" OFF)
# -D IMTOOLS_THREADS:STRING=ON
option(IMTOOLS_THREADS "Enable threading" ON)
# -D IMTOOLS_EXTRA:STRING=OFF
option(IMTOOLS_EXTRA "Enable extra tools" OFF)
# -D IMTOOLS_DEBUG_PROFILER:STRING=OFF
cmake_dependent_option(IMTOOLS_DEBUG_PROFILER "Enable profiling info in debug messages" OFF
  "IMTOOLS_DEBUG" OFF)
# -D IMTOOLS_SERVER:STRING=OFF
option(IMTOOLS_SERVER "Enable WebSocket server" OFF)

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

if (IMTOOLS_SERVER)
  # Boost library
  list (APPEND IMTOOLS_BOOST_COMPONENTS system)
  set(Boost_FOUND 0)
  include(FindBoost)
  find_package(Boost "1.52.0" REQUIRED COMPONENTS ${IMTOOLS_BOOST_COMPONENTS})
  if (NOT Boost_FOUND)
    message (FATAL_ERROR "Boost library with '${IMTOOLS_BOOST_COMPONENTS}' components not found")
  endif (NOT Boost_FOUND)

  ## The GNU linker requires dependent libraries to come before their
  ## dependencies. So we'll put `pthreads` to the end of ${Boost_LIBRARIES}.
  #set(CMAKE_THREAD_PREFER_PTHREAD)
  #find_package(Threads)
  #list(APPEND Boost_LIBRARIES "${CMAKE_THREAD_LIBS_INIT}")

  # Websocketpp library
  find_package(websocketpp "0.5.0" REQUIRED)
  if (WEBSOCKETPP_FOUND)
    message (STATUS "Found websocketpp in ${WEBSOCKETPP_INCLUDE_DIR}")
  else (WEBSOCKETPP_FOUND)
    message (FATAL_ERROR "websocketpp package not found")
  endif (WEBSOCKETPP_FOUND)
  include_directories(${WEBSOCKETPP_INCLUDE_DIR})

  set(CMAKE_REQUIRED_INCLUDES "${CMAKE_REQUIRED_INCLUDES} ${Boost_INCLUDE_DIRS} ${WEBSOCKETPP_INCLUDE_DIR}")
endif (IMTOOLS_SERVER)

if (IMTOOLS_THREADS)
    include(FindOpenMP)
    find_package(OpenMP)

    if (NOT OPENMP_FOUND)
      message (FATAL_ERROR "OpenMP is not supported")
    endif (NOT OPENMP_FOUND)

    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
    list(APPEND imtools_threads_src src/threads.cxx)

    add_definitions(-DUSE_OPENMP)
  add_definitions(-DIMTOOLS_THREADS)
endif (IMTOOLS_THREADS)

set(CMAKE_REQUIRED_INCLUDES "${CMAKE_REQUIRED_INCLUDES} ${LIBOPENCV_INCLUDE_DIR}")
set(CMAKE_REQUIRED_LIBRARIES "${LIBOPENCV_CORE_LIB} ${LIBOPENCV_IMGPROC_LIB}
${LIBOPENCV_HIGHGUI_LIB}")

list(APPEND LIBS ${LIBOPENCV_LIBS} ${Boost_LIBRARIES})
list(APPEND common_src src/imtools.cxx src/exceptions.cxx src/log.cxx ${imtools_threads_src})

list(APPEND imtools_targets immerge imresize)
if (IMTOOLS_EXTRA)
  list(APPEND imtools_targets imdiff impatch imtpl imboundboxes)
  add_definitions(-DIMTOOLS_EXTRA)
endif (IMTOOLS_EXTRA)

include_directories(${LIBOPENCV_INCLUDE_DIR})

if (NOT CMAKE_SYSTEM_NAME STREQUAL FreeBSD)
  set(CMAKE_REQUIRED_DEFINITIONS -D_GNU_SOURCE)
endif ()
include(CheckIncludeFiles)
check_include_files(sys/prctl.h HAVE_SYS_PRCTL_H)
include(CheckSymbolExists)
check_symbol_exists(PR_SET_DUMPABLE "sys/prctl.h" HAVE_PR_SET_DUMPABLE)

foreach(exec_item IN ITEMS ${imtools_targets})
  set(src_api src/${exec_item}-api.cxx)
  set(src src/${exec_item}.cxx ${common_src})

  if (EXISTS ${src_api})
    list(APPEND src ${src_api} src/Command.cxx)
    list(APPEND src_api_list ${src_api})
  endif (EXISTS ${src_api})

  add_executable(${exec_item} ${src})
  target_link_libraries(${exec_item} ${LIBS})
endforeach()

if (IMTOOLS_SERVER)
  add_definitions(-DIMTOOLS_SERVER)
  add_executable(imserver ${common_src} src/Command.cxx src/MetaCommand.cxx ${src_api_list} src/imserver.cxx)
  target_link_libraries(imserver ${LIBS})
  list(APPEND imtools_targets imserver)
endif (IMTOOLS_SERVER)

install(TARGETS ${imtools_targets} DESTINATION "bin")
# vim: et ts=2 sts=2 sw=2
