#include <SDL3/SDL.h>

#include "backends/imgui_impl_sdl3.h"
#include "backends/imgui_impl_sdlrenderer3.h"
#include "imgui.h"

#include <cstdint>
#include <fstream>
#include <string>
#include <vector>

#include <png.h>
#include <webp/decode.h>

#include <windows.h>
// 🤣🤣
#include <commdlg.h>

#pragma comment(lib, "Comdlg32.lib")

static bool LoadWebP(const wchar_t *fileName, std::vector<uint8_t> &imageData,
                     int &imageWidth, int &imageHeight, std::string &status) {
  std::ifstream file(fileName, std::ios::binary);
  if (!file) {
    status = "Failed to open file.";
    return false;
  }

  file.seekg(0, std::ios::end);
  const std::streamsize fileSize = file.tellg();
  file.seekg(0, std::ios::beg);

  if (fileSize <= 0) {
    return false;
  }

  std::vector<uint8_t> compressedData(static_cast<size_t>(fileSize));

  if (!file.read(reinterpret_cast<char *>(compressedData.data()), fileSize)) {
    status = "Failed to read WebP file.";
    return false;
  }

  int width = 0;
  int height = 0;

  uint8_t *decodedData = WebPDecodeRGBA(compressedData.data(),
                                        compressedData.size(), &width, &height);

  if (!decodedData || width <= 0 || height <= 0) {
    status = "Failed to decode WebP.";
    if (decodedData)
      WebPFree(decodedData);
    return false;
  }

  const size_t pixelBytes =
      static_cast<size_t>(width) * static_cast<size_t>(height) * 4;

  imageData.assign(decodedData, decodedData + pixelBytes);
  WebPFree(decodedData);

  imageWidth = width;
  imageHeight = height;

  status = "Loaded: " + std::to_string(width) + " x " + std::to_string(height);

  return true;
}

static SDL_Texture *CreateTexture(SDL_Renderer *renderer,
                                  const std::vector<uint8_t> &imageData,
                                  int width, int height, std::string &status) {
  if (!renderer || imageData.empty() || width <= 0 || height <= 0) {
    status = "Invalid image data.";
    return nullptr;
  }

  SDL_Texture *texture =
      SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32,
                        SDL_TEXTUREACCESS_STATIC, width, height);

  if (!texture) {
    status = "Failed to create SDL texture: " + std::string(SDL_GetError());
    return nullptr;
  }

  // SDL3 uses bool return semantics here: true = success.
  if (!SDL_UpdateTexture(texture, nullptr, imageData.data(), width * 4)) {
    status =
        "Failed to upload image to SDL texture: " + std::string(SDL_GetError());

    SDL_DestroyTexture(texture);
    return nullptr;
  }

  // Prevent texture filtering from changing the pixel values.
  SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_NEAREST);

  return texture;
}

int main() {
  if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO)) {
    return 1;
  }

  SDL_Window *window =
      SDL_CreateWindow("WebP Viewer", 1920, 1080, SDL_WINDOW_RESIZABLE);

  if (!window) {
    SDL_Quit();
    return 1;
  }

  SDL_Renderer *renderer = SDL_CreateRenderer(window, nullptr);

  if (!renderer) {
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 1;
  }

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();

  ImGuiIO &io = ImGui::GetIO();
  (void)io;

  ImGui::StyleColorsDark();

  ImGui_ImplSDL3_InitForSDLRenderer(window, renderer);
  ImGui_ImplSDLRenderer3_Init(renderer);

  std::vector<uint8_t> imageData;
  SDL_Texture *texture = nullptr;

  int imageWidth = 0;
  int imageHeight = 0;

  std::string status = "Please select a WebP file.";

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

    ImGui::SetNextWindowSize(ImVec2(320.0f, 110.0f), ImGuiCond_Always);

    ImGui::Begin("WebP Viewer", nullptr,
                 ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize);

    if (ImGui::Button("Load WebP")) {
      wchar_t fileName[MAX_PATH] = L"";

      OPENFILENAMEW ofn{};
      ofn.lStructSize = sizeof(ofn);
      ofn.lpstrFile = fileName;
      ofn.nMaxFile = MAX_PATH;

      ofn.lpstrFilter = L"WebP Files (*.webp)\0*.webp\0"
                        L"All Files (*.*)\0*.*\0";

      ofn.nFilterIndex = 1;

      ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

      if (GetOpenFileNameW(&ofn)) {
        std::vector<uint8_t> newImageData;
        int newWidth = 0;
        int newHeight = 0;
        std::string newStatus;

        if (LoadWebP(fileName, newImageData, newWidth, newHeight, newStatus)) {

          SDL_Texture *newTexture = CreateTexture(
              renderer, newImageData, newWidth, newHeight, newStatus);

          if (newTexture) {
            if (texture) {
              SDL_DestroyTexture(texture);
            }

            texture = newTexture;
            imageData = std::move(newImageData);
            imageWidth = newWidth;
            imageHeight = newHeight;
            status = newStatus;
          } else {
            status = newStatus;
          }
        } else {
          status = newStatus;
        }
      }
    }

    ImGui::TextWrapped("%s", status.c_str());

    ImGui::End();

    ImGui::Render();

    int windowWidth = 0;
    int windowHeight = 0;

    SDL_GetWindowSize(window, &windowWidth, &windowHeight);

    // Clear first.
    SDL_SetRenderDrawColor(renderer, 30, 30, 30, 255);

    SDL_RenderClear(renderer);

    if (texture && imageWidth > 0 && imageHeight > 0 && windowWidth > 0 &&
        windowHeight > 0) {

      const float imageAspect =
          static_cast<float>(imageWidth) / static_cast<float>(imageHeight);

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

    // Render ImGui after the image.
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
