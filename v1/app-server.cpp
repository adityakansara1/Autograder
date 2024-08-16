#include <iostream>

#include <sys/socket.h> // for socket(), connect(), send(), recv()

#include <string.h> // strtok(), strchr()

#include <unistd.h> // for close()

#include <netdb.h> // for gethostbyname()

#include <fcntl.h>

#include <cerrno>  // for errno
#include <cstring> // for strerror

#include <fstream>
#include <iostream>
#include <string>

using namespace std;

char *read_from_file(const char *);
int write_to_file(const char *, char *);
char *read_from_client(int);
int write_to_client(int, char *);
int is_empty_(const char *);
char *merge(const char *, const char *);

int main(int argc, char *argv[])
{
    // Check for correct number of arguments
    if (argc != 2)
    {
        cerr << "Usage: " << argv[0] << " port" << endl;
        return 1;
    }

    int socket_listen_fd = 1, socket_fd;
    struct sockaddr_in server_address, client_address;
    socklen_t client_addr_length = sizeof(client_address);

    // Set server address
    bzero((char *)&server_address, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(atoi(argv[1]));

    // Create socket
    if ((socket_listen_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        cerr << "Socket creation failed: " << strerror(errno) << endl;
        return 1;
    }

    // Connect socket
    if (bind(socket_listen_fd, (struct sockaddr *)&server_address, sizeof(server_address)) < 0)
        cerr << "Error on binding: " << strerror(errno) << endl;

    // Listening
    listen(socket_listen_fd, 500);

    while (1)
    {
        if ((socket_fd = accept(socket_listen_fd, (struct sockaddr *)&client_address, &client_addr_length)) < 0)
            cerr << "Error on accepting: " << strerror(errno) << endl;

        // CONNECTION ESTABLISHED
        while (1)
        {
            char *c_file_data = read_from_client(socket_fd);
            if (c_file_data == NULL)
                break;
            write_to_file("test1.c", c_file_data);

            char *reply = new char[1024];
            system("gcc test1.c -o test1 2> compiletime_error.txt");

            if (is_empty_("compiletime_error.txt"))
            {
                system("sh -c './test1' > output.txt 2> runtime_error.txt");
                if (!is_empty_("runtime_error.txt"))
                {
                    reply = merge("RUNTIME ERROR", "runtime_error.txt");
                }
                else
                {
                    system("diff output.txt expected_output > diff.txt");
                    if (is_empty_("diff.txt"))
                    {
                        reply = merge("PASS", "output.txt");
                    }
                    else
                    {
                        system("echo '\n' >> output.txt");
                        system("cat diff.txt >> output.txt");
                        reply = merge("OUTPUT ERROR", "output.txt");
                    }
                }
            }
            else
            {
                reply = merge("COMPILER ERROR", "compiletime_error.txt");
            }

            if (write_to_client(socket_fd, reply) < 0)
                cerr << "Error in writing data to client: " << strerror(errno) << endl;
        }
        close(socket_fd);
    }
    close(socket_listen_fd);

    return 0;
}

char *read_from_file(const char *filename)
{
    ifstream file(filename);
    if (!file.is_open())
    {
        cerr << "Error opening file: " << filename << endl;
        return NULL;
    }

    file.seekg(0, ios::end);
    int file_size = file.tellg();
    file.seekg(0, ios::beg);

    char *file_data = new char[file_size + 1];

    file.read(file_data, file_size);
    file_data[file_size] = '\0';

    file.close();

    return file_data;
}

int write_to_file(const char *filename, char *data)
{
    int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    int bytes_written = write(fd, data, strlen(data));
    return bytes_written;
}

char *read_from_client(int socket_fd)
{
    int len;
    int bytes_read = read(socket_fd, &len, sizeof(int));
    if (bytes_read == 0)
        return NULL;
    char *c_file_data = new char[len + 1];
    bzero(c_file_data, len + 1);
    bytes_read = read(socket_fd, c_file_data, len + 1);
    return c_file_data;
}

int write_to_client(int socket_fd, char *msg)
{
    int len = strlen(msg);
    int bytes_written = write(socket_fd, &len, sizeof(int));
    bytes_written = write(socket_fd, msg, strlen(msg) + 1);
    return bytes_written;
}

int is_empty_(const char *filename)
{
    ifstream file(filename);
    if (!file.is_open())
    {
        cerr << "Error opening file: " << filename << endl;
        return -1;
    }

    file.seekg(0, ios::end);
    int file_size = file.tellg();
    file.close();

    if (file_size == 0)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

char *merge(const char *msg, const char *filename)
{
    char *data = read_from_file(filename);
    char *buffer = new char[strlen(data) + strlen(msg) + 2];
    if (msg == "PASS")
    {
        strcpy(buffer, msg);
        return buffer;
    }
    strcpy(buffer, msg);
    strcat(buffer, "\n");
    strcat(buffer, data);

    return buffer;
}
