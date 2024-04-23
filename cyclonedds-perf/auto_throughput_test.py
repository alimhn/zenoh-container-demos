import time
import argparse
import zenoh
import subprocess
import re
import matplotlib.pyplot as plt
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

def parse_size(size_str):
    match = re.match(r'^(\d+)([kK])?$', size_str)
    if match:
        size = int(match.group(1))
        unit = match.group(2)
        if unit:
            if unit.lower() == 'k':
                size *= 1024
        return size
    else:
        raise ValueError(f"Invalid size format: {size_str}. Size should be a positive integer with an optional 'k' (for kilobytes).")



def modify_compose_yml(size):
    with open('compose.yml', 'r') as file:
        data = file.read()

    modified_data = re.sub(r'command: cyclonedds performance publish --size \d+k?', f'command: cyclonedds performance publish --size {size}', data)

    with open('compose.yml', 'w') as file:
        file.write(modified_data)


def start_containers():
    subprocess.run(['docker', 'compose', 'up', '-d'])

def stop_containers():
    subprocess.run(['docker', 'compose', 'down'])

def plot_throughput_msg_per_sec(payload_, throughput_):
    print("these are payloads", payload_)
    print("these are throughputs", throughput_)
        
    plt.plot(payload_, throughput_)
    plt.xlabel('Payload size (Bytes)')
    plt.ylabel('msg/s')
    plt.title('Throughput(msg/s)')
    plt.show()


def main():

    parser = argparse.ArgumentParser(description='Automate performance testing of Docker containers.')
    parser.add_argument('sizes_arg', metavar='N', type=str, nargs='+', help='Message sizes to test (in format Nx)')
    args = parser.parse_args()
    sizes_arg = [parse_size(size_str) for size_str in args.sizes_arg]    

    payloads = []
    msgs_per_sec = []
    
    global payload

    for size in sizes_arg:

        print ("inside iteration for message size: ",size)
        modify_compose_yml(size)
        time.sleep(5)
        start_containers()
        time.sleep(5)


        conf = zenoh.Config()
        session = zenoh.open(conf)

        with lock:
            # Start measuring time
            start_time = time.time()
            print (time.time(),"time starts here*******************************************************************************************************************", start_time,flush=False)
        
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
            payloads.append(payload)

        msgs_per_sec.append(msg_per_sec)

        stop_containers()
        time.sleep(12)
        
        with lock:
            # message_count = 0
            payload = 0
            # start_time = None

        

    plot_throughput_msg_per_sec(payloads, msgs_per_sec)    

# make sure containers are down upon start
if __name__ == '__main__':
    try:
        main()
    except Exception as exc:
        stop_containers()
        raise exc
