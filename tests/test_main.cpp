#include <exception>
#include <iostream>

void run_neighbor_resolver_tests();

int main() {
  try {
    run_neighbor_resolver_tests();
    return 0;
  } catch (const std::exception& error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
