#include <iostream>
#include <sys/socket.h> // for socket(), connect(), send(), recv()
#include <string.h>     // strtok(), strchr()
#include <unistd.h>     // for close()
#include <netdb.h>      // for gethostbyname()
#include <fcntl.h>      // for O_RDONLY, etc.
#include <cerrno>       // for errno
#include <cstring>      // for strerror
#include <sys/time.h>   // for gettimeofday()
#include <fstream>      // for ifstream
#include <string>       // for string
#include <thread>       // for thread

using namespace std;

char *read_from_file(const char *);
int write_to_file(const char *, char *);
char *read_from_client(int);
int write_to_client(int, char *, fd_set *, struct timeval *);
char *merge(const char *, const char *);

void *worker(void *fd)
{
    int socket_fd = *(int *)fd;
    free(fd);
    fd = nullptr;
    cout << "Thread: " << socket_fd << endl;
    struct timeval t, timeout;
    timeout.tv_sec = 10;
    timeout.tv_usec = 0;

    fd_set writefds;              // File descriptor set for reading
    FD_ZERO(&writefds);           // Initialize the set
    FD_SET(socket_fd, &writefds); // Add your socket to the set

    gettimeofday(&t, NULL);
    string time = to_string(t.tv_sec * 1000000 + t.tv_usec);

    string create_dir = "mkdir " + time;
    string copy_expected_output = "cp expected_output ./" + time;
    string remove_dir = "rm -r " + time;

    system(create_dir.c_str());
    system(copy_expected_output.c_str());

    string source_file = "./" + time + "/test.c";
    string compilation_error_file = "./" + time + "/compilation_error.txt";
    string runtime_error_file = "./" + time + "/runtime_error.txt";
    string executable_file = "./" + time + "/test" + time;
    string output_file = "./" + time + "/output.txt";
    string expected_output_file = "./" + time + "/expected_output";
    string diff_output_file = "./" + time + "/diff.txt";

    char *c_file_data = read_from_client(socket_fd);
    if (c_file_data == NULL)
    {
        cerr << "Error in reading data from client: " << strerror(errno) << endl;
        exit(1);
    }

    if (write_to_file(source_file.c_str(), c_file_data) < 0)
    {
        cerr << "Error in writing data to file: " << strerror(errno) << endl;
        exit(1);
    }

    char *reply = new char[1024];

    string compilation_command = "gcc -o " + executable_file + " " + source_file + " 2> " + compilation_error_file;
    string run_command = "sh -c ./" + executable_file + " > " + output_file + " 2> " + runtime_error_file;
    string diff_command = "diff " + output_file + " " + expected_output_file + " > " + diff_output_file;

    if (system(compilation_command.c_str()) == 0)
    {
        if (system(run_command.c_str()) != 0)
        {
            reply = merge("RUNTIME ERROR", runtime_error_file.c_str());
        }
        else
        {
            if (system(diff_command.c_str()) == 0)
            {
                reply = merge("PASS", output_file.c_str());
            }
            else
            {
                string merge_diff_output_command1 = "echo '\n' >> " + output_file;
                string merge_diff_output_command2 = "cat " + diff_output_file + " >> " + output_file;
                system(merge_diff_output_command1.c_str());
                system(merge_diff_output_command2.c_str());
                reply = merge("OUTPUT ERROR", output_file.c_str());
            }
        }
    }
    else
    {
        reply = merge("COMPILiER ERROR", compilation_error_file.c_str());
    }

    cout << reply << endl;
    if (write_to_client(socket_fd, reply, &writefds, &timeout) == -2) // Timeout
        ;

    delete[] c_file_data;
    delete[] reply;
    system(remove_dir.c_str());
    close(socket_fd);
    pthread_exit(NULL);
    return NULL;
}

int main(int argc, char *argv[])
{
    // Check for correct number of arguments
    if (argc != 2)
    {
        cerr << "Usage: " << argv[0] << " port" << endl;
        return 1;
    }

    int socket_listen_fd;
    struct sockaddr_in server_address, client_address;
    socklen_t client_addr_length = sizeof(client_address);

    // Set server address
    memset(&server_address, 0, sizeof(server_address));
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
        int *socket_fd = (int *)malloc(sizeof(int));
        if ((*socket_fd = accept(socket_listen_fd, (struct sockaddr *)&client_address, &client_addr_length)) < 0)
            cerr << "Error on accepting: " << strerror(errno) << endl;

        cout << "Main: " << *socket_fd << endl;
        // CONNECTION ESTABLISHED
        pthread_t t;
        if (pthread_create(&t, NULL, &worker, socket_fd) < 0)
        {
            cerr << "Error in creating thread: " << strerror(errno) << endl;
        }
        // pthread_detach(t);

        // std::thread t1(worker, (void *)socket_fd);
        // t1.detach();
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
    if (fd < 0)
    {
        cerr << "Error opening file: " << strerror(errno) << endl;
        return -1;
    }

    int bytes_written = write(fd, data, strlen(data));
    if (bytes_written < 0)
    {
        cerr << "Error writing to file: " << strerror(errno) << endl;
        close(fd);
        return -1;
    }
    close(fd);
    return bytes_written;
}

char *read_from_client(int socket_fd)
{
    int len;
    int bytes_read = recv(socket_fd, &len, sizeof(int), 0);
    if (bytes_read == 0)
        return NULL;
    char *c_file_data = new char[len + 1];

    memset(c_file_data, 0, len + 1);
    bytes_read = recv(socket_fd, c_file_data, len + 1, 0);
    if (bytes_read == 0)
        return NULL;
    return c_file_data;
}

int write_to_client(int socket_fd, char *msg, fd_set *writefds, struct timeval *timeout)
{
    int len = strlen(msg);

    int ready = select(socket_fd + 1, NULL, writefds, NULL, timeout);
    if (ready == 0)
        return -2;
    int bytes_written = send(socket_fd, &len, sizeof(int), 0);
    if (bytes_written < 0)
        cerr << "Error in writing data to client: " << strerror(errno) << endl;

    ready = select(socket_fd + 1, NULL, writefds, NULL, timeout);
    if (ready == 0)
        return -2;
    bytes_written = send(socket_fd, msg, strlen(msg) + 1, 0);
    if (bytes_written < 0)
        cerr << "Error in writing data to client: " << strerror(errno) << endl;
    return bytes_written;
}

char *merge(const char *msg, const char *filename)
{
    char *data = read_from_file(filename);
    if (strcmp(msg, "PASS") == 0)
    {
        char *buffer = new char[strlen(msg) + 2];
        strcpy(buffer, msg);
        return buffer;
    }
    char *buffer = new char[strlen(data) + strlen(msg) + 2];
    strcpy(buffer, msg);
    strcat(buffer, "\n");
    strcat(buffer, data);

    delete[] data;
    delete[] msg;
    delete[] filename;
    return buffer;
}
