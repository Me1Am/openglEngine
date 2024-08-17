#pragma once

#include <vector>

/// @brief Holds a unique id and (optionally) a pointer to the parent and vector to child nodes
struct Node {
	unsigned int UID;

	Node* parent = nullptr;
	std::vector<Node*> childs;

	~Node() {
		for(Node* child : childs) {
			delete child;
		}
	}
};
