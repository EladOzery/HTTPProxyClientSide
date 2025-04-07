#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/stat.h>


char *extract_hostname(char *address);

int extract_port(char *address);

char *extract_path(char *address);

int is_port_in_address(char *address);

int establish_connection(char *host_name, int port);

void check_http(char *address);

int check_arguments(int argc, char *argv[]);

char *create_request(char *address, char *path);

int check_file_exist(char *path);

void send_request(char *request, int socket_fd);

FILE *create_dir_path(const char *path, char *hostname);

void write_to_file(int socket_fd, char *path, char *hostname);

void open_in_browser(const char *url);

int count_digits(size_t number);

size_t get_file_size_with_digits(FILE *file);

void print_file_content(FILE *file);

FILE *open_file(char *hostname_and_path);

#define MAX_SIZE 4096
#define DEFAULT_PORT 80
#define HEADER_SIZE 37

int main(int argc, char *argv[]) {
    char *URL = argv[1];
    check_http(URL);

    // checking if URL needs to be opened in browser
    int s_flag = check_arguments(argc, argv);

    // extracting hostname, port and path from the URL
    char *hostname = extract_hostname(URL);
    int port = extract_port(URL);
    char *path = extract_path(URL);

    // constructing a string that holds the hostname and path
    size_t hostname_and_path_len = strlen(hostname) + strlen(path);
    char *hostname_and_path = (char *) malloc(hostname_and_path_len + 1);

    if (hostname_and_path == NULL) {
        printf("malloc\n");
        free(path), free(hostname);
        exit(EXIT_FAILURE);
    }

    strcpy(hostname_and_path, hostname);
    strcat(hostname_and_path, path);
    hostname_and_path[hostname_and_path_len] = '\0';

    // checking for file existence
    if (check_file_exist(hostname_and_path)) { // exist
        // open the file with existing file path
        FILE *existing_file = open_file(hostname_and_path);

        // count how many chars in the header + body
        size_t file_size = get_file_size_with_digits(existing_file);
        size_t file_size_with_header = HEADER_SIZE + file_size;

        // print header
        printf("File is given from local filesystem\n");
        printf("HTTP/1.0 200 OK\r\n");
        printf("Content-Length: %ld\r\n\r\n", file_size);

        print_file_content(existing_file);

        printf("\n   Total response bytes: %lu\n", file_size_with_header);

        fclose(existing_file);
    }

    else { // not exist
        int socket_fd = establish_connection(hostname, port); // creating connection
        char *request = create_request(hostname, path); // formatting socket request
        send_request(request, socket_fd); // sending request to the socket
        free(request);
        write_to_file(socket_fd, path, hostname); // creating path files and writing to the file

        close(socket_fd);
    }

    if (s_flag == 1) // if '-s' flag = open file in browser
        open_in_browser(URL);

    free(hostname_and_path), free(hostname), free(path);
}

int is_port_in_address(char *address) {
    int count = 0;
    char colon = ':';

    // Iterate through the string
    while (*address) {
        // Use strchr to find the next occurrence of the target character
        if (*address == colon)
            count++;

        address++; // Move to the next character in the string
    }

    if (count == 2) {
        return 1; // port in address
    }
    else {
        return 0; // port isn't in address
    }
}

char *extract_hostname(char *address) {
    size_t hostname_len = 0;

    char hostname_end_char;

    if (is_port_in_address(address))
        hostname_end_char = ':';
    else
        hostname_end_char = '/';

    // Determine the length of the hostname
    for (int i = 7; address[i] != '\0' && address[i] != hostname_end_char; ++i)
        hostname_len++;

    // Allocate memory for the result, including space for the null terminator
    char *res = (char *) malloc(hostname_len + 1);

    // Check if memory allocation is successful
    if (res == NULL) {
        fprintf(stderr, "malloc\n");
        exit(1);  // or handle the error in your own way
    }

    // Copy the substring into the allocated memory
    strncpy(res, address + 7, hostname_len);

    // Null-terminate the result string
    res[hostname_len] = '\0';

    return res;
}


