#ifndef TREENODE_HEADER
#define TREENODE_HEADER

typedef struct FreeNode FreeNode;

void
bst_insert_node(FreeNode **root_ptr, FreeNode *node, const size_t size);

FreeNode *
bst_get_most_suitable_node(FreeNode *root, const size_t size);

void
bst_pop_node(FreeNode **root_ptr, FreeNode *node, const size_t size);

#endif