#include <gtk/gtk.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

#define N_THREADS 8
#define KERNEL_SIZE 3

int image_width, image_height, image_channels, image_pitch;
unsigned char* image_data;
unsigned char* output_data;

// Initialize Sobel kernels
int kernelX[] = {-1, 0, 1, -2, 0, 2, -1, 0, 1};
int kernelY[] = {-1, -2 - 1, 0, 0, 0, 1, 2, 1};

void* sobel_filter(void* arg) {
    int thread_id = *(int*)arg;
    int start_row = thread_id * (image_height / N_THREADS);
    int end_row = (thread_id + 1) * (image_height / N_THREADS);
    if (thread_id == N_THREADS - 1) {
        end_row = image_height;
    }

    for (int y = start_row; y < end_row; y++) {
        for (int x = 0; x < image_width; x++) {
            int gradientX = 0;
            int gradientY = 0;
            for (int i = 0; i < KERNEL_SIZE; i++) {
                for (int j = 0; j < KERNEL_SIZE; j++) {
                    int pixelX = x + j - KERNEL_SIZE / 2;
                    int pixelY = y + i - KERNEL_SIZE / 2;
                    if (pixelX < 0 || pixelX >= image_width || pixelY < 0 || pixelY >= image_height) {
                        continue;
                    }
                    int kernel_index = i * KERNEL_SIZE + j;
                    gradientX += kernelX[kernel_index] * image_data[pixelY * image_pitch + pixelX * image_channels];
                    gradientY += kernelY[kernel_index] * image_data[pixelY * image_pitch + pixelX * image_channels];
                }
            }
            int gradient = abs(gradientX) + abs(gradientY);
            output_data[y * image_pitch + x * image_channels] = gradient;
            output_data[y * image_pitch + x * image_channels + 1] = gradient;
            output_data[y * image_pitch + x * image_channels + 2] = gradient;
        }
    }

    pthread_exit(NULL);
}

int main(int argc, char** argv) {
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
    output_data = (unsigned char*)malloc(image_pitch * image_height);

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
        pthread_create(&threads[i], NULL, sobel_filter, (void*)&thread_ids[i]);
    }
    for (int i = 0; i < N_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }

    gettimeofday(&end_time, NULL);

    // Print elapsed time
    long long elapsed_time =
        (end_time.tv_sec - start_time.tv_sec) * 1000000LL + (end_time.tv_usec - start_time.tv_usec);
    printf("Elapsed time: %lld us\n", elapsed_time);

    // Save output image
    GdkPixbuf* output_pixbuf = gdk_pixbuf_new_from_data(output_data, GDK_COLORSPACE_RGB, FALSE, 8,
                                                        image_width, image_height, image_pitch, NULL, NULL);
    gdk_pixbuf_save(output_pixbuf, "build/output.png", "png", NULL, NULL);

    // Clean up
    free(output_data);
    g_object_unref(pixbuf);
    g_object_unref(output_pixbuf);

    return 0;
}