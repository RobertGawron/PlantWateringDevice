# Purpose

This document describes how to run unit tests and optionally generate a code coverage report.

# Prerequisites

* Docker installed and running

# Building and Launching the Docker Image

Unit tests are executed inside Docker to ensure isolation from the host environment.

From the project root directory, build the Docker image:

```bash
docker-compose build
```

Launch the Docker container:

```bash
docker-compose run --rm plant-watering-dev
```

You will be automatically logged into the container shell.

# Running Unit Tests

Once inside the Docker container, configure the project:

```bash
cd /workspace/Software && meson setup build -Db_coverage=true --wipe
```

Note: This step only needs to be done once (unless the build directory is removed).

Compile and run the tests:

```bash
meson test -C build
```

Generating Code Coverage (Optional)

```bash
gcovr -r . \
  --exclude 'external/.*' \
  --exclude 'tests/.*' \
  --html --html-details \
  -o coverage.html
```

Open coverage.html in a browser to view the report.
