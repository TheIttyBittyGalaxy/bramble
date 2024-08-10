#include <exception>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <vector>

using namespace std;

// POINTERS //

template <class T>
using ptr = shared_ptr<T>;

#define CREATE(T) (ptr<T>(new T))

// SOURCE FILES AND TOKENS  //

typedef struct
{
    enum class Kind
    {
        INVALID,

        EQUAL,
        DIVIDE,
        BRACKET_L,
        BRACKET_R,
        CURLY_L,
        CURLY_R,

        NUM_LIT,
        IDENTITY,

        KEY_FUN,
        KEY_VAR,

        END_OF_FILE,
    };

    Kind kind;
    size_t line;
    size_t column;
    size_t position;
    size_t length;
} Token;

string token_name(Token::Kind kind)
{
    switch (kind)
    {
    case Token::Kind::INVALID:
        return "INVALID";

    case Token::Kind::EQUAL:
        return "EQUAL";
    case Token::Kind::DIVIDE:
        return "DIVIDE";
    case Token::Kind::BRACKET_L:
        return "BRACKET_L";
    case Token::Kind::BRACKET_R:
        return "BRACKET_R";
    case Token::Kind::CURLY_L:
        return "CURLY_L";
    case Token::Kind::CURLY_R:
        return "CURLY_R";

    case Token::Kind::NUM_LIT:
        return "NUM_LIT";
    case Token::Kind::IDENTITY:
        return "IDENTITY";

    case Token::Kind::KEY_FUN:
        return "KEY_FUN";
    case Token::Kind::KEY_VAR:
        return "KEY_VAR";

    case Token::Kind::END_OF_FILE:
        return "END_OF_FILE";

    default:
        return "name of token kind not specified in token_name(Token::Kind)";
    }
};

typedef struct
{
    bool loaded_successfully;
    string content;
    vector<Token> tokens;
    size_t length;
} SourceFile;

string get_str(SourceFile *source, Token token)
{
    return source->content.substr(token.position, token.length);
}

SourceFile load_source_file(string file_path)
{
    ifstream src_file;
    src_file.open(file_path, ios::in);

    if (!src_file)
    {
        return {false, "", {}, 0};
    }

    string content = string((istreambuf_iterator<char>(src_file)), istreambuf_iterator<char>());
    src_file.close();

    return {true, content, {}, content.length()};
}

// LEX TOKENS //

bool is_space(char c)
{
    return c == ' ' ||
           c == '\t' ||
           c == '\n';
}

bool is_alpha(char c)
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

bool is_digit(char c)
{
    return c >= '0' && c <= '9';
}

bool is_word(char c)
{
    return is_alpha(c) || is_digit(c) || c == '_';
}

void lex_source_file(SourceFile &source)
{
    size_t line = 1;
    size_t column = 1;
    size_t position = 0;

    size_t token_column = 1;
    size_t token_position = 0;

    bool chars_left = position < source.length;
    char character = chars_left ? source.content[0] : '\0';

    auto advance = [&]()
    {
        position += 1;
        column += 1;

        if (character == '\n')
        {
            line += 1;
            column = 1;
        }

        chars_left = position < source.length;
        character = chars_left ? source.content[position] : '\0';
    };

    auto emit = [&](Token::Kind kind)
    {
        size_t token_length = position - token_position;
        source.tokens.emplace_back(Token{kind, line, token_column, token_position, token_length});
    };

    while (chars_left)
    {
        // SKIP SPACE
        while (is_space(character) && chars_left)
            advance();

        if (!chars_left)
            break;

        // START NEW TOKEN
        token_column = column;
        token_position = position;

        // COMMENTS / DIVISION SYMBOL
        if (character == '/')
        {
            advance();

            if (character == '/') // Line comment
            {
                do
                    advance();
                while (character != '\n' && chars_left);
                advance();
            }

            // TODO: Nested multiline comments

            else // Division
            {
                emit(Token::Kind::DIVIDE);
            }
        }

        // SYMBOLS
        else if (character == '=')
        {
            advance();
            emit(Token::Kind::EQUAL);
        }

        else if (character == '(')
        {
            advance();
            emit(Token::Kind::BRACKET_L);
        }
        else if (character == ')')
        {
            advance();
            emit(Token::Kind::BRACKET_R);
        }

        else if (character == '{')
        {
            advance();
            emit(Token::Kind::CURLY_L);
        }
        else if (character == '}')
        {
            advance();
            emit(Token::Kind::CURLY_R);
        }

        // NUMBER LITERALS
        else if (is_digit(character))
        {
            while (is_digit(character))
                advance();

            if (character == '.')
            {
                advance();
                while (is_digit(character))
                    advance();
            }

            emit(Token::Kind::NUM_LIT);
        }

        // KEYWORDS AND IDENTITIES
        else if (is_word(character))
        {
            while (is_word(character))
                advance();

            string word = source.content.substr(token_position, position - token_position);

            if (word == "fun")
                emit(Token::Kind::KEY_FUN);
            else if (word == "var")
                emit(Token::Kind::KEY_VAR);
            else
                emit(Token::Kind::IDENTITY);
        }

        // ERROR
        else
        {
            // TODO: Implement better error handling
            cout << "Unexpected character '" << character << "' on line " << line << endl;
            advance();
        }
    }

    source.tokens.emplace_back(Token{Token::Kind::END_OF_FILE, line, column, position, 0});
}

