#include <iostream>
#include <fstream>
#include <vector>
#include <thread>
#include <chrono>
#include <zlib.h>
#include <sstream>
#include <iomanip>

// Configuration
const int CHUNK_SIZE = 16384; // Size of data chunks for processing
const int NUM_THREADS = 4;    // Number of threads to use

// Function to compress a chunk of data
int compress_chunk(const std::vector<unsigned char>& in_data, std::vector<unsigned char>& out_data, int level) {
    z_stream strm;
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    if (deflateInit(&strm, level) != Z_OK) {
        return -1; // Error initializing zlib
    }

    strm.avail_in = in_data.size();
    strm.next_in = const_cast<unsigned char*>(in_data.data()); // Cast away const for zlib (it doesn't modify)
    strm.avail_out = out_data.size();
    strm.next_out = out_data.data();

    int ret = deflate(&strm, Z_FINISH);
    if (ret != Z_STREAM_END) {
        deflateEnd(&strm);
        return -2; // Error during compression
    }

    out_data.resize(strm.total_out); // Resize output to actual compressed size
    deflateEnd(&strm);
    return 0;
}

// Function to decompress a chunk of data
int decompress_chunk(const std::vector<unsigned char>& in_data, std::vector<unsigned char>& out_data) {
    z_stream strm;
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    strm.avail_in = 0;
    strm.next_in = Z_NULL;
    if (inflateInit(&strm) != Z_OK) {
        return -1; // Error initializing zlib
    }

    strm.avail_in = in_data.size();
    strm.next_in = const_cast<unsigned char*>(in_data.data()); // Cast away const for zlib
    strm.avail_out = out_data.size();
    strm.next_out = out_data.data();

    int ret = inflate(&strm, Z_FINISH);
    if (ret != Z_STREAM_END) {
        inflateEnd(&strm);
        return -2; // Error during decompression
    }

    out_data.resize(strm.total_out); // Resize output to actual decompressed size
    inflateEnd(&strm);
    return 0;
}

// Function to compress a file using multiple threads
bool compress_file_threaded(const std::string& input_file, const std::string& output_file, int level) {
    std::ifstream infile(input_file, std::ios::binary);
    std::ofstream outfile(output_file, std::ios::binary);

    if (!infile.is_open() || !outfile.is_open()) {
        std::cerr << "Error opening files." << std::endl;
        return false;
    }

    // Determine file size
    infile.seekg(0, std::ios::end);
    size_t file_size = infile.tellg();
    infile.seekg(0, std::ios::beg);

    // Calculate number of chunks
    size_t num_chunks = (file_size + CHUNK_SIZE - 1) / CHUNK_SIZE;

    // Vectors to store input and output chunks
    std::vector<std::vector<unsigned char>> input_chunks(num_chunks);
    std::vector<std::vector<unsigned char>> output_chunks(num_chunks);

    // Read data into input chunks
    for (size_t i = 0; i < num_chunks; ++i) {
        input_chunks[i].resize(std::min((size_t)CHUNK_SIZE, file_size - i * CHUNK_SIZE));
        infile.read(reinterpret_cast<char*>(input_chunks[i].data()), input_chunks[i].size());
        output_chunks[i].resize(compressBound(input_chunks[i].size())); // Allocate maximum possible compressed size
    }

    // Thread management
    std::vector<std::thread> threads;
    size_t chunk_index = 0;

    auto compress_task = [&](size_t start_index, size_t end_index) {
        for (size_t i = start_index; i < end_index; ++i) {
            if (compress_chunk(input_chunks[i], output_chunks[i], level) != 0) {
                std::cerr << "Compression failed for chunk " << i << std::endl;
                return;
            }
        }
    };

    // Launch threads
    size_t chunks_per_thread = num_chunks / NUM_THREADS;
    for (int i = 0; i < NUM_THREADS; ++i) {
        size_t start_index = i * chunks_per_thread;
        size_t end_index = (i == NUM_THREADS - 1) ? num_chunks : (i + 1) * chunks_per_thread;
        threads.emplace_back(compress_task, start_index, end_index);
    }

    // Join threads
    for (auto& thread : threads) {
        thread.join();
    }

    // Write compressed data to output file
    for (const auto& chunk : output_chunks) {
        outfile.write(reinterpret_cast<const char*>(chunk.data()), chunk.size());
    }

    infile.close();
    outfile.close();
    return true;
}

