#include "ppm_image.h"

#include <array>
#include <fstream>
#include <string_view>

using namespace std;

namespace img_lib {

static const string_view PPM_SIG = "P6"sv;
static const int PPM_MAX = 255;

bool SavePPM(const Path& file, const Image& image) {
    ofstream ofs(file, ios::binary);
    if (!ofs.is_open()) {
        return false;
    }

    const int w = image.GetWidth();
    const int h = image.GetHeight();
    ofs << PPM_SIG << '\n' << w << ' ' << h << '\n' << PPM_MAX << std::endl;

    vector<char> buffer(w * 3);
    for (int y = 0; y < h; ++y) {
        const Color* line = image.GetLine(y);
        for (int x = 0; x < w; ++x) {
            buffer[x * 3 + 0] = static_cast<char>(line[x].r);
            buffer[x * 3 + 1] = static_cast<char>(line[x].g);
            buffer[x * 3 + 2] = static_cast<char>(line[x].b);
        }
        ofs.write(buffer.data(), w * 3);
    }
    return ofs.good();
}

Image LoadPPM(const Path& file) {
    // открываем поток с флагом ios::binary
    // поскольку будем читать даные в двоичном формате
    ifstream ifs(file, ios::binary);
    if (!ifs.is_open()) {
        return {};
    }

    string sign;
    int w, h, color_max;

    // читаем заголовок: он содержит формат, размеры изображения
    // и максимальное значение цвета
    ifs >> sign >> w >> h >> color_max;

    // мы поддерживаем изображения только формата P6
    // с максимальным значением цвета 255
    if (sign != PPM_SIG || color_max != PPM_MAX) {
        return {};
    }

    // пропускаем один байт - это конец строки
    if (const char next = ifs.get(); next != '\n') {
        return {};
    }

    Image result(w, h, Color::Black());
    std::vector<char> buffer(w * 3);

    for (int y = 0; y < h; ++y) {
        Color* line = result.GetLine(y);
        ifs.read(buffer.data(), w * 3);

        for (int x = 0; x < w; ++x) {
            line[x].r = static_cast<byte>(buffer[x * 3 + 0]);
            line[x].g = static_cast<byte>(buffer[x * 3 + 1]);
            line[x].b = static_cast<byte>(buffer[x * 3 + 2]);
        }
    }
    return result;
}

}  // namespace img_lib