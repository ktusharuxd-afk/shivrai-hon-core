# ---- build stage ----
FROM ubuntu:22.04 AS build
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential cmake git curl ca-certificates \
    libssl-dev libsqlite3-dev nlohmann-json3-dev \
 && rm -rf /var/lib/apt/lists/*
WORKDIR /src
COPY . .
RUN mkdir -p build && cd build \
 && cmake .. -DCMAKE_BUILD_TYPE=Release \
 && make -j"$(nproc)" \
 && ctest --output-on-failure

# ---- runtime stage ----
FROM ubuntu:22.04
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y --no-install-recommends \
    libssl3 libsqlite3-0 ca-certificates \
 && rm -rf /var/lib/apt/lists/*
WORKDIR /app
COPY --from=build /src/build/shivrai-node /usr/local/bin/shivrai-node
COPY --from=build /src/build/shivrai-cli  /usr/local/bin/shivrai-cli
COPY --from=build /src/static             /app/static
WORKDIR /app
EXPOSE 8545
VOLUME ["/data"]
ENTRYPOINT ["shivrai-node","--db","/data/shivrai.db","--host","0.0.0.0","--port","8545"]
