//A lot of code for this was modified from Goldleaf. Thank you Xor (Again).
#include <SDL.h>
#include <SDL2/SDL_ttf.h>
#include <UI.h>
#include "Networking.h"
#include "nlohmann/json.hpp"
#include <fstream>
#include "Utils.h"
using namespace std;
using json = nlohmann::json;

class UpdaterUI
{
	private:
	void DrawText(std::string);
	int UpdateState = 0;
	bool CheckForNewVersion();
	std::string UpdateText = " ";
	TTF_Font *TextFont;
	SDL_Color TextColour = {0, 0, 0};
	json GitAPIData;
	std::string LatestID;
	public:
	UpdaterUI();
	void DrawUI();
	SDL_Event *Event;
	int *WindowState;
	SDL_Renderer *renderer;
	int *Width;
	int *Height;
	int *IsDone;
	bool NewVersion;
	std::string NROPath = "sdmc:/switch/Amiigo.nro";
};

UpdaterUI::UpdaterUI()
{
	nifmInitialize(NifmServiceType_User); //Init nifm for connection stuff
	TextFont = GetSharedFont(48);
}

void UpdaterUI::DrawUI()
{
	//Handle input
	bool BPressed = false;
	SDL_Rect BackFooterRect = {10,10, 960, 520};
	int TouchX = -1;
	int TouchY = -1;
	while (SDL_PollEvent(Event))
	{
				TouchX = Event->tfinger.x * *Width;
				TouchY = Event->tfinger.y * *Height;
		switch (Event->type)
		{
			case SDL_JOYBUTTONDOWN:
			if (Event->jbutton.which == 0)
			{
				//Plus pressed
				if (Event->jbutton.button == 10)
				{
					*IsDone = 1;
				}
				//B pressed
				else if(Event->jbutton.button == 1)
				{
					BPressed = true;
				}
			}
		}
		break;
	}
	
	//Do the update stages
	switch(UpdateState)
	{
		//Check connection stage
		case 0:
		{
			//Check we have a connection before trying to access the network
			if(HasConnection())
			{
				NewVersion = CheckForNewVersion();
				UpdateState++;
			}
			else
			{
				UpdateText = "Waiting for connection.";
				if(BPressed)
				{
					*WindowState = 0;
				}
			}
		}
		break;
		//Check if newer version stage
		case 1:
		{
			if(NewVersion)
			{
				UpdateState++;
				UpdateText = "Downloading " + LatestID + ". This might take a while.";
			}
			else
			{
				UpdateText = "Already on the latest version.";
				if(BPressed||CheckButtonPressed(&BackFooterRect, TouchX, TouchY))
				{
					*WindowState = 0;
				}
			}
		}
		break;
		//Download update stage
		case 2:
		{
			//change download path
			if(CheckFileExists("sdmc:/switch/Amiigo/Amiigo.nro"))
			NROPath = "sdmc:/switch/Amiigo/Amiigo.nro";
			
			string UpdateFileURL = "https://github.com/StarDustCFW/Amiigo/releases/download/" + LatestID + "/Amiigo.nro";
			RetrieveToFile(UpdateFileURL, "sdmc:/switch/Failed_Amiigo_Update.nro");
			romfsExit();
			remove(NROPath.c_str());
			rename("sdmc:/switch/Failed_Amiigo_Update.nro", NROPath.c_str());
			*IsDone = 1;
		}
		break;
		//Somethign went wrong. User should not normally end up here.
		case 999:
		{
			UpdateText = "Error! Is GitHub rate limiting you?";
			if(BPressed)
			{
				*WindowState = 0;
			}
		}
		break;
	}
	DrawText(UpdateText);
}

bool UpdaterUI::CheckForNewVersion()
{
	//Get data from GitHub API
	string Data = RetrieveContent("https://api.github.com/repos/StarDustCFW/Amiigo/releases", "application/json");
	//Get the release tag string from the data
	GitAPIData = json::parse(Data);
	//Check if GitAPI gave us a release tag otherwise we'll crash
	if(Data.length() < 300)
	{
		//User is probably rate limited.
		UpdateState = 999;
		return false;
	}
    LatestID = GitAPIData[0]["tag_name"].get<std::string>();
	//Check if we're running the latest version
	return (LatestID != VERSION);
}

void UpdaterUI::DrawText(std::string Message)
{
	//Draw the rect
	DrawJsonColorConfig(renderer, "UpdaterUI_DrawText");
	SDL_Rect MessageRect = {0,0, *Width, *Height};
	SDL_RenderFillRect(renderer, &MessageRect);
	//Draw the text
	SDL_Surface* MessageTextSurface = TTF_RenderUTF8_Blended_Wrapped(TextFont, Message.c_str(), TextColour, *Width);
	SDL_Texture* MessagerTextTexture = SDL_CreateTextureFromSurface(renderer, MessageTextSurface);
	SDL_Rect HeaderTextRect = {(*Width - MessageTextSurface->w) / 2, (*Height - MessageTextSurface->h) / 2, MessageTextSurface->w, MessageTextSurface->h};
	SDL_RenderCopy(renderer, MessagerTextTexture, NULL, &HeaderTextRect);
	//Clean up
	SDL_DestroyTexture(MessagerTextTexture);
	SDL_FreeSurface(MessageTextSurface);
}