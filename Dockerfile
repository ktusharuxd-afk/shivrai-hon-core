FROM ubuntu:22.04
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y build-essential libtool autotools-dev automake pkg-config libevent-dev libboost-dev libsqlite3-dev cmake
COPY . /shivrai-hon
WORKDIR /shivrai-hon
RUN cmake -B build -DENABLE_IPC=OFF && cmake --build build -j4
RUN mkdir -p /root/.shivrai-hon
RUN echo "rpcuser=shivraiuser\nrpcpassword=honpassword123\nserver=1\ndaemon=0\ntxindex=1\nrpcbind=0.0.0.0\nrpcallowip=0.0.0.0/0\n[regtest]\nrpcport=8332\nport=18556" > /root/.shivrai-hon/shivrai-hon.conf
EXPOSE 8332
CMD ["./build/bin/bitcoind", "-regtest", "-datadir=/root/.shivrai-hon", "-conf=/root/.shivrai-hon/shivrai-hon.conf"]
