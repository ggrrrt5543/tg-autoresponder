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

# Copy source
WORKDIR /app
COPY src/ src/

# Compile the bot
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

COPY --from=builder /usr/local/lib/libtdjson.so* /usr/local/lib/
COPY --from=builder /app/tg-autoresponder /app/tg-autoresponder

RUN ldconfig

WORKDIR /app
CMD ["./tg-autoresponder"]
