FROM ubuntu:22.04 AS builder

RUN apt-get update && apt-get install -y \
    g++ \
    cmake \
    git \
    gperf \
    libssl-dev \
    zlib1g-dev \
    && rm -rf /var/lib/apt/lists/*

# Build TDLib
RUN git clone --depth 1 https://github.com/tdlib/td.git /td && \
    cd /td && mkdir build && cd build && \
    cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr/local && \
    cmake --build . --target install -j$(nproc)

# List what was built (for debugging)
RUN ls -la /usr/local/lib/libtd*

# Copy source and compile
WORKDIR /app
COPY src/ src/

RUN g++ -std=c++17 -O2 \
    -o tg-autoresponder \
    src/main.cpp src/config.cpp src/handler.cpp src/utils.cpp \
    -Isrc -ldl -lpthread

# --- Runtime stage ---
FROM ubuntu:22.04

RUN apt-get update && apt-get install -y \
    libssl3 \
    zlib1g \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

# Copy ALL TDLib shared libraries
COPY --from=builder /usr/local/lib/ /usr/local/lib/

# Update linker cache
RUN ldconfig

# Copy binary
COPY --from=builder /app/tg-autoresponder /app/tg-autoresponder

# Copy pre-authorized session
COPY tdlib_session/ /app/tdlib_session/

WORKDIR /app
ENV LD_LIBRARY_PATH=/usr/local/lib
CMD ["./tg-autoresponder"]
