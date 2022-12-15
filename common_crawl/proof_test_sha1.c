#include <string.h>
#include <openssl/sha.h>
#include "unity.h"
#include "sha1_gpu.cuh"
#include "sha1_cpu.h"

void setUp(void) {}    /* Is run before every test, put unit init calls here. */
void tearDown(void) {} /* Is run after every test, put unit clean-up calls here. */

void test_hex_to_ushort_int() {
    // A struct to hold our test cases. Each
    // one has a char[4] and a ushort int.
    typedef struct {
        char hex[4];
        unsigned short int expected;
    } test_case_t;

    // An array of test cases.
    test_case_t test_cases[] = {
        {"0000", 0},
        {"1f92", 0x1f92},
        {"292e", 0x292e},
        {"f102", 0xf102},
        {"6ace", 0x6ace},
        {"56a9", 0x56a9},
        {"ca23", 0xca23},
        {"53d0", 0x53d0},
        {"3c86", 0x3c86},
        {"2f29", 0x2f29},
        {"ffff", 65535}
    };
    for (int i = 0; i < sizeof(test_cases) / sizeof(test_case_t); i++) {
        test_case_t test_case = test_cases[i];
        unsigned short int actual = hex_to_ushort_int(test_case.hex);
        TEST_ASSERT_EQUAL(test_case.expected, actual);
    }
}


/**
 * @brief Test that sha-1 hash on the cpu matches the gpu and also
 * the OpenSSL SHA1 hash for random data.
 *
 */
static void test_three_way_sha1()
{
    // We're going to generate 2^14 random samples.
    unsigned long int num_samples = (1 << 14);
    // Each sample will be 127 bytes. That is, they
    // are each of equal length.
    unsigned int sample_length = 127;

    // Array to hold random input
    BYTE *input;
    input = (BYTE *)malloc(sample_length * num_samples * sizeof(BYTE));

    // Array to hold hashes from CPU
    BYTE *cpu_hashes;
    cpu_hashes = (BYTE *)malloc(SHA1_BLOCK_SIZE * num_samples * sizeof(BYTE));
    // Array to hold hashes from GPU
    BYTE *gpu_hashes;
    gpu_hashes = (BYTE *)malloc(SHA1_BLOCK_SIZE * num_samples * sizeof(BYTE));
    // Array to hold hashes from OpenSSL
    BYTE *openssl_hashes;
    openssl_hashes = (BYTE *)malloc(SHA1_BLOCK_SIZE * num_samples * sizeof(BYTE));

    // Initialize the random number generator
    srand(0);

    // Populate the input with random bytes
    // printf("Populating random data");
    for (unsigned int i = 0; i < num_samples * sample_length; i++)
    {
        // Recall, a byte is 8 bits, or 2^8 = 256
        input[i] = rand() % 256;
    }

    // Hash each sample on the CPU. This will happen in serial.
    for (int i = 0; i < num_samples; i++)
    {
        SHA1_CTX ctx;
        // Notice there's some C pointer arithmetic going on here.
        // We're sliding a window over `input` and then storing the
        // resulting hashes in `cpu_hashes`. Each input is of equal
        // length and each output hash is of equal length (the
        // latter, obviously).
        sha1_hash(&ctx, input + sample_length * i, sample_length, cpu_hashes + SHA1_BLOCK_SIZE * i);
        // Compute the OpenSSL hash
        SHA1(input + sample_length * i, sample_length, openssl_hashes + SHA1_BLOCK_SIZE * i);
    }

    // Hash each sample on the GPUs. This will happen in parallel.
    mcm_cuda_sha1_hash_batch(input, sample_length, gpu_hashes, num_samples);

    // Compare the CPU hashes to the GPU hashes.
    TEST_ASSERT_EQUAL_MEMORY(cpu_hashes, gpu_hashes, SHA1_BLOCK_SIZE * num_samples);
    TEST_ASSERT_EQUAL_MEMORY(cpu_hashes, openssl_hashes, SHA1_BLOCK_SIZE * num_samples);

    // Just for grins, test each hash individually.
    for (size_t i = 0; i < num_samples; i++)
    {
        // Each byte is 8 bits. If we find a non-matching
        // piece of memory, output that 8 bits in hex, which
        // is two hex characters.
        TEST_ASSERT_EQUAL_HEX8_ARRAY(cpu_hashes + SHA1_BLOCK_SIZE * i, gpu_hashes + SHA1_BLOCK_SIZE * i, SHA1_BLOCK_SIZE);
        TEST_ASSERT_EQUAL_HEX8_ARRAY(cpu_hashes + SHA1_BLOCK_SIZE * i, openssl_hashes + SHA1_BLOCK_SIZE * i, SHA1_BLOCK_SIZE);
    }
    free(input);
    free(cpu_hashes);
    free(gpu_hashes);
    free(openssl_hashes);
}

