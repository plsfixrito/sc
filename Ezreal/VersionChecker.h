#pragma once
#include <Windows.h>
#include <string>
#include <iostream>
#include <Wininet.h>
#include <string>
#pragma comment(lib, "wininet.lib")
using namespace std;

namespace VersionChecker
{
	string Version = "1.0.0.0 Initial Release";
	string Project = "Ezreal";
	string PluginName = "kEzreal";

	string replaceAll(string subject, const string& search, const string& replace)
	{
		size_t pos = 0;
		while ((pos = subject.find(search, pos)) != string::npos)
		{
			subject.replace(pos, search.length(), replace);
			pos += replace.length();
		}
		return subject;
	}

	string DownloadString(string URL)
	{
		HINTERNET interwebs = InternetOpenA("Mozilla/5.0", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, NULL);
		HINTERNET urlFile;
		string rtn;
		if (interwebs)
		{
			urlFile = InternetOpenUrlA(interwebs, URL.c_str(), NULL, NULL, NULL, NULL);
			if (urlFile)
			{
				char buffer[2000];
				DWORD bytesRead;
				do {
					InternetReadFile(urlFile, buffer, 2000, &bytesRead);
					rtn.append(buffer, bytesRead);
					memset(buffer, 0, 2000);
				} while (bytesRead);
				InternetCloseHandle(interwebs);
				InternetCloseHandle(urlFile);
				string p = replaceAll(rtn, "|n", "\r\n");
				return p;
			}
		}
		InternetCloseHandle(interwebs);
		string p = replaceAll(rtn, "|n", "\r\n");
		return p;
	}

	bool CheckForUpdates()
	{
		string onlineVersion = DownloadString("https://raw.githubusercontent.com/plsfixrito/sc/master/" + Project + "/version");
		g_Common->ChatPrint((PluginName + ": Loaded!").c_str());

		if (Version != onlineVersion)
		{
			g_Common->ChatPrint((PluginName + ": Outdated, " + onlineVersion).c_str());
			return false;
		}

		g_Common->ChatPrint((PluginName + ": " + Version).c_str());
		return true;
	}
}