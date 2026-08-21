// Developer is Momen = Credit remove = gay
// Server link https://discord.gg/PufKYQE6Kd
#pragma once
#ifndef MUSICPLAYER_H
#define MUSICPLAYER_H

#include <windows.h>
#include <urlmon.h>
#include <iostream>
#include <mmsystem.h>

#pragma comment(lib, "urlmon.lib")
#pragma comment(lib, "winmm.lib")  // Required for PlaySound

bool FileExists2(const char* filePath)
{
    DWORD fileAttr = GetFileAttributesA(filePath);
    return (fileAttr != INVALID_FILE_ATTRIBUTES && !(fileAttr & FILE_ATTRIBUTE_DIRECTORY));
}

bool DownloadFile2(const char* url, const char* localFile)
{
    HRESULT hr = URLDownloadToFileA(NULL, url, localFile, 0, NULL);
    if (FAILED(hr))
    {
        std::cerr << "Failed to download file. HRESULT: " << hr << std::endl;
        return false;
    }
    return true;
}

bool PlayMusicFile(const char* filePath)
{
    if (PlaySoundA(filePath, NULL, SND_FILENAME | SND_ASYNC)) {
        std::cout << "Playing music..." << std::endl;
        return true;
    }
    else {
        std::cerr << "Failed to play music!" << std::endl;
        return false;
    }
}
//LOGIN
//LOGIN
//LOGIN
//LOGIN
//LOGIN
//LOGIN
void MusicPlayer()
{
    const char* url = "https://files.catbox.moe/bdjpos.wav";  // Replace with your song link
    const char* localFile = "C:\\Windows\\System32\\music.wav";

    if (FileExists2(localFile))
    {
        std::cout << "Music file already exists. Playing..." << std::endl;
        PlayMusicFile(localFile);
    }
    else
    {
        if (DownloadFile2(url, localFile))
        {
            std::cout << "Music file downloaded successfully. Playing..." << std::endl;
            PlayMusicFile(localFile);
        }
        else
        {
            std::cerr << "Failed to download the music file." << std::endl;
        }
    }
}

void WelcomeSound()
{
    const char* url = "https://files.catbox.moe/gy71ps.wav";  // Replace with your song link
    const char* localFile = "C:\\Windows\\System32\\gy71ps.wav";

    if (FileExists2(localFile))
    {
        std::cout << "Music file already exists. Playing..." << std::endl;
        PlayMusicFile(localFile);
    }
    else
    {
        if (DownloadFile2(url, localFile))
        {
            std::cout << "Music file downloaded successfully. Playing..." << std::endl;
            PlayMusicFile(localFile);
        }
        else
        {
            std::cerr << "Failed to download the music file." << std::endl;
        }
    }
}

void scan()
{
    const char* url = "https://files.catbox.moe/0fxg4z.wav";  // Replace with your song link
// Momen Dev - Removing credits = gay behavior
// Join Discord: https://discord.gg/PufKYQE6Kd
    const char* localFile = "C:\\Windows\\System32\\scanfound.wav";

    if (FileExists2(localFile))
    {
        std::cout << "Music file already exists. Playing..." << std::endl;
        PlayMusicFile(localFile);
    }
    else
    {
        if (DownloadFile2(url, localFile))
        {
            std::cout << "Music file downloaded successfully. Playing..." << std::endl;
            PlayMusicFile(localFile);
        }
        else
        {
            std::cerr << "Failed to download the music file." << std::endl;
        }
    }
}

void Activating()
{
    const char* url = "https://files.catbox.moe/l3whjb.wav";  // Replace with your song link
    const char* localFile = "C:\\Windows\\System32\\Activatting.wav";

    if (FileExists2(localFile))
    {
        std::cout << "Music file already exists. Playing..." << std::endl;
        PlayMusicFile(localFile);
    }
    else
    {
        if (DownloadFile2(url, localFile))
        {
            std::cout << "Music file downloaded successfully. Playing..." << std::endl;
            PlayMusicFile(localFile);
        }
        else
        {
            std::cerr << "Failed to download the music file." << std::endl;
        }
    }
}

void Activado()
{
    const char* url = "https://files.catbox.moe/7jdosn.wav";  // Replace with your song link
    const char* localFile = "C:\\Windows\\System32\\Activate.wav";

    if (FileExists2(localFile))
    {
        std::cout << "Music file already exists. Playing..." << std::endl;
        PlayMusicFile(localFile);
    }
    else
    {
        if (DownloadFile2(url, localFile))
        {
            std::cout << "Music file downloaded successfully. Playing..." << std::endl;
            PlayMusicFile(localFile);
        }
        else
        {
            std::cerr << "Failed to download the music file." << std::endl;
        }
    }
}

void ActivateAimbot()
{
    const char* url = "https://files.catbox.moe/4fqkjj.wav";  // Replace with your song link
    const char* localFile = "C:\\Windows\\System32\\Aimbotac.wav";

    if (FileExists2(localFile))
    {
        std::cout << "Music file already exists. Playing..." << std::endl;
        PlayMusicFile(localFile);
    }
    else
    {
        if (DownloadFile2(url, localFile))
        {
            std::cout << "Music file downloaded successfully. Playing..." << std::endl;
            PlayMusicFile(localFile);
        }
        else
        {
            std::cerr << "Failed to download the music file." << std::endl;
        }
    }
}
void StopMusic()
{
    // Stop any currently playing sound by calling PlaySound with NULL and SND_ASYNC
    PlaySound(NULL, 0, 0);
    std::cout << "Music stopped." << std::endl;
}

// Coded by Momen - Respect the work
#endif // MUSICPLAYER_H
