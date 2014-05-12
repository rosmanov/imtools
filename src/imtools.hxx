#ifndef IMTOOLS_HXX
#define IMTOOLS_HXX

#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include <stdio.h>
#include <getopt.h>

#include <sys/stat.h>
#include <stdlib.h>
#include <iostream>

namespace imtools {

int file_exists(const char *filename);

}

#endif // IMTOOLS_HXX
