#ifndef rs_sf_gl_context_hpp
#define rs_sf_gl_context_hpp

#include "example.hpp"

struct rs_sf_gl_context
{
    GLFWwindow* win = nullptr;
    std::string key;
    texture_buffer texture;

    rs_sf_gl_context(const std::string& _key, int w = 1280, int h = 480 * 2) : key(_key) {
        glfwInit();
        win = glfwCreateWindow(w, h, key.c_str(), nullptr, nullptr);
        glfwSetWindowSize(win, w / 2, h / 2);
        glfwMakeContextCurrent(win);
    }

    virtual ~rs_sf_gl_context() {
        if (win) {
            glfwDestroyWindow(win);
            glfwTerminate();
        }
    }

    bool imshow(const rs_sf_image* image, int num_images = 1, const char* text = nullptr)
    {
        if (!glfwWindowShouldClose(win))
        {
            // Wait for new images
            glfwPollEvents();

            // Clear the framebuffer
            int w, h;
            glfwGetFramebufferSize(win, &w, &h);
            glViewport(0, 0, w, h);
            glClear(GL_COLOR_BUFFER_BIT);

            // Draw the images
            glPushMatrix();
            glfwGetWindowSize(win, &w, &h);
            glOrtho(0, w, h, 0, -1, +1);

            auto tiles = static_cast<int>(ceil(sqrt(num_images)));// frames.size())));
            auto tile_w = static_cast<float>(w) / tiles;
            auto tile_h = static_cast<float>(h) / (num_images <= 2 ? 1 : tiles);

            rs_format stream_format[] = { RS_FORMAT_RAW8, RS_FORMAT_Z16, RS_FORMAT_RGB8 };
            for (int index = 0; index < num_images; ++index)
            {
                auto col_id = index / tiles;
                auto row_id = index % tiles;
                auto& frame = image[index];

                texture.upload((void*)frame.data, frame.img_w, frame.img_h, stream_format[frame.byte_per_pixel - 1]);
                texture.show({ row_id * tile_w, col_id * tile_h, tile_w, tile_h }, 1);
            }

            if (text != nullptr)
                draw_text(20 + w / 2, h - 20, text);

            glPopMatrix();
            glfwSwapBuffers(win);

            return true;
        }
        return false;
    }
};

#endif //! rs_sf_gl_context_hpp