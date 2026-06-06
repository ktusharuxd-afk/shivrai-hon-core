FROM ubuntu:22.04
RUN apt-get update && apt-get install -y \
    build-essential libtool autotools-dev automake pkg-config \
    libevent-dev libboost-dev libsqlite3-dev cmake
COPY . /shivrai-hon
WORKDIR /shivrai-hon
RUN cmake -B build -DENABLE_IPC=OFF && cmake --build build -j4
EXPOSE 8444 18445
CMD ["./build/bin/bitcoind", "-regtest", "-rpcuser=shivraiuser", "-rpcpassword=honpassword123", "-rpcbind=0.0.0.0", "-rpcallowip=0.0.0.0/0", "-server=1", "-daemon=0"]
