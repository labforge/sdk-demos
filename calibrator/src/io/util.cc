#include "io/util.hpp"

using namespace std;

// Different handling of processes on Win vs Linux
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__)
#define POPEN _popen
#define PCLOSE _pclose
#else
#define POPEN popen
#define PCLOSE pclose
#endif

string labforge::io::exec(const char*cmd) {
  char buffer[128];
  std::string result = "";
  FILE* pipe = POPEN(cmd, "r");
  if (!pipe) throw std::runtime_error("popen() failed!");
  try {
    while (fgets(buffer, sizeof buffer, pipe) != NULL) {
      result += buffer;
    }
  } catch (...) {
    PCLOSE(pipe);
    throw;
  }
  PCLOSE(pipe);
  return result;
}