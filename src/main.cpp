#include <exception>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <vector>

using namespace std;

// SOURCE FILES AND TOKENS  //

typedef struct
{
    enum class Kind
    {
        INVALID,

        DIVIDE,

        NUM_LIT,
        IDENTITY,

        END_OF_FILE,
    };

    Kind kind;
    size_t line;
    size_t column;
    size_t position;
    size_t length;

    // Token(Kind kind, size_t line, size_t column, size_t position, size_t length)
    //     : kind(kind),
    //       line(line),
    //       column(column),
    //       position(position),
    //       length(length) {}
} Token;

string token_name(Token::Kind kind)
{
    switch (kind)
    {
    case Token::Kind::INVALID:
        return "INVALID";

    case Token::Kind::DIVIDE:
        return "DIVIDE";

    case Token::Kind::NUM_LIT:
        return "NUM_LIT";
    case Token::Kind::IDENTITY:
        return "IDENTITY";

    case Token::Kind::END_OF_FILE:
        return "END_OF_FILE";

    default:
        return "name not specified in token_name(Token::Kind)";
    }
};

typedef struct
{
    bool loaded_successfully;
    string content;
    vector<Token> tokens;
    size_t length;
} SourceFile;

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

    for (auto token : source.tokens)
    {
        cout << token_name(token.kind) << "\t"
             << token.line << ":" << token.column << "\t"
             << source.content.substr(token.position, token.length) << "\t"
             << endl;
    }

    // TODO: Parse
    // TODO: Generate

    // Complete
    cout << "Compilation complete" << endl;
    return 0;
}