int extract_port(char *address) {
    if (!is_port_in_address(address)) // default port is 80
        return DEFAULT_PORT;

    char *colon_ptr = strchr(address + 7, ':');
    colon_ptr += 1; // removes colon

    int port_len = 0;
    for (; port_len < strlen(colon_ptr); ++port_len)
        if (colon_ptr[port_len] == '\0' || colon_ptr[port_len] == '/')
            break;
    char *port = (char *) malloc(port_len + 1);
    strncpy(port, colon_ptr, port_len);
    int port_num = atoi(port);
    free(port);
    return port_num;
}

char *extract_path(char *address) {
    char *slash_ptr = strchr(address + 7, '/');

    if (slash_ptr == NULL || slash_ptr[1] == '\0') {
        // If there is no path or only a '/', assign a single '/'
        char *path_str = (char *) malloc(2);
        if (path_str == NULL) {
            perror("malloc\n");
            exit(EXIT_FAILURE);
        }
        strcpy(path_str, "/");
        return path_str;
    }

    // Calculate the path length
    int path_len = (int) strlen(slash_ptr);

    // Allocate memory for path_str with enough space for the path and a null terminator
    char *path_str = (char *) malloc(path_len + 1);

    if (path_str == NULL) {
        perror("malloc\n");
        exit(EXIT_FAILURE);
    }

    // Copy the path to path_str
    strcpy(path_str, slash_ptr);

    // Null-terminate the string
    path_str[path_len] = '\0';

    return path_str;
}


