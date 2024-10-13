#include "jpeg_image.h"
#include "ppm_image.h"
#include <jpeglib.h>

#include <array>
#include <fstream>
#include <setjmp.h>
#include <stdio.h>

using namespace std;

namespace img_lib {

// структура из примера LibJPEG
struct my_error_mgr {
    struct jpeg_error_mgr pub;
    jmp_buf setjmp_buffer;
};

typedef struct my_error_mgr* my_error_ptr;

// функция из примера LibJPEG
METHODDEF(void)
my_error_exit(j_common_ptr cinfo) {
    my_error_ptr myerr = (my_error_ptr)cinfo->err;
    (*cinfo->err->output_message) (cinfo);
    longjmp(myerr->setjmp_buffer, 1);
}

void SaveLineImageToBuffer(JSAMPLE* row, int y, const Image& image) {
    const Color* line = image.GetLine(y);
    for (int x = 0; x < image.GetWidth(); ++x) {
        JSAMPLE* pixel = row + x * 3;
        pixel[0] = static_cast<JSAMPLE>(line[x].r);
        pixel[1] = static_cast<JSAMPLE>(line[x].g);
        pixel[2] = static_cast<JSAMPLE>(line[x].b);
    }
}

bool SaveJPEG(const Path& file, const Image& image) {
    jpeg_compress_struct cinfo;
    jpeg_error_mgr jerr;

    FILE* outfile;       /* целевой файл */
    JSAMPROW row_pointer[1];  /* указатель на JSAMPLE row[s] */

    /* Шаг 1: выделите и инициализируйте объект сжатия JPEG */

    cinfo.err = jpeg_std_error(&jerr);
    /* Теперь мы можем инициализировать объект сжатия JPEG. */
    jpeg_create_compress(&cinfo);

    /* Шаг 2: укажите место назначения данных (например, файл) */

#ifdef _MSC_VER
    if ((outfile = _wfopen(file.wstring().c_str(), L"wb")) == NULL) {
#else
    if ((outfile = fopen(file.string().c_str(), "wb")) == NULL) {
#endif
        return false;
    }

    jpeg_stdio_dest(&cinfo, outfile);

    /* Шаг 3: задаем параметры сжатия */

    /* Сначала мы предоставляем описание входного изображения.
     * Необходимо заполнить четыре поля структуры cinfo:
     */    
    cinfo.image_width = image.GetWidth();  /* ширина и высота изображения в пикселях */
    cinfo.image_height = image.GetHeight();
    cinfo.input_components = 3;       /* # количество цветовых составляющих на пиксель */
    cinfo.in_color_space = JCS_RGB;   /* цветовое пространство входного изображения */

    jpeg_set_defaults(&cinfo);

    /* Шаг 4: Запустите компрессор */

    jpeg_start_compress(&cinfo, TRUE);

    /* Шаг 5: пока (остается записать строки сканирования) */
    /* строки jpeg_write_scan(...); */

    std::vector<JSAMPLE> image_buffer(image.GetWidth() * 3);
    row_pointer[0] = image_buffer.data();

    while (cinfo.next_scanline < cinfo.image_height) {
        SaveLineImageToBuffer(row_pointer[0], cinfo.next_scanline, image);
        (void)jpeg_write_scanlines(&cinfo, row_pointer, 1);
    }

    /* Шаг 6: Завершите сжатие */

    jpeg_finish_compress(&cinfo);
    /* После finish_compress мы можем закрыть выходной файл. */
    fclose(outfile);

    /* Шаг 7: освободите объект сжатия JPEG */

    /* Это важный шаг, поскольку он освободит большой объем памяти. */
    jpeg_destroy_compress(&cinfo);

    /* И мы закончили! */
    return true;
}

// тип JSAMPLE фактически псевдоним для unsigned char
void SaveSсanlineToImage(const JSAMPLE* row, int y, Image& out_image) {
    Color* line = out_image.GetLine(y);
    for (int x = 0; x < out_image.GetWidth(); ++x) {
        const JSAMPLE* pixel = row + x * 3;
        line[x] = Color{ byte{ pixel[0] }, byte{ pixel[1] }, byte{ pixel[2] }, byte{ 255 } };
    }
}

Image LoadJPEG(const Path& file) {
    jpeg_decompress_struct cinfo;
    my_error_mgr jerr;

    FILE* infile;
    JSAMPARRAY buffer;
    int row_stride;

    // Тут не избежать функции открытия файла из языка C,
    // поэтому приходится использовать конвертацию пути к string.
    // Под Visual Studio это может быть опасно, и нужно применить
    // нестандартную функцию _wfopen
#ifdef _MSC_VER
    if ((infile = _wfopen(file.wstring().c_str(), L"rb")) == NULL) {
#else
    if ((infile = fopen(file.string().c_str(), "rb")) == NULL) {
#endif
        return {};
    }

    /* Шаг 1: выделяем память и инициализируем объект декодирования JPEG */

    cinfo.err = jpeg_std_error(&jerr.pub);
    jerr.pub.error_exit = my_error_exit;

    if (setjmp(jerr.setjmp_buffer)) {
        jpeg_destroy_decompress(&cinfo);
        fclose(infile);
        return {};
    }

    jpeg_create_decompress(&cinfo);

    /* Шаг 2: устанавливаем источник данных */

    jpeg_stdio_src(&cinfo, infile);

    /* Шаг 3: читаем параметры изображения через jpeg_read_header() */

    (void)jpeg_read_header(&cinfo, TRUE);

    /* Шаг 4: устанавливаем параметры декодирования */

    // установим желаемый формат изображения
    cinfo.out_color_space = JCS_RGB;
    cinfo.output_components = 3;

    /* Шаг 5: начинаем декодирование */

    (void)jpeg_start_decompress(&cinfo);

    row_stride = cinfo.output_width * cinfo.output_components;

    buffer = (*cinfo.mem->alloc_sarray)
        ((j_common_ptr)&cinfo, JPOOL_IMAGE, row_stride, 1);

    /* Шаг 5a: выделим изображение ImgLib */
    Image result(cinfo.output_width, cinfo.output_height, Color::Black());

    /* Шаг 6: while (остаются строки изображения) */
    /*                     jpeg_read_scanlines(...); */

    while (cinfo.output_scanline < cinfo.output_height) {
        int y = cinfo.output_scanline;
        (void)jpeg_read_scanlines(&cinfo, buffer, 1);

        SaveSсanlineToImage(buffer[0], y, result);
    }

    /* Шаг 7: Останавливаем декодирование */

    (void)jpeg_finish_decompress(&cinfo);

    /* Шаг 8: Освобождаем объект декодирования */

    jpeg_destroy_decompress(&cinfo);
    fclose(infile);

    return result;
    }

} // of namespace img_lib