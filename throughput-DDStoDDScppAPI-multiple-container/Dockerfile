# Build CycloneDDS from source
FROM debian:bookworm-slim as builder_cyclone


# Install dependencies
RUN apt update && apt install -y \
        cmake \
        g++ \
        git \
        make \
        libssl-dev \
    && apt-get clean && rm -rf /var/lib/apt/lists/*

ARG CYCLONEDDS_VERSION=0.10.4
ARG CYCLONEDDS_BUILD_TYPE=RelWithDebInfo

# Clone and build CycloneDDS
RUN git clone https://github.com/eclipse-cyclonedds/cyclonedds.git /cyclonedds && \
    cd /cyclonedds && \
    git checkout --quiet ${CYCLONEDDS_VERSION} && \
    mkdir build && cd build && \
    cmake -DCMAKE_INSTALL_PREFIX=/usr/local -DCMAKE_BUILD_TYPE=${CYCLONEDDS_BUILD_TYPE} .. && \
    cmake --build . --target install
    
ARG CYCLONEDDSCXX_BUILD_TYPE=RelWithDebInfo

# Clone and build CycloneDDS CXX
RUN git clone https://github.com/eclipse-cyclonedds/cyclonedds-cxx.git /cyclonedds-cxx && \
    cd /cyclonedds-cxx && \
    git checkout --quiet ${CYCLONEDDS_VERSION} && \
    mkdir build && cd build && \
    cmake -DCMAKE_INSTALL_PREFIX=/cyclonedds-cxx/install \
          -DCMAKE_BUILD_TYPE=${CYCLONEDDSCXX_BUILD_TYPE} \
          -DCMAKE_PREFIX_PATH=/usr/local \
          -DENABLE_TYPELIB=YES \
          -DENABLE_TOPIC_DISCOVERY=YES \
          -DENABLE_QOS_PROVIDER=YES \
          .. && \
    cmake --build . && \
    cmake --build . --target install

# Build the throughput example directly from the source directory
WORKDIR /app
COPY . .

RUN cmake -DCMAKE_PREFIX_PATH="/usr/local;/cyclonedds-cxx/install" . && \
    cmake --build .
