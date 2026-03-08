project(
  'plant-watering-simulator',
  'c',
  version : '0.1.0',
  default_options : ['buildtype=debug']
)

fs = import('fs')

# Include directories
software_inc = include_directories('../Software')

# WebAssembly build with ASYNCIFY (no threads)
wasm_link_args = [
  '-sWASM=1',
  '-sASYNCIFY',
  '-sASYNCIFY_STACK_SIZE=32768',
  '-sEXPORTED_FUNCTIONS=["_main"]',
  '-sEXPORTED_RUNTIME_METHODS=["ccall","cwrap"]',
  '-sMODULARIZE=1',
  '-sEXPORT_NAME=createPlantWateringModule',
  '-sALLOW_MEMORY_GROWTH=1',
  '-sINITIAL_MEMORY=16MB',
  '-sTOTAL_STACK=1MB',
  '-sSTRICT_JS=0',
  '-sINVOKE_RUN=0'

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

# Copy files to build directory
fs.copyfile('index.html')

fs.copyfile('css/main.css')
fs.copyfile('css/display.css')
fs.copyfile('css/graph.css')

fs.copyfile('app.bundle.js')