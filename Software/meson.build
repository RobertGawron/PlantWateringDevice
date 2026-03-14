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
# UNIT TESTS
# ================================================================

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



