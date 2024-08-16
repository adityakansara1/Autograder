#include <iostream>

#include <sys/socket.h> // for socket(), connect(), send(), recv()

#include <string.h> // strtok(), strchr()

#include <unistd.h> // for close()

#include <netdb.h> // for gethostbyname()

#include <fcntl.h> // for O_RDONLY, etc.

#include <cerrno>  // for errno
#include <cstring> // for strerror

#include <fstream>
#include <iostream>
#include <string>

#include <sys/time.h>

using namespace std;

char *read_from_file(char *);
char *read_from_server(int);
int write_to_server(int, char *);

int main(int argc, char *argv[])
{
    // Check for correct number of arguments
    if (argc != 5)
    {
        cerr << "Usage: " << argv[0] << " <hostname:port> filename loopNum sleepTimeSeconds" << endl;
        return 1;
    }

    int socket_fd, loopNum = atoi(argv[3]), sleepTimeSeconds = atoi(argv[4]);
    struct sockaddr_in server_address;
    struct hostent *server;
    char *hostname = strtok(argv[1], ":");
    int port = atoi(strtok(NULL, ":"));

    // Get host
    if ((server = gethostbyname(hostname)) == NULL)
    {
        cerr << "Host not found" << endl;
        return 1;
    }

    // Set server address
    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    memcpy(&server_address.sin_addr.s_addr, server->h_addr, server->h_length);

    // Set port
    server_address.sin_port = htons(port);

    // Create socket
    if ((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        cerr << "Socket creation failed: " << strerror(errno) << endl;
        return 1;
    }

    // Connect socket
    if (connect(socket_fd, (sockaddr *)&server_address, sizeof(server_address)) < 0)
        cerr << "Socket connection failed: " << strerror(errno) << endl;

    // CONNECTION ESTABLISHED

    for (int i = 0; i < loopNum; i++)
    {
        char *c_file_data = read_from_file(argv[2]);

        if (write_to_server(socket_fd, c_file_data) < 0)
            cerr << "Error in writing data to server: " << strerror(errno) << endl;

        char *response = read_from_server(socket_fd);

        cout << response << endl;
        sleep(sleepTimeSeconds);

        delete[] c_file_data;
        delete[] response;
    }
    close(socket_fd);
    return 0;
}

char *read_from_file(char *filename)
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

char *read_from_server(int socket_fd)
{
    int len;
    int bytes_read = read(socket_fd, &len, sizeof(int));
    char *response = new char[len + 1];
    bytes_read = read(socket_fd, response, len + 1);
    if (bytes_read == 0)
        return NULL;
    return response;
}

int write_to_server(int socket_fd, char *msg)
{
    int len = strlen(msg);
    int bytes_written = write(socket_fd, &len, sizeof(int));
    bytes_written = write(socket_fd, msg, strlen(msg));
    return bytes_written;
}