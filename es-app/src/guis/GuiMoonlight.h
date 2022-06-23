// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2022-present kkoshelev

#pragma once

#include "GuiSettings.h"

class GuiMoonlight : public GuiSettings 
{
public:
	static void show(Window* window);

protected:
  GuiMoonlight(Window* window);
  static bool ParseServerIp(const std::string& line, std::string* server_ip);
  static std::vector<std::string> ParseAppList(const std::vector<std::string>& vec);
};