// Function to decompress a file using multiple threads
bool decompress_file_threaded(const std::string& input_file, const std::string& output_file) {
    std::ifstream infile(input_file, std::ios::binary);
    std::ofstream outfile(output_file, std::ios::binary);

    if (!infile.is_open() || !outfile.is_open()) {
        std::cerr << "Error opening files." << std::endl;
        return false;
    }

    // Determine file size
    infile.seekg(0, std::ios::end);
    size_t file_size = infile.tellg();
    infile.seekg(0, std::ios::beg);

    // Calculate number of chunks
    size_t num_chunks = (file_size + CHUNK_SIZE - 1) / CHUNK_SIZE;

    // Vectors to store input and output chunks
    std::vector<std::vector<unsigned char>> input_chunks(num_chunks);
    std::vector<std::vector<unsigned char>> output_chunks(num_chunks);
    for (size_t i = 0; i < num_chunks; ++i) {
        input_chunks[i].resize(std::min((size_t)CHUNK_SIZE, file_size - i * CHUNK_SIZE));
        output_chunks[i].resize(CHUNK_SIZE * 2); // Initial guess for decompressed size (can be adjusted)
    }

    // Read data into input chunks
    for (size_t i = 0; i < num_chunks; ++i) {
        infile.read(reinterpret_cast<char*>(input_chunks[i].data()), input_chunks[i].size());
    }

    // Thread management
    std::vector<std::thread> threads;

    auto decompress_task = [&](size_t start_index, size_t end_index) {
        for (size_t i = start_index; i < end_index; ++i) {
            if (decompress_chunk(input_chunks[i], output_chunks[i]) != 0) {
                std::cerr << "Decompression failed for chunk " << i << std::endl;
                return;
            }
        }
    };

    // Launch threads
    size_t chunks_per_thread = num_chunks / NUM_THREADS;
    for (int i = 0; i < NUM_THREADS; ++i) {
        size_t start_index = i * chunks_per_thread;
        size_t end_index = (i == NUM_THREADS - 1) ? num_chunks : (i + 1) * chunks_per_thread;
        threads.emplace_back(decompress_task, start_index, end_index);
    }

    // Join threads
    for (auto& thread : threads) {
        thread.join();
    }

    // Write decompressed data to output file
    for (const auto& chunk : output_chunks) {
        outfile.write(reinterpret_cast<const char*>(chunk.data()), chunk.size());
    }

    infile.close();
    outfile.close();
    return true;
}

// Function to measure the execution time of a function
template <typename Func>
long long measure_execution_time(Func func) {
    auto start = std::chrono::high_resolution_clock::now();
    func();
    auto end = std::chrono::high_resolution_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
}

int main() {
    // Create a dummy file for testing
    std::string input_file = "test.txt";
    std::ofstream outfile(input_file);
    for (int i = 0; i < 1000000; ++i) {
        outfile << "This is a test line. " << i << std::endl;
    }
    outfile.close();

    std::string compressed_file = "test.txt.gz";
    std::string decompressed_file = "test_decompressed.txt";

    // Compression
    std::cout << "Starting compression..." << std::endl;
    long long compression_time = measure_execution_time([&]() {
        compress_file_threaded(input_file, compressed_file, Z_BEST_COMPRESSION);
    });
    std::cout << "Compression time: " << compression_time << " ms" << std::endl;

    // Decompression
    std::cout << "Starting decompression..." << std::endl;
    long long decompression_time = measure_execution_time([&]() {
        decompress_file_threaded(compressed_file, decompressed_file);
    });
    std::cout << "Decompression time: " << decompression_time << " ms" << std::endl;

    std::cout << "Done." << std::endl;

    return 0;
}
