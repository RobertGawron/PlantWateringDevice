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
    just \
    curl \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

# ------------------------------------------------------------
# Python virtual environment for docs tools
# ------------------------------------------------------------
RUN python3 -m venv /opt/venv
ENV PATH="/opt/venv/bin:$PATH"

RUN pip install --no-cache-dir --upgrade pip && \
    pip install --no-cache-dir coverxygen

# ------------------------------------------------------------
# Node.js + TypeScript
# ------------------------------------------------------------
RUN curl -fsSL https://deb.nodesource.com/setup_22.x | bash - && \
    apt-get install -y --no-install-recommends nodejs && \
    rm -rf /var/lib/apt/lists/*

RUN npm install -g typescript ts-node

# ------------------------------------------------------------
# Emscripten SDK
# ------------------------------------------------------------
RUN git clone https://github.com/emscripten-core/emsdk.git /opt/emsdk && \
    cd /opt/emsdk && \
    ./emsdk install latest && \
    ./emsdk activate latest && \
    echo "NODE_JS = '/usr/bin/node'" >> /opt/emsdk/.emscripten

ENV EMSDK=/opt/emsdk
ENV PATH="/opt/emsdk:/opt/emsdk/upstream/emscripten:${PATH}"

RUN echo 'source /opt/emsdk/emsdk_env.sh > /dev/null 2>&1' >> /root/.bashrc

# Rename emsdk's node directory to prevent PATH conflict with system node
# (make finds /opt/emsdk/node directory before /usr/bin/node binary → "Permission denied")
RUN mv /opt/emsdk/node /opt/emsdk/_node_runtime && \
    sed -i 's|/emsdk/node/|/emsdk/_node_runtime/|g' /opt/emsdk/.emscripten

# ------------------------------------------------------------
# Frama-C Dependencies
# ------------------------------------------------------------
RUN apt-get update && apt-get install -y --no-install-recommends \
    # OCaml and OPAM
    opam \
    # Build dependencies
    autoconf \
    automake \
    pkg-config \
    libgmp-dev \
    libmpfr-dev \
    # GUI dependencies (GTK)
    libgtk-3-dev \
    libgtksourceview-3.0-dev \
    # SMT Solver
    z3 \
    libz3-dev \
    # X11 support for GUI
    x11-apps \
    xauth \
    # Useful tools
    graphviz \
    # Additional build deps
    m4 \
    unzip \
    bubblewrap \
    xdd \
    && rm -rf /var/lib/apt/lists/*

# ------------------------------------------------------------
# Initialize OPAM
# ------------------------------------------------------------
RUN opam init --disable-sandboxing --yes --bare

# Create OCaml switch
RUN opam switch create 4.14.1 --yes

# Set OPAM environment
ENV OPAM_SWITCH_PREFIX="/root/.opam/4.14.1"
ENV CAML_LD_LIBRARY_PATH="/root/.opam/4.14.1/lib/stublibs:/root/.opam/4.14.1/lib/ocaml/stublibs:/root/.opam/4.14.1/lib/ocaml"
ENV OCAML_TOPLEVEL_PATH="/root/.opam/4.14.1/lib/toplevel"
ENV PATH="/root/.opam/4.14.1/bin:${PATH}"

# Update OPAM
RUN opam update --yes

# ------------------------------------------------------------
# Install Frama-C with WP and GUI
# ------------------------------------------------------------

# Install system dependencies for frama-c (OPAM 2.1+ has depext built-in)
RUN opam install --yes --deps-only frama-c || true

# Install additional provers
RUN opam install --yes frama-c alt-ergo

# Install Why3
RUN opam install --yes why3

# ------------------------------------------------------------
# Install CVC5 (binary, avoids opam/python conflicts)
# ------------------------------------------------------------
RUN wget -q https://github.com/cvc5/cvc5/releases/download/cvc5-1.0.8/cvc5-Linux -O /usr/local/bin/cvc5 && \
    chmod +x /usr/local/bin/cvc5

# ------------------------------------------------------------
# Configure Why3 Provers
# ------------------------------------------------------------
RUN why3 config detect

# ------------------------------------------------------------
# Verify Installations
# ------------------------------------------------------------
RUN frama-c --version && \
    why3 --version && \
    alt-ergo --version && \
    z3 --version && \
    cvc5 --version

# ------------------------------------------------------------
# noVNC + VNC server — browser-based GUI (Doro Wu style)
#   Run `start-vnc` inside the container, then open
#   http://localhost:6080 in your browser.
# ------------------------------------------------------------
RUN apt-get update && apt-get install -y --no-install-recommends \
    tigervnc-standalone-server \
    novnc \
    python3-websockify \
    fluxbox \
    xterm \
    xfonts-base \
    fonts-dejavu-core \
    dbus-x11 \
    librsvg2-common \
    adwaita-icon-theme \
    # Electron dependencies (for Ivette)
    libnss3 \
    libatk1.0-0 \
    libatk-bridge2.0-0 \
    libcups2 \
    libdrm2 \
    libxkbcommon0 \
    libxcomposite1 \
    libxdamage1 \
    libxrandr2 \
    libgbm1 \
    libpango-1.0-0 \
    libcairo2 \
    libasound2t64 \
    && rm -rf /var/lib/apt/lists/* \
    && ln -sf /usr/share/novnc/vnc.html /usr/share/novnc/index.html

# Launch script — call `start-vnc` when you need the GUI
RUN printf '#!/bin/sh\n\
echo "Starting VNC on :1 and noVNC on http://0.0.0.0:6080 ..."\n\
Xtigervnc :1 -geometry 1280x800 -depth 24 -SecurityTypes None -AlwaysShared &\n\
sleep 1\n\
DISPLAY=:1 fluxbox &\n\
websockify --web /usr/share/novnc 6080 localhost:5901\n' \
    > /usr/local/bin/start-vnc && chmod +x /usr/local/bin/start-vnc

# ------------------------------------------------------------
# Ivette (new Frama-C GUI)
#   Requires VNC running: start-vnc first, then ivette
# ------------------------------------------------------------
RUN npm install --global yarn
RUN ivette || true
ENV ELECTRON_DISABLE_SANDBOX=1
ENV DBUS_SESSION_BUS_ADDRESS=/dev/null
ENV ELECTRON_DISABLE_GPU=1
ENV DBUS_SESSION_BUS_ADDRESS="unix:path=/dev/null"

# Set DISPLAY for GUI (VNC virtual display)
ENV DISPLAY=:1

EXPOSE 6080

# Add OPAM env to bashrc
RUN echo 'eval $(opam env)' >> /root/.bashrc

CMD ["bash"]