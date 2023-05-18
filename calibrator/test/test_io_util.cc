#include "test_io_util.hpp"

using namespace labforge::test;
using namespace labforge::io;
using namespace std;

void TestIO::TestExec() {
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__)
  // Skip Windows, not needed
#else
  string res = labforge::io::exec("uname");
  QVERIFY2(res.find("Linux") != string::npos, "Command cannot execute");
#endif
}

QTEST_MAIN(TestIO)

