
# Multiple container DDS to Zenoh latency test

This example demonstrates a DDS network and Zenoh DDS bridge in containers. Both DDS and Zenoh networks are isolated from the `host` network. Zenoh router is exposed at `localhost:7474` and allows local Zenoh apps to read the `ddsperf` application's `DDSPerfRDataKS` topic over the bridge.

## Prerequisites

- A container runtime engine such as `docker` or `podman`
- A container compose provider such as `docker-compose` or `podman-compose`
- Python `eclipse-zenoh` and `cyclonedds` libraries


## Run latency test

```sh
# Launch Zenoh network and Zenoh bridge in containers 
# Inside the folder "latency-DDStoZenoh-multiple-container"
# Run the containers, you can change message rate in compose.yml file (e.g. ddsperf ping 100000Hz) :
docker compose up 

# You can see the latency result in terminal 

# Stop containers
docker compose down

## ToDo
bash script to change the message payload for a run