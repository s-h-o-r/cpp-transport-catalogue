#include "json_builder.h"

using namespace std::literals;

namespace json {

Builder::Builder()
    : root_(nullptr) 
{
    nodes_stack_.push_back(&root_);
}

DictValueContext Builder::Key(std::string string) {
    IsNodeReady();
    if (nodes_stack_.back()->IsDict()) {
        auto& dict = std::get<Dict>(nodes_stack_.back()->GetValue());
        dict[string] = Node{nullptr};
        nodes_stack_.push_back(&dict[string]);
    } else {
        throw std::logic_error("Cannot put the key not in a dict."s);
    }
    return DictValueContext(*this);
}

BaseContext Builder::Value(json::Value value) {
    IsNodeReady();
    if (nodes_stack_.back()->IsNull()) {
        nodes_stack_.back()->GetValue() = value;
        nodes_stack_.pop_back();
    } else if (nodes_stack_.back()->IsArray()) {
        auto& array = std::get<Array>(nodes_stack_.back()->GetValue());
        array.emplace_back(Node());
        array.back().GetValue() = value;
    } else {
        throw std::logic_error("Cannot put the value."s);
    }

    return BaseContext(*this);
}

DictItemContext Builder::StartDict() {
    IsNodeReady();
    if (nodes_stack_.back()->IsNull()) {
        nodes_stack_.back()->GetValue() = Dict();
    } else if (nodes_stack_.back()->IsArray()) {
        auto& array = std::get<Array>(nodes_stack_.back()->GetValue());
        array.emplace_back(Dict());
        nodes_stack_.push_back(&array.back());
    } else {
        throw std::logic_error("Cannot start a dict."s);
    }
    return DictItemContext(*this);
}

ArrayItemContest Builder::StartArray() {
    IsNodeReady();
    if (nodes_stack_.back()->IsNull()) {
        nodes_stack_.back()->GetValue() = Array();
    } else if (nodes_stack_.back()->IsArray()) {
        auto& array = std::get<Array>(nodes_stack_.back()->GetValue());
        array.emplace_back(Array());
        nodes_stack_.push_back(&array.back());
    } else {
        throw std::logic_error("Cannot start an array."s);
    }
    return ArrayItemContest(*this);
}

BaseContext Builder::EndDict(){
    IsNodeReady();
    if (!nodes_stack_.back()->IsDict()) {
        throw std::logic_error("Node is not a dict."s);
    }
    nodes_stack_.pop_back();
    return BaseContext(*this);
}

BaseContext Builder::EndArray() {
    IsNodeReady();
    if (!nodes_stack_.back()->IsArray()) {
        throw std::logic_error("Node is not an array."s);
    }
    nodes_stack_.pop_back();
    return BaseContext(*this);
}

Node Builder::Build() {
    if (!nodes_stack_.empty()) {
        throw std::logic_error("Node is not ready."s);
    }
    return root_;
}

void Builder::IsNodeReady() {
    if (nodes_stack_.empty()) {
        throw std::logic_error("Node is ready and cannot be modified."s);
    }
}

BaseContext::BaseContext(Builder& builder)
    : builder_(builder) {
}

DictValueContext BaseContext::Key(std::string string) {
    return builder_.Key(std::move(string));
}

BaseContext BaseContext::Value(json::Value value) {
    return builder_.Value(std::move(value));
}

DictItemContext BaseContext::StartDict() {
    return builder_.StartDict();
}

ArrayItemContest BaseContext::StartArray() {
    return builder_.StartArray();
}

BaseContext BaseContext::EndDict() {
    return builder_.EndDict();
}

BaseContext BaseContext::EndArray() {
    return builder_.EndArray();
}

Node BaseContext::Build() {
    return builder_.Build();
}

DictItemContext::DictItemContext(Builder& builder)
    : BaseContext(builder) {
}

DictValueContext DictItemContext::Key(std::string string) {
    return BaseContext::Key(std::move(string));
}

BaseContext DictItemContext::EndDict() {
    return BaseContext::EndDict();
}

DictValueContext::DictValueContext(Builder& builder)
    : BaseContext(builder) {
}

DictItemContext DictValueContext::Value(json::Value value) {
    BaseContext::Value(std::move(value));
    return DictItemContext(builder_);
}

DictItemContext DictValueContext::StartDict() {
    return BaseContext::StartDict();;
}

ArrayItemContest DictValueContext::StartArray() {
    return builder_.StartArray();
}

ArrayItemContest::ArrayItemContest(Builder& builder)
    : BaseContext(builder) {
}

ArrayItemContest ArrayItemContest::Value(json::Value value) {
    BaseContext::Value(std::move(value));
    return ArrayItemContest(builder_);
}

DictItemContext ArrayItemContest::StartDict() {
    return BaseContext::StartDict();
}
ArrayItemContest ArrayItemContest::StartArray() {
    return BaseContext::StartArray();
}

BaseContext ArrayItemContest::EndArray() {
    return BaseContext::EndArray();
}

}
