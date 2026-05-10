#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cctype>

using namespace std;

string readFile(string filename) {
    ifstream file(filename);
    string content = "", line;

    if (!file.is_open()) {
        cout << "Error: Could not open file: " << filename << endl;
        return "";
    }

    while (getline(file, line)) {
        content += line + " ";
    }

    file.close();
    return content;
}

string removeBrackets(string text) {
    string result = "";
    bool inBracket = false;

    for (int i = 0; i < (int)text.length(); i++) {
        if (text[i] == '[') inBracket = true;
        else if (text[i] == ']') inBracket = false;
        else if (!inBracket) result += text[i];
    }

    return result;
}

string cleanText(string text) {
    string result = "";

    for (int i = 0; i < (int)text.length(); i++) {
        char c = tolower((unsigned char)text[i]);
        if (isalpha((unsigned char)c) || isdigit((unsigned char)c)) {
            result += c;
        } else {
            result += ' ';
        }
    }

    return result;
}

vector<string> tokenize(string text) {
    vector<string> words;
    string word = "";

    for (int i = 0; i < (int)text.length(); i++) {
        if (text[i] == ' ') {
            if (word != "") {
                words.push_back(word);
                word = "";
            }
        } else {
            word += text[i];
        }
    }

    if (word != "") words.push_back(word);

    return words;
}

void saveCleanText(vector<string> words, string filename) {
    ofstream file(filename);

    for (int i = 0; i < (int)words.size(); i++) {
        file << words[i];
        if (i != (int)words.size() - 1) file << " ";
    }

    file.close();
    cout << "Cleaned text saved to: " << filename << endl;
}

int main() {

    cout << "=== PREPROCESSOR STARTED ===" << endl;

    string rawText = readFile("uploads/user_text.txt");
    if (rawText == "") return 1;
    cout << "File read successfully" << endl;
    
    rawText = removeBrackets(rawText);

    string cleaned = cleanText(rawText);
    cout << "Text cleaned" << endl;

    vector<string> words = tokenize(cleaned);
    cout << "Tokenized into " << words.size() << " words" << endl;

    saveCleanText(words, "uploads/cleaned_text.txt");

    cout << "=== PREPROCESSING DONE ===" << endl;

    return 0;
}