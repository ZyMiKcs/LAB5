#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/time.h>
#include <gtk/gtk.h>

#define N_THREADS 16
#define KERNEL_SIZE 3

int image_width, image_height, image_channels, image_pitch;
unsigned char* image_data;
unsigned char* output_data;
int* kernel_x;
int* kernel_y;

void* sobel_filter(void* arg)
{
    int thread_id = *(int*) arg;
    int start_row = thread_id * (image_height / N_THREADS);
    int end_row = (thread_id + 1) * (image_height / N_THREADS);
    if (thread_id == N_THREADS - 1) {
        end_row = image_height;
    }

    for (int y = start_row; y < end_row; y++) {
        for (int x = 0; x < image_width; x++) {
            int gx = 0;
            int gy = 0;
            for (int i = 0; i < KERNEL_SIZE; i++) {
                for (int j = 0; j < KERNEL_SIZE; j++) {
                    int pixel_x = x + j - KERNEL_SIZE/2;
                    int pixel_y = y + i - KERNEL_SIZE/2;
                    if (pixel_x < 0 || pixel_x >= image_width || pixel_y < 0 || pixel_y >= image_height) {
                        continue;
                    }
                    int kernel_index = i * KERNEL_SIZE + j;
                    gx += kernel_x[kernel_index] * image_data[pixel_y*image_pitch + pixel_x*image_channels];
                    gy += kernel_y[kernel_index] * image_data[pixel_y*image_pitch + pixel_x*image_channels];
                }
            }
            int gradient = abs(gx) + abs(gy);
            output_data[y*image_pitch + x*image_channels] = gradient;
            output_data[y*image_pitch + x*image_channels + 1] = gradient;
            output_data[y*image_pitch + x*image_channels + 2] = gradient;
        }
    }

    pthread_exit(NULL);
}

int main(int argc, char** argv)
{
    if (argc < 2) {
        printf("Usage: %s <image file>\n", argv[0]);
        return -1;
    }

    // Load image data
    GdkPixbuf* pixbuf = gdk_pixbuf_new_from_file(argv[1], NULL);
    image_width = gdk_pixbuf_get_width(pixbuf);
    image_height = gdk_pixbuf_get_height(pixbuf);
    image_channels = gdk_pixbuf_get_n_channels(pixbuf);
    image_pitch = gdk_pixbuf_get_rowstride(pixbuf);
    image_data = gdk_pixbuf_get_pixels(pixbuf);
    output_data = (unsigned char*) malloc(image_pitch * image_height);

    // Initialize Sobel kernels
    kernel_x = (int*) malloc(KERNEL_SIZE * KERNEL_SIZE * sizeof(int));
    kernel_y = (int*) malloc(KERNEL_SIZE * KERNEL_SIZE * sizeof(int));
    kernel_x[0] = -1; kernel_x[1] = 0; kernel_x[2] = 1;
    kernel_x[3] = -2; kernel_x[4] = 0; kernel_x[5] = 2;
    kernel_x[6] = -1; kernel_x[7] = 0; kernel_x[8] = 1;
    kernel_y[0] = -1; kernel_y[1] = -2; kernel_y[2] = -1;
    kernel_y[3] = 0;  kernel_y[4] = 0;  kernel_y[5] = 0;
    kernel_y[6] = 1;  kernel_y[7] = 2;  kernel_y[8] = 1;

    // Initialize threads
    pthread_t threads[N_THREADS];
    int thread_ids[N_THREADS];
    for (int i = 0; i < N_THREADS; i++) {
        thread_ids[i] = i;
    }

    // Apply Sobel filter with multiple threads
    struct timeval start_time, end_time;
    gettimeofday(&start_time, NULL);
    for (int i = 0; i < N_THREADS; i++) {
        pthread_create(&threads[i], NULL, sobel_filter, (void*) &thread_ids[i]);
    }
    for (int i = 0; i < N_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }
    gettimeofday(&end_time, NULL);
    long long elapsed_time = (end_time.tv_sec - start_time.tv_sec) * 1000000LL + (end_time.tv_usec - start_time.tv_usec);
    printf("Elapsed time: %lld us\n", elapsed_time);

    // Save output image
    GdkPixbuf* output_pixbuf = gdk_pixbuf_new_from_data(output_data, GDK_COLORSPACE_RGB, FALSE, 8, image_width, image_height, image_pitch, NULL, NULL);
    gdk_pixbuf_save(output_pixbuf, "build/output.png", "png", NULL, NULL);

    // Clean up
    free(kernel_x);
    free(kernel_y);
    free(output_data);
    g_object_unref(pixbuf);
    g_object_unref(output_pixbuf);

    return 0;
}