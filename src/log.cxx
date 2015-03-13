#include "log.hxx"

namespace imtools { namespace log {

errors_t error_stack;

void push_error(const char* format, ...)
{
  if (format) {
    va_list args;
    va_start(args, format);
    char message[1024];
    int message_len = vsnprintf(message, sizeof(message), format, args);
    error_stack.push_back(std::string(message, message_len));
    va_end(args);
  }
}


}} // namespace imtools::log
