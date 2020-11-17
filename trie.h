#ifndef TRIE_H
#define TRIE_H
#pragma once

#include <algorithm>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

template <typename Alphabet, typename Value>
class Trie {
public:
	using ItemList = std::vector<std::pair<std::string, const Value&>>;

	class Node {
	public:
		friend class Trie;

		Node()
		    : parent_(nullptr) {}

		explicit Node(Node* parent)
		    : parent_(parent) {}

		const Node* child(char letter) const {
			if (Alphabet::ord(letter) == -1) {
				throw std::out_of_range("Letter is not member of alphabet");
			}

			for (auto& child : children_) {
				if (child != nullptr && child.get()->key_ == letter) {
					return child.get();
				}
			}
			return nullptr;
		}

		const Node* parent() const {
			return parent_;
		}

		const Value* value() const {
			return value_.get();
		}

	private:
		std::array<std::unique_ptr<Node>, Alphabet::size> children_{};
		Node* parent_ = nullptr;
		std::unique_ptr<Value> value_ = nullptr;
		char key_{};

		void setValue() {
			value_ = std::make_unique<Value>(Value{});
		}

		void setValue(const Value& value) {
			value_ = std::make_unique<Value>(value);
		}

		bool hasValue() const {
			return value_.get() != nullptr;
		}

		bool hasChildren() const {
			for (auto& child : children_) {
				if (child != nullptr) {
					return true;
				}
			}
			return false;
		}

		bool hasParent() const {
			return parent() != nullptr;
		}

		Node* createChild(char key) {
			std::unique_ptr<Node> ptrChild = std::make_unique<Node>(this);
			ptrChild.get()->key_ = key;
			children_[Alphabet::ord(key)] = std::move(ptrChild);
			return children_[Alphabet::ord(key)].get();
		}

		bool removeChild(const Node* ptr) {
			for (auto& child : children_) {
				if (child.get() == ptr) {
					child = nullptr;
					return true;
				}
			}
			return false;
		}

		void removeValue() {
			value_ = nullptr;
		}
		void removeChildren() {
			for (auto& child : children_) {
				child = nullptr;
			}
		}

		std::vector<Node*> getPath(const std::string& key) const {
			std::vector<Node*> path;
			auto node = const_cast<Node*>(this);
			path.push_back(node);
			for (const char& c : key) {
				node = const_cast<Node*>(node->child(c));
				if (node == nullptr) {
					path.push_back(node);
					return path;
				} else {
					path.push_back(node);
				}
			}
			return path;
		}
	};

private:
	Node root_;

	void swap(Trie& other) {
		using std::swap;
		swap(root_, other.root_);
	}

	void getItems(const Node& node, ItemList& items, std::string& key) const {
		if (node.parent() != nullptr) {
			char _key = node.key_;
			key += _key;
		}
		if (node.hasValue()) {
			const Value& value = *node.value();
			items.push_back({ key, value });
		}

		for (auto& child : node.children_) {
			if (child != nullptr) {
				getItems(*child, items, key);
			}
		}
		key.pop_back();
	}

	void draw(const Node& node, std::stringstream& ss, int& id) const {
		int _id = id;
		ss << "\"" << _id << "\" "
		   << "[label=\"";
		if (node.value() != nullptr) {
			ss << *node.value();
		}
		ss << "\"]" << std::endl;

		for (auto& child : node.children_) {
			if (child != nullptr) {
				ss << "\"" << _id << "\" -> \"";
				id++;
				ss << id << "\" [label=\"" << child.get()->key_ << "\"]" << std::endl;
				draw(*child, ss, id);
			}
		}
	}

	bool isInAlphabet(const char letter) const {
		return Alphabet::ord(letter) != -1;
	}

	bool isKeyCorrect(const std::string& key) const {
		for (auto& letter : key) {
			if (!isInAlphabet(letter)) {
				return false;
			}
		}
		return true;
	}

	void copyTrie(const Trie& otherTrie) {
		if (this != &otherTrie) {
			copyTrieRec(root_, otherTrie.root_);
		}
	}

