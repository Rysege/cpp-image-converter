#include "bmp_image.h"
#include "pack_defines.h"

#include <array>
#include <fstream>

using namespace std;

namespace img_lib {

static const uint16_t BMP_SIG = 0x4D42; // BM
static const uint32_t BMP_BIT = 24;     // количество бит на пиксель
static const uint32_t BMP_COMPR = 0;    // 0 — отсутствие сжатия

// функция вычисления отступа по ширине
// деление, а затем умножение на 4 округляет до числа, кратного четырём
// увеличение ширины на единицу гарантирует, что округление будет вверх
static int GetBMPStride(int width) {
    static const int multiple = 4;
    static const int bytes_per_pixel = 3;
    return multiple * ((++width * bytes_per_pixel) / multiple);
}

PACKED_STRUCT_BEGIN BitmapFileHeader{
    uint16_t bf_type = BMP_SIG; // отметка для отличия формата от других
    uint32_t bf_size{};         // суммарный размер заголовка и данных
    uint32_t bf_reserved = 0;   // зарезервированное пространство и должно содержать ноль
    uint32_t bf_off_bits = 54;  // размер двух частей заголовка

    BitmapFileHeader() = default;
    BitmapFileHeader(int width, int height) {
        bf_size = GetBMPStride(width) * height + bf_off_bits;
    }
}
PACKED_STRUCT_END

PACKED_STRUCT_BEGIN BitmapInfoHeader{
    uint32_t bi_size = 40;                  // размер второй части заголовка
    int32_t bi_width{};                     // ширина изображения в пикселях
    int32_t bi_height{};                    // высота изображения в пикселях
    uint16_t bi_planes = 1;                 // в BMP допустимо только значение 1
    uint16_t bi_bit_count = BMP_BIT;        // количество бит на пиксель
    uint32_t bi_compression = BMP_COMPR;    // тип сжатия
    uint32_t bi_size_image{};               // количество байт в данных
    uint32_t bi_x_pels_per_meter = 11811;   // разрешение изображения (300dpi по горизонтали и вертикали)
    uint32_t bi_y_pels_per_meter = 11811;
    uint32_t bi_clr_used = 0;               // размер таблицы цветов в ячейках.  0 — значение не определено
    uint32_t bi_clr_important = 0x1000000;  // количество ячеек от начала таблицы цветов до последней используемой (включая её саму)

    BitmapInfoHeader() = default;
    BitmapInfoHeader(int width, int height)
        : bi_width(width)
        , bi_height(height)
        , bi_size_image(GetBMPStride(width) * height) {
    }
}
PACKED_STRUCT_END

bool SaveBMP(const Path& file, const Image& image) {
    ofstream ofs(file, ios::binary);
    if (!ofs.is_open()) {
        return false;
    }

    BitmapFileHeader file_header(image.GetWidth(), image.GetHeight());
    BitmapInfoHeader info_header(image.GetWidth(), image.GetHeight());

    ofs.write(reinterpret_cast<char*>(&file_header), sizeof(file_header));
    ofs.write(reinterpret_cast<char*>(&info_header), sizeof(info_header));

    int stride = GetBMPStride(info_header.bi_width);
    std::vector<char> buffer(stride);

    for (int y = info_header.bi_height - 1; y >= 0; --y) {
        const Color* line = image.GetLine(y);
        for (int x = 0; x < info_header.bi_width; ++x) {
            buffer[x * 3 + 0] = static_cast<char>(line[x].b);
            buffer[x * 3 + 1] = static_cast<char>(line[x].g);
            buffer[x * 3 + 2] = static_cast<char>(line[x].r);
        }
        ofs.write(buffer.data(), stride);
    }
    return ofs.good();
}

Image LoadBMP(const Path& file) {
    ifstream ifs(file, ios::binary);
    if (!ifs.is_open()) {
        return {};
    }

    BitmapFileHeader file_header;
    BitmapInfoHeader info_header;

    ifs.read(reinterpret_cast<char*>(&file_header), sizeof(file_header));
    if (ifs.bad() || file_header.bf_type != BMP_SIG) {
        return {};
    }

    ifs.read(reinterpret_cast<char*>(&info_header), sizeof(info_header));
    if (ifs.bad() || info_header.bi_bit_count != BMP_BIT || info_header.bi_compression != BMP_COMPR) {
        return {};
    }

    int stride = GetBMPStride(info_header.bi_width);
    Image result(info_header.bi_width, info_header.bi_height, Color::Black());
    std::vector<char> buffer(stride);

    for (int y = info_header.bi_height - 1; y >= 0; --y) {
        Color* line = result.GetLine(y);
        ifs.read(buffer.data(), stride);
        for (int x = 0; x < info_header.bi_width; ++x) {
            line[x].b = static_cast<byte>(buffer[x * 3 + 0]);
            line[x].g = static_cast<byte>(buffer[x * 3 + 1]);
            line[x].r = static_cast<byte>(buffer[x * 3 + 2]);
        }
    }
    return result;
}

}  // namespace img_lib