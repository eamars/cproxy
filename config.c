#include <stdio.h>
#include <string.h>
#include <stdlib.h> // provides malloc
#include <ctype.h> // provides isspace
#include "config.h"

#define MAX_KEY_LEN 512
#define MAX_VALUE_LEN 512

typedef struct node_s Node;
struct node_s
{
	char *key;
	char *value;
	Node *left;
	Node *right;
};


// Global variables
static Node *config_bst;

static char *trim_whitespace(char *s)
{
	char *end;

	// trim leading space
	while (isspace(*s))
	{
		s++;
	}

	if (*s == 0)
	{
		return s;
	}

	// trim trailing space
	end = s + strlen(s) - 1;
	while (end > s && isspace(*end))
	{
		end--;
	}
	*(end + 1) = 0;

	return s;
}


// Create and copy to new node
static Node *create_new_node(char *key, char *value)
{
	// Allocate memory for node
	Node *node = (Node *)malloc(sizeof(Node));

	// Allocate memory for key
	node->key = (char *)malloc(strlen(key) + 1);
	node->value = (char *)malloc(strlen(value) + 1);

	// Copy content to node
	strcpy(node->key, key);
	strcpy(node->value, value);

	// Set left and right to NULL
	node->left = NULL;
	node->right = NULL;

	return node;
}


// Insert into BST
static Node *insert(Node *head, char *key, char *value)
{
	int compare;

	// Base case: empty list
	if (head == NULL)
	{
		head = create_new_node(key, value);
	}
	else
	{
		compare = strcmp(key, head->key);
		// All keys in cfg are unique
		if (compare == 0)
		{
			printf("oops!\n");
		}
		else if (compare < 0)
		{
			head->left = insert(head->left, key, value);
		}
		else
		{
			head->right = insert(head->right, key, value);
		}
	}

	return head;
}

static void free_node(Node *node)
{
	free(node->key);
	free(node->value);
	free(node);
}

static void free_nodes(Node *head)
{
	if (head != NULL)
	{
		free_nodes(head->left);
		free_nodes(head->right);
		free_node(head);
	}
}

static int search(Node *head, char *key, char *value)
{
	int compare;

	if (head == NULL)
	{
		return 0;
	}
	else
	{
		compare = strcmp(key, head->key);

		if (compare == 0)
		{
			strcpy(value, head->value);
			return 1;
		}
		else if (compare < 0)
		{
			return search(head->left, key, value);
		}
		else
		{
			return search(head->right, key, value);
		}
	}

}

static int scanner(FILE *fp)
{
	int c;
	int comment_flag = 0;

	int key_value_flag = 0; // 0 for key, 1 for value

	char key_buf[MAX_KEY_LEN];
	char value_buf[MAX_VALUE_LEN];
	int key_index = 0;
	int value_index = 0;

	// Initilize buffers
	memset(key_buf, 0, sizeof(key_buf));
	memset(value_buf, 0, sizeof(value_buf));

	// Initilize bst
	config_bst = NULL;

	while ((c = fgetc(fp)) != EOF)
	{
		// is comments
		if (c == '#')
		{
			comment_flag = 1;
			if (key_index != 0)
			{
				key_value_flag = 0;
				config_bst = insert(config_bst, trim_whitespace(key_buf), trim_whitespace(value_buf));

				memset(key_buf, 0, sizeof(key_buf));
				memset(value_buf, 0, sizeof(value_buf));
				key_index = 0;
				value_index = 0;
			}
			continue;
		}

		// is new line
		if (c == '\n')
		{
			comment_flag = 0;
			key_value_flag = 0;

			// load to linked list
			if (key_index != 0)
			{
				config_bst = insert(config_bst, trim_whitespace(key_buf), trim_whitespace(value_buf));
			}

			// Set to initial state
			memset(key_buf, 0, sizeof(key_buf));
			memset(value_buf, 0, sizeof(value_buf));
			key_index = 0;
			value_index = 0;

			continue;
		}

		// skip any comments
		if (comment_flag == 1)
		{
			continue;
		}

		// is '=' and is key
		if (c == '=' && key_value_flag == 0)
		{
			key_value_flag = 1;
			continue;
		}

		// read key
		if (key_value_flag == 0)
		{
			key_buf[key_index++] = c;
		}
		else
		{
			value_buf[value_index++] = c;
		}
	}

	// still remained some in buffer
	if (key_index != 0)
	{
		config_bst = insert(config_bst, trim_whitespace(key_buf), trim_whitespace(value_buf));
		// Set to initial state
		memset(key_buf, 0, sizeof(key_buf));
		memset(value_buf, 0, sizeof(value_buf));
		key_index = 0;
		value_index = 0;
	}




	return 0;
}


int config_open(char *cname)
{
	FILE *fp;
	int ret;

	if ((fp = fopen(cname, "r")) == NULL)
	{
		// Failed to open file
		return -1;
	}

	if ((ret = scanner(fp)) != 0)
	{
		// Failed to read file
		fclose(fp);
		return -2;
	}
	fclose(fp);

	return 0;

}

void config_close()
{
	free_nodes(config_bst);
}

int config_get_value(char *key, char *value)
{
	// perform an inorder search
	return search(config_bst, key, value);
}

