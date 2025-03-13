#include <cstdlib>
#include <mandelbrot.h>
#include <window.h>
#include <thread>
#include <iostream>

// MAIN PIPELINE
// the window context is responsible for dispatching image data to the wayland server.
// it must reside in the main thread
// the buffers sent to the server must be distinct at each frame
// the drawing code mustn't see this: it perceives a single buffer shared with the window context
// to which he draws.
// window resizing is allowed, however the canvas stays of fixed size.
// resizes are notified to the drawer and it is up to him to redraw all when he wants onto a bigger canvas
// hence: on each frame a copy of the current canvas onto the backbuffer is performed.
// the drawer runs without perception of frames, and a mutex allows locking the drawer when copying the canvas
// when the drawer finishes drawing the drawing threads join,
// and whenever a new input is received the drawing context is updated and threads are respawned

    // BONUS: handle inputs to:
        // render current image
        // zoom in on a certain point
        // go back to initial condition
        // move left right up and down
    // BONUS: add a ui that informs when the drawing is finished and we can input
    // BONUS: add ui for changing fractal from a list

Color HSVtoRGB(Vec3f c) {
    int i = floor(c.x * 6);
    float f = c.x * 6 - i;
    float p = c.z * (1 - c.y);
    float q = c.z * (1 - f * c.y);
    float t = c.z * (1 - (1 - f) * c.y);

    Color result;
    switch(i % 6){
        case 0: result.r = c.z, result.g = t,   result.b = p; break;
        case 1: result.r = q,   result.g = c.z, result.b = p; break;
        case 2: result.r = p,   result.g = c.z, result.b = t; break;
        case 3: result.r = p,   result.g = q,   result.b = c.z; break;
        case 4: result.r = t,   result.g = p,   result.b = c.z; break;
        case 5: result.r = c.z, result.g = p,   result.b = q; break;
    }

    return result;
}

static constexpr usize palette_size = 200;
static Color palette[palette_size];
static void generatePalette() {
    float theta = 0.6f; // warm colors
    float value = 0.2f;
    float saturation = 1.0f;
    int increase = 0;
    //float theta = 0.1f; // green style
    for (usize i = 0; i < palette_size; ++i) {
        switch (increase) {
            case 0: 
                value += 0.1;
                break;
            case 1: 
                saturation -= 0.1;
                break;
            case 2: 
                theta += 0.1;
                value = 0.2f;
                increase = 0;
                break;
            default: exit(EXIT_FAILURE);
        }

        if (saturation <= 0.5f) {
            saturation = 1.0f;
            increase = 2;
        } else if (theta >= 1.0f) {
            theta = 0.0f;
        } else if (value >= 1.0f) {
            value = 1.0f;
            increase = 1;
        }


        palette[i] = HSVtoRGB({theta, saturation, value});
    }
}

static void generatePaletteMonochrome(float hue) {
    float step = 2.0f / (float)palette_size;
    float value = 0.2f; 
    float saturation = 1.0f;
    bool increase_value = true;
    //float value = 0.1f; // green style
    for (usize i = 0; i < palette_size; ++i) {
        palette[i] = HSVtoRGB({hue, saturation, value});
        if (increase_value) {
            value += step;
        } else {
            saturation -= step;
        }
        if (saturation <= 0.0) saturation = 0.0f;
        if (value > 1.0f) { 
            value = 1.0f;
            increase_value = false;
        }
    }
}

Color getPaletteColor(u32 i) { return palette[i % palette_size]; }


void fillBuffer(Buffer *buf, Color color) {
    for (i32 i = 0; i < buf->width * buf->height; ++i) {
        buf->data[i] = getColorHex(color);
    }
}

