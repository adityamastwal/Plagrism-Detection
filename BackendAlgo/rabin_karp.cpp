#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <sstream>
#include <filesystem>
#include <cctype>
#include <chrono>

using namespace std;
namespace fs = filesystem;
using namespace chrono;

const long long BASE = 31;
const long long MOD  = 1e9 + 9;

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

vector<string> splitWords(string text) {
    vector<string> words;
    stringstream ss(text);
    string word;

    while (ss >> word) words.push_back(word);

    return words;
}

long long computeHash(vector<string>& words, int start, int windowSize) {
    long long hash  = 0;
    long long power = 1;

    for (int i = start; i < start + windowSize; i++) {
        long long wordHash = 0;
        for (char c : words[i]) {
            wordHash = (wordHash * BASE + (c - 'a' + 1)) % MOD;
        }
        hash  = (hash + wordHash * power) % MOD;
        power = (power * BASE) % MOD;
    }

    return hash;
}

int rabinKarpSearch(vector<string>& text, vector<string>& pattern) {
    int n = text.size();
    int m = pattern.size();
    int matchCount = 0;

    if (m == 0 || n == 0 || m > n) return 0;

    long long patternHash = computeHash(pattern, 0, m);

    for (int i = 0; i <= n - m; i++) {
        long long windowHash = computeHash(text, i, m);

        if (windowHash == patternHash) {
            bool match = true;
            for (int k = 0; k < m; k++) {
                if (text[i + k] != pattern[k]) {
                    match = false;
                    break;
                }
            }
            if (match) matchCount++;
        }
    }

    return matchCount;
}

int calculateSimilarity(vector<string>& userWords, vector<string>& sourceWords, int windowSize) {
    int matchCount   = 0;
    int totalWindows = 0;

    for (int i = 0; i <= (int)userWords.size() - windowSize; i++) {
        totalWindows++;
        vector<string> pattern(userWords.begin() + i, userWords.begin() + i + windowSize);
        if (rabinKarpSearch(sourceWords, pattern) > 0) matchCount++;
    }

    return totalWindows == 0 ? 0 : (matchCount * 100) / totalWindows;
}


vector<string> getDatasetFiles(string folderPath) {
    vector<string> files;

    for (auto& entry : fs::directory_iterator(folderPath)) {
        if (entry.path().extension() == ".txt") {
            files.push_back(entry.path().string());
        }
    }

    return files;
}

int main() {

    cout << "=== RABIN-KARP MATCHING STARTED ===" << endl;

    string userText = cleanText(readFile("uploads/cleaned_text.txt"));
    if (userText == "") return 1;

    vector<string> userWords = splitWords(userText);
    cout << "User text words: " << userWords.size() << endl;

    vector<string> datasetFiles = getDatasetFiles("dataset");
    if (datasetFiles.empty()) {
        cout << "No dataset files found!" << endl;
        return 1;
    }

    cout << "Dataset files found: " << datasetFiles.size() << endl;

    int highestScore = 0;
    int windowSize   = 3;

    auto startTime = high_resolution_clock::now();

    for (int i = 0; i < (int)datasetFiles.size(); i++) {
        string sourceText = cleanText(readFile(datasetFiles[i]));
        if (sourceText == "") continue;

        vector<string> sourceWords = splitWords(sourceText);
        int score = calculateSimilarity(userWords, sourceWords, windowSize);

        cout << "vs " << datasetFiles[i] << " : " << score << "%" << endl;

        if (score > highestScore) highestScore = score;
    }

    auto endTime  = high_resolution_clock::now();
    auto duration = duration_cast<milliseconds>(endTime - startTime).count();

    cout << "=== RESULT ===" << endl;
    cout << "Similarity Score : " << highestScore << "%" << endl;
    cout << "Execution Time : " << duration << "ms" << endl;

    if (highestScore < 20)       cout << "Verdict : LOW SIMILARITY" << endl;
    else if (highestScore < 50)  cout << "Verdict : MODERATE SIMILARITY" << endl;
    else                         cout << "Verdict : HIGH SIMILARITY" << endl;

    cout << "=== RABIN-KARP MATCHING DONE ===" << endl;

    return 0;
}