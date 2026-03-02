project(
  'plant-watering',
  'c',
  version : '0.1.0',
  default_options : ['buildtype=debug']
)

unity_inc = include_directories('external/unity/src')
stub_inc  = include_directories('tests/stubs')

common_sources = [
  'watering.c',
  'external/unity/src/unity.c',
  'tests/stubs/xc.c',
]

# ================================================================
# UNIT TESTS (only for native builds, not WASM)
# ================================================================
if not meson.is_cross_build()
  # Pump tests
  pump_test = executable(
    'test_pump',
    common_sources + [
      'tests/test_pump.c',
    ],
    include_directories : [unity_inc, stub_inc],
    c_args : ['-DUNIT_TEST']
  )

  test('test_pump', pump_test)

  # Initialize tests
  init_test = executable(
    'test_initialize',
    common_sources + [
      'tests/test_initialize.c',
    ],
    include_directories : [unity_inc, stub_inc],
    c_args : ['-DUNIT_TEST']
  )

  test('test_initialize', init_test)

  # Display tests
  display_test = executable(
    'test_display',
    common_sources + [
      'tests/test_display.c',
    ],
    include_directories : [unity_inc, stub_inc],
    c_args : ['-DUNIT_TEST']
  )

  test('test_display', display_test)

  # Update pump duration tests
  update_pump_duration_test = executable(
    'test_update_pump_duration',
    common_sources + [
      'tests/test_update_pump_duration.c',
    ],
    include_directories : [unity_inc, stub_inc],
    c_args : ['-DUNIT_TEST']
  )

  test('test_update_pump_duration', update_pump_duration_test)

  # Handle sensor check tests
  handle_sensor_check_test = executable(
    'test_handle_sensor_check',
    common_sources + [
      'tests/test_handle_sensor_check.c',
    ],
    include_directories : [unity_inc, stub_inc],
    c_args : ['-DUNIT_TEST']
  )

  test('test_handle_sensor_check', handle_sensor_check_test)

  # Handle button tests
  handle_button_test = executable(
    'test_handle_button',
    common_sources + [
      'tests/test_handle_button.c',
    ],
    include_directories : [unity_inc, stub_inc],
    c_args : ['-DUNIT_TEST']
  )

  test('test_handle_button', handle_button_test)

  # Main integration tests
  main_integration_test = executable(
    'test_main_integration',
    common_sources + [
      'tests/test_main_integration.c',
    ],
    include_directories : [unity_inc, stub_inc],
    c_args : ['-DUNIT_TEST']
  )

  test('test_main_integration', main_integration_test)

  # Native simulation executable
  simulation = executable(
    'simulation',
    [
      'main.c',
      'watering.c',
      'tests/stubs/xc.c',
    ],
    include_directories : [stub_inc],
    c_args : ['-DSIMULATION']
  )
endif

# ================================================================
# WEBASSEMBLY BUILD (cross-compile with Emscripten)
# ================================================================
# To build:
#   meson setup buildwasm --cross-file emscripten.ini
#   meson compile -C buildwasm
#
# To run:
#   cd buildwasm
#   python3 -m http.server 8000
#   # Open http://localhost:8000/plant-watering.html

if meson.is_cross_build()

  wasm_compile_args = [
    '-DEMSCRIPTEN',
    '-pthread',
    '-sUSE_PTHREADS=1'
  ]

  wasm_link_args = [
    '-pthread',
    '-sWASM=1',
    '-sASYNCIFY=1',
    '-sUSE_PTHREADS=1',
    '-sPROXY_TO_PTHREAD=1',
    '-sPTHREAD_POOL_SIZE=1',
    '-sEXPORTED_FUNCTIONS=["_main","_advance_tick","_button_press","_button_release","_set_soil_moisture","_get_pump_state","_get_display_state","_get_button_state","_get_soil_state"]',
    '-sEXPORTED_RUNTIME_METHODS=["ccall","cwrap"]',
    '-sMODULARIZE=1',
    '-sEXPORT_NAME=createPlantWateringModule',
    '-sALLOW_MEMORY_GROWTH=1',
    '-sINITIAL_MEMORY=16MB',
    '-sTOTAL_STACK=1MB',
  ]

  wasm_simulator = executable(
    'plant-watering',
    [
      'main.c',
      'watering.c',
      'simulation/xc.c',
    ],
    include_directories : [stub_inc],
    c_args : wasm_compile_args,
    link_args : wasm_link_args,
  )

  configure_file(
    input : 'plant-watering.html',
    output : 'plant-watering.html',
    copy : true
  )
endif