void zoomBufferInterpolate(Buffer *b, i32 focus_x, i32 focus_y, f32 zoom) {
    i32 new_width = (f32)b->width * zoom;
    i32 new_height = (f32)b->height * zoom;
    Buffer *work = initBuffer(new_width, new_height);
    for (i32 y = 0; y < new_height; ++y) {
        for (i32 x = 0; x < new_width; ++x) {
            f32 original_x = (f32)x / zoom;
            f32 original_y = (f32)y / zoom;
            i32 x1 = original_x;
            i32 y1 = original_y;
            i32 x2 = min(x1 + 1, b->width - 1);
            i32 y2 = min(y1 + 1, b->height - 1);
            f32 x_frac = original_x - x1;
            f32 y_frac = original_y - y1;
            u32 r11 = (b->data[y1 * b->width + x1] & 0xff0000) >> 16;
            u32 r12 = (b->data[y2 * b->width + x1] & 0xff0000) >> 16;
            u32 r21 = (b->data[y1 * b->width + x2] & 0xff0000) >> 16;
            u32 r22 = (b->data[y2 * b->width + x2] & 0xff0000) >> 16;
            u32 g11 = (b->data[y1 * b->width + x1] & 0x00ff00) >>  8;
            u32 g12 = (b->data[y2 * b->width + x1] & 0x00ff00) >>  8;
            u32 g21 = (b->data[y1 * b->width + x2] & 0x00ff00) >>  8;
            u32 g22 = (b->data[y2 * b->width + x2] & 0x00ff00) >>  8;
            u32 b11 = (b->data[y1 * b->width + x1] & 0x0000ff) >>  0;
            u32 b12 = (b->data[y2 * b->width + x1] & 0x0000ff) >>  0;
            u32 b21 = (b->data[y1 * b->width + x2] & 0x0000ff) >>  0; 
            u32 b22 = (b->data[y2 * b->width + x2] & 0x0000ff) >>  0;
            f32 rtop = (1 - x_frac) * r11 + x_frac * r21;
            f32 gtop = (1 - x_frac) * g11 + x_frac * g21;
            f32 btop = (1 - x_frac) * b11 + x_frac * b21;
            f32 rbottom = (1 - x_frac) * r12 + x_frac * r22;
            f32 gbottom = (1 - x_frac) * g12 + x_frac * g22;
            f32 bbottom = (1 - x_frac) * b12 + x_frac * b22;

            u32 r = (1 - y_frac) * rtop + y_frac * rbottom;
            u32 g = (1 - y_frac) * gtop + y_frac * gbottom;
            u32 b = (1 - y_frac) * btop + y_frac * bbottom;

            u32 hex = (0xffu << 24) | (r << 16) | (g << 8) | (b << 0);
            work->data[y * new_width + x] = hex;
        }
    }

     i32 new_focus_x = (f64)focus_x * zoom;
     i32 new_focus_y = (f64)focus_y * zoom;

    for (i32 y = 0; y < b->height; ++y) {
        for (i32 x = 0; x < b->height; ++x) {
            u32 hex = work->data[(y + new_focus_y - focus_y) * new_width + x + (new_focus_x - focus_x)];
            b->data[y * b->width + x] = hex;
        }
    }
}

void zoomCropBuffer(Buffer *dst, Buffer *src, i32 focus_x, i32 focus_y, f32 zoom) {
    i32 new_width = (f32)dst->width * zoom;
    i32 new_height = (f32)dst->height * zoom;

     i32 new_focus_x = (f64)focus_x * zoom;
     i32 new_focus_y = (f64)focus_y * zoom;

    for (i32 y = 0; y < dst->height; ++y) {
        for (i32 x = 0; x < dst->height; ++x) {
            u32 hex = src->data[(y + new_focus_y - focus_y) * new_width + x + (new_focus_x - focus_x)];
            dst->data[y * dst->width + x] = hex;
        }
    }
}

