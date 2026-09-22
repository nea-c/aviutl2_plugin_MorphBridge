#include <exception>
#include <iostream>

void run_neighbor_resolver_tests();
void run_signature_tests();
void run_cache_state_tests();
void run_endpoint_capture_tests();
void run_canvas_tests();
void run_sdf_encoding_tests();
void run_standard_transform_tests();
void run_sdf_plan_tests();
void run_gpu_renderer_contract_tests();
void run_plugin_registration_tests();
void run_filter_object_tests();

int main() {
  try {
    run_neighbor_resolver_tests();
    run_signature_tests();
    run_cache_state_tests();
    run_endpoint_capture_tests();
    run_canvas_tests();
    run_sdf_encoding_tests();
    run_standard_transform_tests();
    run_sdf_plan_tests();
    run_gpu_renderer_contract_tests();
    run_plugin_registration_tests();
    run_filter_object_tests();
    return 0;
  } catch (const std::exception& error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
