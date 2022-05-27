#pragma once

#include "GuiSettings.h"

class GuiMoonlight : public GuiSettings 
{
public:
	static void show(Window* window);

protected:
  GuiMoonlight(Window* window);
  static std::pair<std::string, int> executeScript(const std::string& command, const std::function<void(const std::string)>& func);
  static std::vector<std::string> executeScript(const std::string& command);
  static bool ParseServerIp(const std::string& line, std::string* server_ip);
  static std::vector<std::string> ParseAppList(const std::vector<std::string>& vec);
};