void blurBufferGaussian(Buffer *buf, u8 kernel_size, f32 sigma) {
    std::vector<std::vector<f32>> kernel(kernel_size, std::vector<f32>(kernel_size));
    u8 half_kernel_size = kernel_size / 2;

    f32 kernel_sum = 0.0f;
    f32 variance_scaled = 2.0 * sigma * sigma;

    for (i32 x = -half_kernel_size; x <= half_kernel_size; ++x) {

        for (i32 y = -half_kernel_size; y <= half_kernel_size; ++y) {
            f32 r = sqrt(x * x + y * y);
            kernel[x + half_kernel_size][y + half_kernel_size] = exp(-(r * r) / variance_scaled) / (M_PI * variance_scaled);
            kernel_sum += kernel[x + half_kernel_size][y + half_kernel_size];
        }
    }

    for (i32 i = 0; i < kernel_size; ++i) {
        for (i32 j = 0; j < kernel_size; ++j) {
            kernel[i][j] /= kernel_sum;
        }
    }

     for (i32 y = half_kernel_size; y < buf->height - half_kernel_size; ++y) {
        for (i32 x = half_kernel_size; x < buf->width - half_kernel_size; ++x) {
            f32 sum_red = 0.0, sum_green = 0.0, sum_blue = 0.0;
            // Alpha can remain the same, no blur on alpha
            for (i32 ky = -half_kernel_size; ky <= half_kernel_size; ++ky) {
                for (i32 kx = -half_kernel_size; kx <= half_kernel_size; ++kx) {
                    i32 pixel = buf->data[(y + ky) * buf->width + x + kx];
                    f32 weight = kernel[ky + half_kernel_size][kx + half_kernel_size];
                    // Add weighted values for each channel
                    sum_red   += ((f32)((pixel & 0xff0000) >> 16)) * weight;
                    sum_green += ((f32)((pixel & 0x00ff00) >> 8) ) * weight;
                    sum_blue  += ((f32)((pixel & 0x0000ff) >> 0) ) * weight;
                }
            }
            // Clamp values to 0-255 and combine i32o output pixel
            u32 blurred_red   = (u32)(min(max(sum_red, 0.0f), 255.0f));
            u32 blurred_green = (u32)(min(max(sum_green, 0.0f), 255.0f));
            u32 blurred_blue  = (u32)(min(max(sum_blue, 0.0f), 255.0f));
            buf->data[y * buf->width + x] = (
                ((0xffu) << 24) |
                ((u32)(blurred_red) << 16) | 
                ((u32)(blurred_green) << 8) | 
                ((u32)(blurred_blue) << 0) 
            );
        }
    }
}

bool writeBitmap(const char *filename, const Buffer buf) {
    struct BMPHeader {
        struct __attribute__((packed)) {
            u16 magic;               // The header field used to identify the BMP and DIB file is 0x42 0x4D
            u32 size;                // The size of the BMP file in bytes 
            u16 reserved0;           // Reserved, if created manually can be 0
            u16 reserved1;           // Reserved, if created manually can be 0 
            u32 offset;              // starting address, of the byte where the pixel array can be found. 
        } header;

        struct __attribute__((packed)) {
            u32 size;                // 4  the size of this header, in bytes (40) 
            u32 width;               //   bitmap width in pixels
            u32 height;              //   bitmap height in pixels
            u16 n_planes;            //   number of color planes, must be 1
            u16 bpp;                 //   number of bits per pixel, which is the color depth of the image
            u32 compression;         //   the compression method being used (0 for no compression)
            u32 original_size;       //   the image size before compression, if compression == 0, set to 0
            u32 horizontal_res;      //   the horizontal resolution of the image. (pixel per metre, signed integer)
            u32 vertical_res;        //   the vertical resolution of the image. (pixel per metre, signed integer)
            u32 n_palette_colors;    //   the number of colors in the color palette, or 0 to default to 2n
            u32 n_important_colors;  //   the number of important colors used, or 0 when every color is important
        } bitmapinfoheader;
    };

    FILE *fptr = fopen(filename, "wb");
    if (!fptr) return false;
    size_t data_size = getBufferSize(&buf);

    BMPHeader h = {};
    h.header.magic = 0x4D42;
    h.header.size = sizeof(h) + data_size;
    h.header.offset = sizeof(h);
    h.bitmapinfoheader.size = 40;
    h.bitmapinfoheader.width = buf.width;
    h.bitmapinfoheader.height = buf.height;
    h.bitmapinfoheader.n_planes = 1;
    h.bitmapinfoheader.bpp = sizeof(u32) * 8;
    h.bitmapinfoheader.horizontal_res = 500;
    h.bitmapinfoheader.vertical_res = 500;

    fwrite(&h, sizeof(h), 1, fptr);
    fwrite(buf.data, data_size, 1, fptr);
    fclose(fptr);
    return true;
}

