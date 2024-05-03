
# Multiple container Zenoh to Zenoh latency test

This example demonstrates a DDS network and Zenoh DDS bridge in containers. Both DDS and Zenoh networks are isolated from the `host` network. Zenoh router is exposed at `localhost:7474` and allows local Zenoh apps to read the `ddsperf` application's `DDSPerfRDataKS` topic over the bridge.

## Prerequisites

- A container runtime engine such as `docker` or `podman`
- A container compose provider such as `docker-compose` or `podman-compose`
- Python `eclipse-zenoh` and `cyclonedds` libraries


## Run latency test

```sh
# Launch Zenoh network and Zenoh bridge in containers 
# Inside the folder "latency-ZenohtoZenoh-multiple-container"
# Run the following command to start containers:
docker compose up -d

# Open a terminal and execute following command to get into container
docker exec -it latency-zenohtozenoh-multiple-container-talker-1 bash

# Run the latency test for different message rates with flag -r
publisher -r 100000

# Stop containers after test is finished
docker compose down
```


## ToDo

Run the compose command with environment variable indicating payload size