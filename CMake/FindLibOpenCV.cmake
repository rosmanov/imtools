foreach(prefix IN ITEMS /usr/local /usr /opt /opt/local "$ENV{HOME}")
  list(APPEND LibOpenCV_INCLUDE_PATHS "${prefix}/include")
  list(APPEND LibOpenCV_LIB_PATHS "${prefix}/lib")
endforeach()

find_path(LIBOPENCV_INCLUDE_DIR opencv2/imgproc/imgproc.hpp PATHS ${LibOpenCV_INCLUDE_PATHS})
find_library(LIBOPENCV_CORE_LIB NAMES opencv_core PATHS ${LibOpenCV_LIB_PATHS})
find_library(LIBOPENCV_IMGPROC_LIB NAMES opencv_imgproc PATHS ${LibOpenCV_LIB_PATHS})
find_library(LIBOPENCV_HIGHGUI_LIB NAMES opencv_highgui PATHS ${LibOpenCV_LIB_PATHS})

set(LIBOPENCV_LIBS ${LIBOPENCV_CORE_LIB} ${LIBOPENCV_IMGPROC_LIB} ${LIBOPENCV_HIGHGUI_LIB})

if (LIBOPENCV_INCLUDE_DIR AND LIBOPENCV_CORE_LIB AND LIBOPENCV_IMGPROC_LIB AND LIBOPENCV_HIGHGUI_LIB)
  message(STATUS "Found libopencv_core: ${LIBOPENCV_CORE_LIB}")
else ()
  message(FATAL_ERROR "Could NOT find libopencv.")
endif ()

if (LIBOPENCV_IMGPROC_LIB)
  message(STATUS "Found libopencv_imgproc: ${LIBOPENCV_IMGPROC_LIB}")
endif (LIBOPENCV_IMGPROC_LIB)

if (LIBOPENCV_HIGHGUI_LIB)
  message(STATUS "Found libopencv_highgui: ${LIBOPENCV_HIGHGUI_LIB}")
endif (LIBOPENCV_HIGHGUI_LIB)

mark_as_advanced(
  LIBOPENCV_CORE_LIB
  LIBOPENCV_IMGPROC_LIB
  LIBOPENCV_HIGHGUI_LIB
  LIBOPENCV_INCLUDE_DIR
  LIBOPENCV_LIBS
)
