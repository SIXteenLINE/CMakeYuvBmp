#include <iostream>
#include <fstream>
#include <vector>
#include <thread>
#include <cmath>
#include <cstring>
#include <string>
#include <memory>
#include <exception>

using namespace std;

class FileNotWork : public exception {
    string message;
public:
    FileNotWork (const string& msg) : message(msg){}
    const char* what() const noexcept override {
        return message.c_str();
    }
};

// Структура для хранения RGB пикселя
struct RGB {
    uint8_t r;
    uint8_t g;
    uint8_t b;
};

// Структура для хранения YUV пикселя
struct YUV {
    uint8_t y;
    uint8_t u;
    uint8_t v;
};

class BMPReader {
public:
    static bool read(const string& filename, vector<RGB>& image, int& width, int& height) {
        ifstream file(filename, ios::binary);
        if (!file.is_open()) {
            throw FileNotWork("Error opening BMP file");
        }

        uint16_t bfType;
        file.read(reinterpret_cast<char*>(&bfType), 2);
        if (bfType != 0x4D42) {
            throw FileNotWork("Unsupported BMP format");
        }

        file.seekg(18);
        file.read(reinterpret_cast<char*>(&width), 4);
        file.read(reinterpret_cast<char*>(&height), 4);

        uint16_t bfReserved;
        file.read(reinterpret_cast<char*>(&bfReserved), 2);
        file.read(reinterpret_cast<char*>(&bfReserved), 2);

        uint32_t bfOffBits;
        file.read(reinterpret_cast<char*>(&bfOffBits), 4);
        file.seekg(bfOffBits);

        int row_padded = (width * 3 + 3) & (~3);
        image.resize(width * height);
        for (int i = height - 1; i >= 0; --i) {
            for (int j = 0; j < width; ++j) {
                file.read(reinterpret_cast<char*>(&image[i].b), 1);
                file.read(reinterpret_cast<char*>(&image[i].g), 1);
                file.read(reinterpret_cast<char*>(&image[i].r), 1);
            }
            file.ignore(row_padded - width * 3);
        }
        file.close();
        return true;
    }
};


// для чтения YUV420 файла
class YUV420Reader{
public:
    static bool read(const string& filename, vector<uint8_t>& yuvData, int width, int height) {
        ifstream file(filename, ios::binary);
        if (!file.is_open()) {
            cerr << "Error opening YUV file" << endl;
            return false;
        }

        size_t frameSize = width * height * 3 / 2;
        yuvData.resize(frameSize);
        file.read(reinterpret_cast<char*>(yuvData.data()), frameSize);

        file.close();
        return true;
    }
};

class YUV420Writer {
public:
    static bool write(const string& filename, const vector<uint8_t>& yuvData) {
        ofstream file(filename, ios::binary);
        if (!file.is_open()) {
            throw FileNotWork("Error opening output YUV file");
        }

        file.write(reinterpret_cast<const char*>(yuvData.data()), yuvData.size());
        file.close();
        return true;
    }
};

// для преобразования RGB в YUV420
class convertRGBToYUV420 {
public:

static void RGBToYUV420(const vector<RGB>& rgbImage, vector<YUV>& yuvImage, int width, int height) {
    yuvImage.resize(width * height);

        auto rgbToYuv = [](RGB rgb) -> YUV {
            YUV yuv;
            yuv.y = static_cast<uint8_t>(0.299 * rgb.r + 0.587 * rgb.g + 0.114 * rgb.b);
            yuv.u = static_cast<uint8_t>(-0.169 * rgb.r - 0.331 * rgb.g + 0.5 * rgb.b + 128);
            yuv.v = static_cast<uint8_t>(0.5 * rgb.r - 0.419 * rgb.g - 0.081 * rgb.b + 128);
            return yuv;
        };

        int numThreads = thread::hardware_concurrency();
        vector<thread> threads(numThreads);
        int partHeight = height / numThreads;

        for (int t = 0; t < numThreads; ++t) {
            threads[t] = thread([&, t]() {
                int startY = t * partHeight;
                int endY = (t == numThreads - 1) ? height : startY + partHeight;
                for (int y = startY; y < endY; ++y) {
                    for (int x = 0; x < width; ++x) {
                        int index = y * width + x;
                        yuvImage[index] = rgbToYuv(rgbImage[index]);
                    }
                }
                });
        }

        for (auto& thread : threads) {
            thread.join();
        }
    }
};
// для наложения одного изображения на другое
class Overlay {
public:

    static void overlayImage(vector<uint8_t>& frame, const vector<YUV>& overlay, int width, int height, int overlayWidth, int overlayHeight, int posX, int posY) {
        for (int y = 0; y < overlayHeight; ++y) {
            for (int x = 0; x < overlayWidth; ++x) {
                int frameIndex = (posY + y) * width + (posX + x);
                int overlayIndex = y * overlayWidth + x;

                frame[frameIndex] = overlay[overlayIndex].y;

                if (y % 2 == 0 && x % 2 == 0) {
                    int chromaIndex = (posY + y) / 2 * (width / 2) + (posX + x) / 2;
                    frame[width * height + chromaIndex] = overlay[overlayIndex].u;
                    frame[width * height + width * height / 4 + chromaIndex] = overlay[overlayIndex].v;
                }
            }
        }
    }
};
/* Функция для записи YUV420 файла
bool writeYUV420(const string& filename, const vector<uint8_t>& yuvData) {
    ofstream file(filename, ios::binary);
    if (!file.is_open()) {
        cerr << "Error opening output YUV file" << endl;
        return false;
    }

    file.write(reinterpret_cast<const char*>(yuvData.data()), yuvData.size());

    file.close();
    return true;
}
*/
int main() {
    

    // Пути к файлам и размеры
    string inputYUVFile = "C:/Users/Зуфар/Desktop/Yo/C/CMakeYuvBmp/akiyo_qcif.yuv";
    string inputBMPFile = "C:/Users/Зуфар/Desktop/Yo/C/CMakeYuvBmp/picture.bmp";
    string outputYUVFile = "output.yuv";
    int width = 1920;
    int height = 1080;

    vector<RGB> bmpImage;
    int bmpWidth, bmpHeight;
    try {
        BMPReader::read(inputBMPFile, bmpImage, bmpWidth, bmpHeight);
    }
    catch (const FileNotWork& e) {
        cerr << e.what() << endl;
        return 1;
    }

    vector<uint8_t> yuvFrame;
    try {
        YUV420Reader::read(inputYUVFile, yuvFrame, width, height);
    }
    catch (const FileNotWork& e) {
        cerr << e.what() << endl;
        return 1;
    }

    vector<YUV> yuvImage;
    convertRGBToYUV420::RGBToYUV420(bmpImage, yuvImage, bmpWidth, bmpHeight);

    Overlay::overlayImage(yuvFrame, yuvImage, width, height, bmpWidth, bmpHeight, 0, 0);

    try {
        YUV420Writer::write(outputYUVFile, yuvFrame);
    }
    catch (const FileNotWork& e) {
        cerr << e.what() << endl;
        return 1;
    }

    cout << "Overlay completed successfully!" << endl;

    return 0;
}
