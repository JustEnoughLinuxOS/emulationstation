#include "guis/GuiDecorationOptions.h"

#include "Window.h"
#include "GuiSettings.h"
#include "components/OptionListComponent.h"
#include "components/SliderComponent.h"
#include "components/SwitchComponent.h"
#include "guis/GuiMsgBox.h"

#include "Settings.h"
#include "LocaleES.h"
#include "ApiSystem.h"
#include "guis/GuiTextEditPopup.h"
#include "guis/GuiTextEditPopupKeyboard.h"

#include "SystemConf.h"
#include "ApiSystem.h"

GuiDecorationOptions::GuiDecorationOptions(Window *window,
										   const std::string &configName,
										   const std::map<std::string, std::string> &decorationSetNameToPath) : GuiSettings(window, _("DECORATION OPTIONS")),
																											   mConfigName(configName),
																											   mDecorationSetNameToPath(decorationSetNameToPath)

{
	auto selectedDecoration = SystemConf::getInstance()->get(mConfigName + ".bezel");

	//Check for both <system>.bezel and then global.bezel if <system>.bezel is set to auto
	if (Utils::String::toLower(selectedDecoration) == "auto" || selectedDecoration == "")
	{
		selectedDecoration = SystemConf::getInstance()->get("global.bezel");
		if (Utils::String::toLower(selectedDecoration) == "auto" || selectedDecoration == "")
		{
			selectedDecoration = "default";
		}
	}

	std::string decorationPath;

	try
	{
		decorationPath = mDecorationSetNameToPath.at(selectedDecoration);
		LOG(LogDebug) << "Decoration: " << selectedDecoration<< " Path: " << decorationPath;
	}
	catch (...)
	{
		LOG(LogError) << "No path found for decoration: " << selectedDecoration;
		return;
	}

	mMenu.addGroup(_("OVERRIDES"));
	// System Options

	auto systemsList = std::make_shared<OptionListComponent<std::string>>(mWindow, _("SYSTEM"), false);

	std::string systemsType = SystemConf::getInstance()->get(mConfigName + ".bezel.system.override");

	LOG(LogDebug) << "Systems type: " << systemsType;

	for (auto system : getAvailableSystems(decorationPath))
	{
		systemsList->add(system, system, systemsType == system);
	}
	if (systemsList->getSelectedName() == "")
	{
		systemsList->selectFirstItem();
	}

	addWithDescription(_("SYSTEM"), _("Forces specific system bezel"), systemsList);
	systemsList->setSelectedChangedCallback([this](std::string system)
	{
		if(Utils::String::toLower(system) == "auto") {
			system = "";
		}
		if (SystemConf::getInstance()->set(mConfigName + ".bezel.system.override", Utils::String::toLower(system)))
		{
			LOG(LogDebug) << "Pushing new GUI Decorations page: " << mConfigName;

			//Make temporary copy of variables to survive delete
			auto newConfig = std::string(mConfigName);
			const std::map<std::string, std::string> newDSNTP(mDecorationSetNameToPath);

			Window *pw = mWindow;
			delete this;

			try
			{
				pw->pushGui(new GuiDecorationOptions(pw, newConfig, newDSNTP));
			}
			catch (...)
			{
				LOG(LogError) << "Caught an exception in systems list callback.  Not sure the deal";
			}
													
		}
	});

	std::string systemOverride = SystemConf::getInstance()->get(mConfigName + ".bezel.system.override");


    //deal mConfigName will like like <systemname>['<rom name'] in the rom specific case.  Just remove stuff in after '['
    auto systemName =  Utils::String::split(mConfigName, '[', true).front();
	LOG(LogDebug) << "System Override: " << systemOverride << " Current System: " << systemName;

	if (systemOverride != "" && Utils::String::toLower(systemOverride) != "auto")
	{
		systemName = systemOverride;
	}

	// System Options
	auto gamesList = std::make_shared<OptionListComponent<std::string>>(mWindow, _("GAME"), false);
	std::string gamesType = SystemConf::getInstance()->get(mConfigName + ".bezel.game.override");
	for (auto game : getAvailableGames(decorationPath, systemName))
	{
		gamesList->add(game, game, gamesType == game);
	}
	if (gamesList->getSelectedName() == "")
	{
		gamesList->selectFirstItem();
	}

	addWithDescription(_("GAME"), _("Forces specific game bezel"), gamesList);
	gamesList->setSelectedChangedCallback([this](std::string game)
	{
		if(Utils::String::toLower(game) == "auto") {
			game = "";
		}
		LOG(LogDebug) << "Saving game override: " << game << "for system: "+mConfigName;

		SystemConf::getInstance()->set(mConfigName + ".bezel.game.override", game);
	});

	auto options = getOptions(decorationPath, systemName);
	if (options.size() > 0)
	{
		LOG(LogDebug) << "Options exist.  Number: " << options.size();

		mMenu.addGroup(_("OVERLAYS"));

		for (auto option : options)
		{

			auto fileName = Utils::FileSystem::getFileName(option);
			auto fileConfigName = Utils::String::replace(fileName, ".png", "");
			const auto fileConfigNameUpper = Utils::String::toUpper(fileConfigName);
			auto optionConfigName = ".bezel.overlay." + fileConfigName;

			LOG(LogDebug) << "Option config : " << optionConfigName << " option file: " << option;

			auto optionComponent = std::make_shared<OptionListComponent<std::string>>(mWindow, _(fileConfigNameUpper.c_str()));

			optionComponent->add(_("AUTO"), "auto", SystemConf::getInstance()->get(configName + optionConfigName) != "0" && SystemConf::getInstance()->get(configName + optionConfigName) != "1");
			optionComponent->add(_("YES"), "1", SystemConf::getInstance()->get(configName + optionConfigName) == "1");
			optionComponent->add(_("NO"), "0", SystemConf::getInstance()->get(configName + optionConfigName) == "0");

			addWithLabel(_(fileConfigNameUpper.c_str()), optionComponent);
			addSaveFunc([this, optionComponent, optionConfigName]
						{ SystemConf::getInstance()->set(mConfigName + optionConfigName, optionComponent->getSelected()); });
		}
	}
}

