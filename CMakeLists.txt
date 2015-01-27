cmake_minimum_required (VERSION 2.6)
project (ImTools CXX C)

include(CMakeDependentOption)

# -D IMTOOLS_DEBUG:STRING=OFF
option(IMTOOLS_DEBUG "Enable debugging support in ImTools" OFF)
# -D IMTOOLS_THREADS:STRING=ON
option(IMTOOLS_THREADS "Enable threading" ON)
# -D IMTOOLS_THREADS_BOOST:STRING=OFF
cmake_dependent_option(IMTOOLS_THREADS_BOOST
  "Use boost libraries for threading" OFF
  "IMTOOLS_THREADS" OFF)
# -D IMTOOLS_EXTRA:STRING=OFF
option(IMTOOLS_EXTRA "Enable extra tools" OFF)
# -D IMTOOLS_DEBUG_PROFILER:STRING=OFF
cmake_dependent_option(IMTOOLS_DEBUG_PROFILER "Enable profiling info in debug messages" OFF
  "IMTOOLS_DEBUG" OFF)

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
  set(imtools_threads_src src/threads.cxx)

  if (NOT IMTOOLS_THREADS_BOOST)
    # OpenMP
    include(FindOpenMP)
    find_package(OpenMP)

    if (NOT OPENMP_FOUND)
      message (FATAL_ERROR "OpenMP is not supported")
    endif (NOT OPENMP_FOUND)

    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
    set(imtools_threads_src ${imtools_threads_src} src/OmpGuard.cxx)

    add_definitions(-DUSE_OPENMP)
  else (NOT IMTOOLS_THREADS_BOOST)
    # Boost library
    set(Boost_FOUND 0)
    include(FindBoost)
    find_package(Boost REQUIRED COMPONENTS system thread)
    if (NOT Boost_FOUND)
      message (FATAL_ERROR "Boost library with 'thread' component not found")
    endif (NOT Boost_FOUND)
    set(CMAKE_REQUIRED_INCLUDES "${CMAKE_REQUIRED_INCLUDES} ${Boost_INCLUDE_DIRS}")

    # Threadpool library
    find_path(BOOST_THREADPOOL_INCLUDE_DIRS
      NAMES threadpool.hpp
      PATHS ${Boost_INCLUDE_DIRS}
      PATH_SUFFIXES boost)
    if (NOT BOOST_THREADPOOL_INCLUDE_DIRS)
      message (FATAL_ERROR "Threadpool library headers not found")
    else (NOT BOOST_THREADPOOL_INCLUDE_DIRS)
      message (STATUS "Found threadpool library in ${BOOST_THREADPOOL_INCLUDE_DIRS}")
    endif (NOT BOOST_THREADPOOL_INCLUDE_DIRS)
    set(CMAKE_REQUIRED_INCLUDES "${CMAKE_REQUIRED_INCLUDES}
    ${BOOST_THREADPOOL_INCLUDE_DIRS}")

    add_definitions(-DIMTOOLS_BOOST_LIB)
  endif (NOT IMTOOLS_THREADS_BOOST)

  # The GNU linker requires dependent libraries to come before their
  # dependencies. So we'll put `pthreads` to the end of ${Boost_LIBRARIES}.
  set(CMAKE_THREAD_PREFER_PTHREAD)
  find_package(Threads)
  set(Boost_LIBRARIES ${Boost_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})

  add_definitions(-DIMTOOLS_THREADS)
else (IMTOOLS_THREADS)
  set(imtools_threads_src)
endif (IMTOOLS_THREADS)

set(CMAKE_REQUIRED_INCLUDES "${CMAKE_REQUIRED_INCLUDES} ${LIBOPENCV_INCLUDE_DIR}")
set(CMAKE_REQUIRED_LIBRARIES "${LIBOPENCV_CORE_LIB} ${LIBOPENCV_IMGPROC_LIB}
${LIBOPENCV_HIGHGUI_LIB}
")

set(LIBS ${LIBS} ${LIBOPENCV_LIBS} ${Boost_LIBRARIES})
set(common_src src/imtools.cxx src/exceptions.cxx src/log.cxx ${imtools_threads_src})

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
# vim: et ts=2 sts=2 sw=2
