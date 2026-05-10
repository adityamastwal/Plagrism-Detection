#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <set>
#include <sstream>
#include <filesystem>
#include <cctype>
#include <chrono>
#include <cmath>

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
    bool lastWasSpace = true;

    for (int i = 0; i < (int)text.length(); i++) {
        unsigned char c = (unsigned char)text[i];
        if (isalpha(c) || isdigit(c)) {
            result += (char)tolower(c);
            lastWasSpace = false;
        } else {
            if (!lastWasSpace) {
                result += ' ';
                lastWasSpace = true;
            }
        }
    }

    if (!result.empty() && result.back() == ' ') {
        result.pop_back();
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


//Generate word shingles
set<string> generateWordShingles(vector<string>& words, int k) {
    set<string> shingles;

    if ((int)words.size() < k) {
        for (auto& w : words) shingles.insert(w);
        return shingles;
    }

    for (int i = 0; i <= (int)words.size() - k; i++) {
        string shingle = "";
        for (int j = 0; j < k; j++) {
            if (j > 0) shingle += " ";
            shingle += words[i + j];
        }
        shingles.insert(shingle);
    }

    return shingles;
}


// Generate character shingles
set<string> generateCharShingles(const string& text, int k) {
    set<string> shingles;

    if ((int)text.length() < k) {
        shingles.insert(text);
        return shingles;
    }

    for (int i = 0; i <= (int)text.length() - k; i++) {
        shingles.insert(text.substr(i, k));
    }

    return shingles;
}

int chooseK(int wordCount) {
    if (wordCount < 20)  return 1;
    if (wordCount < 50)  return 2;
    if (wordCount < 200) return 3;
    return 5;
}

int containmentSimilarity(set<string>& querySet, set<string>& sourceSet) {
    if (querySet.empty()) return 0;
    if (sourceSet.empty()) return 0;

    int intersectionSize = 0;
    for (const string& shingle : querySet) {
        if (sourceSet.count(shingle) > 0) {
            intersectionSize++;
        }
    }

    // Divide by QUERY size only — not union
    return (int)round(((double)intersectionSize / (double)querySet.size()) * 100.0);
}


//  Get all .txt files from dataset folder 
vector<string> getDatasetFiles(string folderPath) {
    vector<string> files;

    for (auto& entry : fs::directory_iterator(folderPath)) {
        string filename = entry.path().filename().string();
        if (entry.path().extension() == ".txt" && filename != "urls.txt") {
            files.push_back(entry.path().string());
        }
    }

    return files;
}

int main() {

    cout << "=== SHINGLING STARTED ===" << endl;

    string userText = cleanText(readFile("uploads/cleaned_text.txt"));
    if (userText == "") return 1;

    vector<string> userWords = splitWords(userText);
    cout << "User text words: " << userWords.size() << endl;

    int k = chooseK((int)userWords.size());
    cout << "Using k = " << k << " (word shingles)" << endl;

    set<string> userWordShingles = generateWordShingles(userWords, k);
    set<string> userCharShingles = generateCharShingles(userText, 9);
    cout << "User word shingles: " << userWordShingles.size() << endl;
    cout << "User char shingles: " << userCharShingles.size() << endl;

    vector<string> datasetFiles = getDatasetFiles("dataset");
    if (datasetFiles.empty()) {
        cout << "No dataset files found!" << endl;
        return 1;
    }

    cout << "Dataset files found: " << datasetFiles.size() << endl;

    int highestScore = 0;

    // Start timer
    auto startTime = high_resolution_clock::now();

    for (int i = 0; i < (int)datasetFiles.size(); i++) {
        string sourceText = cleanText(readFile(datasetFiles[i]));
        if (sourceText == "") continue;

        vector<string> sourceWords = splitWords(sourceText);

        // Use same k as query so shingle sizes match
        set<string> sourceWordShingles = generateWordShingles(sourceWords, k);
        set<string> sourceCharShingles = generateCharShingles(sourceText, 9);

        // Containment: what % of query shingles appear in source
        int wordScore = containmentSimilarity(userWordShingles, sourceWordShingles);
        int charScore = containmentSimilarity(userCharShingles, sourceCharShingles);

        // Take max of word-shingle and char-shingle scores
        int score = max(wordScore, charScore);

        cout << "vs " << datasetFiles[i]
             << " | word: " << wordScore << "%"
             << " | char: " << charScore << "%"
             << " | final: " << score << "%" << endl;

        if (score > highestScore) highestScore = score;
    }

    // Stop timer
    auto endTime  = high_resolution_clock::now();
    auto duration = duration_cast<milliseconds>(endTime - startTime).count();

    cout << "=== RESULT ===" << endl;
    cout << "Similarity Score : " << highestScore << "%" << endl;
    cout << "Execution Time : " << duration << "ms" << endl;

    if (highestScore < 20)       cout << "Verdict : LOW SIMILARITY" << endl;
    else if (highestScore < 50)  cout << "Verdict : MODERATE SIMILARITY" << endl;
    else                         cout << "Verdict : HIGH SIMILARITY" << endl;

    cout << "=== SHINGLING DONE ===" << endl;

    return 0;
}