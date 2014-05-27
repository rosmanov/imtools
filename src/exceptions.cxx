#include "imtools.hxx"

using std::ostringstream;
using std::string;
using cv::Mat;
using cv::Point;

namespace imtools
{

TemplateOutOfBoundsException::TemplateOutOfBoundsException(const Mat& tplMat, const Mat& outMat, const Point& pt)
{
  ostringstream ostr;

  ostr << "Template is out of bounds, location: " << pt.x << ";" << pt.y
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


InvalidTargetDimensionsException::InvalidTargetDimensionsException(const cv::Mat& orgMat, const cv::Mat& targetMat)
{
  ostringstream ostr;

  ostr << "Invalid target dimensions, original: " << orgMat.cols << "x" << orgMat.rows
    << ", target: " << targetMat.cols << "x" << targetMat.rows;

  mMsg = ostr.str();
}

}

// vim: et ts=2 sts=2 sw=2
