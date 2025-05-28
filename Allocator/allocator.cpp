#include <iostream>

// Node structure for the linked list
struct heapNode {
    int id;
    int size;
    int index;
    heapNode* prev;
    heapNode* next;

    heapNode(int id, int size, int index) : id(id), size(size), index(index), prev(nullptr), next(nullptr) {}
};

class HeapManager {
public:
    heapNode* head;
    heapNode* tail;
    pthread_mutex_t heapLock;

    HeapManager() : head(nullptr), tail(nullptr) {
        pthread_mutex_init(&heapLock, NULL);
    }

    //// Iterator/Helper methods to simplify operations
    heapNode* begin() {
        return head;
    }

    heapNode* end() {
        return nullptr;
    }

    heapNode* next(heapNode* node) {
        return node->next;
    }

    heapNode* prev(heapNode* node) {
        return node->prev;
    }

    // An additional print method to simplify the code
    void print() {
        for (heapNode* it = begin(); it != end(); it = next(it)) {
            std::cout << "[" << it->id << "][" << it->size << "][" << it->index << "]";
            if (next(it) != end()) {
                std::cout << "---";
            }
        }
        std::cout << std::endl;
    }
    //// End of Iterator/Helper methods

    //// myMalloc helper methods
    // Helper method to find the first free node of a given size
    heapNode* firstFree(int size) {
        for (heapNode* it = begin(); it != end(); it = next(it)) {
            if (it->id == -1 && it->size >= size) {
                return it;
            }
        }
        return nullptr;
    }

    void splitNode(heapNode* node, int size) {
        if (node->size > size) {
            heapNode* newNode = new heapNode(-1, node->size - size, node->index + size);
            newNode->next = node->next;
            newNode->prev = node;
            if (node->next) {
                node->next->prev = newNode;
            }
            node->next = newNode;
            node->size = size;
            if (tail == node) {
                tail = newNode;
            }
        }
    }
    //// End of myMalloc helper methods

    //// myFree helper methods
    heapNode* findById(int id, int index) {
        for (heapNode* it = begin(); it != end(); it = next(it)) {
            if (it->id == id && it->index == index) {
                return it;
            }
        }
        return nullptr;
    }

    void mergeNodes(heapNode* node) {
        if (node->prev && node->prev->id == -1) {
            node->prev->size += node->size;
            node->prev->next = node->next;
            if (node->next) {
                node->next->prev = node->prev;
            }
            if (tail == node) {
                tail = node->prev;
            }
            delete node;
            node = node->prev;
        }
        if (node->next && node->next->id == -1) {
            node->size += node->next->size;
            heapNode* toDelete = node->next;
            node->next = node->next->next;
            if (node->next) {
                node->next->prev = node;
            }
            if (tail == toDelete) {
                tail = node;
            }
            delete toDelete;
        }
    }
    //// End of myFree helper methods

    //// HeapManager methods
    int initHeap(int size) {
        pthread_mutex_lock(&heapLock);
        heapNode* node = new heapNode(-1, size, 0);
        head = tail = node;
        print();
        pthread_mutex_unlock(&heapLock);
        return 1;
    }

    int myMalloc(int id, int size) {
        pthread_mutex_lock(&heapLock);
        heapNode* node = firstFree(size);
        if (node) {
            node->id = id;
            splitNode(node, size);
            print();
            pthread_mutex_unlock(&heapLock);
            return node->index;
        }
        print();
        pthread_mutex_unlock(&heapLock);
        return -1;
    }

    int myFree(int id, int index) {
        pthread_mutex_lock(&heapLock);
        heapNode* node = findById(id, index);
        if (node) {
            node->id = -1;
            mergeNodes(node);
            print();
            pthread_mutex_unlock(&heapLock);
            return 1;
        }
        print();
        pthread_mutex_unlock(&heapLock);
        return -1;
    }
};







