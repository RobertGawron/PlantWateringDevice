cd /workspace/Simulation
meson setup buildwasm --cross-file emscripten.ini

# Now compile
meson compile -C buildwasm

# Check if files were created
ls -lh buildwasm/plant-watering.*

cd buildwasm

#use mrun to send the files to the browser, it has bellow headers to allow the browser to load the files correctly
#Cross-Origin-Opener-Policy: same-origin
#Cross-Origin-Embedder-Policy: require-corp


emrun --no_browser --port 8000 .
