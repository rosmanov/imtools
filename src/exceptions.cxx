#include <cstdarg>
#include <stdexcept>
#include <string>
#include <sstream>
#include "exceptions.hxx"
#include "opencv2/highgui/highgui.hpp"

using std::ostringstream;
using std::string;

namespace imtools {

ErrorException::ErrorException(const char* format, ...)
  : std::runtime_error("")
{
  if (format) {
    va_list args;
    va_start(args, format);
    char message[1024];
    int message_len = vsnprintf(message, sizeof(message), format, args);
    mMsg = string(message, message_len);
    va_end(args);
  }
}


TemplateOutOfBoundsException::TemplateOutOfBoundsException(const cv::Mat& tplMat, const cv::Mat& outMat, const cv::Rect& roi)
{
  ostringstream ostr;

  ostr << "Template is out of bounds, location: " << roi.x << ";" << roi.y
    << ", tpl cols: " << tplMat.cols
    << ", tpl rows: " << tplMat.rows
    << ", out cols: " << outMat.cols
    << ", out rows: " << outMat.rows;

  mMsg = ostr.str();
}


FileWriteErrorException::FileWriteErrorException(string filename)
{
  mMsg = "Failed to write to " + filename + ", check for access permissions";
}


LowMssimException::LowMssimException(double mssim, cv::Rect& roi, string& filename)
{
  ostringstream ostr;

  ostr << filename << ": low MSSIM: " << mssim
    << ", skipped " << roi.width << "x" << roi.height
    << " @ " << roi.x << "," << roi.y;

  mMsg = ostr.str();
}


InvalidCliArgException::InvalidCliArgException(const char* format, ...)
{
  if (format) {
    va_list args;
    va_start(args, format);
    char message[1024];
    int message_len = vsnprintf(message, sizeof(message), format, args);
    mMsg = string(message, message_len);
    va_end(args);
  }
}

}

// vim: et ts=2 sts=2 sw=2