// void drawLetterA(Buffer *canvas, Color c, Vec2<i32> pos, Vec2<i32> size) {
//     u8 letter_a_alpha[] = {
//         0, 1, 1, 1, 0,
//         1, 0, 0, 0, 1,
//         1, 0, 0, 0, 1,
//         1, 0, 0, 0, 1,
//         1, 1, 1, 1, 1,
//         1, 0, 0, 0, 1,
//         1, 0, 0, 0, 1,
//         1, 0, 0, 0, 1,
//     };

//     Buffer *ref = initBuffer(5, 8);
//     for (i32 i = 0; i < 5*8; ++i) {
//         if (letter_a_alpha[i] == 1) {
//             ref->data[i] = getColorHex(c);
//         } else {
//             ref->data[i] = 0;
//         }
//     }

//     std::cout << "here\n";
//     //zoomBufferInterpolate(ref,);

//     for (i32 x = pos.x; x < pos.x + size.x; ++x) {
//         for (i32 y = pos.y; y < pos.y + size.y; ++y) {
//             // (x, y), x in 0 .. width, y in 0... height
//             // (x, y)  k = x / (width / 5)
//             // y = k
//             i32 scaled_x = x / ((size.x - pos.x) / 5);
//             i32 scaled_y = y / ((size.y - pos.y) / 8);
//             u32 hex = ref->data[scaled_y * 8 + scaled_x];
//             if (hex) canvas->data[y * canvas->width + x] = getColorHex(c);
//         }
//     }

//     freeBuffer(ref);
// }

// u8 letterB[] = {
//     1, 1, 1, 1, 0,
//     1, 0, 0, 0, 1,
//     1, 0, 0, 0, 1,
//     1, 1, 1, 1, 0,
//     1, 0, 0, 0, 1,
//     1, 0, 0, 0, 1,
//     1, 0, 0, 0, 1,
//     1, 1, 1, 1, 1,
// };

int main() {
    //generatePaletteMonochrome(0.8);
    generatePalette();
    Window window{800, 800, "fractal explorer"};
    FractalExplorer f{&window};
    //FractalExplorer f{&window, {0.4, 0.4}};

    while (!window.shouldClose()) {
        if (window.wasResized()) {
            f.resizeCanvas(window.size());
        }

        if (window.buttonHeld(MOUSE_BUTTON_LEFT)) {
            f.pan(window.mousePositionDelta());
        }

        Vec2<f64> scroll = window.scrollVector();
        if (scroll.y != 0.0) {
            scroll.y = 5.0 * (scroll.y / window.size().y); // 0,5
            scroll.y += scroll.y < 0.0 ? -1.0 : 1.0; // 1, 6
            if (scroll.y < 0.0) { scroll.y = 1.0 / -scroll.y; }
            f.zoom(window.mousePosition(), scroll.y);
        }

       // drawLetterA(f.getCanvas(), BLACK, {0, 0}, {10, 16});
        window.update();
    }
}

// scroll vector lies in [-height, height]
// split it into [0, height]
// 0 => 1.0
// 
