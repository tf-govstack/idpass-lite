#include "idpass.h"
#include "protogen/card_access.pb.h"
#include "sodium.h"

#include <gtest/gtest.h>

#include <chrono>
#include <ctime>
#include <fstream>
#include <iostream>
#include <iterator>
#include <map>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

unsigned char knownHash[] = {
    0x93, 0x99, 0x99, 0xab, 0xbe, 0xd0, 0x74, 0x64,
    0xe8, 0x74, 0x11, 0xdb, 0x41, 0x29, 0xc7, 0xbe,
    0x67, 0x6e, 0xad, 0xd1, 0x9f, 0x23, 0xd6, 0xa0,
    0xfa, 0x59, 0x4f, 0x4d, 0xc8, 0x4e, 0x95, 0x54};

unsigned char encryptionKey[] = {
  0xf8, 0x0b, 0x95, 0x79, 0x69, 0xd1, 0xe8, 0x60, 0x6c, 0x33, 0x56, 0x00,
  0x76, 0x31, 0xe9, 0x2d, 0x14, 0x79, 0x9d, 0x65, 0x1f, 0x35, 0x0f, 0x89,
  0x87, 0x7c, 0x05, 0xa0, 0x3e, 0xc5, 0xaa, 0x3e
}; // 32

unsigned char signature_pk[] = {
  0x57, 0xc6, 0x33, 0x09, 0xfa, 0xc2, 0x1b, 0x60, 0x04, 0x76, 0x4e, 0xf6,
  0xf7, 0xc6, 0x2f, 0x28, 0xcf, 0x63, 0x40, 0xbe, 0x13, 0x10, 0x6e, 0x80,
  0xed, 0x70, 0x41, 0x8f, 0xa1, 0xb9, 0x27, 0xb4
}; // 32

unsigned char signature_sk[] = {
  0x2d, 0x52, 0xf8, 0x6a, 0xaa, 0x4d, 0x62, 0xfc, 
  0xab, 0x4d, 0xb0, 0x0a, 0x21, 0x1a, 0x12, 0x60, 
  0xf8, 0x17, 0xc5, 0xf2, 0xba, 0xb7, 0x3e, 0xfe,
  0xd6, 0x36, 0x07, 0xbc, 0x9d, 0xb3, 0x96, 0xee, 
  0x57, 0xc6, 0x33, 0x09, 0xfa, 0xc2, 0x1b, 0x60, 
  0x04, 0x76, 0x4e, 0xf6, 0xf7, 0xc6, 0x2f, 0x28,
  0xcf, 0x63, 0x40, 0xbe, 0x13, 0x10, 0x6e, 0x80, 
  0xed, 0x70, 0x41, 0x8f, 0xa1, 0xb9, 0x27, 0xb4
}; // 64

unsigned char verification_pk[] = {
  0x57, 0xc6, 0x33, 0x09, 0xfa, 0xc2, 0x1b, 0x60, 
  0x04, 0x76, 0x4e, 0xf6, 0xf7, 0xc6, 0x2f, 0x28,
  0xcf, 0x63, 0x40, 0xbe, 0x13, 0x10, 0x6e, 0x80, 
  0xed, 0x70, 0x41, 0x8f, 0xa1, 0xb9, 0x27, 0xb4
}; // 32

