#pragma once

#include <iostream>
#include <map>
#include <string>
#include <variant>
#include <vector>

namespace json {

    class Node;
    using Dict = std::map<std::string, Node>;
    using Array = std::vector<Node>;

    class ParsingError : public std::runtime_error {
    public:
        using runtime_error::runtime_error;
    };




    class Node final
        : private std::variant<std::nullptr_t, Array, Dict, bool, int, double, std::string> {
    public:
        using variant::variant;
        using Value = variant;


        bool IsNull() const;
        bool IsBool() const;
        bool IsInt() const;
        bool IsDouble() const;
        bool IsPureDouble() const;
        bool IsString() const;
        bool IsArray() const;
        bool IsDict() const;


        bool AsBool() const;
        int AsInt() const;
        double AsDouble() const;

        const std::string& AsString() const;
        std::string& AsString();

        const Array& AsArray() const;
        Array& AsArray();

        const Dict& AsDict() const;
        Dict& AsDict();



        const Value& GetValue() const;
        Value& GetValue();

        bool operator==(const Node& rhs) const;
    };

    inline bool operator!=(const Node& lhs, const Node& rhs) {
        return !(lhs == rhs);
    }




    class Document {
    public:
        explicit Document(Node root)
            : root_(std::move(root)) {
        }

        const Node& GetRoot() const {
            return root_;
        }

    private:
        Node root_;
    };

    inline bool operator==(const Document& lhs, const Document& rhs) {
        return lhs.GetRoot() == rhs.GetRoot();
    }
    inline bool operator!=(const Document& lhs, const Document& rhs) {
        return !(lhs == rhs);
    }

    Document Load(std::istream& input);
    void Print(const Document& doc, std::ostream& output);

}  // namespace json