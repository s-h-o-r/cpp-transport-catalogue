#pragma once

#include "json.h"

#include <string>
#include <vector>

namespace json {

class BaseContext ;
class DictItemContext;
class DictValueContext;
class ArrayItemContest;

class Builder {
public:
    Builder();
    DictValueContext Key(std::string string);
    BaseContext Value(json::Value value);
    DictItemContext StartDict();
    ArrayItemContest StartArray();
    BaseContext EndDict();
    BaseContext EndArray();
    Node Build();

private:
    Node root_;
    std::vector<Node*> nodes_stack_;

    void IsNodeReady();
};

class BaseContext {
public:
    BaseContext(Builder& builder);
    DictValueContext Key(std::string string);
    BaseContext Value(json::Value value);
    DictItemContext StartDict();
    ArrayItemContest StartArray();
    BaseContext EndDict();
    BaseContext EndArray();
    Node Build();

    Builder& builder_;
};

class DictItemContext : public BaseContext {
public:
    DictItemContext(Builder& builder);
    DictValueContext Key(std::string string);
    BaseContext EndDict();
    Builder& Value(json::Value value) = delete;
    DictItemContext StartDict() = delete;
    ArrayItemContest StartArray() = delete;
    BaseContext EndArray() = delete;
    Node Build() = delete;
};

class DictValueContext : public BaseContext {
public:
    DictValueContext(Builder& builder);
    DictItemContext Value(json::Value value);
    DictItemContext StartDict();
    ArrayItemContest StartArray();
    DictValueContext Key(std::string string) = delete;
    BaseContext EndDict() = delete;
    BaseContext EndArray() = delete;
    Node Build() = delete;
};

class ArrayItemContest : public BaseContext {
public:
    ArrayItemContest(Builder& builder);
    ArrayItemContest Value(json::Value value);
    DictItemContext StartDict();
    ArrayItemContest StartArray();
    BaseContext EndArray();
    DictValueContext Key(std::string string) = delete;
    BaseContext EndDict() = delete;
    Node Build() = delete;
};
}