void savetobitmap(int qrcode_size, unsigned char* pixelbits,
    const char* outfile = "/tmp/qrcode.bmp")
{
    auto TestBit = [](unsigned char A[], int k) {
        return ((A[k / 8] & (1 << (k % 8))) != 0);
    };

    int width = qrcode_size;
    int height = qrcode_size;

    int size = width * height * 4;
    char header[54] = {0};
    strcpy(header, "BM");
    memset(&header[2], (int)(54 + size), 1);
    memset(&header[10], (int)54, 1); // always 54
    memset(&header[14], (int)40, 1); // always 40
    memset(&header[18], (int)width, 1);
    memset(&header[22], (int)height, 1);
    memset(&header[26], (short)1, 1);
    memset(&header[28], (short)32, 1); // 32bit
    memset(&header[34], (int)size, 1); // pixel size

    unsigned char* pixelbytes = new unsigned char[width * height * 4];
    std::memset(pixelbytes, 0x00, width * height * 4);
    int q = 0;

    for (uint8_t y = 0; y < width; y++) {
        // Each horizontal module
        for (uint8_t x = 0; x < height; x++) {
            int p = (y * height + x) * 4;

            if (TestBit(pixelbits, q++)) {
                pixelbytes[p + 0] = 255;
                pixelbytes[p + 1] = 0;
                pixelbytes[p + 2] = 0;
            } else {
                pixelbytes[p + 0] = 255;
                pixelbytes[p + 1] = 255;
                pixelbytes[p + 2] = 255;
            }
        }
    }

    FILE* fout = fopen(outfile, "wb");
    fwrite(header, 1, 54, fout);
    fwrite(pixelbytes, 1, size, fout);

#ifdef _FIXVALS
    unsigned char hash[crypto_generichash_BYTES];
    crypto_generichash(hash, sizeof hash, pixelbytes, size, NULL, 0);
    ASSERT_TRUE(0 == std::memcmp(hash, knownHash, crypto_generichash_BYTES));
#endif

    delete[] pixelbytes;
    fclose(fout);
}

class idpass_api_tests : public testing::Test
{
protected:
    time_t start_time_;
    int status;
    void* ctx;

    void SetUp() override
    {
        srand(time(0));
        start_time_ = time(nullptr);
        status = sodium_init();

        if (status < 0) {
            std::cout << "sodium_init failed";
        }

        ctx = idpass_api_init(
            encryptionKey, 
            crypto_aead_chacha20poly1305_IETF_KEYBYTES, 
            signature_sk, 
            crypto_sign_SECRETKEYBYTES,
            verification_pk,
            crypto_sign_PUBLICKEYBYTES);
    }

    void TearDown() override
    {
        // Gets the time when the test finishes
        const time_t end_time = time(nullptr);
    }
};

TEST_F(idpass_api_tests, check_qrcode_md5sum)
{
    std::ifstream f1("data/manny1.bmp", std::ios::binary);
    std::vector<char> img1(std::istreambuf_iterator<char>{f1}, {});

    int eSignedIDPassCard_len;
    unsigned char* eSignedIDPassCard = idpass_api_create_card_with_face(
		ctx,
        &eSignedIDPassCard_len,
        "Pacquiao",
        "Manny",
        "1978/12/17",
        "Kibawe, Bukidnon",
        "gender:male, sports:boxing, children:5, height:1.66m",
        img1.data(),
        img1.size(),
        "12345");

    if (eSignedIDPassCard != nullptr) {

        int qrsize = 0;
        unsigned char* pixel = idpass_api_qrpixel(
            ctx,
            eSignedIDPassCard,
            eSignedIDPassCard_len,
            &qrsize);

        FILE *fp = fopen("/tmp/qrcode.dat", "wb");
        int nwritten = 0;
        nwritten = fwrite(eSignedIDPassCard , 1, eSignedIDPassCard_len, fp);
        while (nwritten < eSignedIDPassCard_len) {
            nwritten += fwrite(eSignedIDPassCard , 1, eSignedIDPassCard_len + nwritten, fp);
        }
        fclose(fp);

        savetobitmap(qrsize, pixel);
    }
}

TEST_F(idpass_api_tests, createcard_manny_verify_as_brad)
{
    std::ifstream f1("data/manny1.bmp", std::ios::binary);
    std::vector<char> img1(std::istreambuf_iterator<char>{f1}, {});

    std::ifstream f2("data/brad.jpg", std::ios::binary);
    std::vector<char> img3(std::istreambuf_iterator<char>{f2}, {});

    int ecard_len;
    unsigned char* ecard = idpass_api_create_card_with_face(
        ctx,
        &ecard_len,
        "Pacquiao",
        "Manny",
        "1978/12/17",
        "Kibawe, Bukidnon",
        "gender:male, sports:boxing, children:5, height:1.66m",
        img1.data(),
        img1.size(),
        "12345");

    int details_len;
    unsigned char* details = idpass_api_verify_card_with_face(
        ctx, 
        &details_len, 
        ecard, 
        ecard_len, 
        img3.data(), 
        img3.size()
        );

    idpass::CardDetails cardDetails;
    cardDetails.ParseFromArray(details, details_len);
    std::cout << cardDetails.surname() << ", " << cardDetails.givenname() << std::endl;

    ASSERT_STRNE(cardDetails.surname().c_str(), "Pacquiao");
}

