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
    cmake \
    wget \
    && rm -rf /var/lib/apt/lists/*

# ------------------------------------------------------------
# Python virtual environment for docs tools
# ------------------------------------------------------------
RUN python3 -m venv /opt/venv
ENV PATH="/opt/venv/bin:$PATH"

RUN pip install --no-cache-dir --upgrade pip && \
    pip install --no-cache-dir coverxygen

# ------------------------------------------------------------
# Emscripten SDK
# ------------------------------------------------------------
RUN git clone https://github.com/emscripten-core/emsdk.git /opt/emsdk && \
    cd /opt/emsdk && \
    ./emsdk install latest && \
    ./emsdk activate latest

# Set up Emscripten environment permanently
ENV EMSDK=/opt/emsdk
ENV PATH="/opt/emsdk:/opt/emsdk/upstream/emscripten:${PATH}"

# Source emsdk_env.sh for every bash session
RUN echo 'source /opt/emsdk/emsdk_env.sh > /dev/null 2>&1' >> /root/.bashrc

CMD ["bash"]