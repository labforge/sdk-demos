#ifndef __io_util_hpp_
#define __io_util_hpp_

#include <cstdio>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <array>

namespace labforge::io {
  /**
   * Executes a command and returns the result.
   * @param cmd
   * @return
   */
  std::string exec(const char*cmd);
}

#endif // __io_util_hpp_