#include <iostream>

#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_sdl.h>

#include <SDL.h>
#include <SDL_gpu.h>

#include <OpenGL/gl.h>

#include <array>

struct Window final {
    SDL_Window* window;
    GPU_Target* target;

    GPU_ShaderBlock block;
};

Window w;

class NineSliced final
{
public:
    NineSliced(uint32_t left, uint32_t right, uint32_t top, uint32_t bottom, const char* filename) {
        image = GPU_LoadImage(filename);

        // Todo: asserts on left < right < image->w
        //                  top < bottom < image->h

        // 0---l---r---w     0---1---2---3
        // |   |   |   |     | \ | \ | \ |
        // t---+---+---+     4---5---6---7
        // |   |   |   |     |\  |\  |\  |
        // |   |   |   |     |  \|  \|  \|
        // b---+---+---+     8---9---A---B
        // |   |   |   |     | \ | \ | \ |
        // h---+---+---*     C---D---E---F

        l = static_cast<float>(left);
        r = static_cast<float>(right);
        t = static_cast<float>(top);
        b = static_cast<float>(bottom);

        auto w = static_cast<float>(image->w);
        auto h = static_cast<float>(image->h);

        float l_u = l / w;
        float r_u = r / w;
        float t_v = t / h;
        float b_v = b / h;

        //            x     y     u     v        r     g     b     a
        verts = { { { 0.0f, 0.0f, 0.0f, 0.0f,    1.0f, 1.0f, 1.0f, 1.0f },
                    {    l, 0.0f,  l_u, 0.0f,    1.0f, 1.0f, 1.0f, 1.0f },
                    {    r, 0.0f,  r_u, 0.0f,    1.0f, 1.0f, 1.0f, 1.0f },
                    {    w, 0.0f, 1.0f, 0.0f,    1.0f, 1.0f, 1.0f, 1.0f },

                    { 0.0f,    t, 0.0f,  t_v,    1.0f, 1.0f, 1.0f, 1.0f },
                    {    l,    t,  l_u,  t_v,    1.0f, 1.0f, 1.0f, 1.0f },
                    {    r,    t,  r_u,  t_v,    1.0f, 1.0f, 1.0f, 1.0f },
                    {    w,    t, 1.0f,  t_v,    1.0f, 1.0f, 1.0f, 1.0f },

                    { 0.0f,    b, 0.0f,  b_v,    1.0f, 1.0f, 1.0f, 1.0f },
                    {    l,    b,  l_u,  b_v,    1.0f, 1.0f, 1.0f, 1.0f },
                    {    r,    b,  r_u,  b_v,    1.0f, 1.0f, 1.0f, 1.0f },
                    {    w,    b, 1.0f,  b_v,    1.0f, 1.0f, 1.0f, 1.0f },

                    { 0.0f,    h, 0.0f, 1.0f,    1.0f, 1.0f, 1.0f, 1.0f },
                    {    l,    h,  l_u, 1.0f,    1.0f, 1.0f, 1.0f, 1.0f },
                    {    r,    h,  r_u, 1.0f,    1.0f, 1.0f, 1.0f, 1.0f },
                    {    w,    h, 1.0f, 1.0f,    1.0f, 1.0f, 1.0f, 1.0f }, } };

        r = image->w - r;
        b = image->h - b;

        set_size(image->w, image->h);
    }

    void set_size(float width, float height) {
        // Todo: asserts, as usual

        auto new_r = width - r;
        auto new_b = height - b;

        verts[2].x = verts[6].x = verts[10].x = verts[14].x = verts[0].x + new_r;
        verts[3].x = verts[7].x = verts[11].x = verts[15].x = verts[0].x + width;

        verts[8].y = verts[9].y = verts[10].y = verts[11].y = verts[0].y + new_b;
        verts[12].y = verts[13].y = verts[14].y = verts[15].y = verts[0].y + height;
    }

    float* get_data() const
    {
        return const_cast<float*>(reinterpret_cast<const float*>(&verts));
    }

    GPU_Image* get_image()
    {
        return image;
    }

    uint16_t* get_indices() {
        return const_cast<uint16_t*>(reinterpret_cast<const uint16_t*>(&indices));
    }

    void set_pos(float x, float y) {
        auto start_x = verts[0].x;
        auto start_y = verts[0].y;

        for (auto& v : verts) {
            v.x += x - start_x;
            v.y += y - start_y;
        }
    }

private:
    GPU_Image* image;

    float l;
    float r;
    float t;
    float b;

    std::array<uint16_t, 54> indices = {
        0, 5, 4, 0, 1, 5, 1, 6, 5, 1, 2, 6, 2, 7, 6, 2, 3, 7,
        4, 9, 8, 4, 5, 9, 5, 0xA, 9, 5, 6, 0xA, 6, 0xB, 0xA, 6, 7, 0xB,
        8, 0xD, 0xC, 8, 9, 0xD, 9, 0xE, 0xD, 9, 0xA, 0xE, 0xA, 0xF, 0xE, 0xA, 0xF, 0xB,
    };

    struct Vertex
    {
        float x;
        float y;
        float u;
        float v;
        float r;
        float g;
        float b;
        float a;
    } __attribute__((packed));

    std::array<Vertex, 16> verts;
};

void render() {
    GPU_Clear(w.target);

    auto n = NineSliced(138, 454, 103, 103, "frame.png");
    n.set_size(800, 500);

    GPU_TriangleBatch(n.get_image(), w.target, 16, n.get_data(), 54, n.get_indices(), GPU_BATCH_XY | GPU_BATCH_ST | GPU_BATCH_RGBA);

    GPU_Flip(w.target);
}

bool main_loop() {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        switch (e.type) {
        case SDL_QUIT:
            return false;
        case SDL_KEYDOWN:
            switch (e.key.keysym.sym) {
            case SDLK_ESCAPE:
                return false;
            }
        default:
            break;
        }
    }
    SDL_Delay(100);
    render();
    return true;
}

int main() {
    SDL_Init(SDL_INIT_VIDEO);

    w.target = GPU_Init(800, 600, GPU_DEFAULT_INIT_FLAGS);
    if (!w.target) {
        std::cout << GPU_PopErrorCode().details << '\n';
    }

    w.window = SDL_GetWindowFromID(w.target->context->windowID);

    ImGui::CreateContext();
    ImGui::GetIO().IniFilename = nullptr;

    ImGui_ImplSDL2_InitForOpenGL(w.window, w.target->context->context);

    while(main_loop());

    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
    return 0;
}
