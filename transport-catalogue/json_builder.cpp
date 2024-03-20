#include "json_builder.h"

namespace json {

	Builder::DictAccess::DictAccess(Builder& builder) : builder_(builder) { }

	Builder::KeyAccess Builder::DictAccess::Key(std::string value) {
		return builder_.Key(value);
	}

	Builder& Builder::DictAccess::EndDict() {
		return builder_.EndDict();
	}



	Builder::ArrayAccess::ArrayAccess(Builder& builder) : builder_(builder) { }

	Builder::ArrayAccess& Builder::ArrayAccess::Value(Node::Value value) {
		builder_.Value(value);
		return *this;
	}

	Builder::DictAccess Builder::ArrayAccess::StartDict() {
		return builder_.StartDict();
	}

	Builder::ArrayAccess Builder::ArrayAccess::StartArray() {
		return builder_.StartArray();
	}

	Builder& Builder::ArrayAccess::EndArray() {
		return builder_.EndArray();
	}




	Builder::KeyAccess::KeyAccess(Builder& builder) : builder_(builder) { }

	Builder::DictAccess Builder::KeyAccess::Value(Node::Value value) {
		builder_.Value(value);
		return DictAccess(builder_);
	}

	Builder::DictAccess Builder::KeyAccess::StartDict() {
		return builder_.StartDict();
	}

	Builder::ArrayAccess Builder::KeyAccess::StartArray() {
		return builder_.StartArray();
	}


	// ***************************



	Builder& Builder::Value(Node::Value value) {
		try {
			AddNode(NodeFromValue(std::move(value)));
		}
		catch (const std::logic_error&) {
			throw std::logic_error("invalid value placement");
		}

		return *this;
	}


	Builder::KeyAccess Builder::Key(std::string key) {
		if (CurentNodeInStack()->IsDict()) {
			nodes_stack_.push_back(new Node(std::move(key)));
		}
		else
			throw std::logic_error("invalid key placement");

		return KeyAccess(*this);
	}

	Builder::DictAccess Builder::StartDict() {
		try {
			nodes_stack_.push_back(AddNode(Dict{}));
		}
		catch (const std::logic_error&) {
			throw std::logic_error("invalid array placement");
		}

		return DictAccess(*this);
	}

	Builder::ArrayAccess Builder::StartArray() {
		try {
			nodes_stack_.push_back(AddNode(Array{}));
		}
		catch (const std::logic_error&) {
			throw std::logic_error("invalid array placement");
		}

		return ArrayAccess(*this);
	}

	Builder& Builder::EndDict() {
		if (!nodes_stack_.empty() and CurentNodeInStack()->IsDict())
			nodes_stack_.pop_back();
		else
			throw std::logic_error("calling the EndDict() method before calling StartDict() is not possible");

		return *this;
	}

	Builder& Builder::EndArray() {
		if (!nodes_stack_.empty() and CurentNodeInStack()->IsArray())
			nodes_stack_.pop_back();
		else
			throw std::logic_error("calling the EndArray() method before calling StartArray() is not possible");

		return *this;
	}



	Node Builder::Build() {
		if (IsDone())
			return std::move(*root_);
		else
			throw std::logic_error((nodes_stack_.empty() ? "impossible to construct an empty JSON" : "attempt to build an incomplete JSON"));
	}





	bool Builder::IsDone() const {
		return (root_.has_value() and nodes_stack_.empty());
	}



	Node* Builder::AddNode(Node node) {
		if (IsDone())
			throw std::logic_error("invalid node placement");

		Node* new_node;

		if (!root_) {
			root_ = node;
			new_node = &*root_;
		}
		else if (CurentNodeInStack()->IsArray()) {
			CurentNodeInStack()->AsArray().push_back(node);
			new_node = &(CurentNodeInStack()->AsArray().back());
		}
		else if (CurentNodeIsDictKey()) {
			Node& pair_value = PrevNodeInStack()->AsDict()[CurentNodeInStack()->AsString()];

			pair_value = node;
			nodes_stack_.pop_back();
			new_node = &pair_value;
		}
		else
			throw std::logic_error("invalid node placement");

		return new_node;
	}





	Node Builder::NodeFromValue(Node::Value&& value) {
		json::Node node;
		node.GetValue() = std::move(value);
		return node;
	}



	bool Builder::CurentNodeIsDictKey() const {
		return (nodes_stack_.size() > 1 and CurentNodeInStack()->IsString() and PrevNodeInStack()->IsDict());
	}

	Node* Builder::CurentNodeInStack() const {
		if (nodes_stack_.size() > 0)
			return nodes_stack_.back();
		else
			throw std::out_of_range("there are no nodes in the stack");
	}

	Node* Builder::PrevNodeInStack() const {
		if (nodes_stack_.size() > 1)
			return (*std::prev(nodes_stack_.end(), 2));
		else
			throw std::out_of_range("there are one or no nodes in the stack");
	}
}