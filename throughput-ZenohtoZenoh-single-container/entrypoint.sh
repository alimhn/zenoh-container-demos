#!/bin/bash

# Start the publisher
publisher 100000 &

# Start the subscriber
subscriber
