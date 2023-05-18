#ifndef __test_io_util_hpp__
#define __test_io_util_hpp__

#include <QtTest/QtTest>
#include "io/util.hpp"

namespace labforge::test {

  class TestIO : public QObject {
    Q_OBJECT

  private slots:
    void TestExec();
  private:
  };

}

#endif // __test_io_util_hpp__