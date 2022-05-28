#include "guis/GuiMoonlight.h"

#include <algorithm>
#include <iostream>
#include <sstream>
#include <fstream>

#include "Scripting.h"
#include "Window.h"
#include "guis/GuiMsgBox.h"
#include "platform.h"
#include "Log.h"
#include "SystemConf.h"

void GuiMoonlight::show(Window* window)
{
	window->pushGui(new GuiMoonlight(window));
}

GuiMoonlight::GuiMoonlight(Window* window)
 : GuiSettings(window, "MOONLIGHT GAME STREAMING")
{
  char pin[5];
  snprintf(pin, sizeof pin, "%04d", rand() % 10000);

	auto theme = ThemeData::getMenuTheme();
	std::shared_ptr<Font> font = theme->Text.font;
	unsigned int color = theme->Text.color;
	auto pinUI = std::make_shared<TextComponent>(window, pin, font, color);

	addGroup(_("TOOLS"));

  addEntry(_("UPDATE MOONLIGHT GAMES"), false, [window] {
    std::string server_ip = SystemConf::getInstance()->get("moonlight.host");
    auto apps = ParseAppList(executeScript("moonlight list " + server_ip));
    if (apps.empty()) {
      window->pushGui(new GuiMsgBox(window, _("Unable to connect to server")));
    } else {
      executeScript("rm /storage/roms/moonlight/*");
      executeScript("mkdir -p /storage/roms/moonlight");
      for (auto app : apps) {
        std::string filename = app;
        std::replace(filename.begin(), filename.end(), '/', ' ');
        std::ofstream app_file("/storage/roms/moonlight/" + filename + ".sh");
        app_file << "#!/bin/bash" << std::endl;
        app_file << "moonlight stream -app \"" << app << "\" -platform sdl " << server_ip << std::endl;
        app_file.close();
      }
      Scripting::fireEvent("quit", "restart");
			quitES(QuitMode::QUIT);
    }
  });

  addEntry(_("PAIR WITH SERVER"), false, [window, pin] {
    std::string server_ip = SystemConf::getInstance()->get("moonlight.host");

    char cmd[1024];
    snprintf(cmd, sizeof cmd, "moonlight pair -pin %s %s", pin, server_ip.c_str());
		executeScript(cmd, [server_ip, window](std::string line) {
      std::string new_server_ip;
      if (ParseServerIp(line, &new_server_ip) && server_ip != new_server_ip) {
  			SystemConf::getInstance()->set("moonlight.host", new_server_ip);
	  		SystemConf::getInstance()->saveSystemConf();
      }
      const std::string pared_ok = "Succesfully paired";
      if (line == "Succesfully paired") {
        window->pushGui(new GuiMsgBox(window, _("Succesfully paired with server")));
      }
		});
	});

  addEntry(_("UNPAIR WITH SERVER"), false, [this, window] {
		runSystemCommand("rm -r ~/.cache/moonlight", "", nullptr);
	});

	addGroup(_("SETTINGS"));
  addInputTextRow(_("SERVER IP"), "moonlight.host", false);
  addWithLabel(_("PARING PIN"), pinUI);
}

std::vector<std::string> GuiMoonlight::ParseAppList(const std::vector<std::string>& vec) {
  std::vector<std::string> apps;
  for (auto line : vec) {
    int pos = line.find(". ");
    if (pos == -1) continue;
    apps.push_back(line.substr(pos+2));
  }
  return apps;
}

bool GuiMoonlight::ParseServerIp(const std::string& line, std::string* server_ip) {
  // Watching for "Connect to 10.1.10.217..." line.
  const std::string prompt = "Connect to ";
  if (line.find(prompt) != 0) return false;

  const std::string ip = line.substr(prompt.length());
  if (ip.substr(ip.length() - 3) != "...") return false;

  *server_ip = ip.substr(0, ip.length() - 3);
  return true;
}

std::vector<std::string> GuiMoonlight::executeScript(const std::string& command) {
  std::vector<std::string> vec;
  executeScript(command, [&vec](const std::string& line) {
    vec.push_back(line);
  });
  return vec;
}

std::pair<std::string, int> GuiMoonlight::executeScript(const std::string& command, const std::function<void(const std::string)>& func)
{  
  std::cout << "executeScript -> " << command << std::endl;
	LOG(LogInfo) << "executeScript -> " << command;

	FILE *pipe = popen(command.c_str(), "r");
	if (pipe == NULL)
	{
		LOG(LogError) << "Error executing " << command;
		return std::pair<std::string, int>("Error starting command : " + command, -1);
	}

  std::stringstream output_stream;
	char buff[1024];
	while (fgets(buff, 1024, pipe))
	{
    std::stringstream output_stream(buff);
    std::string line;
    while (getline(output_stream, line).good()) {
      if (line.empty()) continue;
      std::cout << line << std::endl;
      if (func != nullptr) {
        func(std::string(line));
      }
    }
	}

	int exitCode = WEXITSTATUS(pclose(pipe));
	return std::pair<std::string, int>("", exitCode);
}
