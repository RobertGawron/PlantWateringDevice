project(
  'plant-watering',
  'c',
  version : '0.1.0',
  default_options : ['buildtype=debug']
)

unity_inc = include_directories('external/unity/src')

test_exe = executable(
  'unit_tests',
  [
    'external/unity/src/unity.c',
    'tests/test_pump.c',
  ],
  include_directories : unity_inc,
)

test('pump_tests', test_exe)