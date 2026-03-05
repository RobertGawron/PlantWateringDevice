project(
  'plant-watering-simulator',
  'c',
  version : '0.1.0',
  default_options : ['buildtype=debug']
)

# Include directories
software_inc = include_directories('../Software')

# WebAssembly build
wasm_link_args = [
  '-pthread',
  '-s', 'WASM=1',
  '-s', 'ASYNCIFY=1',
  '-s', 'ASYNCIFY_IMPORTS=["HW_DELAY_MS"]',
  '-s', 'PROXY_TO_PTHREAD=1',
  '-s', 'PTHREAD_POOL_SIZE=1',
  '-s', 'EXPORTED_FUNCTIONS=["_main","_advanceTick"]',
  '-s', 'EXPORTED_RUNTIME_METHODS=["ccall","cwrap"]',
  '-s', 'MODULARIZE=1',
  '-s', 'EXPORT_NAME=createPlantWateringModule',
  '-s', 'ALLOW_MEMORY_GROWTH=1',
  '-s', 'INITIAL_MEMORY=16MB',
  '-s', 'TOTAL_STACK=1MB',
]

wasm_simulator = executable(
  'plant-watering',
  [
    '../Software/main.c',
    '../Software/hal.c',
    '../Software/watering.c',
  ],
  include_directories : [software_inc],
  c_args : ['-DEMSCRIPTEN', '-DTARGET_HOST'],
  link_args : wasm_link_args,
)

configure_file(
  input : 'plant-watering.css',
  output : 'plant-watering.css',
  copy : true
)

# Copy HTML file to build directory
configure_file(
  input : 'index.html',
  output : 'index.html',
  copy : true
)
