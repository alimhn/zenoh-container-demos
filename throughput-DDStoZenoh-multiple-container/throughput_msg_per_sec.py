import time
import argparse
import zenoh

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


message_count = 0
#callback 
def chatter_callback(sample: zenoh.Sample):
    global message_count
    message_count += 1

def main():

    global message_count
    print ("Calculating throughput(msg/s)...\n\n")

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

    sub = session.declare_subscriber('DDSPerfRDataKS', chatter_callback)

    try:
        duration = 15
        time.sleep(duration)
    finally:
        sub.undeclare()
        session.close()
  
    
    # Calculate message rate (msg/s)
    msg_per_sec = message_count / duration
        
    print("Throughput as rate(msg/s)]: ", msg_per_sec)

if __name__ == '__main__':
    main()



