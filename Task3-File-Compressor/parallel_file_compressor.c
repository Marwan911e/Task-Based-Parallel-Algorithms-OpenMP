/**
 * Parallel File Compressor - Task Pipeline with OpenMP
 * 
 * This program demonstrates a three-stage pipeline using OpenMP tasks:
 * 1. Read file chunks
 * 2. Compress chunks using Run-Length Encoding (RLE)
 * 3. Write compressed chunks to output file
 * 
 * Uses OpenMP task dependencies to create a producer-consumer pipeline.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <omp.h>

#define CHUNK_SIZE 1024  // Size of each chunk to process
#define MAX_CHUNKS 100   // Maximum number of chunks to process

// Structure to hold chunk data
typedef struct {
    char *data;          // Original data
    char *compressed;    // Compressed data
    int original_size;   // Size of original data
    int compressed_size; // Size of compressed data
    int chunk_id;        // Chunk identifier
    int valid;           // Whether this chunk contains valid data
} Chunk;

/**
 * Run-Length Encoding (RLE) Compression
 * 
 * Compresses data by encoding consecutive repeating characters as:
 * count + character
 * 
 * Example: "AAABBBCC" -> "3A3B2C"
 */
int compress_rle(const char *input, int input_size, char *output, int max_output_size) {
    if (input_size == 0) return 0;
    
    int out_pos = 0;
    int i = 0;
    
    while (i < input_size && out_pos < max_output_size - 10) {
        char current = input[i];
        int count = 1;
        
        // Count consecutive occurrences
        while (i + count < input_size && input[i + count] == current && count < 255) {
            count++;
        }
        
        // Improved compression logic
        if (count >= 3) {
            // Use RLE for runs >= 3 (more efficient)
            if (count <= 9 && current != '@') {
                // For small counts, use simple digit format: 3A
                output[out_pos++] = '0' + count;
                output[out_pos++] = current;
            } else {
                // For larger counts or @ character, use marker format: @<count><char>
                out_pos += snprintf(output + out_pos, max_output_size - out_pos, 
                                   "@%c%c", (char)count, current);
            }
        } else if (current == '@') {
            // Special handling for @ character
            out_pos += snprintf(output + out_pos, max_output_size - out_pos, 
                               "@%c%c", (char)count, current);
        } else {
            // For short runs (1-2 chars), just write the characters
            for (int j = 0; j < count && out_pos < max_output_size - 1; j++) {
                output[out_pos++] = current;
            }
        }
        
        i += count;
    }
    
    output[out_pos] = '\0';
    return out_pos;
}

/**
 * Task 1: Read file chunk
 */
void read_chunk(FILE *input_file, Chunk *chunk, int chunk_id) {
    chunk->chunk_id = chunk_id;
    chunk->data = (char *)malloc(CHUNK_SIZE);
    
    int bytes_read = fread(chunk->data, 1, CHUNK_SIZE, input_file);
    chunk->original_size = bytes_read;
    chunk->valid = (bytes_read > 0);
    
    if (chunk->valid) {
        printf("[READ] Chunk %d: Read %d bytes\n", chunk_id, bytes_read);
    }
}

/**
 * Task 2: Compress chunk using RLE
 */
void compress_chunk(Chunk *chunk) {
    if (!chunk->valid) return;
    
    // Allocate memory for compressed data (worst case: 3x original size)
    chunk->compressed = (char *)malloc(CHUNK_SIZE * 3);
    
    double start_time = omp_get_wtime();
    chunk->compressed_size = compress_rle(chunk->data, chunk->original_size, 
                                         chunk->compressed, CHUNK_SIZE * 3);
    double end_time = omp_get_wtime();
    
    double compression_ratio = (chunk->original_size > 0) ? 
        (100.0 * chunk->compressed_size / chunk->original_size) : 0.0;
    
    printf("[COMPRESS] Chunk %d: %d bytes -> %d bytes (%.1f%%, %.3f ms)\n",
           chunk->chunk_id, chunk->original_size, chunk->compressed_size,
           compression_ratio, (end_time - start_time) * 1000);
}

/**
 * Task 3: Write compressed chunk to output file
 */
void write_chunk(FILE *output_file, Chunk *chunk) {
    if (!chunk->valid) return;
    
    // Write chunk header (chunk_id, original_size, compressed_size)
    fprintf(output_file, "[CHUNK %d: %d -> %d]\n", 
            chunk->chunk_id, chunk->original_size, chunk->compressed_size);
    
    // Write compressed data
    fwrite(chunk->compressed, 1, chunk->compressed_size, output_file);
    fprintf(output_file, "\n");
    
    printf("[WRITE] Chunk %d: Written %d compressed bytes to output\n",
           chunk->chunk_id, chunk->compressed_size);
}

/**
 * Cleanup chunk memory
 */
void cleanup_chunk(Chunk *chunk) {
    if (chunk->data) {
        free(chunk->data);
        chunk->data = NULL;
    }
    if (chunk->compressed) {
        free(chunk->compressed);
        chunk->compressed = NULL;
    }
}

/**
 * Main compression pipeline using OpenMP tasks with dependencies
 */
