import time
import argparse
import zenoh

from dataclasses import dataclass

from cyclonedds.idl import IdlStruct
from cyclonedds.idl.types import int8, uint8, int16, uint16, int32, uint32, int64, uint64, sequence, array
from cyclonedds.idl.annotations import key, keylist

# Key-value pair and baggage data
@dataclass
@keylist(["keyval"])
class KeyedSeq(IdlStruct, typename="KeyedSeq"):
    seq: uint32
    keyval: uint32
    baggage: sequence[uint8]

payload = 0


# callback class
class Chatter_Callback:
    def __init__(self) -> None:
        self.message_count = 0
        self._last_time = None
        self._start_time = time.time()

    # Ensure deterministic execution time by calculating the genuine duration of the callback function
    def get_duration(self):
        if self._start_time and self._last_time:
            return self._last_time - self._start_time
    
    def __call__(self, sample: zenoh.Sample):
        global payload

        # Increment message count for throughput calculation
        self.message_count += 1
        msg = KeyedSeq.deserialize(sample.payload)
        # Store exchanged message size
        payload = len(msg.baggage)
        self._last_time = time.time()

def main():

    print ("Calculating throughput(bit/s)...\n\n")
    global payload

    parser = argparse.ArgumentParser(
        prog='listener',
        description='Cyclone DDS listener example')
    parser.add_argument('--config', '-c', dest='config',
                        metavar='FILE',
                        type=str,
                        help='A configuration file.')
    args = parser.parse_args()

    # Create Zenoh Config from file if provided, or a default one otherwise
    conf = zenoh.Config.from_file(args.config) if args.config is not None else zenoh.Config()
    session = zenoh.open(conf)

    chatter_callback = Chatter_Callback()
    sub = session.declare_subscriber('DDSPerfRDataKS', chatter_callback)

    try:
        duration = 15
        time.sleep(duration)
    finally:
        sub.undeclare()
        session.close()

    # Calculate elapsed time based on how long the callback was up and working based on receiving message events
    elapsed_time = chatter_callback.get_duration()

    # Calculate bit rate (bit/s)
    bit_per_sec = (chatter_callback.message_count * 8 * payload) / elapsed_time

    throughput_tuple = (payload, bit_per_sec)

    print("Throughput tuple: [payload(Bytes), rate(bits/s)]:\n\n                ", throughput_tuple)

if __name__ == '__main__':
    main()