// PROGRAM MODEL //

// Forward declarations
struct Scope;

// Expressions
typedef struct
{
    double value;
} Expression;

// Program structure
typedef struct
{
    string name;
    ptr<Scope> scope;
} Function;

typedef struct
{
    string name;
} Variable;

struct Scope
{
    ptr<Scope> parent;
    vector<ptr<Variable>> variables;
};

typedef struct
{
    vector<ptr<Function>> functions;
    vector<ptr<Expression>> expressions;
    vector<ptr<Variable>> variables;
    vector<ptr<Scope>> scopes;

    ptr<Scope> root;

    ptr<Expression> create_expression(double value)
    {
        auto expr = CREATE(Expression);
        expr->value = value;

        expressions.emplace_back(expr);
        return expr;
    }

    ptr<Function> create_function(string name, ptr<Scope> scope)
    {
        auto funct = CREATE(Function);
        funct->name = name;
        funct->scope = scope;

        functions.emplace_back(funct);
        return funct;
    }

    ptr<Variable> create_variable(ptr<Scope> scope, string name)
    {
        auto var = CREATE(Variable);
        var->name = name;

        variables.emplace_back(var);
        scope->variables.push_back(var);
        return var;
    }

    ptr<Scope> create_scope(ptr<Scope> parent)
    {
        auto scope = CREATE(Scope);
        scope->parent = parent;

        scopes.emplace_back(scope);
        return scope;
    }
} Program;

ptr<Program> create_program()
{
    ptr<Program> program = CREATE(Program);
    program->root = program->create_scope(nullptr);
    return program;
}

// JSON //

string to_json(string value)
{
    std::string json;
    for (char c : value)
    {
        switch (c)
        {
        case '\"':
            json += "\\\"";
            break;
        case '\n':
            json += "\\n";
            break;
        case '\\':
            json += "\\\\";
            break;
        default:
            json += c;
            break;
        }
    }
    return "\"" + json + "\"";
}

string to_json(int value)
{
    return to_string(value);
}

string to_json(double value)
{
    return to_string(value);
}

string to_json(bool value)
{
    return value ? "true" : "false";
}

template <typename T>
string to_json(const vector<T> &value)
{
    if (value.size() == 0)
        return "[]";

    string json = to_json(value.at(0));
    for (size_t i = 1; i < value.size(); i++)
        json += "," + to_json(value.at(i));

    return "[" + json + "]";
}

#define NODE_START()     \
    if (node == nullptr) \
        return "null";   \
    bool list = false;   \
    string json = "";

