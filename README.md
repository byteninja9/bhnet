# Net Tool

**Net Tool** is a versatile network utility designed for various network interactions and operations. It supports listening for incoming connections, executing commands, file uploads, and sending/receiving data. Ideal for network administrators, penetration testers, and cybersecurity enthusiasts, this tool provides a robust set of features for managing network connections and performing tasks remotely.

## Features

- **Listening Mode**: Set up a server to listen for incoming connections on a specified host and port.
- **File Upload**: Upload files from the client to the server with a specified destination path.
- **Command Execution**: Execute commands on the server upon connection and retrieve the output.
- **Interactive Command Shell**: Open a command shell on the server to interact with clients in real-time.
- **Client-Server Communication**: Send and receive data between the client and server, with support for command execution and file transfer.

## Usage

To get started, use the following command-line options:

```bash
Usage: bhnet.py -t target_host -p port
-l --listen - Listen on [host]:[port] for incoming connections
-e --execute=file_to_run - Execute the given file upon receiving connections
-c --command - Initialize a command shell
-u --upload=destination - Upload a file and write to [destination] upon receiving connection
-t --target=target_host - Specify the target host
-p --port=port - Specify the port number

```

### Examples

- Start a server to listen for incoming connections and open a command shell:
```bash 
python bhnet.py -t 192.168.0.1 -p 5555 -l -c
```
- Upload a file from the client to the server:
```bash
python bhnet.py -t 192.168.0.1 -p 5555 -l -u=c:\target.exe 
```
- Execute a command on the server:
```bash 
python bhnet.py -t 192.168.0.1 -p 5555 -l -e="cat /etc/passwd"
```


## Dependencies

- `colorama` for colored output

Install the dependencies using pip:

```bash
pip install colorama
```

## Contributing

Feel free to open issues or submit pull requests. Contributions are welcome!