std::vector<std::string> GuiDecorationOptions::getAvailableSystems(std::string path)
{
	std::vector<std::string> systems;
	systems.push_back("AUTO");
	systems.push_back("NONE");

	std::string systemsDir = path + "/systems/";

	LOG(LogDebug) << "Looking for available systems in: " << systemsDir;

	auto systemsPngs = Utils::FileSystem::getDirContent(systemsDir.c_str(), false);
	for (Utils::FileSystem::stringList::const_iterator j = systemsPngs.cbegin(); j != systemsPngs.cend(); ++j)
	{
		auto systemsPng = *j;
		if (Utils::FileSystem::isRegularFile(systemsPng) && Utils::String::endsWith(systemsPng, ".png"))
		{
			auto availableSystem = Utils::FileSystem::getStem(systemsPng);
			LOG(LogDebug) << "Found system: " << availableSystem;
			systems.push_back(availableSystem);
		}
	}
	return systems;
}

std::vector<std::string> GuiDecorationOptions::getAvailableGames(std::string path, std::string systemName)
{
	std::vector<std::string> availableGames;
	availableGames.push_back("AUTO");
	availableGames.push_back("NONE");
	if (Utils::String::toUpper(systemName) == "NONE")
	{
		return availableGames;
	}
	std::string gamesDir = path + "/systems/" + systemName + "/games";
	LOG(LogDebug) << "Looking for available games in: " << gamesDir;

	if (Utils::FileSystem::exists(gamesDir.c_str()))
	{
		auto gamesFiles = Utils::FileSystem::getDirContent(gamesDir.c_str(), true);

		for (Utils::FileSystem::stringList::const_iterator j = gamesFiles.cbegin(); j != gamesFiles.cend(); ++j)
		{
			auto gameFile = *j;
			if (Utils::FileSystem::isRegularFile(gameFile) && Utils::String::endsWith(gameFile, ".cfg"))
			{
				auto gameName = Utils::FileSystem::getStem(gameFile);
				LOG(LogDebug) << "Game Name: " << gameName << "Game Config file: " << gameFile;;
				availableGames.push_back(gameName);
			}
		}
	}
	return availableGames;
}
std::vector<std::string> GuiDecorationOptions::getOptions(std::string path, std::string systemName)
{
	std::vector<std::string> options;
	if (Utils::String::toUpper(systemName) == "NONE")
	{
		return options;
	}
	std::string overlay_dir = path + "/systems/" + systemName + "/overlays";
	LOG(LogDebug) << "Looking for options in: " << overlay_dir;
	if (Utils::FileSystem::exists(overlay_dir.c_str()))
	{
		auto bezel_files = Utils::FileSystem::getDirContent(overlay_dir.c_str(), true);
		for (Utils::FileSystem::stringList::const_iterator j = bezel_files.cbegin(); j != bezel_files.cend(); ++j)
		{
			auto bezel_file = *j;
			if (Utils::FileSystem::isRegularFile(bezel_file) && Utils::String::endsWith(bezel_file, ".png"))
			{
				LOG(LogDebug) << "Option file found: " << bezel_file;
				options.push_back(bezel_file);
			}
		}
	}
	return options;
}