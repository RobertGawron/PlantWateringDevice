FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive
WORKDIR /workspace

# ------------------------------------------------------------
# System + Build tools
# ------------------------------------------------------------
RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    meson \
    ninja-build \
    git \
    gcovr \
    lcov \
    graphviz \
    doxygen \
    python3 \
    python3-pip \
    python3-venv \
    cppcheck \
    clang-tidy \
    clang-format \
    tree \
    vim \
    && rm -rf /var/lib/apt/lists/*

# ------------------------------------------------------------
# Python virtual environment for docs tools
# ------------------------------------------------------------
RUN python3 -m venv /opt/venv
ENV PATH="/opt/venv/bin:$PATH"

RUN pip install --no-cache-dir --upgrade pip && \
    pip install --no-cache-dir coverxygen

CMD ["bash"]