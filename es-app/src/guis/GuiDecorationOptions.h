#pragma once
#ifndef ES_APP_GUIS_GUI_DECORATION_OPTIONS_H
#define ES_APP_GUIS_GUI_DECORATION_OPTIONS_H

#include "GuiSettings.h"
#include "components/OptionListComponent.h"
#include "guis/GuiMenu.h"

class TextComponent;

class GuiDecorationOptions : public GuiSettings
{
public:
    GuiDecorationOptions(Window *window, const std::string &configName, const std::map<std::string, std::string> &decorationSetNameToPath);

private:
    std::vector<std::string> getAvailableSystems(std::string path);
    std::vector<std::string> getAvailableGames(std::string path, std::string systemName);
    std::vector<std::string> getOptions(std::string path, std::string systemName);
    std::map<std::string, std::string> mDecorationSetNameToPath;
    std::string mConfigName;
};

#endif // ES_APP_GUIS_GUI_DECORATION_OPTIONS_H
