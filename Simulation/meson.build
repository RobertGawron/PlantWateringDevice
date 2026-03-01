project(
  'plant-watering-simulator',
  'c',
  version : '0.1.0',
  default_options : ['buildtype=debug']
)

fs = import('fs')

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
  '-s', 'EXPORTED_RUNTIME_METHODS=["ccall","cwrap","addFunction"]',
  '-s', 'STRICT_JS=0',
  '-s', 'MODULARIZE=1',
  '-s', 'EXPORT_NAME=createPlantWateringModule',
  '-s', 'ALLOW_MEMORY_GROWTH=1',
  '-s', 'INITIAL_MEMORY=16MB',
  '-s', 'TOTAL_STACK=1MB',


  '-sRESERVED_FUNCTION_POINTERS=10',

]

executable(
  'plant-watering',
  [
    '../Software/main.c',
    '../Software/hal.c',
    '../Software/watering.c',
  ],
  include_directories : software_inc,
  c_args : ['-DEMSCRIPTEN', '-DTARGET_HOST'],
  link_args : wasm_link_args,
)

# Copy simple files
fs.copyfile('index.html')

fs.copyfile('css/main.css')
fs.copyfile('css/display.css')
fs.copyfile('css/graph.css')

# Copy JavaScript files, one by one for now since Meson doesn't have a way to copy an entire directory
#fs.copyfile('js/app.js')
#fs.copyfile('js/gpio.js')
#fs.copyfile('js/graph.js')
#fs.copyfile('js/logging.js')
#fs.copyfile('js/simulation.js')
#fs.copyfile('js/state.js')
#fs.copyfile('js/ui.js')
#fs.copyfile('js/wasm-loader.js')
fs.copyfile('app.bundle.js')
