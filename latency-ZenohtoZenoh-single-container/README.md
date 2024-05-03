
# Single container Zenoh to Zenoh latency test

This example demonstrates a DDS network and Zenoh DDS bridge in containers. Both DDS and Zenoh networks are isolated from the `host` network. Zenoh router is exposed at `localhost:7474` and allows local Zenoh apps to read the `ddsperf` application's `DDSPerfRDataKS` topic over the bridge.

## Prerequisites

- A container runtime engine such as `docker` or `podman`
- A container compose provider such as `docker-compose` or `podman-compose`
- Python `eclipse-zenoh` and `cyclonedds` libraries


## Run latency test

```sh
# Launch Zenoh network and Zenoh bridge in containers 
# Inside the folder "latency-ZenohtoZenoh-single-container"
# Build the container:
docker build -t ls-zenoh-zenoh-img .

# Run the docker:
docker run -it -d ls-zenoh-zenoh-img

# Now run the following command and copy our container id
docker ps 

# Open a terminal and execute following command to get into container
docker exec -it <container_id> bash

# Run the latency test for different message rates with flag -r
publisher -r 100000


# Exit and Stop container 
## CTRL-C and CTRL-D
docker stop <container_id>

# Remove container
docker rm <container_id>

# You can also remove docker image
docker rmi ls-zenoh-zenoh-img:latest 


## ToDo
bash script to change the message payload for a run