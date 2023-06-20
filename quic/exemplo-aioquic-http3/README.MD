# QUIC/HTTP3 Server-Client Example

This repository contains a simple example of a QUIC/HTTP3 server and client using a digital certificate for secure communication.

## Prerequisites

Before running the server and client, make sure you have the following:

- Python 3 installed
- aioquic library installed (you can install it using `pip install aioquic`)

If you don't have these dependences you can execute the archive

```cl 
bash verificacao.sh 
```

## Server

To run the server, follow these steps:

1. Place your digital certificate image in a directory accessible by the server.
2. Open a terminal and navigate to the project directory.
3. Run the following command:

```cl 
bash server-execucao.sh 
```

## Client

In another terminal window(after you start the server correctly), type:

```cl
bash client-execucao.sh
```