/**
 * @brief Test that the GPU SHA-1 function correctly hashes "foo"
 *
 */
static void test_known_hash()
{
    BYTE input[] = "foo";
    BYTE output[SHA_DIGEST_LENGTH];
    unsigned char expected[] = "0beec7b5ea3f0fdbc95d0dd47f3c5bc275da8a33";
    unsigned char actual[2 * SHA_DIGEST_LENGTH + 1] = "";

    mcm_cuda_sha1_hash_batch(input, sizeof(input) - 1, output, 1);
    for (int i = 0; i < SHA_DIGEST_LENGTH; i++)
    {
        sprintf(actual + 2 * i, "%02x", output[i]);
    }
    // printf("Expected: %s", expected);
    // printf("Actual: %s", actual);
    TEST_ASSERT_EQUAL_CHAR_ARRAY(actual, expected, strlen(expected));
}

static void test_ip_to_int()
{
    TEST_ASSERT_EQUAL_UINT(0, ip_to_int("0.0.0.0"));
    TEST_ASSERT_EQUAL_UINT(1, ip_to_int("0.0.0.1"));
    TEST_ASSERT_EQUAL_UINT(256, ip_to_int("0.0.1.0"));
    TEST_ASSERT_EQUAL_UINT(4294967295, ip_to_int("255.255.255.255"));
    TEST_ASSERT_EQUAL_UINT(2935132330, ip_to_int("174.242.144.170"));
}

/**
 * @brief Test that the GPU SHA-1 function correctly hashes "foo"
 *
 */
static void test_ejmr_username_batch()
{
    uint16_t actual[2];

    typedef struct
    {
        uint32_t topic_id;
        uint32_t ip;
        // The expected username and the *next*
        // expected username for ip+1. 
        uint16_t expected[2];

    } test_case_t;

    test_case_t test_cases[] = {
        {54321U, ip_to_int("0.0.0.0"), {0x76db, 0x7ff9}},
        {1012283U, ip_to_int("174.242.144.170"), {0x4282, 0x4512}},
        {7188U, ip_to_int("64.224.255.104"), {0xe986, 0x4185}},
        {52U, ip_to_int("46.253.189.49"), {0x85be, 0x77b7}},
        {1011148U, ip_to_int("64.224.255.104"), {0xf873, 0xd901}}
    };

    for (size_t i = 0; i < sizeof(test_cases) / sizeof(test_case_t); i++)
    {
        cuda_ejmr_username_batch(test_cases[i].topic_id, test_cases[i].ip, &actual[0], 2);
        // printf("Expected: %04x, Actual: %04x (second %04x)\n", test_cases[i].expected, actual[0], actual[1]);
        TEST_ASSERT_EQUAL_UINT16_ARRAY(test_cases[i].expected, actual, 2);
    }
}

// A function that takes a pointer to an array of unsigned
// short ints and a parameter `n` and populates the array
// with random unsigned short ints after allocating enough
// memory.
void random_array(unsigned short int **array, size_t n)
{
    *array = malloc(n * sizeof(unsigned short int));
    for (size_t i = 0; i < n; i++)
    {
        (*array)[i] = rand();
    }
}

