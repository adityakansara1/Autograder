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

#define MAX_TRIES 5

using namespace std;

char *read_from_file(char *);
char *read_from_server(int, fd_set *, struct timeval *);
int write_to_server(int, char *);

int timeout_count = 0;
int error_count = 0;

int main(int argc, char *argv[])
{
    // Check for correct number of arguments
    if (argc != 6)
    {
        cerr << "Usage: " << argv[0] << " <hostname:port> filename loopNum sleepTimeSeconds timeout" << endl;
        return 1;
    }

    int socket_fd, loopNum = atoi(argv[3]), sleepTimeSeconds = atoi(argv[4]), timeout = atoi(argv[5]);
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

    struct timeval t1, t2, t3, t4, timeout_tv;
    long long int total_time = 0;
    int correct_responses = 0;

    timeout_tv.tv_sec = timeout;
    timeout_tv.tv_usec = 0;
    // setsockopt(socket_fd, SOL_SOCKET, SO_RCVTIMEO, (const char *)&timeout_tv, sizeof(timeout_tv));

    gettimeofday(&t1, NULL);
    for (int i = 0; i < loopNum; i++)
    {
        // Create socket
        if ((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        {
            cerr << "Socket creation failed: " << strerror(errno) << endl;
            return 1;
        }

        fd_set readfds;              // File descriptor set for reading
        FD_ZERO(&readfds);           // Initialize the set
        FD_SET(socket_fd, &readfds); // Add your socket to the set

        if (connect(socket_fd, (struct sockaddr *)&server_address, sizeof(server_address)) < 0)
        {
            cout << -1 << endl;
            exit(1);
        }

        // CONNECTION ESTABLISHED
        char *c_file_data = read_from_file(argv[2]);

        gettimeofday(&t2, NULL);
        if (write_to_server(socket_fd, c_file_data) < 0)
            cerr << "Error in writing data to server: " << strerror(errno) << endl;

        char *response = read_from_server(socket_fd, &readfds, &timeout_tv);
        if (response != NULL)
        {
            correct_responses += 1;
        }
        gettimeofday(&t3, NULL);

        // cout << response << endl;
        sleep(sleepTimeSeconds);

        delete[] c_file_data;
        delete[] response;

        long long int time1 = (t3.tv_sec - t2.tv_sec) * 1000000 + (t3.tv_usec - t2.tv_usec);
        total_time += time1;
        close(socket_fd);
    }
    gettimeofday(&t4, NULL);
    long long int time2 = (t4.tv_sec - t1.tv_sec) * 1000000 + (t2.tv_usec - t1.tv_usec);

    double average_response_time = (double)total_time / (loopNum * 1000000);
    double throughput = (double)correct_responses * 1000000 / total_time;
    double loop_time = (double)time2 / 1000000;
    double timeout_rate = (double)timeout_count * 1000000 / total_time;
    double error_rate = (double)error_count * 1000000 / total_time;

    cout << average_response_time << " " << throughput << " " << loop_time << " " << timeout_rate << " " << error_count << " " << endl;
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

char *read_from_server(int socket_fd, fd_set *readfds, struct timeval *timeout_tv)
{
    int len, bytes_read;

    int ready = select(socket_fd + 1, readfds, NULL, NULL, timeout_tv);
    if (ready == 0)
    {
        timeout_count++;
        return NULL;
    }

    bytes_read = recv(socket_fd, &len, sizeof(int), 0);
    char *response = new char[len + 1];

    ready = select(socket_fd + 1, readfds, NULL, NULL, timeout_tv);
    if (ready == 0)
    {
        timeout_count++;
        return NULL;
    }

    bytes_read = recv(socket_fd, response, len + 1, 0);
    return response;
}

int write_to_server(int socket_fd, char *msg)
{
    int len = strlen(msg), bytes_written;
    bytes_written = send(socket_fd, &len, sizeof(int), 0);
    bytes_written = send(socket_fd, msg, strlen(msg), 0);
    return bytes_written;
}