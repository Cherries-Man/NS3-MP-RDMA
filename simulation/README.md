# MP-RDMA NS-3 simulator

## Build

`./waf configure`

Please note if gcc version > 5, compilation will fail due to some ns3 code style.  If this what you encounter, please use:

`CC='gcc-5' CXX='g++-5' ./waf configure`

## Experiment config

Please see `mix/config.txt` for example.

`mix/config_doc.txt` is a explanation of the example (texts in {..} are explanations).

`mix/fat.txt` is the topology used in HPCC paper's evaluation, and also in PINT paper's HPCC-PINT evaluation.

## Run

The direct command to run is:
`./waf --run 'scratch/mp-rdma-simulator mix/config.txt'`

## Debug

1. The direct command to debug is:
`./waf --run scratch/mp-rdma-simulator --command-template="gdb --args %s mix/config.txt"`
2. At the GDB prompt, enter the run command to start the program.
3. Analyzing errors
    - When the program crashes, GDB stops and you can use a command such as bt (backtrace) to view the call stack. This will show you the order in which each function was called when the crash occurred.
    - Viewing the specific line of code and call stack that caused the crash can help you determine the cause of the crash.

## Files added/edited based on NS3

`internet/model/rocev2-data-header.cc/h`: the header of RDMA data packet
`internet/model/rocev2-ack-header.cc/h`: the header of RDMA ACK packet
`point-to-point/helper/mp-qbb-helper.cc/h`: the helper class for MP-QBB
`point-to-point/model/mp-qbb-channel.cc/h`: the channel of MP-QBB
`point-to-point/model/mp-qbb-net-device.cc/h`: the net-device of MP-RDMA
`point-to-point/model/mp-qbb-remote-channel.cc/h`: the remote channel of MP-QBB
`point-to-point/model/mp-rdma-driver.cc/h`: layer of assigning qp and manage multiple NICs
`point-to-point/model/mp-rdma-hw.cc/h`: the core logic of MP-RDMA
`point-to-point/model/mp-rdma-queue-pair.cc/h`: the queue pair for MP-RDMA
`applications/helper/mp-rdma-client-helper.cc/h`: the helper class for MP-RDMA client
`applications/model/mp-rdma-client.cc/h`: the client of MP-RDMA
`network/utils/custom-header.cc/h`: a customized header class for speeding up header parsing
