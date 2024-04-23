import time
import argparse
import zenoh
import threading

from dataclasses import dataclass

from cyclonedds.idl import IdlStruct
from cyclonedds.idl.types import int8, uint8, int16, uint16, int32, uint32, int64, uint64, sequence, array
from cyclonedds.idl.annotations import key, keylist

# key-value pair and baggage data
@dataclass
@keylist(["keyval"])
class KeyedSeq(IdlStruct, typename="KeyedSeq"):
    seq: uint32
    keyval: uint32
    baggage: sequence[uint8]

payload = 0

lock = threading.Lock()

# refactoring callback into a class. encapsulating state and behavior of callback function
# using instance variables to maintain state within class and ensuring each callback instance has its own state avoiding race conditions handling shared resources 
class Chatter_Calback:
    def __init__(self) -> None:
        self.message_count = 0
        self._last_time = None
        self._start_time = time.time()

    #ensuring deterministic execution time by getting the genuine duration of callback function
    def get_duration(self):
        if self._start_time and self._last_time:
            return self._last_time - self._start_time # return the duration in which callback instance is up and receiving messages for throughput calculation 
    
    def __call__(self, sample: zenoh.Sample):
        global payload

        with lock:
            # Increment message count for throughput calculation
            self.message_count += 1
            print ("message count incrementing", self.message_count, flush=False)

            msg = KeyedSeq.deserialize(sample.payload)

            print('time: {t}, size: {size}'.format(t=time.time(), size=len(msg.baggage)),flush = False)

            # each message size
            payload = len(msg.baggage)

            self._last_time = time.time()

def main():

    global payload

    parser = argparse.ArgumentParser(
        prog='listener',
        description='Cyclone DDS listener example')
    parser.add_argument('--config', '-c', dest='config',
        metavar='FILE',
        type=str,
        help='A configuration file.')
    args = parser.parse_args()
   
    # Create Zenoh Config from file if provoded, or a default one otherwise
    conf = zenoh.Config.from_file(args.config) if args.config is not None else zenoh.Config()
    session = zenoh.open(conf)

    chatter_callback = Chatter_Calback()
    sub = session.declare_subscriber('DDSPerfRDataKS', chatter_callback)

    try:
        duration = 15
        time.sleep(duration)
    finally:
        sub.undeclare()
        session.close()
        time.sleep(5) # being sure all threads are exited

    
   
    # Calculate elapsed time based on for how long the callback was up and working based on receiving message events
    elapsed_time = chatter_callback.get_duration()
    print (time.time(),"time elapses here*************************************", elapsed_time, flush = False)
    
    with lock:
        # Calculate message rate (msg/s)
        msg_per_sec = chatter_callback.message_count / elapsed_time

    with lock:
        throughput_tuple = (payload,msg_per_sec)
    
    print ("this is tuple for throughput (msg/s)", throughput_tuple)

if __name__ == '__main__':
    main()



