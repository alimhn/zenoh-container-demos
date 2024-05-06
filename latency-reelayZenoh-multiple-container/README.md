
# Multiple container reelayZenoh latency test

This example demonstrates a DDS network and Zenoh DDS bridge in containers. Both DDS and Zenoh networks are isolated from the `host` network. Zenoh router is exposed at `localhost:7474` and allows local Zenoh apps to read the `ddsperf` application's `DDSPerfRDataKS` topic over the bridge.

## Prerequisites

- A container runtime engine such as `docker` or `podman`
- A container compose provider such as `docker-compose` or `podman-compose`
- Python `eclipse-zenoh` and `cyclonedds` libraries

## Run throughput test

```sh
# Launch a DDS network and Zenoh bridge in containers 
# Inside the folder "latency-reelayZenoh-multiple-container"
# You can change message rate inside compose.yml file (e.x. -r 100000)
docker compose up 

# Stop containers
docker compose down