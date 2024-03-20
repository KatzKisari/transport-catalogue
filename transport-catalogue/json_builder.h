#include "json.h"
#include <string>
#include <optional>

namespace json {

	class Builder {
	private:
		class KeyAccess;

		class DictAccess {
		public:
			DictAccess(Builder& builder);

			KeyAccess Key(std::string value);
			Builder& EndDict();

		private:
			Builder& builder_;
		};

		class ArrayAccess {
		public:
			ArrayAccess(Builder& builder);

			ArrayAccess& Value(Node::Value value);
			DictAccess StartDict();
			ArrayAccess StartArray();
			Builder& EndArray();

		private:
			Builder& builder_;
		};

		class KeyAccess {
		public:
			KeyAccess(Builder& builder);

			DictAccess Value(Node::Value value);
			DictAccess StartDict();
			ArrayAccess StartArray();

		private:
			Builder& builder_;
		};

		

	public:

		Builder& Value(Node::Value value);
		KeyAccess Key(std::string key);
		DictAccess StartDict();
		ArrayAccess StartArray();
		Builder& EndDict();
		Builder& EndArray();

		Node Build();

	private:
		std::optional<Node> root_;
		std::vector<Node*> nodes_stack_;



		bool IsDone() const;

		Node* AddNode(Node node);

		static Node NodeFromValue(Node::Value&& value);

		bool CurentNodeIsDictKey() const;
		Node* CurentNodeInStack() const;
		Node* PrevNodeInStack() const;
	};
}