void compress_file_pipeline(const char *input_filename, const char *output_filename) {
    FILE *input_file = fopen(input_filename, "rb");
    if (!input_file) {
        fprintf(stderr, "Error: Cannot open input file '%s'\n", input_filename);
        return;
    }
    
    FILE *output_file = fopen(output_filename, "wb");
    if (!output_file) {
        fprintf(stderr, "Error: Cannot open output file '%s'\n", output_filename);
        fclose(input_file);
        return;
    }
    
    printf("\n=== Parallel File Compressor Pipeline ===\n");
    printf("Input: %s\n", input_filename);
    printf("Output: %s\n", output_filename);
    printf("Chunk size: %d bytes\n", CHUNK_SIZE);
    printf("OpenMP threads: %d\n\n", omp_get_max_threads());
    
    double total_start = omp_get_wtime();
    
    // Allocate array of chunks
    Chunk *chunks = (Chunk *)calloc(MAX_CHUNKS, sizeof(Chunk));
    int total_chunks = 0;
    int total_original_bytes = 0;
    int total_compressed_bytes = 0;
    
    // OpenMP parallel region with task-based pipeline
    #pragma omp parallel
    {
        #pragma omp single
        {
            int chunk_id = 0;
            int done = 0;
            
            while (!done && chunk_id < MAX_CHUNKS) {
                Chunk *chunk = &chunks[chunk_id];
                
                // Task 1: Read chunk
                #pragma omp task depend(out: chunk[0]) firstprivate(chunk_id)
                {
                    read_chunk(input_file, chunk, chunk_id);
                    if (!chunk->valid) {
                        printf("[PIPELINE] No more data to read\n");
                    }
                }
                
                // Task 2: Compress chunk (depends on read)
                #pragma omp task depend(in: chunk[0]) depend(out: chunk[1]) firstprivate(chunk_id)
                {
                    if (chunk->valid) {
                        compress_chunk(chunk);
                    }
                }
                
                // Task 3: Write chunk (depends on compress)
                #pragma omp task depend(in: chunk[1]) firstprivate(chunk_id)
                {
                    if (chunk->valid) {
                        write_chunk(output_file, chunk);
                    }
                }
                
                // Check if we've read all data
                #pragma omp taskwait
                
                if (chunks[chunk_id].valid) {
                    total_original_bytes += chunks[chunk_id].original_size;
                    total_compressed_bytes += chunks[chunk_id].compressed_size;
                    total_chunks++;
                    chunk_id++;
                } else {
                    done = 1;
                }
            }
            
            // Wait for all tasks to complete
            #pragma omp taskwait
        }
    }
    
    double total_end = omp_get_wtime();
    
    // Cleanup
    for (int i = 0; i < total_chunks; i++) {
        cleanup_chunk(&chunks[i]);
    }
    free(chunks);
    
    fclose(input_file);
    fclose(output_file);
    
    // Print statistics
    printf("\n=== Compression Statistics ===\n");
    printf("Total chunks processed: %d\n", total_chunks);
    printf("Total original size: %d bytes\n", total_original_bytes);
    printf("Total compressed size: %d bytes\n", total_compressed_bytes);
    
    if (total_original_bytes > 0) {
        double compression_ratio = 100.0 * total_compressed_bytes / total_original_bytes;
        double space_saved = 100.0 * (total_original_bytes - total_compressed_bytes) / total_original_bytes;
        printf("Compression ratio: %.2f%%\n", compression_ratio);
        printf("Space saved: %.2f%%\n", space_saved);
    }
    
    printf("Total time: %.3f seconds\n", total_end - total_start);
    printf("\nOutput written to: %s\n", output_filename);
}

/**
 * Create a sample test file with compressible data
 * Modified to generate RLE-friendly data with long runs of identical characters
 */
void create_test_file(const char *filename, int size_kb) {
    FILE *file = fopen(filename, "wb");
    if (!file) {
        fprintf(stderr, "Error: Cannot create test file '%s'\n", filename);
        return;
    }
    
    printf("Creating test file '%s' (%d KB)...\n", filename, size_kb);
    
    int bytes_to_write = size_kb * 1024;
    int bytes_written = 0;
    
    // Create highly compressible data with LONG runs of identical characters
    // This demonstrates RLE compression effectively
    const char chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    int num_chars = strlen(chars);
    int char_index = 0;
    
    while (bytes_written < bytes_to_write) {
        char current_char = chars[char_index % num_chars];
        
        // Write long runs of 50-100 identical characters for good compression
        int run_length = 50 + (char_index * 7) % 51;  // 50-100 chars
        
        for (int i = 0; i < run_length && bytes_written < bytes_to_write; i++) {
            fputc(current_char, file);
            bytes_written++;
        }
        
        // Add occasional newlines for readability (every ~200 bytes)
        if (bytes_written % 200 < 5 && bytes_written < bytes_to_write) {
            fputc('\n', file);
            bytes_written++;
        }
        
        char_index++;
    }
    
    fclose(file);
    printf("Test file created successfully.\n\n");
}

int main(int argc, char *argv[]) {
    const char *input_file;
    const char *output_file = "compressed_output.txt";
    
    if (argc < 2) {
        printf("Usage: %s <input_file> [output_file]\n", argv[0]);
        printf("   or: %s --test [size_in_kb]\n\n", argv[0]);
        
        // Default: create and compress a test file
        printf("No input file specified. Creating test file...\n\n");
        input_file = "test_input.txt";
        create_test_file(input_file, 10);  // 10 KB test file
    } else if (strcmp(argv[1], "--test") == 0) {
        input_file = "test_input.txt";
        int size_kb = (argc > 2) ? atoi(argv[2]) : 10;
        if (size_kb < 1) size_kb = 10;
        create_test_file(input_file, size_kb);
    } else {
        input_file = argv[1];
        if (argc > 2) {
            output_file = argv[2];
        }
    }
    
    // Run the compression pipeline
    compress_file_pipeline(input_file, output_file);
    
    return 0;
}

