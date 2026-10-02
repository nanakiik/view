#include <SDL3/SDL.h>

#include "backends/imgui_impl_sdl3.h"
#include "backends/imgui_impl_sdlrenderer3.h"
#include "imgui.h"

#include <cstdint>
#include <cstdio>
#include <fstream>
#include <string>
#include <vector>

static constexpr int IMAGE_WIDTH = 1920;
static constexpr int IMAGE_HEIGHT = 1080;
static constexpr int CHANNELS = 3;
static constexpr float TOOLBAR_HEIGHT = 60.0f;

static constexpr size_t IMAGE_SIZE =
    static_cast<size_t>(IMAGE_WIDTH) * IMAGE_HEIGHT * CHANNELS;

int main() {
  // ==================================================
  // SDL 初始化
  // ==================================================

  if (!SDL_Init(SDL_INIT_VIDEO)) {
    std::fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());

    return 1;
  }

  SDL_Window *window =
      SDL_CreateWindow("RGB Viewer", 1280, 720, SDL_WINDOW_RESIZABLE);

  if (!window) {
    std::fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());

    SDL_Quit();
    return 1;
  }

  SDL_Renderer *renderer = SDL_CreateRenderer(window, nullptr);

  if (!renderer) {
    std::fprintf(stderr, "SDL_CreateRenderer failed: %s\n", SDL_GetError());

    SDL_DestroyWindow(window);
    SDL_Quit();

    return 1;
  }

  // ==================================================
  // ImGui 初始化
  // ==================================================

  IMGUI_CHECKVERSION();

  ImGui::CreateContext();

  ImGuiIO &io = ImGui::GetIO();
  (void)io;

  ImGui::StyleColorsDark();

  ImGui_ImplSDL3_InitForSDLRenderer(window, renderer);

  ImGui_ImplSDLRenderer3_Init(renderer);

  // ==================================================
  // RGB 数据
  // ==================================================

  std::vector<uint8_t> imageData;

  SDL_Texture *texture = nullptr;

  char filename[1024] = {};

  std::string status = "Please select an RGB file.";

  // ==================================================
  // 主循环
  // ==================================================

  bool running = true;

  while (running) {
    // --------------------------------------------------
    // SDL Event
    // --------------------------------------------------

    SDL_Event event;

    while (SDL_PollEvent(&event)) {
      ImGui_ImplSDL3_ProcessEvent(&event);

      if (event.type == SDL_EVENT_QUIT) {
        running = false;
      }
    }

    // --------------------------------------------------
    // ImGui Frame
    // --------------------------------------------------

    ImGui_ImplSDLRenderer3_NewFrame();
    ImGui_ImplSDL3_NewFrame();

    ImGui::NewFrame();

    // ==================================================
    // UI
    // ==================================================

    ImGui::Begin("RGB Viewer");

    ImGui::Text("Image: %d x %d RGB8", IMAGE_WIDTH, IMAGE_HEIGHT);

    ImGui::Separator();

    ImGui::InputText("File", filename, sizeof(filename));

    ImGui::SameLine();

    if (ImGui::Button("Load")) {
      // ----------------------------------------------
      // 打开文件
      // ----------------------------------------------

      std::ifstream file(filename, std::ios::binary);

      if (!file) {
        status = "Failed to open file.";

        if (texture) {
          SDL_DestroyTexture(texture);
          texture = nullptr;
        }

        imageData.clear();
      } else {
        // ------------------------------------------
        // 检查文件大小
        // ------------------------------------------

        file.seekg(0, std::ios::end);

        const std::streamsize fileSize = file.tellg();

        file.seekg(0, std::ios::beg);

        // ------------------------------------------
        // RGB 文件必须正好是 1920x1080x3
        // ------------------------------------------

        if (fileSize != static_cast<std::streamsize>(IMAGE_SIZE)) {
          status = "Invalid file size: " + std::to_string(fileSize) +
                   " bytes. Expected " + std::to_string(IMAGE_SIZE) + " bytes.";

          imageData.clear();
        } else {
          // --------------------------------------
          // 读取 RGB 数据
          // --------------------------------------

          imageData.resize(IMAGE_SIZE);

          file.read(reinterpret_cast<char *>(imageData.data()), IMAGE_SIZE);

          if (!file) {
            status = "Failed to read RGB data.";

            imageData.clear();
          } else {
            // ----------------------------------
            // 删除旧 Texture
            // ----------------------------------

            if (texture) {
              SDL_DestroyTexture(texture);
              texture = nullptr;
            }

            // ----------------------------------
            // 创建 RGB Texture
            // ----------------------------------

            texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB24,
                                        SDL_TEXTUREACCESS_STATIC, IMAGE_WIDTH,
                                        IMAGE_HEIGHT);

            if (!texture) {
              status = "Failed to create SDL texture: " +
                       std::string(SDL_GetError());

              imageData.clear();
            } else {
              // ------------------------------
              // RGB 数据上传 GPU
              // ------------------------------

              if (!SDL_UpdateTexture(texture, nullptr, imageData.data(),
                                     IMAGE_WIDTH * CHANNELS)) {
                status =
                    "Failed to update texture: " + std::string(SDL_GetError());

                SDL_DestroyTexture(texture);
                texture = nullptr;
              } else {
                status = "Loaded: " + std::string(filename);
              }
            }
          }
        }
      }
    }

    ImGui::TextWrapped("%s", status.c_str());

    ImGui::Separator();

    ImGui::Text("Expected size: %zu bytes", IMAGE_SIZE);

    ImGui::End();

    // ==================================================
    // SDL Rendering
    // ==================================================

    ImGui::Render();

    SDL_SetRenderDrawColor(renderer, 30, 30, 30, 255);

    SDL_RenderClear(renderer);

    // --------------------------------------------------
    // 显示 RGB 图片
    // --------------------------------------------------

    if (texture) {
      int windowWidth;
      int windowHeight;

      SDL_GetWindowSize(window, &windowWidth, &windowHeight);

      const float imageAspect =
          static_cast<float>(IMAGE_WIDTH) / static_cast<float>(IMAGE_HEIGHT);

      const float windowAspect =
          static_cast<float>(windowWidth) / static_cast<float>(windowHeight);

      SDL_FRect dst{};

      if (windowAspect > imageAspect) {
        // 窗口比较宽
        // 以高度为基准

        dst.h = static_cast<float>(windowHeight);
        dst.w = dst.h * imageAspect;

        dst.x = (static_cast<float>(windowWidth) - dst.w) * 0.5f;

        dst.y = 0.0f;
      } else {
        // 窗口比较高
        // 以宽度为基准

        dst.w = static_cast<float>(windowWidth);
        dst.h = dst.w / imageAspect;

        dst.x = 0.0f;

        dst.y = (static_cast<float>(windowHeight) - dst.h) * 0.5f;
      }

      SDL_RenderTexture(renderer, texture, nullptr, &dst);
    }

    // --------------------------------------------------
    // ImGui
    // --------------------------------------------------

    ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer);

    SDL_RenderPresent(renderer);
  }

  // ==================================================
  // 清理
  // ==================================================

  if (texture) {
    SDL_DestroyTexture(texture);
    texture = nullptr;
  }

  ImGui_ImplSDLRenderer3_Shutdown();
  ImGui_ImplSDL3_Shutdown();

  ImGui::DestroyContext();

  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);

  SDL_Quit();

  return 0;
}