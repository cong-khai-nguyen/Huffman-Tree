// Please note this is a C++ program
// It compiles without warnings with g++
#include <unistd.h>
#include <iostream>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <pthread.h>
#include <unistd.h>
using namespace std;

struct arguments
{
    string line;
    string *arr;
    int portno;
    string hostName;
    pthread_mutex_t *sem;
};

void *connect(void *arg)
{
    arguments *argu = (arguments *)arg;
    int sockfd, portno, n;

    struct sockaddr_in serv_addr;
    struct hostent *server;

    portno = argu->portno;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        std::cerr << "ERROR opening socket";
    }
    pthread_mutex_lock(argu->sem);
    server = gethostbyname(argu->hostName.c_str());
    if (server == NULL)
    {
        std::cerr << "ERROR, no such host\n";
        exit(0);
    }

    bzero((char *)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr,
          (char *)&serv_addr.sin_addr.s_addr,
          server->h_length);
    serv_addr.sin_port = htons(portno);
    pthread_mutex_unlock(argu->sem);
    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        std::cerr << "ERROR connecting";
        exit(1);
    }
    string line = argu->line, binaryCode = "";
    istringstream ss(line);

    ss >> binaryCode;
    // Convert binary string to char
    n = write(sockfd, binaryCode.data(), binaryCode.size());
    if (n < 0)
    {
        std::cerr << "ERROR writing to socket client side";
        exit(1);
    }

    char letter = '\0';
    n = read(sockfd, &letter, sizeof(char));
    if (n < 0)
    {
        std::cerr << "ERROR reading from socket client side";
        exit(1);
    }

    size_t pos;
    while (ss >> pos)
        (*argu->arr)[pos] = letter;

    close(sockfd);
    return NULL;
}

int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        std::cerr << "usage " << argv[0] << "hostname port\n";
        exit(0);
    }

    //////////////////////////////////////////////////////////////////////////// Multithreading Begins Here //////////////////////////////////////////////

    // Read in the file from input redirection
    vector<string> list;
    int wordLen;
    string line = "";
    while (getline(cin, line))
    {
        list.push_back(line);
        istringstream ss(line); // string stream

        while (ss >> line)
            wordLen++;
    }

    const int THREAD_SIZE = list.size();
    pthread_t tid[THREAD_SIZE];
    static arguments *args = new arguments[THREAD_SIZE];
    string arr = string(wordLen, '\0');
    pthread_mutex_t sem;
    pthread_mutex_init(&sem, nullptr);

    for (int i = 0; i < THREAD_SIZE; i++)
    {
        args[i].line = list[i];
        args[i].arr = &arr;
        args[i].portno = stoi(argv[2]);
        args[i].hostName = argv[1];
        args[i].sem = &sem;

        if (pthread_create(&tid[i], NULL, connect, &args[i]))
        {
            fprintf(stderr, "Error creating thread\n");
            return 1;
        }
    }

    for (int i = 0; i < THREAD_SIZE; i++)
        pthread_join(tid[i], NULL);

    pthread_mutex_destroy(&sem);
    string decoded = "";

    for (int i = 0; i < wordLen; i++)
        decoded = decoded + arr[i];

    cout << "Original message: " << decoded << endl;
    return 0;
}
