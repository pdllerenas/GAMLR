# Clock-offset approximation over IP networks
We provide an implementation in modern C++ for the clock-offset approximation
over IP networks presented in Edmar Mota-Garcia and Rogelio Hasimoto-Beltran: “A
new model-based clock-offset approximation over IP networks” Computer
Communications, Volume 53, 2014, Pages 26-36, ISSN 0140-3664,
https://doi.org/10.1016/j.comcom.2014.07.006.

## Compilation
`cd build`
`cmake -DCMAKE_BUILD_TYPE=Release ..`
`make`

### Note
If `make/cmake` is not available, and `boost` cannot be installed, we can only compile `client_main.cpp`, as it does not require any of them.
Instead, use `g++ client_main.cpp -O3 -o delay_server`

## Execution
First, execute the server instance (assuming `cd`'d into `build/`):
`./src/delay_server <port>`
`./src/delay_client <server-ip> <port>`

## Molote
### Public IP:
- 148.207.185.20
### Private IP:
- 10.102.1.46

### Available Ports:
- 7500
- 8500
- 8510
- 8520
- 7522
- 9000

### Login:
`ssh user@molote.cimat.mx -p 2235`
`ssh user@148.207.185.20 -p 2235`
`ssh user@10.102.1.46 -p 2235`

### Copy File
`scp -r -P 2235 <file> <user@molote.cimat.mx:/home/user/destination>`

