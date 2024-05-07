#!/bin/bash

# Start the bridge
zenoh-bridge-dds -c /etc/zenohd/zenohd.yaml &
# Start the subscriber (pong)
subscriber  &

# Start ddsperf, pass the arguments to the script
ddsperf $@

# Example run:
# entrypoint.sh --help
# entrypoint.sh ping 100Hz



# Alternatively start processes in a different tty:
# apt-get install -y screen
# screen -dmS zenoh_bridge_dds_session zenoh-bridge-dds -c /etc/zenohd/zenohd.yaml
# screen -dmS subscriber_session subscriber