TEST_F(idpass_api_tests, cansign_and_verify_with_pin)
{
    std::ifstream f1("data/manny1.bmp", std::ios::binary);
    std::vector<char> img1(std::istreambuf_iterator<char>{f1}, {});

    int ecard_len;
    unsigned char* ecard = idpass_api_create_card_with_face(
        ctx,
        &ecard_len,
        "Pacquiao",
        "Manny",
        "1978/12/17",
        "Kibawe, Bukidnon",
        "gender:male, sports:boxing, children:5, height:1.66m",
        img1.data(),
        img1.size(),
        "12345");

    idpass::IDPassCards cards;
    cards.ParseFromArray(ecard, ecard_len);
	unsigned char* e_card = (unsigned char*)cards.encryptedcard().c_str();
	int e_card_len = cards.encryptedcard().size();

    const char* data = "this is a test message";

    int signature_len;
    unsigned char* signature = idpass_api_sign_with_card(
        ctx,
        &signature_len,
		e_card,
		e_card_len,
        (unsigned char*)data,
        std::strlen(data));

    ASSERT_TRUE(signature != nullptr);

    int card_len;
    unsigned char* card = idpass_api_verify_card_with_pin(
        ctx, 
        &card_len, 
		e_card,
		e_card_len,
        "12345");

    idpass::CardDetails cardDetails;
    cardDetails.ParseFromArray(card, card_len);
    std::cout << cardDetails.surname() << ", " << cardDetails.givenname() << std::endl;

    ASSERT_STREQ(cardDetails.surname().c_str(), "Pacquiao");
    ASSERT_STREQ(cardDetails.givenname().c_str(), "Manny");
}

TEST_F(idpass_api_tests, create_card_verify_with_face)
{
    std::ifstream f1("data/manny1.bmp", std::ios::binary);
    std::vector<char> img1(std::istreambuf_iterator<char>{f1}, {});

    std::ifstream f2("data/manny2.bmp", std::ios::binary);
    std::vector<char> img2(std::istreambuf_iterator<char>{f2}, {});

    std::ifstream f3("data/brad.jpg", std::ios::binary);
    std::vector<char> img3(std::istreambuf_iterator<char>{f3}, {});

    int ecard_len;
    unsigned char* ecard = idpass_api_create_card_with_face(
        ctx,
        &ecard_len,
        "Pacquiao",
        "Manny",
        "1978/12/17",
        "Kibawe, Bukidnon",
        "gender:male, sports:boxing, children:5, height:1.66m",
        img1.data(),
        img1.size(),
        "12345");

    idpass::IDPassCards cards;
    cards.ParseFromArray(ecard, ecard_len);
	unsigned char* e_card =(unsigned char*)cards.encryptedcard().c_str();
	int e_card_len = cards.encryptedcard().size();

    int details_len;
    unsigned char* details = idpass_api_verify_card_with_face(
        ctx, &details_len, 
		e_card,
		e_card_len,
		img3.data(), img3.size());

    ASSERT_TRUE(nullptr == details); // different person's face should not verify

    details = idpass_api_verify_card_with_face(
        ctx, &details_len, 
		e_card, 
		e_card_len, 
		img2.data(), img2.size());

    ASSERT_TRUE(nullptr != details); // same person's face should verify

    idpass::CardDetails cardDetails;
    cardDetails.ParseFromArray(details, details_len);
    std::cout << cardDetails.surname() << ", " << cardDetails.givenname() << std::endl;

	// Once verified, the details field should match
    ASSERT_STREQ(cardDetails.surname().c_str(), "Pacquiao");
    ASSERT_STREQ(cardDetails.givenname().c_str(), "Manny"); 
}

int main (int argc, char *argv[])
{
    ::testing::InitGoogleTest(&argc, argv); 
    return RUN_ALL_TESTS();
}