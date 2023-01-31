#include <iostream>
#include <string>
#include <queue>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <pthread.h>
#include <unistd.h>
using namespace std;

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

struct arguments
{
    Node *top;
    string line;
    string *arr;
};

void dfs(Node *parent, string dir)
{
    if (parent == nullptr)
        return;
    if (parent->letter.length() == 1)
    {
        cout << "Symbol: " << parent->letter << ", Frequency: " << parent->value << ", Code: " << dir;
        cout << endl;
        return;
    }
    dfs(parent->left, dir + "0");
    dfs(parent->right, dir + "1");
}

// Decode function
void *decode(void *arg)
{
    arguments *argu = (arguments *)arg;
    string line = argu->line;
    Node *node = argu->top;
    string dir;

    istringstream ss(line);
    ss >> dir;

    for (size_t i = 0; i < dir.length(); i++)
    {
        if (dir[i] == '1')
            node = node->right;
        else
            node = node->left;
    }

    string letter = node->letter;
    size_t pos = 0;
    // cout << "Start" << endl;
    while (ss >> pos)
        argu->arr[pos] = letter;
    // cout << "End" << endl;
    // string dir = line.substr(0, line.find(" "));
    // line = line.substr(line.find(" ") + 1);

    // while (line.find(" ") != string::npos)
    // {
    //     argu->arr[stoi(line.substr(0, line.find(" ")))] = letter;
    //     line = line.substr(line.find(" ") + 1);
    // }
    // argu->arr[stoi(line)] = letter;

    return NULL;
}

// Main
int main()
{
    // Initialize pq
    heapq<Node *, vector<Node *>, compareNode> pq;

    string input;
    string compressed;
    cin >> input;
    cin >> compressed;

    int string_len = 0, numLine = 0;
    vector<Node *> vec;
    string line;
    ifstream input_file(input);

    // Read input and add to the vector vec
    if (input_file.is_open())
    {
        while (getline(input_file, line))
        {
            int freq = stoi(line.substr(2));
            string_len += freq;
            // cout << line.substr(0, 1) << " " << freq << endl;
            vec.push_back(new Node(line.substr(0, 1), freq, line.substr(0, 1), 0));
            numLine++;
        }
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

    // Proccess
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

    dfs(pq.top(), "");

    string arr[string_len];
    ifstream com_file(compressed);

    static arguments *args = new arguments[numLine];
    pthread_t tid[numLine];

    for (int i = 0; i < numLine; i++)
    {
        getline(com_file, line);

        args[i].top = pq.top();
        args[i].line = line;
        args[i].arr = arr;

        if (pthread_create(&tid[i], NULL, decode, &args[i]))
        {
            fprintf(stderr, "Error creating thread\n");
            return 1;
        }
    }

    for (int i = 0; i < numLine; i++)
        pthread_join(tid[i], NULL);

    string decoded = "";

    for (int i = 0; i < string_len; i++)
        decoded = decoded + arr[i];

    cout << "Original message: " << decoded << endl;

    return 0;
}

// Print out the priority queue
void printout(heapq<Node *, vector<Node *>, compareNode> &pq)
{
    for (auto temp : pq)
        cout << temp->str << " " << temp->value << endl;
}