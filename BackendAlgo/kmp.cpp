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

vector<int> buildLPS(vector<string>& pattern) {
    int m = pattern.size();
    vector<int> lps(m, 0);
    int length = 0;
    int i = 1;

    while (i < m) {
        if (pattern[i] == pattern[length]) {
            length++;
            lps[i] = length;
            i++;
        } else {
            if (length != 0) length = lps[length - 1];
            else { lps[i] = 0; i++; }
        }
    }

    return lps;
}

int kmpSearch(vector<string>& text, vector<string>& pattern) {
    int n = text.size();
    int m = pattern.size();
    int matchCount = 0;

    if (m == 0 || n == 0) return 0;

    vector<int> lps = buildLPS(pattern);
    int i = 0, j = 0;

    while (i < n) {
        if (text[i] == pattern[j]) { i++; j++; }

        if (j == m) {
            matchCount++;
            j = lps[j - 1];
        } else if (i < n && text[i] != pattern[j]) {
            if (j != 0) j = lps[j - 1];
            else i++;
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
        if (kmpSearch(sourceWords, pattern) > 0) matchCount++;
    }

    return totalWindows == 0 ? 0 : (matchCount * 100) / totalWindows;
}

vector<string> getDatasetFiles(string folderPath) {
    vector<string> files;

    for (auto& entry : fs::directory_iterator(folderPath)) {
        string filename = entry.path().filename().string();
        if (entry.path().extension() == ".txt") {
            files.push_back(entry.path().string());
        }
    }

    return files;
}

int main() {

    cout << "=== KMP MATCHING STARTED ===" << endl;

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

    cout << "=== KMP MATCHING DONE ===" << endl;

    return 0;
}