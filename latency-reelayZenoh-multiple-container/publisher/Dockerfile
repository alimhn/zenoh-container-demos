FROM ghcr.io/bounverif/boostdevel:latest as publisher_builder

ARG ZENOH_VERSION=0.10.1-rc
ARG ZENOHC_VERSION=${ZENOH_VERSION}
ARG REELAY_VERSION=main
ARG SIMDJSON_VERSION=3.9.1

ARG RUST_VERSION=1.72.1
ARG RUST_PROFILE=minimal
ARG RUST_TOOLCHAIN=${RUST_VERSION}-x86_64-unknown-linux-gnu
ENV CARGO_HOME="/usr/local/cargo"

ENV RUSTUP_HOME="/usr/local/rustup"
ENV PATH "/usr/local/cargo/bin:/usr/local/rustup:${PATH}"

RUN git clone --depth=1 https://github.com/doganulus/timescales.git /tmp/timescales \
    && cd /tmp/timescales \
    && make large \
    && mkdir -p /app/publisher/timescales \
    && mv /tmp/timescales/largesuite /app/publisher/timescales/ \
    && rm -rf /tmp/timescales

RUN curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | \
    sh -s -- -y --no-modify-path --profile ${RUST_PROFILE} --default-toolchain ${RUST_VERSION}

RUN git clone --depth 1 --branch ${ZENOHC_VERSION} https://github.com/eclipse-zenoh/zenoh-c.git /tmp/zenoh-c && \
    cmake -S/tmp/zenoh-c -B/tmp/zenoh-c/build \
        -DCMAKE_BUILD_TYPE=Release \
        -DZENOHC_CARGO_CHANNEL=${RUST_TOOLCHAIN} \
        -DZENOHC_INSTALL_STATIC_LIBRARY=TRUE \
        && \
    cmake --build /tmp/zenoh-c/build/ --target install -j$(nproc) && \
    rm -rf /tmp/zenoh-c


COPY . /app/publisher
WORKDIR /app/publisher

RUN cmake --preset default && \
    cmake --build  --target install -j$(nproc) --preset default 

FROM debian:bookworm-slim
COPY --from=publisher_builder /usr/local/bin/publisher /usr/local/bin

RUN apt-get update && export DEBIAN_FRONTEND=noninteractive \
    && apt-get install -y \
      libssl-dev \
      libzmq3-dev \
    && apt-get autoremove -y && apt-get clean -y && rm -rf /var/lib/apt/lists/*

COPY --from=publisher_builder /app/publisher /app/publisher
WORKDIR /app/publisher