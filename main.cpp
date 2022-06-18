#include <iostream>

#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_sdl.h>
#include <imgui/backends/imgui_impl_opengl3.h>

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

class NineSliced final {
public:
    NineSliced(uint32_t left, uint32_t right, uint32_t top, uint32_t bottom, const GPU_Image* _image)
        : image(_image) {
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
                    {    l,    b,  l_u,  b_v,    1.0f, 1.0f, 1.0f, 0.5f },
                    {    r,    b,  r_u,  b_v,    1.0f, 1.0f, 1.0f, 0.5f },
                    {    w,    b, 1.0f,  b_v,    1.0f, 1.0f, 1.0f, 0.5f },

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

    float* get_data() const {
        return const_cast<float*>(reinterpret_cast<const float*>(&verts));
    }

    const GPU_Image* get_image() {
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

    void set_color(size_t i, float r, float g, float b, float a) {
        verts[i].r = r;
        verts[i].g = g;
        verts[i].b = b;
        verts[i].a = a;
    }

    std::tuple<float, float, float, float> get_color(size_t i) const {
        return { verts[i].r, verts[i].g, verts[i].b, verts[i].a };
    }

private:
    const GPU_Image* image;

    float l;
    float r;
    float t;
    float b;

    std::array<uint16_t, 54> indices = {
        0, 5, 4, 0, 1, 5, 1, 6, 5, 1, 2, 6, 2, 7, 6, 2, 3, 7,
        4, 9, 8, 4, 5, 9, 5, 0xA, 9, 5, 6, 0xA, 6, 0xB, 0xA, 6, 7, 0xB,
        8, 0xD, 0xC, 8, 9, 0xD, 9, 0xE, 0xD, 9, 0xA, 0xE, 0xA, 0xF, 0xE, 0xA, 0xF, 0xB,
    };

    struct Vertex {
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

struct Settings {
    uint32_t border_x;

    uint32_t left, right, top, bottom, x, y, width, height;

    uint32_t max_width, max_height;

    struct Color {
        float r = 1.0f, g = 1.0f, b = 1.0f, a = 1.0f;
    };
    Color colors[16];

    int32_t selected_vertex = 0;

    GPU_Image* image;
};

Settings settings;

void render() {
    GPU_Clear(w.target);

    auto n = NineSliced(settings.left, settings.right, settings.top, settings.bottom, settings.image);

    // Settings menu
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();
    ImGui::Begin("Menu", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

    int32_t lr[2] = { static_cast<int32_t>(settings.left), static_cast<int32_t>(settings.right) };
    if (ImGui::SliderInt2("Left Right", lr, 0, settings.max_width))
    {
        settings.left = lr[0];
        settings.right = lr[1];
    }

    int32_t tb[2] = { static_cast<int32_t>(settings.top), static_cast<int32_t>(settings.bottom) };
    if (ImGui::SliderInt2("Top Bottom", tb, 0, settings.max_height))
    {
        settings.top = tb[0];
        settings.bottom = tb[1];
    }

    int32_t xy[2] = { static_cast<int32_t>(settings.x), static_cast<int32_t>(settings.y) };
    if (ImGui::SliderInt2("X Y", xy, 0, 1000))
    {
        settings.x = xy[0];
        settings.y = xy[1];
    }

    int32_t wh[2] = { static_cast<int32_t>(settings.width), static_cast<int32_t>(settings.height) };
    if (ImGui::SliderInt2("Width Height", wh, 0, 1000))
    {
        settings.width = wh[0];
        settings.height = wh[1];
    }

    if (ImGui::InputInt("Vertex num", &settings.selected_vertex))
    {
        settings.selected_vertex = std::clamp(settings.selected_vertex, 0, 15);
    }

    ImGui::SliderFloat4("Vertex Color", reinterpret_cast<float*>(&settings.colors[settings.selected_vertex]), 0.0f, 1.0f);

    ImGui::End();

    GPU_Blit(settings.image, nullptr, w.target, 0.0f, 0.0f);
    GPU_Line(w.target, settings.border_x, 0.0f, settings.border_x, 2000.0f, { 0, 200, 200, 255 });

    GPU_Line(w.target, settings.left, 0.0f, settings.left, settings.max_height, { 200, 0, 0, 255 });
    GPU_Line(w.target, settings.right, 0.0f, settings.right, settings.max_height, { 200, 0, 0, 255 });
    GPU_Line(w.target, 0.0f, settings.top, settings.max_width, settings.top, { 200, 0, 0, 255 });
    GPU_Line(w.target, 0.0f, settings.bottom, settings.max_width, settings.bottom, { 200, 0, 0, 255 });

    n.set_size(settings.width, settings.height);
    n.set_pos(settings.border_x + 2 + settings.x, settings.y);

    for(auto i = 0u; i < 16; ++i) {
        const auto& c = settings.colors[i];
        n.set_color(i, c.r, c.g, c.b, c.a);
    }
    GPU_TriangleBatch(const_cast<GPU_Image*>(n.get_image()), w.target, 16, n.get_data(), 54, n.get_indices(), GPU_BATCH_XY | GPU_BATCH_ST | GPU_BATCH_RGBA);

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

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
            ImGui_ImplSDL2_ProcessEvent(&e);
        }
    }
    SDL_Delay(10);
    render();
    return true;
}

void load_image(const char* filename) {
    settings.image = GPU_LoadImage(filename);
    settings.max_width = settings.image->w;
    settings.max_height = settings.image->h;
    settings.border_x = settings.max_width;
    ImGui::SetNextWindowPos({ 0.0f, settings.max_height + 40.0f });
}

int main() {
    settings.left = 138;
    settings.right = 454;
    settings.top = 101;
    settings.bottom = 102;
    settings.x = 0;
    settings.y = 0;
    settings.width = 500;
    settings.height = 300;

    SDL_Init(SDL_INIT_VIDEO);

    w.target = GPU_Init(1280, 800, GPU_DEFAULT_INIT_FLAGS);
    if (!w.target) {
        std::cout << GPU_PopErrorCode().details << '\n';
    }

    GPU_SetDefaultAnchor(0, 0);

    w.window = SDL_GetWindowFromID(w.target->context->windowID);

    ImGui::CreateContext();
    ImGui::GetIO().IniFilename = nullptr;

    ImGui_ImplOpenGL3_Init(nullptr);
    ImGui_ImplSDL2_InitForOpenGL(w.window, w.target->context->context);

    load_image("frame2.png");

    while(main_loop());

    ImGui_ImplSDL2_Shutdown();
    ImGui_ImplOpenGL3_Shutdown();
    ImGui::DestroyContext();
    return 0;
}
