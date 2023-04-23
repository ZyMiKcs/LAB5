#include <gtk/gtk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <math.h>

// Функция, которая возвращает интенсивность пикселя в позиции (x, y)
double get_intensity(guchar *pixels, int width, int height, int channels, int x, int y) {
    // Проверяем, находится ли пиксель в пределах изображения
    if (x < 0 || x >= width || y < 0 || y >= height) {
        return 0.0;
    }

    // Вычисляем позицию пикселя в массиве pixels
    int i = (y * width + x) * channels;

    // Вычисляем интенсивность пикселя
    // Используем взвешенную сумму красного, зеленого и синего каналов
    double intensity = 0.2989 * pixels[i] + 0.5870 * pixels[i + 1] + 0.1140 * pixels[i + 2];

    return intensity;
}

// Функция, которая применяет фильтр Собеля к изображению
void sobel_filter(guchar *pixels, int width, int height, int channels) {
    // Создаем новый буфер для результирующего изображения
    guchar *new_pixels = g_malloc(width * height * channels);

    // Применяем фильтр Собеля к каждому пикселю
    for (int x = 1; x < width - 1; x++) {
        for (int y = 1; y < height - 1; y++) {
            // Используем два ядра Собеля для нахождения градиента по горизонтали и вертикали
            double gx = -get_intensity(pixels, width, height, channels, x - 1, y - 1) + get_intensity(pixels, width, height, channels, x + 1, y - 1)
                        - 2 * get_intensity(pixels, width, height, channels, x - 1, y) + 2 * get_intensity(pixels, width, height, channels, x + 1, y)
                        - get_intensity(pixels, width, height, channels, x - 1, y + 1) + get_intensity(pixels, width, height, channels, x + 1, y + 1);

            double gy = -get_intensity(pixels, width, height, channels, x - 1, y - 1) - 2 * get_intensity(pixels, width, height, channels, x, y - 1)
                        - get_intensity(pixels, width, height, channels, x + 1, y - 1) + get_intensity(pixels, width, height, channels, x - 1, y + 1)
                        + 2 * get_intensity(pixels, width, height, channels, x, y + 1) + get_intensity(pixels, width, height, channels, x + 1, y + 1);

            // Вычисляем интенсивность пикселя после применения фильтра
            double intensity = sqrt(gx * gx + gy * gy);

            // Ограничиваем интенсивность до диапазона [0, 255]
            intensity = fmax(0.0, fmin(intensity, 255.0));

            // Записываем новую интенсивность пикселя в новый буфер
            int i = (y * width + x) * channels;
            new_pixels[i] = intensity;
            new_pixels[i + 1] = intensity;
            new_pixels[i + 2] = intensity;
        }
    }

    // Копируем новый буфер в исходный
    memmove(pixels, new_pixels, width * height * channels);

    // Освобождаем память, выделенную для нового буфера
    g_free(new_pixels);
}

int main(int argc, char *argv[]) {
    gtk_init(&argc, &argv);

    // Загружаем изображение из файла
    GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file("image.jpg", NULL);

    // Получаем информацию о пикселях изображения
    int width = gdk_pixbuf_get_width(pixbuf);
    int height = gdk_pixbuf_get_height(pixbuf);
    int channels = gdk_pixbuf_get_n_channels(pixbuf);
    guchar *pixels = gdk_pixbuf_get_pixels(pixbuf);

    // Применяем фильтр Собеля к изображению
    sobel_filter(pixels, width, height, channels);

    // Создаем новое изображение из измененных пикселей
    GdkPixbuf *new_pixbuf = gdk_pixbuf_new_from_data(pixels, GDK_COLORSPACE_RGB, FALSE, 8, width, height, channels * width, NULL, NULL);

    // Создаем виджет GtkImage и отображаем нашу новую картинку
    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_default_size(GTK_WINDOW(window), 1440, 960);

    GtkWidget *image = gtk_image_new_from_pixbuf(new_pixbuf);

    gtk_container_add(GTK_CONTAINER(window), image);

    gtk_widget_show_all(window);

    gtk_main();

    return 0;
}