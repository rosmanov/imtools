#include "imtools.hxx"

using std::ostringstream;
using std::string;
using cv::Mat;
using cv::Point;

namespace imtools
{

TemplateOutOfBoundsException::TemplateOutOfBoundsException(const Mat& tplMat, const Mat& outMat, const Rect& roi)
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


LowMssimException::LowMssimException(double mssim, Rect& roi)
{
  ostringstream ostr;

  ostr << "Low MSSIM value (images considered different): " << mssim
    << ", skipping region " << roi.width << "x" << roi.height
    << " at " << roi.x << "," << roi.y;

  mMsg = ostr.str();
}

}

// vim: et ts=2 sts=2 sw=2
