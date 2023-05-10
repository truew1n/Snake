#include <stdio.h>
#include <stdlib.h>
#include <float.h>

typedef struct Point {
    float x;
    float y;
} Point;

typedef struct Node {
    struct Node *next;
    struct Node *last;
    Point value;
} Node;

typedef struct Vector {
    Node *head;
    Node *tail;
    size_t size;
} Vector;

void v_push_back(Vector *vector, float x, float y)
{
    Node *new_node = (Node *) malloc(sizeof(Node));
    new_node->next = NULL;
    new_node->last = vector->tail;
    new_node->value = (Point){x, y};
    if(vector->size == 0) {
        vector->head = new_node;
        vector->tail = new_node;
        vector->size++;
    } else {
        vector->tail->next = new_node;
        vector->tail = new_node;
        vector->size++;
    }
}

void v_pop(Vector *vector)
{
    if(vector->size == 0) return;
    if(vector->size == 1) {
        vector->head = NULL;
        vector->tail = NULL;
        vector->size--;
    } else {
        Node *temp = vector->head->next;
        free(vector->head);
        vector->head = temp;
        vector->size--;
    }
}

Point v_first(Vector *vector)
{
    return vector->head->value;
}

void v_print(Vector *vector)
{
    Node *node = vector->head;
    while(node != NULL) {
        printf("(%.2f, %.2f)\n", node->value.x, node->value.y);
        node = node->next;
    }
}













