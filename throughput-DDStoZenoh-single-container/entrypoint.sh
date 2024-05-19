#!/bin/bash

# Start the CycloneDDS performance publisher
cyclonedds performance publish --size 40 &

# Start the CycloneDDS performance subscriber
# cyclonedds performance subscribe &

# Start the Zenoh-to-DDS bridge
zenoh-bridge-dds -c /etc/zenohd/zenohd.yaml &

# Start the Zenoh router
zenohd  & 

# Keep the container running
tail -f /dev/null
