FROM ubuntu:22.04 AS builder
RUN apt-get update && apt-get install -y build-essential cmake git \
    && rm -rf /var/lib/apt/lists/*
WORKDIR /src
COPY . .
RUN mkdir -p build && cd build && \
    cmake .. -DCMAKE_BUILD_TYPE=Release && \
    make -j$(nproc)

FROM ubuntu:22.04
RUN apt-get update && apt-get install -y ca-certificates \
    && rm -rf /var/lib/apt/lists/*
WORKDIR /app
COPY --from=builder /src/build/shivrai-node /app/shivrai-node
EXPOSE 8545
ENTRYPOINT ["/app/shivrai-node"]
CMD ["8545"]
