project(
  'plant-watering',
  'c',
  version : '0.1.0',
  default_options : ['buildtype=debug']
)

unity_inc = include_directories('external/unity/src')
stub_inc  = include_directories('tests/stubs')

test_exe = executable(
  'unit_tests',
  [
    'watering.c',
    'external/unity/src/unity.c',
    'tests/stubs/xc.c',
    'tests/test_pump.c',
  ],
  include_directories : [unity_inc, stub_inc]
)

test('pump_tests', test_exe)