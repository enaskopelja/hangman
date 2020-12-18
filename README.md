# Hangman
A simple client-server application for the game of hangman.

## Build
```
mkdir build
cd build
cmake ..
cmake build all
```

## Usage
Server
```
cd build 
./server port word
```
`word` specifies the word the clients will be guessing

accepts up to 3 clients at the same time
(can be changed by changing the MAX_THREADS constant in `server.c`)

Client
```
cd build 
./client IP-adress port 
```
 