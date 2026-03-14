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

# Formal Verification

## Interactive version using Ivette

Once inside the Docker container launch once:

```bash
start-vnc&
```

Then:

```bash
cd /workspace/Software && just ivette
```

On host machine open http://localhost:6080/ in web browser.

## Non-interactive version

```bash
cd /workspace/Software && just prove-report
```

Results are in DevOps/Build/Verification.

# Running Unit Tests

Once inside the Docker container:

```bash
cd /workspace/Software && just ut
```

Results (including code coverage) are in DevOps/Build/Coverage.

# Running Firmware Simulation

Once inside the Docker container:

```bash
cd /workspace/Simulation/ && just sim
```

On host machine open http://localhost:8000/ in web browser.
