FROM ghcr.io/bounverif/boostdevel:latest as builder
ARG REELAY_VERSION=main
ARG SIMDJSON_VERSION=3.9.1

RUN apt-get update && export DEBIAN_FRONTEND=noninteractive \
    && apt-get install -y \
      automake \
      autotools-dev \
    #   wget \
    #   bzip2 \
    #   libicu-dev \
      python3 \
      python3-dev \
    && apt-get autoremove -y && apt-get clean -y && rm -rf /var/lib/apt/lists/*

# Install Boost 1.83 from source
# RUN wget https://boostorg.jfrog.io/artifactory/main/release/1.83.0/source/boost_1_83_0.tar.bz2 \
#     && tar --bzip2 -xf boost_1_83_0.tar.bz2 \
#     && cd boost_1_83_0 \
#     && ./bootstrap.sh --prefix=/usr/local --with-libraries=program_options \
#     && ./b2 install \
#     && cd .. \
#     && rm -rf boost_1_83_0 boost_1_83_0.tar.bz2

RUN git clone --depth 1 https://github.com/doganulus/cudd.git /tmp/cudd \
    && cd /tmp/cudd \
    && ./configure && aclocal && autoconf \
    && ./configure --enable-silent-rules --enable-shared --enable-obj \
    && make && make install \
    && cd / && rm -rf /tmp/cudd

RUN git clone --depth 1 --branch ${REELAY_VERSION} https://github.com/doganulus/reelay.git /tmp/reelay \
    && cmake -S/tmp/reelay -B/tmp/reelay/build \
      -DCMAKE_BUILD_TYPE=Release \
      -DBUILD_REELAY_APPS=OFF \
      -DBUILD_TESTS=OFF \
      -DBUILD_PYTHON_BINDINGS=OFF \
    && cmake --build /tmp/reelay/build/ --target install -j$(nproc) \
    && rm -rf /tmp/reelay

RUN git clone --depth 1 --branch v${SIMDJSON_VERSION} https://github.com/lemire/simdjson.git /tmp/simdjson \
    && cmake -S/tmp/simdjson -B/tmp/simdjson/build \
      -DSIMDJSON_BUILD_STATIC_LIB=ON \
    && cmake --build /tmp/simdjson/build/ --target install -j$(nproc) \
    && rm -rf /tmp/simdjson

FROM python:3.10 as builder_timescales
RUN git clone --depth=1 https://github.com/doganulus/timescales.git /tmp/timescales \
    && cd /tmp/timescales \
    && make large \
    && mkdir -p /app/timescales \
    && mv /tmp/timescales/largesuite /app/timescales/ \
    && rm -rf /tmp/timescales

# Build CycloneDDS from source
FROM debian:bookworm-slim as builder_cyclone
COPY --from=builder /usr/local/bin /usr/local/bin
COPY --from=builder /usr/local/lib /usr/local/lib
COPY --from=builder /usr/local/include /usr/local/include
COPY --from=builder /usr/local/share /usr/local/share
COPY --from=builder_timescales /app/timescales /app/timescales

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
