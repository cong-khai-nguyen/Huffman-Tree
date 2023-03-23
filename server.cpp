#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/wait.h>
#include <string>
#include <queue>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <pthread.h>
using namespace std;

#define BUFFER_SIZE 256

// Template Source for debugging: https://stackoverflow.com/a/61925948/13979218
template <class T, class C = std::vector<T>, class P = std::less<typename C::value_type>>
struct heapq : std::priority_queue<T, C, P>
{
    typename C::iterator begin() { return std::priority_queue<T, C, P>::c.begin(); }
    typename C::iterator end() { return std::priority_queue<T, C, P>::c.end(); }
};

struct Node
{
    string letter;
    string str; // solely for the purpose of debugging
    int value;
    int order;
    Node *left;
    Node *right;
    explicit Node(std::string _letter, int _value, string _str, int _order) : letter(_letter), value(_value), str(_str), order(_order), left(nullptr), right(nullptr) {}
};

struct compareNode
{
    // Sort increasingly based on values, then alphbetically, then decreasingly based on order
    bool operator()(Node *n1, Node *n2)
    {
        if (n1->value == n2->value)
        {
            if (n1->letter.length() > 0 && n2->letter.length() > 0)
                return n1->letter >= n2->letter;
            return n1->order < n2->order;
        }

        return n1->value > n2->value;
    }
};

void dfs(Node *parent, string dir)
{
    if (parent == nullptr)
        return;
    if (parent->letter.length() == 1)
    {
        cout << "Symbol: " << parent->letter << ", Frequency: " << parent->value << ", Code: " << dir << endl;
        return;
    }
    dfs(parent->left, dir + "0");
    dfs(parent->right, dir + "1");
}

// Generate Priority Queue with Huffman Tree Algorithm
int pqGen(heapq<Node *, vector<Node *>, compareNode> &pq)
{
    int string_len = 0, alphaChar = 0;
    vector<Node *> vec;
    string line;

    // Read input and add to the vector vec
    while (getline(cin, line))
    {
        int freq = stoi(line.substr(2));
        string_len += freq;
        // cout << line.substr(0, 1) << " " << freq << endl;
        vec.push_back(new Node(line.substr(0, 1), freq, line.substr(0, 1), 0));
        alphaChar++;
    }

    // Sort the vector based on values then lexicographically
    sort(vec.begin(), vec.end(), [](Node *a, Node *b)
         {
        if(a -> value == b -> value)
            return a -> letter < b -> letter;
        return a -> value < b -> value; });

    // Add sorted inputs into the pq and assign each with an order. The higher the order will be at the top of the pq
    int order = string_len;
    for (auto node : vec)
    {
        node->order = order--;
        pq.push(node);
    }

    order = string_len + 1;

    // Generate Huffman Code
    while (pq.size() > 1)
    {
        // cout << "Round" << endl;
        // printout(pq);
        // cout << endl;
        Node *left = pq.top();
        pq.pop();
        Node *right = pq.top();
        pq.pop();

        Node *parent = new Node("", left->value + right->value, left->str + right->str, order++);
        parent->left = left;
        parent->right = right;

        pq.push(parent);
    }

    // for (auto temp : pq)
    //     cout << temp->str << " " << temp->value << endl;
    return alphaChar;
}

// Decode function
string decode(string binaryCode, Node *node)
{
    for (size_t i = 0; i < binaryCode.length(); i++)
    {
        if (binaryCode[i] == '1')
            node = node->right;
        else
            node = node->left;
    }

    return node->letter;
}

void fireman(int)
{
    while (waitpid(-1, NULL, WNOHANG) > 0)
        ;
}

int main(int argc, char *argv[])
{
    // Initialize pq
    heapq<Node *, vector<Node *>, compareNode> pq;
    int alphaChar = pqGen(pq);

    dfs(pq.top(), "");

    ////////////////////////////////////////////////////////////////////// Creating socket server process begins here ////////////////////////////
    int sockfd, newsockfd, portno, clilen; // Default to 0
    struct sockaddr_in serv_addr, cli_addr;
    int n;

    signal(SIGCHLD, fireman);

    if (argc < 2)
    {
        std::cerr << "ERROR, no port provided\n";
        exit(1);
    }
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        std::cerr << "ERROR opening socket";
        exit(1);
    }
    bzero((char *)&serv_addr, sizeof(serv_addr));

    portno = stoi(argv[1]);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);

    auto option = 1;
    n = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));

    if (bind(sockfd, (struct sockaddr *)&serv_addr,
             sizeof(serv_addr)) < 0)
    {
        std::cerr << "ERROR on binding";
        exit(1);
    }
    listen(sockfd, 5);
    clilen = sizeof(cli_addr);

    for (int i = 0; i < alphaChar; i++)
    {
        newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, (socklen_t *)&clilen);
        if (fork() == 0)
        {
            if (newsockfd < 0)
            {
                std::cerr << "ERROR on accept";
                exit(1);
            }

            // Read in binary code from client
            char buffer[BUFFER_SIZE];
            bzero(buffer, BUFFER_SIZE);

            n = read(newsockfd, buffer, BUFFER_SIZE - 1);
            if (n < 0)
            {
                std::cerr << "ERROR reading from socket server side";
                exit(1);
            }
            // cout << buffer << endl;
            string binaryCode = string(buffer);
            // cout << binaryCode << endl;

            // Decode the binary code
            char letter = decode(binaryCode, pq.top())[0];

            // cout << letter << endl;
            // Write back to the client
            n = write(newsockfd, &letter, sizeof(char));
            if (n < 0)
            {
                std::cerr << "ERROR writing to socket server side";
                exit(1);
            }

            close(newsockfd);
            _exit(0);
        }
    }
    close(sockfd);
    return 0;
}