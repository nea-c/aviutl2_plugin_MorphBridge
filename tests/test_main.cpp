#include <exception>
#include <iostream>

int main() {
  try {
    return 0;
  } catch (const std::exception& error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
