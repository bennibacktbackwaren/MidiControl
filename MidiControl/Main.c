/*
	MidiControl - A background service that maps MIDI tones to actions.
	Copyright (C) 2020 bennibacktbackwaren

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

// Enable Visual Styles for Controls (e.g. Buttons @ TaskDialog)
#pragma comment(linker,"\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

// Standard includes
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

// Windows includes
#include <Windows.h>
#include <CommCtrl.h>

#define error_msg(x) TaskDialog(NULL, NULL, L"MidiControl", L"An error occured", TEXT(x), TDCBF_CLOSE_BUTTON, TD_ERROR_ICON, NULL)

#pragma region Structs

// Assigns an action to a MIDI tone
typedef struct
{
	unsigned int tone, action;
} action_t;

// Contains app configuration
typedef struct
{
	action_t *actions;
	unsigned int input_device, number_actions;
} config_t;

#pragma endregion

#pragma region Function prototypes

// Loads app configuration
bool load_config(config_t *config);

// Sends input to windows
void send_input(int key);

// Required for MIDI interface
void CALLBACK MidiInProc(HMIDIIN hMidiIn, UINT wMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2);

#pragma endregion

int main(void)
{
	config_t cfg;
	HMIDIIN input_device;
	MMRESULT result;

	if (!load_config(&cfg))
		return EXIT_FAILURE;
	
#ifdef _DEBUG
	printf("Input device: %i\nNumber of actions: %i\n\n", cfg.input_device, cfg.number_actions);
	for (int i = 0; i < cfg.number_actions; i++)
		printf("%i - %i\n", (cfg.actions + i)->action, (cfg.actions + i)->tone);
	printf("\n");
#endif

	// Try to connect to input device
	result = midiInOpen(&input_device, cfg.input_device, MidiInProc, &cfg, CALLBACK_FUNCTION);
	if (result != MMSYSERR_NOERROR)
	{
		error_msg("Unable to connect to the input device.");
		return EXIT_FAILURE;
	}

	// Try to start listening
	result = midiInStart(input_device);
	if (result != MMSYSERR_NOERROR)
	{
		error_msg("Unable to start listening.");
		return EXIT_FAILURE;
	}
	
	// This function call waits an infinite amount of time,
	// without causing any cpu load
	int wait_var = 0;
	WaitOnAddress(&wait_var, &wait_var, sizeof(int), INFINITE);
		
	// Disconnect, clean up and exit; never gets called, but it's here
	midiInStop(input_device);
	midiInClose(input_device);
	free(cfg.actions);

	return EXIT_SUCCESS;
}

bool load_config(config_t *config)
{
	FILE *file;

	// Try to open file
	switch (fopen_s(&file, "config.ini", "rt"))
	{
		// No error occured
	case 0:
		break;

		// File doesn't exist
	case ENOENT:
		error_msg("An error occured trying to open the config file:\nFile \"config.ini\" not found.");
		return false;

		// Unknown error
	default:
		error_msg("An unknown error occured trying to open the config file.");
		return false;
	}

	// Try to read common config section
	if (fscanf_s(file, "[Common]\nInputDevice=%u\nNumberActions=%u\n\n[Actions]\n", &config->input_device, &config->number_actions) != 2)
	{
		error_msg("Invalid config file.");
		return false;
	}

	// Try to allocate memory for actions
	config->actions = (action_t *)calloc(config->number_actions, sizeof(action_t));
	if (config->actions == NULL)
	{
		if (errno == ENOMEM)
			error_msg("Unable to allocate memory: Not enough memory left.");
		else error_msg("An unknown error occurred trying to allocate memory.");

		return false;
	}

	// Read actions
	for (unsigned int i = 0; i < config->number_actions; i++)
	{
		if (fscanf_s(file, "%u=%u\n", &(config->actions + i)->tone, &(config->actions + i)->action) != 2)			
		{
			error_msg("Invalid config file.");
			free(config->actions);

			return false;
		}	
	}

	// Everything worked ^^
	return true;
}

void CALLBACK MidiInProc(HMIDIIN hMidiIn, UINT wMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2)
{
	int midi_action = LOBYTE(LOWORD(dwParam1));
	int tone = HIBYTE(LOWORD(dwParam1));

	int action = -1;
	config_t *cfg = (config_t *)dwInstance;

	// If input device sends other message than key press, exit function
	if (midi_action != 144)
		return;

	// Search for an action paired with the received tone
	for (unsigned int i = 0; i < cfg->number_actions; i++)
	{
		if ((cfg->actions + i)->tone == tone)
		{
			action = (cfg->actions + i)->action;
			break;
		}
	}

	// Determine and perform action
	switch (action)
	{
		// Previous song
	case 0:
		send_input(VK_MEDIA_PREV_TRACK);
		break;

		// Next song
	case 1:
		send_input(VK_MEDIA_NEXT_TRACK);
		break;

		// Play / Pause
	case 2:
		send_input(VK_MEDIA_PLAY_PAUSE);
		break;
	}
}

void send_input(int key)
{
	INPUT key_input[1];
	KEYBDINPUT keybd;

	keybd.wScan = keybd.dwFlags = keybd.time = 0;
	keybd.dwExtraInfo = NULL;
	keybd.wVk = (unsigned short)key;

	key_input[0].type = INPUT_KEYBOARD;
	key_input[0].ki = keybd;

	SendInput(1, key_input, sizeof(INPUT));
}