static void test_ejmr_username_from_sha1(){
    typedef struct {
        char sha1[2 * SHA_DIGEST_LENGTH + 1];
        char expected_username[4];
        uint16_t expected_username_numeric;
    } test_case_t;
    test_case_t test_cases[] = {
        {"dc7388afb76dbe7cec932cd2c4fa32f5be10ed46", "76db", 0x76db},
        {"3195b74744282cc037e611c5a5838ba28f12c1a3", "4282", 0x4282},
        {"3cea790f6e9860d38bad61a0a7af82bfdc7e299d", "e986", 0xe986},
        {"8380c998a85bebe76f857c825719f227889a6847", "85be", 0x85be},
        {"f8c54ebf2f8738ef98fe1242fa2d81d706becfb3", "f873", 0xf873}
    };
    for(size_t i = 0; i < sizeof(test_cases) / sizeof(test_case_t); i++){
        char *sha_ptr = &(test_cases[i].sha1[0]);

        // Test conversion to username in char format
        char actual_chars[4];
        ejmr_username_from_sha1(sha_ptr, &actual_chars[0]);
        // printf("Expected: %.4s, Actual: %.4s\n", test_cases[i].expected_username, actual_chars);
        TEST_ASSERT_EQUAL_CHAR_ARRAY(test_cases[i].expected_username, actual_chars, 4);

        // Test conversion to username in numeric format
        uint16_t actual_numeric;
        actual_numeric = sha1_to_ejmr_username_uint16(sha_ptr);
        // printf("Expected: %d, Actual: %d\n", test_cases[i].expected_username_numeric, actual_numeric);
        TEST_ASSERT_EQUAL_UINT16(test_cases[i].expected_username_numeric, actual_numeric);
    }
}

static void test_ushort_int_to_hex()
{
    typedef struct
    {
        unsigned short int input;
        char expected[4];
    } test_case_t;
    test_case_t test_cases[] = {
        {0, "0000"},
        {1, "0001"},
        {256, "0100"},
        {65535, "ffff"},
    };
    for (size_t i = 0; i < sizeof(test_cases) / sizeof(test_case_t); i++)
    {
        char actual[4];
        ushort_int_to_hex(test_cases[i].input, &actual[0]);
        TEST_ASSERT_EQUAL_CHAR_ARRAY(test_cases[i].expected, actual, 4);
    }
}

/**
 * @brief Test that our quicksort implementation works.
 *
 */
static void test_quicksort(){
    unsigned short int array_sizes[] = {1, 2, 3, 4, 200, 300};
    for (size_t i = 0; i < sizeof(array_sizes) / sizeof(unsigned short int); i++)
    {
        unsigned short int *array;
        random_array(&array, array_sizes[i]);
        quicksort(array, array_sizes[i]);
        for (size_t j = 0; j < array_sizes[i] - 1; j++)
        {
            TEST_ASSERT_TRUE(array[j] <= array[j + 1]);
        }
        free(array);
    }
}

static void test_find_uint16_in_sorted_array(){
    uint16_t array[] = {1, 20, 31, 42, 53, 64, 75, 80, 91, 1000};
    uint16_t array_size = sizeof(array) / sizeof(uint16_t);
    for (size_t i = 0; i < array_size; i++)
    {
        TEST_ASSERT_TRUE(find_uint16_in_sorted_array(array[i], array, array_size));
        TEST_ASSERT_FALSE(find_uint16_in_sorted_array(array[i]+1, array, array_size));
    }
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_three_way_sha1);
    // RUN_TEST(test_known_hash);
    // RUN_TEST(test_ejmr_username_batch);
    // RUN_TEST(test_ip_to_int);
    // RUN_TEST(test_quicksort);
    // RUN_TEST(test_hex_to_ushort_int);
    // RUN_TEST(test_ushort_int_to_hex);
    // RUN_TEST(test_ejmr_username_from_sha1);
    // RUN_TEST(test_find_uint16_in_sorted_array);
    return UNITY_END();
}