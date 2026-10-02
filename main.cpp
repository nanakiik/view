#include <SDL3/SDL.h>

#include "backends/imgui_impl_sdl3.h"
#include "backends/imgui_impl_sdlrenderer3.h"
#include "imgui.h"

#include <cstdint>
#include <cstdio>

#include <fstream>
#include <string>
#include <vector>

#include <windows.h>
// 🤣🤣
#include <commdlg.h>

#pragma comment(lib, "Comdlg32.lib")

static constexpr int IMAGE_WIDTH = 1920;
static constexpr int IMAGE_HEIGHT = 1080;
static constexpr int CHANNELS = 3;
static constexpr float TOOLBAR_HEIGHT = 60.0f;

static constexpr size_t IMAGE_SIZE = IMAGE_WIDTH * IMAGE_HEIGHT * CHANNELS;

int main() {

  SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
  SDL_Window *window =
      SDL_CreateWindow("RGB Viewer", 1920, 1080, SDL_WINDOW_RESIZABLE);
  SDL_Renderer *renderer = SDL_CreateRenderer(window, nullptr);

  // gui
  IMGUI_CHECKVERSION();

  ImGui::CreateContext();

  ImGuiIO &io = ImGui::GetIO();
  (void)io;

  ImGui::StyleColorsDark();

  ImGui_ImplSDL3_InitForSDLRenderer(window, renderer);

  ImGui_ImplSDLRenderer3_Init(renderer);

  std::vector<uint8_t> imageData;
  SDL_Texture *texture = nullptr;
  std::string status = "Please select an RGB file.";

  bool running = true;

  while (running) {

    SDL_Event event;

    while (SDL_PollEvent(&event)) {
      ImGui_ImplSDL3_ProcessEvent(&event);

      if (event.type == SDL_EVENT_QUIT) {
        running = false;
      }
    }
    if (!running)
      break;

    ImGui_ImplSDLRenderer3_NewFrame();
    ImGui_ImplSDL3_NewFrame();

    ImGui::NewFrame();

    ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(300.0f, 80.0f), ImGuiCond_Always);

    ImGui::Begin("RGB Viewer", nullptr,
                 ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize);

    if (ImGui::Button("Load")) {
      wchar_t fileName[MAX_PATH] = L"";

      OPENFILENAMEW ofn{};
      ofn.lStructSize = sizeof(ofn);
      ofn.lpstrFile = fileName;
      ofn.nMaxFile = MAX_PATH;

      ofn.lpstrFilter = L"Text Files (*.rgb)\0*.rgb\0"
                        L"JSON Files (*.json)\0*.json\0"
                        L"All Files (*.*)\0*.*\0";

      ofn.nFilterIndex = 1;

      ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
      GetOpenFileNameW(&ofn);

      std::ifstream file(fileName, std::ios::binary);

      file.seekg(0, std::ios::end);

      const std::streamsize fileSize = file.tellg();

      file.seekg(0, std::ios::beg);

      if (fileSize != static_cast<std::streamsize>(IMAGE_SIZE)) {
        imageData.clear();
      } else {

        imageData.resize(IMAGE_SIZE);

        file.read(reinterpret_cast<char *>(imageData.data()), IMAGE_SIZE);

        if (texture) {
          SDL_DestroyTexture(texture);
          texture = nullptr;
        }

        texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB24,
                                    SDL_TEXTUREACCESS_STATIC, IMAGE_WIDTH,
                                    IMAGE_HEIGHT);

        if (!texture) {
          status =
              "Failed to create SDL texture: " + std::string(SDL_GetError());

          imageData.clear();
        } else {

          if (!SDL_UpdateTexture(texture, nullptr, imageData.data(),
                                 IMAGE_WIDTH * CHANNELS)) {

            SDL_DestroyTexture(texture);
            texture = nullptr;
          } else {
            status = "Loaded.";
          }
        }
      }
    }

    ImGui::TextWrapped("%s", status.c_str());

    ImGui::End();

    ImGui::Render();

    SDL_SetRenderDrawColor(renderer, 30, 30, 30, 255);

    SDL_RenderClear(renderer);

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

        dst.h = static_cast<float>(windowHeight);
        dst.w = dst.h * imageAspect;

        dst.x = (static_cast<float>(windowWidth) - dst.w) * 0.5f;

        dst.y = 0.0f;
      } else {

        dst.w = static_cast<float>(windowWidth);
        dst.h = dst.w / imageAspect;

        dst.x = 0.0f;

        dst.y = (static_cast<float>(windowHeight) - dst.h) * 0.5f;
      }

      SDL_RenderTexture(renderer, texture, nullptr, &dst);
    }

    ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer);

    SDL_RenderPresent(renderer);
  }

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