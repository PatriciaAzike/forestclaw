set(all ${PROJECT_SOURCE_DIR}/clawpack/transport/2d/all)

add_library(latlong_transport_f OBJECT 
  fdisc.f 
  setprob.f 
  psi.f 
  user_4.6/qinit.f 
  ${all}/transport46_setaux_manifold.f
  ${all}/transport46_set_velocity.f
  ${all}/clawpack46_rpn2cons_fw_manifold.f
  ${all}/clawpack46_rpt2cons_manifold.f
  ${all}/rpn2cons_update_manifold.f
)

# link to clawpatch to get "fclaw2d_metric_terms.i"
target_link_libraries(latlong_transport_f PRIVATE
  FORESTCLAW::CLAWPATCH
)

add_executable(latlong_transport 
  latlong.cpp 
  latlong_options.c 
  latlong_user.cpp 
  ${all}/transport_patch_setup_manifold.c
  ${all}/transport_b4step2_manifold.c
  ${all}/fclaw2d_map_latlong.c 
  $<TARGET_OBJECTS:latlong_transport_f>
)

target_include_directories(latlong_transport PRIVATE 
  ${CMAKE_CURRENT_SOURCE_DIR} 
  ${all}
)

target_link_libraries(latlong_transport PRIVATE 
 FORESTCLAW::CLAWPACK4.6
 
)

add_test(NAME clawpack_transport_2d_latlong COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/regressions.sh WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
set_tests_properties(clawpack_transport_2d_latlong PROPERTIES ENVIRONMENT "${FCLAW_TEST_ENVIRONMENT}")
