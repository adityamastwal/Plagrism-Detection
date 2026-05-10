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

int lcsLength(vector<string>& userWords, vector<string>& sourceWords) {
    int n = min((int)userWords.size(),   500); // limit to 500 words
    int m = min((int)sourceWords.size(), 500); // limit to 500 words

    // dp table
    vector<vector<int>> dp(n + 1, vector<int>(m + 1, 0));

    for (int i = 1; i <= n; i++) {
        for (int j = 1; j <= m; j++) {
            if (userWords[i - 1] == sourceWords[j - 1]) {
                // words match — extend LCS
                dp[i][j] = dp[i - 1][j - 1] + 1;
            } else {
                // words don't match — take best of skipping one word
                dp[i][j] = max(dp[i - 1][j], dp[i][j - 1]);
            }
        }
    }

    return dp[n][m];
}


// Calculate similarity using LCS 
int calculateSimilarity(vector<string>& userWords, vector<string>& sourceWords) {
    int lcs = lcsLength(userWords, sourceWords);

    // similarity = LCS length / user text length * 100
    int userLen = min((int)userWords.size(), 500);

    if (userLen == 0) return 0;

    return (lcs * 100) / userLen;
}


// Get all .txt files from dataset folder ──
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

    cout << "=== LCS MATCHING STARTED ===" << endl;

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

    // Start timer 
    auto startTime = high_resolution_clock::now();

    for (int i = 0; i < (int)datasetFiles.size(); i++) {
        string sourceText = cleanText(readFile(datasetFiles[i]));
        if (sourceText == "") continue;

        vector<string> sourceWords = splitWords(sourceText);
        int score = calculateSimilarity(userWords, sourceWords);

        cout << "vs " << datasetFiles[i] << " : " << score << "%" << endl;

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

    cout << "=== LCS DONE ===" << endl;

    return 0;
}
