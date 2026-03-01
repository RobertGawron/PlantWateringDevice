# Purpose

This document describes how to run unit tests and optionally generate a code coverage report.

# Prerequisites

* Docker installed and running

# Building and Launching the Docker Image

Unit tests are executed inside Docker to ensure isolation from the host environment.

From the project root directory, build the Docker image:


docker-compose build
Launch the Docker container:

Bash

docker-compose run --rm plant-watering-dev
You will be automatically logged into the container shell.

Running Unit Tests
Once inside the Docker container, configure the project:

Bash

cd /workspace/Software
meson setup build
Note: This step only needs to be done once (unless the build directory is removed).

Compile the tests:

Bash

meson compile -C build
Run the tests:

Bash

meson test -C build
Generating Code Coverage (Optional)
Enable coverage when configuring the build:

Bash

meson setup build -Db_coverage=true --wipe
meson test -C build
Generate the HTML coverage report:

Bash

gcovr -r . \
  --exclude 'external/.*' \
  --exclude 'tests/.*' \
  --html --html-details \
  -o coverage.html
Open coverage.html in a browser to view the report.