#include <iostream>
#include <stdexcept>
#include <pthread.h>
#include <queue>
#include "park.h"

#ifndef QUEUE_H
#define QUEUE_H

// Node structure used in the queue
template <typename T>
struct Node {
    T value;          // The stored value
    Node* next;       // Pointer to the next node

    // Default constructor: initializes next pointer to nullptr.
    Node() : next(nullptr) {}

    // Parameterized constructor.
    Node(const T& givenValue) : value(givenValue), next(nullptr) {}
};

template <typename T>
class Queue {
public:
    // Constructor: initializes the queue with a dummy node.
    Queue() {
        head = tail = new Node<T>();  // Create dummy node
        pthread_mutex_init(&head_lock, nullptr);
        pthread_mutex_init(&tail_lock, nullptr);
    }

    // Destructor: deletes all nodes and destroys the mutexes.
    ~Queue() {
        while (head != nullptr) {
            Node<T>* temp = head;
            head = head->next;
            delete temp;
        }
        pthread_mutex_destroy(&head_lock);
        pthread_mutex_destroy(&tail_lock);
    }

    // Enqueue: Inserts an item to the tail of the queue.
    void enqueue(const T& item) {
        Node<T>* newNode = new Node<T>(item);
        pthread_mutex_lock(&tail_lock);
        tail->next = newNode;
        tail = newNode;
        pthread_mutex_unlock(&tail_lock);
    }

    // Dequeue: Removes and returns the item at the front of the queue.
    // Throws std::runtime_error if the queue is empty.
    T dequeue() {
        pthread_mutex_lock(&head_lock);
        Node<T>* first = head->next;  // First real node
        if (first == nullptr) {
            pthread_mutex_unlock(&head_lock);
            throw std::runtime_error("Queue is empty!");
        }
        T returnValue = first->value;
        Node<T>* oldHead = head;
        head = first;
        pthread_mutex_unlock(&head_lock);
        delete oldHead;
        return returnValue;
    }

    // isEmpty: Returns true if the queue is empty; otherwise, false.
    bool isEmpty() {
        pthread_mutex_lock(&head_lock);
        bool empty = (head->next == nullptr);
        pthread_mutex_unlock(&head_lock);
        return empty;
    }

    // print: Prints the elements in the queue from head to tail.
    // If the queue is empty, prints "Empty\n".
    void print() {
        pthread_mutex_lock(&head_lock);
        Node<T>* current = head->next;
        if (current == nullptr) {
            std::cout << "Empty\n";
        } else {
            while (current) {
                std::cout << current->value << " ";
                current = current->next;
            }
            std::cout << "\n";
        }
        pthread_mutex_unlock(&head_lock);
    }

private:
    Node<T>* head;              // Pointer to the dummy head node.
    Node<T>* tail;              // Pointer to the tail node.
    pthread_mutex_t head_lock;  // Mutex for operations on the head.
    pthread_mutex_t tail_lock;  // Mutex for operations on the tail.
};

#endif // QUEUE_H

