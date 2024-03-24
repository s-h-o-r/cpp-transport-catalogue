#include "json.h"

#include <sstream>

namespace json {

using namespace std::literals;

namespace {

namespace detail {

void Trim(std::string& string) {
    const auto start = string.find_first_not_of(' ');
    string.erase(0, start);
    string.erase(string.find_last_not_of(' ') + 1, string.find_last_of(' '));
}

} // namespace json::detail

Node LoadNode(std::istream& input);

Node LoadBool(std::istream& input) {
    std::string true_or_false;
    char c;
    for (; input >> c && c != ',' &&  c != ']' &&  c != '}' &&  c != ':';) {
        true_or_false.push_back(c);
    }
    input.putback(c);

    detail::Trim(true_or_false);
    if (true_or_false == "true"s) {
        return Node{true};
    }
    if (true_or_false == "false"s) {
        return Node{false};
    }
    throw ParsingError("Null parsing error."s);
}

Node LoadNull(std::istream& input) {
    std::string null;
    
    for (char c; input >> c && c != ',' && c != ']' && c != '}' && c != ':';) {
        null.push_back(c);
    }
    
    detail::Trim(null);
    if (null != "null"s) {
        throw ParsingError("Null parsing error."s);
    }
    return Node{nullptr};
}

Node LoadArray(std::istream& input) {
    Array result;
    char c;
    for (; input >> c && c != ']';) {
        if (c != ',') {
            input.putback(c);
        }
        result.push_back(LoadNode(input));
    }

    if (c != ']') {
        throw ParsingError("Failed to read array from stream."s);
    }

    return Node(std::move(result));
}

Node LoadNumber(std::istream& input) {
    using namespace std::literals;

    std::string parsed_num;

    // Считывает в parsed_num очередной символ из input
    auto read_char = [&parsed_num, &input] {
        parsed_num += static_cast<char>(input.get());
        if (!input) {
            throw ParsingError("Failed to read number from stream"s);
        }
    };

    // Считывает одну или более цифр в parsed_num из input
    auto read_digits = [&input, read_char] {
        if (!std::isdigit(input.peek())) {
            throw ParsingError("A digit is expected"s);
        }
        while (std::isdigit(input.peek())) {
            read_char();
        }
    };

    if (input.peek() == '-') {
        read_char();
    }
    // Парсим целую часть числа
    if (input.peek() == '0') {
        read_char();
        // После 0 в JSON не могут идти другие цифры
    } else {
        read_digits();
    }

    bool is_int = true;
    // Парсим дробную часть числа
    if (input.peek() == '.') {
        is_int = false;
        read_char();
        read_digits();
    }

    // Парсим экспоненциальную часть числа
    if (char ch = input.peek(); ch == 'e' || ch == 'E') {
        read_char();
        if (ch = input.peek(); ch == '+' || ch == '-') {
            read_char();
        }
        read_digits();
        is_int = false;
    }

    try {
        if (is_int) {
            // Сначала пробуем преобразовать строку в int
            try {
                int num = std::stoi(parsed_num);
                return Node{num};
            } catch (...) {
                // В случае неудачи, например, при переполнении,
                // код ниже попробует преобразовать строку в double
            }
        }
        double num = std::stod(parsed_num);
        return Node{num};
    } catch (...) {
        throw ParsingError("Failed to convert "s + parsed_num + " to number"s);
    }
}

Node LoadString(std::istream& input) {
    auto it = std::istreambuf_iterator<char>(input);
    auto end = std::istreambuf_iterator<char>();
    std::string s;
    while (true) {
        if (it == end) {
            // Поток закончился до того, как встретили закрывающую кавычку?
            throw ParsingError("String parsing error");
        }
        const char ch = *it;
        if (ch == '"') {
            // Встретили закрывающую кавычку
            ++it;
            break;
        } else if (ch == '\\') {
            // Встретили начало escape-последовательности
            ++it;
            if (it == end) {
                // Поток завершился сразу после символа обратной косой черты
                throw ParsingError("String parsing error");
            }
            const char escaped_char = *(it);
            // Обрабатываем одну из последовательностей: \\, \n, \t, \r, \"
            switch (escaped_char) {
                case 'n':
                    s.push_back('\n');
                    break;
                case 't':
                    s.push_back('\t');
                    break;
                case 'r':
                    s.push_back('\r');
                    break;
                case '"':
                    s.push_back('"');
                    break;
                case '\\':
                    s.push_back('\\');
                    break;
                default:
                    // Встретили неизвестную escape-последовательность
                    throw ParsingError("Unrecognized escape sequence \\"s + escaped_char);
            }
        } else if (ch == '\n' || ch == '\r') {
            // Строковый литерал внутри- JSON не может прерываться символами \r или \n
            throw ParsingError("Unexpected end of line"s);
        } else {
            // Просто считываем очередной символ и помещаем его в результирующую строку
            s.push_back(ch);
        }
        ++it;
    }

    return {s};
}

Node LoadDict(std::istream& input) {
    Dict result;
    std::string key;
    for (char c; input >> c && c != '}';) {
        if (c == '"') {
            key = LoadString(input).AsString();
        } else if (c == ':') {
            if (result.find(key) != result.end()) {
                throw ParsingError("Value is already exist."s);
            }
            result.emplace(std::move(key), LoadNode(input));
        } else if (c == ',') {
            continue;
        }
    }

    if (!input) {
        throw ParsingError("Failed to read map from stream."s);
    }

    return Node(std::move(result));
}

Node LoadNode(std::istream& input) {
    char c;
    input.get(c);
    while (c == ' ' || c == '\n' || c == '\t' || c == '\r') {
        input.get(c);
    }

    if (c == ']' || c == '}') {
        throw ParsingError("Array or Map has been closed before opening."s);
    } else if (c == '[') {
        return LoadArray(input);
    } else if (c == '{') {
        return LoadDict(input);
    } else if (c == '"') {
        return LoadString(input);
    } else if (c == 'n') {
        input.putback(c);
        return LoadNull(input);
    } else if (c == 't' || c == 'f') {
        input.putback(c);
        return LoadBool(input);
    } else {
        input.putback(c);
        return LoadNumber(input);
    }
}

}  // namespace

const Value& Node::GetValue() const {
    return *this;
}

Value& Node::GetValue() {
    return *this;
}

bool Node::operator==(const Node& other) const {
    return other.GetValue() == *this;
}

bool Node::operator!=(const Node& other) const {
    return !(other == *this);
}

bool Node::IsInt() const {
    return std::holds_alternative<int>(*this);
}

bool Node::IsDouble() const {
    return std::holds_alternative<int>(*this)
        || std::holds_alternative<double>(*this);
}

bool Node::IsPureDouble() const {
    return std::holds_alternative<double>(*this);
}

bool Node::IsBool() const {
    return std::holds_alternative<bool>(*this);
}

bool Node::IsString() const {
    return std::holds_alternative<std::string>(*this);
}

bool Node::IsNull() const {
    return std::holds_alternative<std::nullptr_t>(*this);
}

bool Node::IsArray() const {
    return std::holds_alternative<Array>(*this);
}

bool Node::IsDict() const {
    return std::holds_alternative<Dict>(*this);
}

int Node::AsInt() const {
    if (!IsInt()) {
        throw std::logic_error("Value has another type."s);
    }
    return std::get<int>(*this);
}

bool Node::AsBool() const {
    if (!IsBool()) {
        throw std::logic_error("Value has another type."s);
    }
    return std::get<bool>(*this);
}

double Node::AsDouble() const {
    if (!IsPureDouble()) {
        if (IsInt()) {
            return std::get<int>(*this);
        }
        throw std::logic_error("Value has another type."s);
    }
    return std::get<double>(*this);
}

const std::string& Node::AsString() const {
    if (!IsString()) {
        throw std::logic_error("Value has another type."s);
    }
    return std::get<std::string>(*this);
}

const Array& Node::AsArray() const {
    if (!IsArray()) {
        throw std::logic_error("Value has another type."s);
    }
    return std::get<Array>(*this);
}
const Dict& Node::AsDict() const {
    if (!IsDict()) {
        throw std::logic_error("Value has another type."s);
    }
    return std::get<Dict>(*this);
}

Document::Document(Node root)
    : root_(std::move(root)) {
}

bool Document::operator==(const Document& other) const {
    return other.root_ == this->root_;
}

bool Document::operator!=(const Document& other) const {
    return !(other == *this);
}

const Node& Document::GetRoot() const {
    return root_;
}

Document Load(std::istream& input) {
    return Document{LoadNode(input)};
}

struct PrintContext {
    std::ostream& out;
    int indent_step = 4;
    int indent = 0;

