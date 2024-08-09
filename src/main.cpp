#include <exception>
#include <fstream>
#include <string>
#include <vector>
#include <iostream>

using namespace std;

// SOURCE FILES AND TOKENS  //

typedef struct
{
    // TODO: Implement
} Token;

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

    // TODO: Lex tokens
    // TODO: Parse
    // TODO: Generate

    // Complete
    cout << "Compilation complete" << endl;
    return 0;
}