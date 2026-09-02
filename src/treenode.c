// implementation for binary tree traversal, functionas and friends of free nodes
#include <stddef.h>
#include <stdlib.h>
#include <assert.h>
#include <time.h>

#include "allocator.h"
#include "treenode.h"

void
bst_insert_node(FreeNode **root_ptr, FreeNode *node, const size_t size)
{
    assert(root_ptr != NULL && node != NULL);
    node->tree.left = NULL;
    node->tree.right = NULL;
    FreeNode *root = *root_ptr;

    if (root == NULL) {
        *root_ptr = node;
    }
    else {
        FreeNode *parent = NULL;
        FreeNode *current = root;
        BlockHeader *header = NULL;
        while (current) {
            parent = current;
            header = (BlockHeader *)((char *)current - sizeof(BlockHeader));
            if (size >= header->size) {
                current = current->tree.right;
            }
            else {
                current = current->tree.left;
            }
        }
        assert(parent != NULL);
        assert(header != NULL);

        if (header->size > size) {
            parent->tree.left = node;
        }
        else {
            parent->tree.right = node;
        }
    }
}


// Get the smallest possible node size that is needed
FreeNode *
bst_get_most_suitable_node(FreeNode *root, const size_t size)
{
    FreeNode *current = root;
    FreeNode *best_node = NULL;
    while (current) {
        BlockHeader *header = (BlockHeader *)((char *)current - sizeof(BlockHeader));
        if (header->size > size) {
            best_node = current;
            current = current->tree.left;
        }
        else if (header->size == size) {
            return current;
        }
        else {
            current = current->tree.right;
        }
    }
    return best_node;
}


static inline void
replace_child(
    FreeNode **root_ptr,
    FreeNode *parent,
    FreeNode *child,
    FreeNode *replacement)
{
    assert(root_ptr != NULL);

    if (parent == NULL) {
        assert(*root_ptr == child);
        *root_ptr = replacement;
    }
    else if (parent->tree.left == child) {
        parent->tree.left = replacement;
    }
    else {
        assert(parent->tree.right == child);
        parent->tree.right = replacement;
    }
}


void
bst_pop_node(FreeNode **root_ptr, FreeNode *node, const size_t size)
{
    assert(root_ptr != NULL && node != NULL);

    FreeNode *parent = NULL;
    FreeNode *current = *root_ptr;
    while (current) {
        BlockHeader *header = (BlockHeader *)((char *)current - sizeof(BlockHeader));
        if (header->size < size) {
            parent = current;
            current = current->tree.right;
        }
        else if (header->size > size) {
            parent = current;
            current = current->tree.left;
        }
        else if (node == current) {
            break;
        }
        else {
            parent = current;
            current = current->tree.right;
        }
    }

    assert(current != NULL);

    if (current->tree.left == NULL && current->tree.right == NULL) {
        replace_child(root_ptr, parent, current, NULL);
    }   
    else if (current->tree.right == NULL) {
        replace_child(root_ptr, parent, current, current->tree.left);
    }
    else if (current->tree.left == NULL) {
        replace_child(root_ptr, parent, current, current->tree.right);
    }
    else {
        FreeNode *successor = current->tree.right;
        FreeNode *parent_successor = current;
        while (successor->tree.left) {
            parent_successor = successor;
            successor = successor->tree.left;
        }

        if (parent_successor == current) {
            successor->tree.left = current->tree.left;
        }
        else {  
            parent_successor->tree.left = successor->tree.right;
            successor->tree.left = current->tree.left;
            successor->tree.right = current->tree.right;
        }
        
        replace_child(root_ptr, parent, current, successor);
    }

    node->tree.right = NULL;
    node->tree.left = NULL;
}