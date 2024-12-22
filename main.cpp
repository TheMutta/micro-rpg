// Dear ImGui: standalone example application for SDL2 + SDL_Renderer
// (SDL is a cross-platform general purpose library for handling windows, inputs, OpenGL/Vulkan/Metal graphics context creation, etc.)

// Learn about Dear ImGui:
// - FAQ                  https://dearimgui.com/faq
// - Getting Started      https://dearimgui.com/getting-started
// - Documentation        https://dearimgui.com/docs (same as your local docs/ folder).
// - Introduction, links and more at the top of imgui.cpp

// Important to understand: SDL_Renderer is an _optional_ component of SDL2.
// For a multi-platform app consider using e.g. SDL+DirectX on Windows and SDL+OpenGL on Linux/OSX.

#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_sdlrenderer2.h"
#include <stdio.h>
#include <SDL.h>
#include <SDL_image.h>

#if !SDL_VERSION_ATLEAST(2,0,17)
#error This backend requires SDL 2.0.17+ because of SDL_RenderGeometry() function
#endif

#include <vector>
#include <map>
#include <string>
#include <memory>

SDL_Surface *LoadTexture(const char* filename) {
	SDL_Surface *surface = IMG_Load(filename);
	if (!surface) {
		SDL_Log("Unable to load image %s: %s", filename, IMG_GetError());
	}
	return surface;
}

class Object {
public:
	SDL_Texture *Texture;
	ImVec2 Pos;
	ImVec2 Size;

	void Draw() {
		ImGui::GetBackgroundDrawList()->AddImage(
				(ImTextureID)Texture,
				Pos,
				ImVec2(Pos.x + Size.x, Pos.y + Size.y)
				);
	}

	virtual void Act() {

	}
};

class Player : public Object {

};

class Level {
public:
	std::shared_ptr<Player> CurrentPlayer;
	std::vector<std::shared_ptr<Object>> Objects;
	std::map<std::string, SDL_Texture*> Textures;

	std::shared_ptr<Player> GetPlayer() { return CurrentPlayer; }
};

std::shared_ptr<Level> CurrentLevel;
	
class Enemy : public Object {
public:
};

class Canova : public Enemy {
public:
	double lastAttackTimer = 0;

	void Act() override {
		std::shared_ptr<Player> player = CurrentLevel->GetPlayer();
		ImVec2 canovaCentre = ImVec2(this->Pos.x + this->Size.x / 2, this->Pos.y + this->Size.y / 2);
		ImVec2 playerCentre = ImVec2(player->Pos.x + player->Size.x / 2, player->Pos.y + player->Size.y / 2);

		double dirX = playerCentre.x - canovaCentre.x;
		double dirY = playerCentre.y - canovaCentre.y;

		double length = sqrt(dirX*dirX+dirY*dirY);
		double normX = dirX/length;
		double normY = dirY/length;

		ImVec2 movementCentre(canovaCentre.x + normX * 50, canovaCentre.y + normY * 50);

		ImGui::GetForegroundDrawList()->AddLine(canovaCentre, movementCentre, ImGui::GetColorU32(ImVec4(255, 0, 0, 255)), 10);
	}
};

class Obstacle : public Object {

};

class Projectile : public Object {
	ImVec2 Movement;
	void Act() override {

	}
};

class Tilemap {

};

// Main code
int main(int, char**)
{
	// Setup SDL
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0)
	{
		printf("Error: %s\n", SDL_GetError());
		return -1;
	}

	// From 2.0.18: Enable native IME.
#ifdef SDL_HINT_IME_SHOW_UI
	SDL_SetHint(SDL_HINT_IME_SHOW_UI, "1");
