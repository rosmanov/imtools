#include "imtools.hxx"

int
imtools::file_exists(const char *filename)
{
  struct stat st;
  return (stat(filename, &st) == 0);
}
