#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>

#define BUFFER_SIZE 4096

// ANSI escape codes for colors
#define RESET "\033[0m"
#define BRIGHT "\033[1m"
#define CYAN "\033[36m"
#define GREEN "\033[32m"
#define RED "\033[31m"
#define LIGHTCYAN "\033[96m"

// Function prototypes
void print_usage();
void execute_command(int client_socket);
void upload_file(int client_socket, const char *upload_destination);
void run_file(int client_socket, const char *file_to_run);
void server_loop(int port, const char *upload_destination, const char *file_to_run, int command_mode);

// Global variables for server options
int command_mode = 0;
char *upload_destination = NULL;
char *file_to_run = NULL;
char *target = NULL;

void print_usage() {
    printf(BRIGHT CYAN "\n"
           " ________      ________    ___  ___     \n"
           "|\\   __  \\    |\\   __  \\  |\\  \\|\\  \\    \n"
           "\\ \\  \\|\\  \\   \\ \\  \\|\\  \\ \\ \\  \\_\\  \\   \n"
           " \\ \\  \\\\\\  \\   \\ \\   __  \\ \\ \\   __  \\   Net Tool\n"
           "  \\ \\  \\\\\\  \\   \\ \\  \\ \\  \\ \\ \\  \\ \\  \\ \n"
           "   \\ \\_______\\   \\ \\__\\ \\__\\ \\ \\__\\ \\__\\ \n"
           "    \\|_______|    \\|__|\\|__|  \\|__|\\|__| \n"
           "                                \n"
           "                Recreated By Byteninja9\n"
           RESET);

    printf("Usage: net_tool -p port [options]\n");
    printf("-p --port                           - port to listen on\n");
    printf("-e --execute=file_to_run             - execute the given file upon receiving connection\n");
    printf("-c --command                        - initialize a command shell\n");
    printf("-u --upload=destination             - upon receiving connection upload a file and write to [destination]\n");
    printf("-t --target=host                    - target host\n");
    printf("-h --help                           - show this help message\n");
    printf("\n");
    printf(BRIGHT GREEN "Examples:\n");
    printf("net_tool -p 5555 -c\n");
    printf("net_tool -p 5555 -c -u=/path/to/upload/file\n");
    printf("net_tool -p 5555 -e=\"/path/to/script.sh\"\n");
    printf("net_tool -p 5555 -t 192.168.0.1\n");
    printf(RESET);
    exit(0);
}

void execute_command(int client_socket) {
    char buffer[BUFFER_SIZE];
    int n;

    while (1) {
        // Send the command prompt
        const char *prompt = BRIGHT LIGHTCYAN "<BHP:#> " RESET;
        send(client_socket, prompt, strlen(prompt), 0);

        // Read the command from the client
        memset(buffer, 0, BUFFER_SIZE);
        n = recv(client_socket, buffer, BUFFER_SIZE - 1, 0);
        if (n <= 0) {
            perror("recv");
            break;
        }

        // Remove newline character at the end
        buffer[strcspn(buffer, "\n")] = 0;

        printf(BRIGHT CYAN "[+] Received command: %s" RESET "\n", buffer);

        // Execute the command
        FILE *fp;
        char output[BUFFER_SIZE];
        memset(output, 0, BUFFER_SIZE);

        fp = popen(buffer, "r");
        if (fp == NULL) {
            perror("popen");
            send(client_socket, (RED "Failed to execute command.\n" RESET), 27, 0);
            continue;
        }

        // Send the command output to the client
        while (fgets(output, sizeof(output), fp) != NULL) {
            send(client_socket, output, strlen(output), 0);
        }

        printf(BRIGHT GREEN "[+] Command output sent." RESET "\n");

        pclose(fp);
    }

    // Close the client socket
    close(client_socket);
}

