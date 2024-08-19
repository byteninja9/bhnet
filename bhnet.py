#!/usr/bin/python3

import sys
import threading
import subprocess
import getopt
import socket
from colorama import Style, Fore

listen = False
command = False
upload = False
execute = ""
target = ""
upload_destination = ""
port = 0

def usage():
    print(Style.BRIGHT + Fore.CYAN + ''' 
 ________      ________    ___  ___     
|\   __  \    |\   __  \  |\  \|\  \    
\ \  \|\ /_   \ \  \|\  \ \ \  \_\  \   
 \ \   __  \   \ \   ____\ \ \   __  \    Net Tool
  \ \  \|\  \   \ \  \___|  \ \  \ \  \ 
   \ \_______\   \ \__\      \ \__\ \__\      
    \|_______|    \|__|       \|__|\|__|
                                        
                Recreated By Byteninja9
    ''')
    print("Usage: bhpnet.py -t target_host -p port")
    print('''-l --listen                         - listen  on [host]:[port] for 
                                                   incoming connections''')
    print('''-e --execute=file_to_run            - execute the given file upon  
                                                   receiving connections''')
    print('''-c --command                        - initialize a command shell''')
    print('''-u --upload=destination             - upon receiving connection upload a 
                                                   file and write to [destination]''')
    print()

    print(Style.BRIGHT + Fore.GREEN + "Examples: ")
    print("bhpnet.py -t 192.168.0.1 -p 5555 -l -c")
    print("bhpnet.py -t 192.168.0.1 -p 5555 -l -c -u=c:\\target.exe")
    print("bhpnet.py -t 192.168.0.1 -p 5555 -l -e=\"cat /etc/passwd\"")
    print("echo 'ABCDEFGH' | ./bhpnet.py -t 192.168.0.1 -p 135")
    sys.exit(0)

def client_sender(buffer):
    client = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    
    try:
        client.connect((target, port))

        if len(buffer):
            client.send(buffer.encode())
        
        while True:
            recv_len = 1
            res = ""

            while recv_len:
                data = client.recv(4096)
                recv_len = len(data)
                res += data.decode()

                if recv_len < 4096:
                    break
            
            print(res)
            buffer = input(b"").strip()
            buffer += "\n"

            client.send(buffer.encode())

    except Exception as e:
        print(Style.BRIGHT + Fore.RED + f"[+] Exception: {e}. Exiting." + Fore.RESET)
        client.close()

def client_handler(client_socket):
    global upload
    global execute
    global command

    print("[+] Client connected")

    if len(upload_destination):
        file_buffer = b""

        while True:
            data = client_socket.recv(1024)
            if not data:
                break
            else:
                file_buffer += data

        try:
            with open(upload_destination, 'wb') as file_descriptor:
                file_descriptor.write(file_buffer)
            client_socket.send((Style.BRIGHT + Fore.GREEN + f"Successfully saved to {upload_destination}\r\n" + Fore.RESET).encode())

        except Exception as e:
            client_socket.send((Style.BRIGHT + Fore.RED + f"Failed to save file to {upload_destination}: {e}").encode())

    if len(execute):
        output = run_command(execute)
        client_socket.send(output.encode())

    if command:
        while True:
            try:
                client_socket.send((Style.BRIGHT + Fore.LIGHTCYAN_EX + "<BHP:#> " +  Fore.RESET).encode())
                cmd_buffer = b""
                while b"\n" not in cmd_buffer:
                    data = client_socket.recv(1024)
                    if not data:
                        break
                    cmd_buffer += data

                cmd_buffer = cmd_buffer.decode().strip()
                print(f"[+] Received command: {cmd_buffer}")
                
                if cmd_buffer:
                    res = run_command(cmd_buffer)
                    print(f"[+] Command output: {res}")
                    client_socket.send(res.encode())
                else:
                    client_socket.send((Style.BRIGHT + Fore.RED + "No command received\r\n" + Fore.RESET).encode())

            except Exception as e:
                print(f"[+] Exception in command handling: {e}")
                client_socket.close()
                break


def server_loop():
    global target

    if not len(target):
        target = "0.0.0.0"

    server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server.bind((target, port))
    server.listen(5)
    print(f"[+] Listening on {target}:{port}")

    while True:
        client_socket, addr = server.accept()
        print(f"[+] Accepted connection from {addr}")
        client_thread = threading.Thread(target=client_handler, args=(client_socket,))
        client_thread.start()

def run_command(command):
    command = command.rstrip()

    try:
        output = subprocess.check_output(command, stderr=subprocess.STDOUT, shell=True)
    except subprocess.CalledProcessError as e:
        output = f"Failed to execute command: {e}\r\n"

    return output.decode()

def main():
    global listen
    global port
    global execute
    global target
    global upload_destination

    if not len(sys.argv[1:]):
        usage()

    try:
        opts, args = getopt.getopt(sys.argv[1:], "hle:t:p:cu", ["help", "listen", "execute=", "target=", "port=", "command", "upload="])
    
    except getopt.GetoptError as err:
        print(Style.BRIGHT + Fore.RED + str(err) + Fore.RESET)
        usage()

    for o, a in opts:
        if o in ("-h", "--help"):
            usage()
        elif o in ("-l", "--listen"):
            listen = True 
        elif o in ("-e", "--execute"):
            execute = a
        elif o in ("-c", "--command"):
            command = True
        elif o in ("-u", "--upload"):
            upload_destination = a
        elif o in ("-t", "--target"):
            target = a
        elif o in ("-p", "--port"):
            port = int(a)
        else:
            assert False, "Invalid Option"
    
    if not listen and len(target) and port > 0:
        buffer = sys.stdin.read()
        client_sender(buffer)

    if listen:
        server_loop()

if __name__ == "__main__":
    main()
