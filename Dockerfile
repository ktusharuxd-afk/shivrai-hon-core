FROM ubuntu:22.04
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y build-essential libtool autotools-dev automake pkg-config libevent-dev libboost-dev libsqlite3-dev cmake
COPY . /shivrai-hon
WORKDIR /shivrai-hon
RUN cmake -B build -DENABLE_IPC=OFF && cmake --build build -j4
EXPOSE 8332
CMD ["./build/bin/bitcoind", "-regtest", "-datadir=/root/.shivrai-hon", "-rpcuser=shivraiuser", "-rpcpassword=honpassword123", "-rpcbind=0.0.0.0", "-rpcallowip=0.0.0.0/0", "-rpcport=8332", "-server=1", "-daemon=0", "-txindex=1"]
