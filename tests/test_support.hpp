#pragma once

#include <stdexcept>
#include <string>

#define MB_CHECK(expr)                                                        \
  do {                                                                        \
    if (!(expr)) {                                                            \
      throw std::runtime_error(std::string(__FILE__) + ":" +                 \
                               std::to_string(__LINE__) + ": " #expr);       \
    }                                                                         \
  } while (false)

#define MB_CHECK_NEAR(actual, expected, tolerance)                            \
  do {                                                                        \
    const auto mb_actual = (actual);                                          \
    const auto mb_expected = (expected);                                      \
    const auto mb_delta =                                                     \
        mb_actual > mb_expected ? mb_actual - mb_expected                     \
                                : mb_expected - mb_actual;                    \
    if (mb_delta > (tolerance)) {                                             \
      throw std::runtime_error(std::string(__FILE__) + ":" +                 \
                               std::to_string(__LINE__) + ": " #actual       \
                               " is not near " #expected);                    \
    }                                                                         \
  } while (false)
