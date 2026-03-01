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
docker-compose run --service-ports plant-watering-dev
```

You will be automatically logged into the container shell.

# Running Unit Tests

Once inside the Docker container, configure the project:

```bash
cd /workspace/Software && just ut
```

# Running Firmware Simulation

Once inside the Docker container, configure the project:

```bash
cd /workspace/Simulation/ && just sim
```

Simulation is at: http://localhost:8000/