	void copyTrieRec(Node& thisNode, const Node& otherNode) {
		if (!thisNode.hasValue() && otherNode.hasValue()) {
			thisNode.setValue(*otherNode.value());
		}

		for (auto& child : otherNode.children_) {
			if (child != nullptr) {
				Node* thisChild = thisNode.createChild(child.get()->key_);
				copyTrieRec(*thisChild, *child);
			}
		}
	}

public:
	Trie() = default;

	Trie(const Trie& otherTrie) {
		copyTrie(otherTrie);
	}

	Trie& operator=(Trie otherTrie) {
		swap(otherTrie);
		return *this;
	}

	const Node& root() const {
		return root_;
	}

	bool empty() const {
		return !(root_.hasValue() || root_.hasChildren());
	}

	const Value* search(const std::string& key) const {
		if (!isKeyCorrect(key)) {
			throw std::out_of_range("Incorrect key");
		}

		const auto& path = root_.getPath(key);
		if (path.back() != nullptr) {
			return path.back()->value();
		}
		return nullptr;
	}

	Value* search(const std::string& key) {
		const Trie& constSelf = *this;
		return const_cast<Value*>(constSelf.search(key));
	}

	const Value& at(const std::string& key) const {
		if (!isKeyCorrect(key)) {
			throw std::out_of_range("Incorrect key");
		}
		const auto path = root_.getPath(key);
		if (path.back() == nullptr || !path.back()->hasValue())
			throw std::out_of_range("Key not found!");
		else
			return *path.back()->value();
	}

	Value& at(const std::string& key) {
		const Trie& constSelf = *this;
		return const_cast<Value&>(constSelf.at(key));
	}

	void remove(const std::string& key) {
		if (!isKeyCorrect(key)) {
			throw std::out_of_range("Incorrect key");
		}
		std::vector<Node*> path = root_.getPath(key);
		std::reverse(path.begin(), path.end());
		if (path.front() == nullptr) {
			return;
		}
		Node* removeNode = path.front();
		removeNode->removeValue();

		for (Node* node : path) {
			if (node->hasValue() || node->hasChildren() || !node->hasParent())
				return;
			node->parent_->removeChild(node);
		}
	}

	bool insert(const std::string& key, const Value& value) {
		if (!isKeyCorrect(key)) {
			throw std::out_of_range("Incorrect key");
		}
		Node* node = &root_;
		for (const char& c : key) {
			Node* tmpNode;
			tmpNode = const_cast<Node*>(node->child(c));
			if (tmpNode == nullptr) {
				node = node->createChild(c);
			} else {
				node = tmpNode;
			}
		}
		if (!node->hasValue()) {
			node->setValue(value);
			return true;
		}
		return false;
	}

	void clear() {
		root_.removeChildren();
		root_.removeValue();
	}
	/*
	Value& operator[](const std::string& key) {
		if (!isKeyCorrect(key)) {
			throw std::out_of_range("Incorrect key");
		}
		auto path = root_.getPath(key);
		if (path.back() == nullptr) {
			path.pop_back(); //pop null node
			Node* node = path.back(); //Get last existing node and create his ancestors
			for (unsigned long i = path.size() - 1; i < key.length(); ++i) {
				//Create empty nodes that matches key
				node = node->createChild(key[i]);
			}
			node->setValue();
			return *node->value_.get();
		}
		return *path.back()->value_.get();
	}
*/
	Value& operator[](const std::string& key) {
		if (!isKeyCorrect(key)) {
			throw std::out_of_range("Incorrect key");
		}
		const Value* value = search(key);
		if (value == nullptr) {
			insert(key, Value());
			return at(key);
		}
		return const_cast<Value&>(*value);
	}

	ItemList items() const {
		ItemList list{};
		std::string key{};
		getItems(root_, list, key);
		return list;
	}

	void draw(std::ostream& output) const {
		std::stringstream ss;
		int id = 0;
		ss << "digraph {" << std::endl;
		draw(root_, ss, id);
		ss << "}" << std::endl;
		output << ss.str();
	}
};

#endif
