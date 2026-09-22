#include <exception>
#include <iostream>

void run_neighbor_resolver_tests();
void run_signature_tests();
void run_cache_state_tests();

int main() {
  try {
    run_neighbor_resolver_tests();
    run_signature_tests();
    run_cache_state_tests();
    return 0;
  } catch (const std::exception& error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