void upload_file(int client_socket, const char *upload_destination) {
    char buffer[BUFFER_SIZE];
    int n;

    FILE *file = fopen(upload_destination, "wb");
    if (file == NULL) {
        perror("fopen");
        send(client_socket, (RED "Failed to open file for writing.\n" RESET), 34, 0);
        return;
    }

    // Receive the file from the client
    while ((n = recv(client_socket, buffer, BUFFER_SIZE, 0)) > 0) {
        fwrite(buffer, 1, n, file);
    }

    if (n < 0) {
        perror("recv");
        send(client_socket, (RED "Failed to receive file.\n" RESET), 25, 0);
    } else {
        printf(BRIGHT GREEN "[+] Successfully saved file to %s\n" RESET, upload_destination);
        send(client_socket, (GREEN "File successfully uploaded.\n" RESET), 26, 0);
    }

    fclose(file);
}

void run_file(int client_socket, const char *file_to_run) {
    char output[BUFFER_SIZE];

    // Execute the file
    FILE *fp = popen(file_to_run, "r");
    if (fp == NULL) {
        perror("popen");
        send(client_socket, (RED "Failed to execute file.\n" RESET), 25, 0);
        return;
    }

    // Send the file output to the client
    while (fgets(output, sizeof(output), fp) != NULL) {
        send(client_socket, output, strlen(output), 0);
    }

    pclose(fp);
}

void server_loop(int port, const char *upload_destination, const char *file_to_run, int command_mode) {
    int server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len;

    // Create a socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // Set up the server address struct
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    // Bind the socket to the specified port
    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("bind");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    if (listen(server_socket, 5) == -1) {
        perror("listen");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    printf(BRIGHT GREEN "[+] Listening on port %d...\n" RESET, port);

    // Main server loop
    while (1) {
        client_len = sizeof(client_addr);
        client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_len);
        if (client_socket == -1) {
            perror("accept");
            close(server_socket);
            exit(EXIT_FAILURE);
        }

        printf(BRIGHT GREEN "[+] Accepted connection from %s:%d\n" RESET,
               inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

        // Handle client request based on mode
        if (upload_destination) {
            upload_file(client_socket, upload_destination);
        } else if (file_to_run) {
            run_file(client_socket, file_to_run);
        } else if (command_mode) {
            execute_command(client_socket);
        }

        // Close the client socket
        close(client_socket);
    }

    // Close the server socket
    close(server_socket);
}

int main(int argc, char *argv[]) {
    int port = -1;
    char *endptr;

    // Default values for options
    upload_destination = NULL;
    file_to_run = NULL;
    command_mode = 0;

    if (argc < 3 || strcmp(argv[1], "-p") != 0) {
        print_usage();
    }

    port = strtol(argv[2], &endptr, 10);
    if (*endptr != '\0' || port <= 0 || port > 65535) {
        fprintf(stderr, RED "Invalid port number: %s\n" RESET, argv[2]);
        exit(EXIT_FAILURE);
    }

    // Parse additional options
    for (int i = 3; i < argc; i++) {
        if (strcmp(argv[i], "-c") == 0) {
            command_mode = 1;
        } else if (strncmp(argv[i], "-u=", 3) == 0) {
            upload_destination = argv[i] + 3;
        } else if (strncmp(argv[i], "-e=", 3) == 0) {
            file_to_run = argv[i] + 3;
        } else if (strncmp(argv[i], "-t=", 3) == 0) {
            target = argv[i] + 3;
        } else {
            print_usage();
        }
    }

    // Validate options
    if (command_mode && (upload_destination || file_to_run)) {
        fprintf(stderr, RED "Command mode (-c) cannot be used with upload or execute options.\n" RESET);
        exit(EXIT_FAILURE);
    }

    if (upload_destination && file_to_run) {
        fprintf(stderr, RED "Cannot use both upload (-u) and execute (-e) options simultaneously.\n" RESET);
        exit(EXIT_FAILURE);
    }

    if (command_mode || upload_destination || file_to_run) {
        server_loop(port, upload_destination, file_to_run, command_mode);
    } else {
        print_usage();
    }

    return 0;
}
