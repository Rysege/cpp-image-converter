#include <bmp_image.h>
#include <img_lib.h>
#include <jpeg_image.h>
#include <ppm_image.h>

#include <filesystem>
#include <iostream>
#include <string_view>

using namespace std;

class ImageFormatInterface {
public:
    virtual bool SaveImage(const img_lib::Path& file, const img_lib::Image& image) const = 0;
    virtual img_lib::Image LoadImage(const img_lib::Path& file) const = 0;
};

namespace FormatInterfaces {

class BMP : public ImageFormatInterface {
    bool SaveImage(const img_lib::Path& file, const img_lib::Image& image) const override {
        return img_lib::SaveBMP(file, image);
    }
    img_lib::Image LoadImage(const img_lib::Path& file) const override {
        return img_lib::LoadBMP(file);
    }
};

class PPM : public ImageFormatInterface {
    bool SaveImage(const img_lib::Path& file, const img_lib::Image& image) const override {
        return img_lib::SavePPM(file, image);
    }
    img_lib::Image LoadImage(const img_lib::Path& file) const override {
        return img_lib::LoadPPM(file);
    }
};

class JPEG : public ImageFormatInterface {
    bool SaveImage(const img_lib::Path& file, const img_lib::Image& image) const override {
        return img_lib::SaveJPEG(file, image);
    }
    img_lib::Image LoadImage(const img_lib::Path& file) const override {
        return img_lib::LoadJPEG(file);
    }
};

} // namespace FormatInterfaces

enum class Format {
    BMP,
    JPEG,
    PPM,
    UNKNOWN
};

Format GetFormatByExtension(const img_lib::Path& input_file) {
    const string ext = input_file.extension().string();
    if (ext == ".bmp"sv) {
        return Format::BMP;
    }
    if (ext == ".jpg"sv || ext == ".jpeg"sv) {
        return Format::JPEG;
    }
    if (ext == ".ppm"sv) {
        return Format::PPM;
    }
    return Format::UNKNOWN;
}

const ImageFormatInterface* GetFormatInterface(const img_lib::Path& path) {
    Format format = GetFormatByExtension(path);

    static const FormatInterfaces::BMP bmpInterface;
    static const FormatInterfaces::JPEG jpegInterface;
    static const FormatInterfaces::PPM ppmInterface;

    switch (format) {
    case Format::BMP:
        return &bmpInterface;
    case Format::JPEG:
        return &jpegInterface;
    case Format::PPM:
        return &ppmInterface;
    }
    return nullptr;
}

int main(int argc, const char** argv) {
    if (argc != 3) {
        cerr << "Usage: "sv << argv[0] << " <in_file> <out_file>"sv << endl;
        return 1;
    }

    img_lib::Path in_path = argv[1];
    img_lib::Path out_path = argv[2];

    const auto format_in = GetFormatInterface(in_path);
    if (!format_in) {
        cerr << "Unknown format of the input file" << endl;
        return 2;
    }

    const auto format_out = GetFormatInterface(out_path);
    if (!format_out) {
        cerr << "Unknown format of the output file" << endl;
        return 3;
    }

    img_lib::Image image = format_in->LoadImage(in_path);
    if (!image) {
        cerr << "Loading failed"sv << endl;
        return 4;
    }

    if (!format_out->SaveImage(out_path, image)) {
        cerr << "Saving failed"sv << endl;
        return 5;
    }

    cout << "Successfully converted"sv << endl;
}