#endif

	// Create window with SDL_Renderer graphics context
	SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_FULLSCREEN_DESKTOP);
	SDL_Window* window = SDL_CreateWindow("Dear ImGui SDL2+SDL_Renderer example", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, window_flags);
	if (window == nullptr)
	{
		printf("Error: SDL_CreateWindow(): %s\n", SDL_GetError());
		return -1;
	}
	SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_ACCELERATED);
	if (renderer == nullptr)
	{
		SDL_Log("Error creating SDL_Renderer!");
		return -1;
	}
	//SDL_RendererInfo info;
	//SDL_GetRendererInfo(renderer, &info);
	//SDL_Log("Current SDL_Renderer: %s", info.name);

	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();

	// Setup Platform/Renderer backends
	ImGui_ImplSDL2_InitForSDLRenderer(window, renderer);
	ImGui_ImplSDLRenderer2_Init(renderer);

	// Our state
	bool render_game = true;
	bool render_vn = false;
	SDL_Texture *backgroundTexture;
	{
		SDL_Surface* surface = LoadTexture("assets/background.png");
		if (!surface) {
			SDL_Quit();
			return -1;
		}
		backgroundTexture = SDL_CreateTextureFromSurface(renderer, surface);
		SDL_FreeSurface(surface);
	}

	std::shared_ptr<Player> player = std::make_shared<Player>();
	player->Pos = ImVec2(0,0);
	player->Size = ImVec2(64, 64);
	{
		SDL_Surface* surface = LoadTexture("assets/ProtagonistFront.png");
		if (!surface) {
			SDL_Quit();
			return -1;
	 	}
		player->Texture = SDL_CreateTextureFromSurface(renderer, surface);
		SDL_FreeSurface(surface);
	}



	std::shared_ptr<Canova> canova = std::make_shared<Canova>();
	canova->Pos = ImVec2(600, 400);
	canova->Size = ImVec2(64, 64);

	std::shared_ptr<Projectile> scalpel = std::make_shared<Projectile>();
	scalpel->Pos = ImVec2(400, 400);
	scalpel->Size = ImVec2(64, 64);
	{
		SDL_Surface* surface = LoadTexture("assets/CanovaScalpello.png");
		if (!surface) {
			SDL_Quit();
			return -1;
		}
		scalpel->Texture = SDL_CreateTextureFromSurface(renderer, surface);
		SDL_FreeSurface(surface);
	}

	CurrentLevel = std::make_shared<Level>();
	CurrentLevel->CurrentPlayer = player;
	CurrentLevel->Objects.push_back(player);
	CurrentLevel->Objects.push_back(canova);

	// Main loop
	bool done = false;
	while (!done)
	{
		// Poll and handle events (inputs, window resize, etc.)
		// You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
		// - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
		// - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
		// Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
		SDL_Event event;
		while (SDL_PollEvent(&event))
		{
			ImGui_ImplSDL2_ProcessEvent(&event);
			if (event.type == SDL_QUIT)
				done = true;
			if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(window))
				done = true;
		}
		if (SDL_GetWindowFlags(window) & SDL_WINDOW_MINIMIZED)
		{
			SDL_Delay(10);
			continue;
		}

		// Start the Dear ImGui frame
		ImGui_ImplSDLRenderer2_NewFrame();
		ImGui_ImplSDL2_NewFrame();
		ImGui::NewFrame();
			
		ImVec2 screenSize = io.DisplaySize;

		if (render_vn) {
			ImGui::Begin("Text window", nullptr, ImGuiWindowFlags_NoTitleBar);
			ImGui::SetWindowPos(ImVec2(0, screenSize.y * 2.0f / 3.0f));
			ImGui::SetWindowSize(ImVec2(screenSize.x, screenSize.y / 3.0f));
			ImGui::Text("Lorem Ipsum");
			ImGui::Button("Back");
			ImGui::SameLine();
			ImGui::Button("Next");
			ImGui::End();

		}

		if (render_game) {
			if (ImGui::IsKeyDown(ImGuiKey_W)) {
				player->Pos.y -= 5;
			}

			if (ImGui::IsKeyDown(ImGuiKey_S)) {
				player->Pos.y += 5;
			}

			if (ImGui::IsKeyDown(ImGuiKey_A)) {
				player->Pos.x -= 5;
			}

			if (ImGui::IsKeyDown(ImGuiKey_D)) {
				player->Pos.x += 5;
			}

			ImVec2 size(64.0f, 64.0f);
			for (float y = 0; y < screenSize.y; y += 64) {
				for (float x = 0; x < screenSize.x; x += 64) {
					ImVec2 pos(x, y);

					ImGui::GetBackgroundDrawList()->AddImage(
						(ImTextureID)backgroundTexture,
						ImVec2(pos.x, pos.y),
						ImVec2(pos.x + size.x, pos.y + size.y)
					);
				}

			}

			for (auto& object : CurrentLevel->Objects) {
				object->Act();
				object->Draw();
			}

			/*
			ImGui::GetBackgroundDrawList()->AddImage(
						(ImTextureID)player.Texture,
						player.Pos,
						ImVec2(player.Pos.x + player.Size.x, player.Pos.y + player.Size.y)
					);

			ImGui::GetBackgroundDrawList()->AddImage(
						(ImTextureID)canova.Texture,
						canova.Pos,
						ImVec2(canova.Pos.x + canova.Size.x, canova.Pos.y + canova.Size.y)
					);
			*/
		}


		// Rendering
		ImGui::Render();
		SDL_RenderSetScale(renderer, io.DisplayFramebufferScale.x, io.DisplayFramebufferScale.y);
		SDL_SetRenderDrawColor(renderer, (Uint8)(255), (Uint8)(255), (Uint8)(255), (Uint8)(255));
		SDL_RenderClear(renderer);
		ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), renderer);
		SDL_RenderPresent(renderer);
	}

	// Cleanup
	ImGui_ImplSDLRenderer2_Shutdown();
	ImGui_ImplSDL2_Shutdown();
	ImGui::DestroyContext();

	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();

	return 0;
}
