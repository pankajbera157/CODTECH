#include <iostream>
#include <fstream>
#include <vector>
#include <thread>
#include <mutex>
#include <chrono>

using namespace std;

mutex io_mutex;

// Run-Length Encoding (RLE) compression
string rleCompress(const string &input) {
    string output;
    int count = 1;
    for (size_t i = 1; i <= input.length(); ++i) {
        if (i < input.length() && input[i] == input[i - 1]) {
            count++;
        } else {
            output += input[i - 1];
            output += to_string(count);
            count = 1;
        }
    }
    return output;
}

// Decompression for RLE
string rleDecompress(const string &input) {
    string output;
    for (size_t i = 0; i < input.length(); ++i) {
        char ch = input[i];
        string countStr;
        while (isdigit(input[++i]) && i < input.length())
            countStr += input[i];
        i--; // step back from non-digit
        output.append(stoi(countStr), ch);
    }
    return output;
}

// Thread function for compression
void compressChunk(const string &chunk, string &result, int id) {
    string compressed = rleCompress(chunk);
    lock_guard<mutex> lock(io_mutex);
    result += compressed;
    cout << "Thread " << id << " completed compression.\n";
}

// Thread function for decompression
void decompressChunk(const string &chunk, string &result, int id) {
    string decompressed = rleDecompress(chunk);
    lock_guard<mutex> lock(io_mutex);
    result += decompressed;
    cout << "Thread " << id << " completed decompression.\n";
}

// Utility to split string into equal chunks
vector<string> splitChunks(const string &data, int parts) {
    vector<string> chunks;
    size_t len = data.length() / parts;
    for (int i = 0; i < parts; ++i) {
        size_t start = i * len;
        if (i == parts - 1) chunks.push_back(data.substr(start));
        else chunks.push_back(data.substr(start, len));
    }
    return chunks;
}

int main() {
    ifstream infile("input.txt");
    ofstream outfile("compressed.txt");
    ofstream outdecomp("decompressed.txt");
    
    if (!infile.is_open()) {
        cerr << "Failed to open input.txt\n";
        return 1;
    }

    string data((istreambuf_iterator<char>(infile)), istreambuf_iterator<char>());

    // === Single-threaded Compression ===
    auto t1 = chrono::high_resolution_clock::now();
    string single_compressed = rleCompress(data);
    auto t2 = chrono::high_resolution_clock::now();
    chrono::duration<double> single_time = t2 - t1;
    cout << "Single-threaded compression time: " << single_time.count() << " sec\n";

    // === Multi-threaded Compression ===
    const int THREADS = 4;
    vector<string> chunks = splitChunks(data, THREADS);
    vector<thread> threads;
    string multi_compressed;

    t1 = chrono::high_resolution_clock::now();
    for (int i = 0; i < THREADS; ++i) {
        threads.emplace_back(compressChunk, ref(chunks[i]), ref(multi_compressed), i + 1);
    }
    for (auto &t : threads) t.join();
    t2 = chrono::high_resolution_clock::now();
    chrono::duration<double> multi_time = t2 - t1;
    cout << "Multi-threaded compression time: " << multi_time.count() << " sec\n";

    outfile << multi_compressed;
    outfile.close();

    // === Decompression ===
    vector<string> compressedChunks = splitChunks(multi_compressed, THREADS);
    vector<thread> decompThreads;
    string decompressedData;

    t1 = chrono::high_resolution_clock::now();
    for (int i = 0; i < THREADS; ++i) {
        decompThreads.emplace_back(decompressChunk, ref(compressedChunks[i]), ref(decompressedData), i + 1);
    }
    for (auto &t : decompThreads) t.join();
    t2 = chrono::high_resolution_clock::now();
    cout << "Multi-threaded decompression time: " << chrono::duration<double>(t2 - t1).count() << " sec\n";

    outdecomp << decompressedData;
    outdecomp.close();

    return 0;
}
