#include <iostream>
#include <fstream>
#include <string>

using namespace std;

void writeToFile(const string& filename) {
    ofstream outFile(filename); // write mode

    if (!outFile) {
        cerr << "Error opening file for writing!" << endl;
        return;
    }

    outFile << "This is the first line.\n";
    outFile << "This is the second line.\n";
    outFile.close();

    cout << "Data written to file successfully.\n";
}

void appendToFile(const string& filename) {
    ofstream outFile(filename, ios::app); // append mode

    if (!outFile) {
        cerr << "Error opening file for appending!" << endl;
        return;
    }

    outFile << "This is an appended line.\n";
    outFile.close();

    cout << "Data appended to file successfully.\n";
}

void readFromFile(const string& filename) {
    ifstream inFile(filename); // read mode

    if (!inFile) {
        cerr << "Error opening file for reading!" << endl;
        return;
    }

    string line;
    cout << "\nContents of the file:\n";
    while (getline(inFile, line)) {
        cout << line << endl;
    }

    inFile.close();
}

int main() {
    string filename = "sample.txt";

    writeToFile(filename);
    appendToFile(filename);
    readFromFile(filename);

    return 0;
}