#define NODE_KEY(key) \
    if (list)         \
        json += ",";  \
    else              \
        list = true;  \
    json += to_json(string(#key)) + ": " + to_json(node->key);

#define NODE_FLUSH() return ("{" + json + "}")

string to_json(ptr<Expression> node);
string to_json(ptr<Function> node);
string to_json(ptr<Variable> node);
string to_json(ptr<Scope> node);

string to_json(ptr<Expression> node)
{
    return to_json(node->value);
}

string to_json(ptr<Function> node)
{
    NODE_START();
    NODE_KEY(name);
    NODE_KEY(scope);
    NODE_FLUSH();
}

string to_json(ptr<Variable> node)
{
    NODE_START();
    NODE_KEY(name);
    NODE_FLUSH();
}

string to_json(ptr<Scope> node)
{
    NODE_START();
    NODE_KEY(variables);
    NODE_FLUSH();
}

string to_json(ptr<Program> node)
{
    NODE_START();
    NODE_KEY(functions);
    NODE_KEY(root);
    NODE_FLUSH();
}

// PARSER //

class Parser
{
private:
    ptr<Program> program;

    SourceFile *source;
    size_t token_index;
    Token current_token;

    bool peek(Token::Kind kind)
    {
        return current_token.kind == kind;
    }

    void advance()
    {
        if (token_index < source->tokens.size())
        {
            token_index++;
            current_token = source->tokens.at(token_index);
        }
    }

    // TODO: I've created this function just to help get the compiler off the ground.
    //       In practice, the compiler should carry on parsing, even after a syntax error
    //       has occurred. This function should be gradually phased out, and replaced with
    //       'in-context' error handling. (i.e. if the expected token cannot be consumed,
    //       the caller should decide what to do next / what error to output.)
    Token consume_or_throw(Token::Kind kind)
    {
        if (current_token.kind != kind)
        {
            throw "Expected " + token_name(kind) + ", got " + token_name(current_token.kind);
        }

        Token token = current_token;
        advance();
        return token;
    }

public:
    Parser(ptr<Program> program) : program(program) {}

    ptr<Program> parse_source(SourceFile *src)
    {
        source = src;
        token_index = 0;
        current_token = source->tokens.at(0);

        while (!peek(Token::Kind::END_OF_FILE))
        {
            if (peek(Token::Kind::KEY_FUN))
            {
                parse_function(program->root);
            }
            else
            {
                cout << "Unexpected " << token_name(current_token.kind) << "token while parsing at " << current_token.line << ":" << current_token.column << " (expected function)" << endl;
                advance();
            }
        }

        return program;
    }

    ptr<Function> parse_function(ptr<Scope> scope)
    {
        consume_or_throw(Token::Kind::KEY_FUN);

        auto id = consume_or_throw(Token::Kind::IDENTITY);

        consume_or_throw(Token::Kind::BRACKET_L);
        // TODO: Parse parameters
        consume_or_throw(Token::Kind::BRACKET_R);

        auto funct_scope = program->create_scope(scope);
        auto funct = program->create_function(get_str(source, id), funct_scope);

        parse_block(funct_scope);

        return funct;
    }

    void parse_variable_declaration(ptr<Scope> scope)
    {
        consume_or_throw(Token::Kind::KEY_VAR);

        auto id = consume_or_throw(Token::Kind::IDENTITY);

        auto var = program->create_variable(scope, get_str(source, id));

        if (peek(Token::Kind::EQUAL))
        {
            consume_or_throw(Token::Kind::EQUAL);
            auto value = parse_expression(scope);
        }
    }

    ptr<Expression> parse_expression(ptr<Scope> scope)
    {
        auto value_token = consume_or_throw(Token::Kind::NUM_LIT);
        double value = stod(get_str(source, value_token));
        auto expr = program->create_expression(value);
        return expr;
    }

    void parse_block(ptr<Scope> scope)
    {
        consume_or_throw(Token::Kind::CURLY_L);

        // TODO: Parse statements correctly
        parse_variable_declaration(scope);

        consume_or_throw(Token::Kind::CURLY_R);
    }
};

// MAIN PROGRAM //

int main(int argc, char *argv[])
{
    // Validate parameters
    if (argc != 2)
    {
        cout << "Usage: bramble <source_path>" << endl;
        return 1;
    }

    // Load source file
    string source_path = (string)argv[1];
    SourceFile source = load_source_file(source_path);

    if (!source.loaded_successfully)
    {
        cout << "Could not load source file" << endl;
        return 1;
    }

    // Lex tokens
    lex_source_file(source);

    cout << "TOKENS" << endl;
    for (auto token : source.tokens)
    {
        cout << token_name(token.kind) << "\t"
             << token.line << ":" << token.column << "\t"
             << get_str(&source, token) << "\t"
             << endl;
    }
    cout << endl;

    // Parse
    auto program = create_program();
    Parser parser(program);
    parser.parse_source(&source);

    cout << "PARSER" << endl;
    cout << to_json(program);
    cout << endl;

    // TODO: Generate

    // Complete
    cout << "Compilation complete" << endl;
    return 0;
}