int establish_connection(char *host_name, int port1) {
    in_port_t port = port1;
    int s_fd;
    struct hostent *server_info = NULL;

    // Create a socket with the address format of IPV4 over TCP
    if ((s_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) {
        perror("socket\n");
        exit(EXIT_FAILURE);
    }


// Use gethostbyname to translate host name to network byte order IP address
    if (strstr(host_name, "www."))
        server_info = gethostbyname(host_name + 4);
    else
        server_info = gethostbyname(host_name);

    if (!server_info) {
        herror("gethostbyname\n");
        close(s_fd);
        exit(EXIT_FAILURE);
    }

    // Initialize sockaddr_in structure
    struct sockaddr_in sock_info;

    // Set its attributes to 0 to avoid undefined behavior
    memset(&sock_info, 0, sizeof(struct sockaddr_in));

    // Set the type of the address to be IPV4
    sock_info.sin_family = AF_INET;

    // Set the socket's port
    sock_info.sin_port = htons(port);

    // Set the socket's IP
    sock_info.sin_addr.s_addr = ((struct in_addr *) server_info->h_addr)->s_addr;

    // Connect to the server
    if (connect(s_fd, (struct sockaddr *) &sock_info, sizeof(struct sockaddr_in)) == -1) {
        perror("connect\n");
        close(s_fd);
        exit(EXIT_FAILURE);
    }

    // Successfully connected
    return s_fd;
}

void check_http(char *address) {
    if (strstr(address, "http://") == NULL) {
        printf("Usage: cproxy %s [-s]\n", address);
        exit(EXIT_FAILURE);
    }
}

/* checks for number of arguments entered to the program. Also checks if the second argument is the "-s" flag.
 * return 1 if "-s" exist, 0 if not */
int check_arguments(int argc, char *argv[]) {
    // More arguments than expected
    if (argc > 3) {
        exit(EXIT_FAILURE);
    }

        // Second argument is "-s"
    else if (argc == 3 && strcmp(argv[2], "-s") == 0) {
        return 1;
    }

    else if (argc == 2) {
        return 0;
    }

    exit(EXIT_FAILURE);
}

int check_file_exist(char *path) {
    size_t path_len = strlen(path);

    // Check if the path ends with a '/'
    if (path[path_len - 1] == '/') {
        // Allocate memory for "index.html" and the null terminator
        char *new_path = (char *) malloc(path_len + strlen("index.html") + 1);

        // Check for allocation failure
        if (new_path == NULL) {
            perror("malloc\n");
            exit(EXIT_FAILURE);
        }

        // Copy the original path to the new buffer
        strcpy(new_path, path);

        // Concatenate "index.html" to the new path
        strcat(new_path, "index.html");

        // null terminating the new path
        new_path[path_len + strlen("index.html")] = '\0';

        // Check file existence using the new path
        int result = access(new_path, F_OK);
        // Free the dynamically allocated memory
        free(new_path);

        // Return 1 if the file exists, 0 otherwise
        return (result == 0) ? 1 : 0;
    }
    else {
        // No need to modify the path, check file existence directly
        return (access(path, F_OK) == 0) ? 1 : 0;
    }
}

char *create_request(char *address, char *path) {
    // Allocate memory for the request
    char *request = (char *) malloc(MAX_SIZE);
    if (request == NULL) {
        perror("malloc\n");
        exit(EXIT_FAILURE);
    }

    // Construct the HTTP request
    int chars_written = sprintf(request, "GET %s HTTP/1.0\r\nHost: %s\r\n\r\n", path, address);

    if (chars_written < 0) {
        printf("sprintf\n");
        exit(EXIT_FAILURE);
    }

    // Make sure to null-terminate the string
    request[chars_written] = '\0';

    // Print the request
    printf("HTTP request =\n%s\nLEN = %lu\n", request, strlen(request));

    return request;
}


void send_request(char *request, int socket_fd) {
    // Use the write function to send the request over the socket
    size_t request_len = strlen(request);
    ssize_t bytes_written = write(socket_fd, request, request_len);

    // Check for errors during the write operation
    if (bytes_written == -1) {
        perror("write\n");
        close(socket_fd);
        exit(EXIT_FAILURE);
    }
}

FILE *create_dir_path(const char *path, char *hostname) {
    char new_path[MAX_SIZE];
    char file_name[MAX_SIZE];

    memset(new_path, '\0', 4096);
    memset(file_name, '\0', 4096);

    strcpy(new_path, path); // making a copy of path

    char *slash_ptr = strrchr(new_path, '/'); // getting a pointer to last slash

    // if '/' is last char in the path
    if (strcmp(slash_ptr, "/") == 0) {
        strcpy(file_name, "index.html");
    }

        // if not - extract file name from path
    else {
        strcpy(file_name, slash_ptr + 1); // copying file name to its own variable
        size_t file_name_len = strlen(file_name);
        size_t new_path_len = strlen(new_path);

        for (size_t i = new_path_len - file_name_len; i < new_path_len; ++i)
            new_path[i] = '\0';
    }

    // creating hostname dir
    if (mkdir(hostname, 0755) == -1) {
        if (errno != EEXIST) {
            perror("mkdir\n");
            exit(EXIT_FAILURE);
        }
    }
    if (chdir(hostname) == -1) {
        perror("chdir\n");
        exit(EXIT_FAILURE);
    }

    char *token = strtok(new_path, "/");

    // Iterate through the path components
    while (token != NULL) {
        // Create the directory with read, write, and execute permissions
        if (mkdir(token, 0755) == -1) {
            if (errno != EEXIST) {
                perror("mkdir\n");
                exit(EXIT_FAILURE);
            }
        }
        // Change the current working directory to the created directory
        if (chdir(token) == -1) {
            perror("chdir\n");
            exit(EXIT_FAILURE);
        }

        token = strtok(NULL, "/");
    }

    // create the file in the current directory
    FILE *file = fopen(file_name, "w");
    if (file == NULL) {
        perror("fopen\n");
        exit(EXIT_FAILURE);
    }

    // Restore the original working directory
    if (chdir("/") == -1) {
        perror("chdir\n");
        exit(EXIT_FAILURE);
    }

    return file;
}

/* The function reads data from the socket using the read function
 * and writes it to the file using the fwrite function. */
void write_to_file(int socket_fd, char *path, char *hostname) {
    // adding 1 to buffer size in order to read MAX_SIZE to buffer when last char will be the null terminator
    size_t buffer_size = MAX_SIZE + 1;
    size_t size_to_print = 0;
    ssize_t bytes_read = 0;

    int write_flag = 0;
    int header_end = 0;

    unsigned char *header_ptr;
    unsigned char buffer[buffer_size];

    FILE *file = NULL;

    // Read from the socket and write to the file until no more data is available
    while (1) {
        // Clear the buffer before reading data
        memset(buffer, '\0', buffer_size);

        // Read data from the socket
        bytes_read = read(socket_fd, buffer, buffer_size);

        // If no more data is available, exit the loop
        if (bytes_read == 0) break;

        // Handle errors while reading from the socket
        if (bytes_read < 0) {
            perror("read\n");
            fclose(file);
            exit(EXIT_FAILURE);
        }

        // Increment the total size of the response
        size_to_print += bytes_read;

        // Print the received data to the console
//        printf("%s", buffer);
        printf("%.*s", (int) bytes_read, buffer);

        // Check for "200 OK" to start writing to the file
        if (write_flag == 0 && strstr(buffer, "200 OK") != NULL) {
            file = create_dir_path(path, hostname);
            write_flag = 1;
        }

        // Handle HTTP headers by skipping them in the response
        if (header_end == 0 && (header_ptr = strstr(buffer, "\r\n\r\n")) != NULL) {
            size_t header_size = header_ptr - buffer + 4;
            bytes_read -= (int) header_size;

            // Move the buffer pointer to the beginning of the actual content
            memmove(buffer, header_ptr + 4, bytes_read);

            header_end = 1;
        }

        // Write the read data to the file
        if (write_flag == 1 && header_end == 1) {
            if (fwrite(buffer, sizeof(unsigned char), bytes_read, file) != bytes_read) {
                perror("fwrite\n");
                fclose(file);
                exit(EXIT_FAILURE);
            }
        }
    }

    // Print the total size of the response
    printf("\n   Total response bytes: %lu\n", size_to_print);

    // Close the file
    if (file != NULL)
        fclose(file);
}

void open_in_browser(const char *url) {
    // Construct the command to open the URL in the default web browser
    char command[256];
    snprintf(command, sizeof(command), "xdg-open %s", url);

    // Execute the command
    int status = system(command);

    // Check for errors in command execution
    if (status == -1) {
        perror("system\n");
        exit(EXIT_FAILURE);
    }
}

int count_digits(size_t number) {
    int count = 0;

    if (number == 0) {
        return 1;  // Special case for 0
    }

    while (number != 0) {
        number /= 10;
        count++;
    }

    return count;
}

size_t get_file_size_with_digits(FILE *file) {
    fseek(file, 0, SEEK_END);
    size_t size = ftell(file);
    int num_digits = count_digits(size);
    fseek(file, 0, SEEK_SET);  // Reset file pointer to the beginning

    return size + num_digits;
}

void print_file_content(FILE *file) {
    if (file == NULL) {
        printf("file\n");
        exit(EXIT_FAILURE);
    }

    int ch;
    while ((ch = fgetc(file)) != EOF) {
        putchar(ch);  // Print each character to the console
    }
}

FILE *open_file(char *hostname_and_path) {
    size_t len = strlen(hostname_and_path);
    char path[MAX_SIZE];
    memset(path, '\0', MAX_SIZE);
    strcpy(path, hostname_and_path);

    if (path[len - 1] == '/')
        strcat(path, "index.html");

    FILE *file = fopen(path, "r");
    if (file == NULL) {
        perror("fopen\n");
        exit(EXIT_FAILURE);
    }

    return file;
}