    void PrintIndent() const {
        for (int i = 0; i < indent; ++i) {
            out.put(' ');
        }
    }

    // Возвращает новый контекст вывода с увеличенным смещением
    PrintContext Indented() const {
        return {out, indent_step, indent_step + indent};
    }
};

void PrintNode(const Node& node, PrintContext ctx);

struct NodeValuePrinter {
    PrintContext ctx;

    void operator()(std::nullptr_t) {
        ctx.out << "null"sv;
    }

    void operator()(bool value) {
        ctx.out << (value ? "true"sv : "false"sv);
    }

    void operator()(int value) {
        ctx.out << value;
    }

    void operator()(double value) {
        ctx.out << value;
    }

    void operator()(const std::string& string) {
        ctx.out << '"';
        for (const char ch : string) {
            switch (ch) {
                case '\\':
                    ctx.out << "\\\\"s;
                    break;
                case '\n':
                    ctx.out << "\\n"s;
                    break;
                case '\t':
                    ctx.out << "\\t"s;
                    break;
                case '\r':
                    ctx.out << "\\r"s;
                    break;
                case '"':
                    ctx.out << "\\\""s;
                    break;
                default:
                    ctx.out << ch;
                    break;
            }
        }
        ctx.out << '"';
    }

    void operator()(const Array& array) {
        std::ostream& out = ctx.out;

        out << '[' << std::endl;

        PrintContext inner_ctx = ctx.Indented();
        bool need_comma = false;
        for (const Node& value : array) {
            if (need_comma) {
                ctx.out << ',' << std::endl;
            } else {
                need_comma = true;
            }
            inner_ctx.PrintIndent();
            PrintNode(value, inner_ctx);
        }
        out << std::endl;
        ctx.PrintIndent();
        out << ']';
    }

    void operator()(const Dict& map) {
        std::ostream& out = ctx.out;

        out << '{' << std::endl;

        PrintContext inner_ctx = ctx.Indented();
        bool need_comma = false;
        for (const auto& [key, value] : map) {
            if (need_comma) {
                out << ',' << std::endl;
            } else {
                need_comma = true;
            }
            inner_ctx.PrintIndent();
            out << '"' << key << "\": "s;
            PrintNode(value, inner_ctx);
        }
        out << std::endl;
        ctx.PrintIndent();
        out << '}';
    }
};

void PrintNode(const Node& node, PrintContext ctx) {
    std::visit(NodeValuePrinter{ctx}, node.GetValue());
}

void Print(const Document& doc, std::ostream& output) {
    PrintNode(doc.GetRoot(), PrintContext{output});
}

}  // namespace json
