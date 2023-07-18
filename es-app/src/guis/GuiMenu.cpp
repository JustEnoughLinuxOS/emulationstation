#include "guis/GuiMenu.h"

#include "components/OptionListComponent.h"
#include "components/SliderComponent.h"
#include "components/SwitchComponent.h"
#include "guis/GuiCollectionSystemsOptions.h"
#include "guis/GuiDetectDevice.h"
#include "guis/GuiGeneralScreensaverOptions.h"
#include "guis/GuiMsgBox.h"
#include "guis/GuiScraperStart.h"
#include "guis/GuiPackageInstaller.h" //351elec
#include "guis/GuiDecorationOptions.h" //351elec
#include "guis/GuiHashStart.h"
#include "guis/GuiThemeInstaller.h" //batocera
#include "guis/GuiBezelInstaller.h" //batocera
#include "guis/GuiBatoceraStore.h" //batocera
#include "guis/GuiSettings.h"
#include "guis/GuiRetroAchievements.h" //batocera
#include "guis/GuiGamelistOptions.h"
#include "guis/GuiImageViewer.h"
#include "guis/GuiMoonlight.h"
#include "views/UIModeController.h"
#include "views/ViewController.h"
#include "CollectionSystemManager.h"
#include "EmulationStation.h"
#include "Scripting.h"
#include "SystemData.h"
#include "VolumeControl.h"
#include "BrightnessControl.h"
#include <SDL_events.h>
#include <algorithm>
#include "platform.h"

#include "SystemConf.h"
#include "ApiSystem.h"
#include "InputManager.h"
#include "AudioManager.h"
#include <LibretroRatio.h>
#include "guis/GuiUpdate.h"
#include "guis/GuiInstallStart.h"
#include "guis/GuiTextEditPopupKeyboard.h"
#include "guis/GuiBackupStart.h"
#include "guis/GuiTextEditPopup.h"
#include "guis/GuiWifi.h"
#include "guis/GuiBluetooth.h"
#include "scrapers/ThreadedScraper.h"
#include "FileSorts.h"
#include "ThreadedHasher.h"
#include "ThreadedBluetooth.h"
#include "views/gamelist/IGameListView.h"
#include "components/MultiLineMenuEntry.h"
#include "components/BatteryIndicatorComponent.h"
#include "GuiLoading.h"
#include "guis/GuiBios.h"
#include "guis/GuiKeyMappingEditor.h"
#include "Gamelist.h"
#include "TextToSpeech.h"

#if WIN32
#include "Win32ApiSystem.h"
#endif

#define fake_gettext_fade _("fade")
#define fake_gettext_slide _("slide")
#define fake_gettext_instant _("instant")
#define fake_gettext_fadeslide _("fade & slide")

// batocera-info
#define fake_gettext_system       _("System")
#define fake_gettext_architecture _("Architecture")
#define fake_gettext_diskformat   _("Disk format")
#define fake_gettext_temperature  _("Temperature")
#define fake_gettext_avail_memory _("Available memory")
#define fake_gettext_battery      _("Battery")
#define fake_gettext_model        _("Model")
#define fake_gettext_cpu_model    _("Cpu model")
#define fake_gettext_cpu_number   _("Cpu number")
#define fake_gettext_cpu_frequency _("Cpu max frequency")
#define fake_gettext_cpu_feature  _("Cpu feature")

#define fake_gettext_scanlines		_("SCANLINES")
#define fake_gettext_retro			_("RETRO")
#define fake_gettext_enhanced		_("ENHANCED")
#define fake_gettext_curvature		_("CURVATURE")
#define fake_gettext_zfast			_("ZFAST")
#define fake_gettext_flatten_glow	_("FLATTEN-GLOW")
#define fake_gettext_rgascaling		_("RGA SCALING")

#define fake_gettext_glvendor		_("VENDOR")
#define fake_gettext_glvrenderer	_("RENDERER")
#define fake_gettext_glversion		_("VERSION")
#define fake_gettext_glslversion	_("SHADERS")

#define gettext_controllers_settings				_("CONTROLLER SETTINGS")
#define gettext_controllers_and_bluetooth_settings  _("CONTROLLER & BLUETOOTH SETTINGS")

// Windows build does not have bluetooth support, so affect the label for Windows
#if WIN32
#define controllers_settings_label		gettext_controllers_settings
#else
#define controllers_settings_label		gettext_controllers_and_bluetooth_settings
#endif

GuiMenu::GuiMenu(Window *window, bool animate) : GuiComponent(window), mMenu(window, _("MAIN MENU").c_str()), mVersion(window)
{
	// MAIN MENU
	bool isFullUI = UIModeController::getInstance()->isUIModeFull();
#ifdef _ENABLEEMUELEC
	bool isKidUI = UIModeController::getInstance()->isUIModeKid();
#endif

	// KODI >
	// GAMES SETTINGS >
	// CONTROLLER & BLUETOOTH >
	// UI SETTINGS >
	// SOUND SETTINGS >
	// NETWORK >
	// SCRAPER >
	// SYSTEM SETTINGS >
	// QUIT >

	// KODI
#ifdef _ENABLE_KODI_
	if (SystemConf::getInstance()->getBool("kodi.enabled", true) && ApiSystem::getInstance()->isScriptingSupported(ApiSystem::KODI))
		addEntry(_("KODI MEDIA CENTER").c_str(), false, [this]
	{
		Window *window = mWindow;
		delete this;
		if (!ApiSystem::getInstance()->launchKodi(window))
			LOG(LogWarning) << "Shutdown terminated with non-zero result!";

	}, "iconKodi");
#endif

	if (isFullUI)
	{
#if !defined(WIN32) || defined(_DEBUG)
		addEntry(_("GAME SETTINGS").c_str(), true, [this] { openGamesSettings_batocera(); }, "iconGames");
		addEntry(_("GAME COLLECTION SETTINGS").c_str(), true, [this] { openCollectionSystemSettings(); }, "iconAdvanced");

		if (ApiSystem::getInstance()->isScriptingSupported(ApiSystem::RETROACHIVEMENTS) &&
			SystemConf::getInstance()->getBool("global.retroachievements") &&
			Settings::getInstance()->getBool("RetroachievementsMenuitem") &&
			SystemConf::getInstance()->get("global.retroachievements.username") != "")
			addEntry(_("RETROACHIEVEMENTS").c_str(), true, [this] { GuiRetroAchievements::show(mWindow); }, "iconRetroachievements");

#ifdef _ENABLEEMUELEC
		addEntry(_("SYSTEM SETTINGS").c_str(), true, [this] { openSystemSettings_batocera(); }, "iconSystem");
		//addEntry(_("EMULATIONSTATION SETTINGS").c_str(), true, [this] { openEmuELECSettings(); }, "iconEmuelec");
#endif
		addEntry(_("UI SETTINGS").c_str(), true, [this] { openUISettings(); }, "iconUI");
		addEntry(controllers_settings_label.c_str(), true, [this] { openControllersSettings_batocera(); }, "iconControllers");
		addEntry(_("SOUND SETTINGS").c_str(), true, [this] { openSoundSettings(); }, "iconSound");

		if (ApiSystem::getInstance()->isScriptingSupported(ApiSystem::WIFI)) {
			addEntry(_("NETWORK SETTINGS").c_str(), true, [this] { openNetworkSettings_batocera(); }, "iconNetwork");
		  addEntry(_("MOONLIGHT GAME STREAMING").c_str(), true, [this] { GuiMoonlight::show(mWindow); }, "iconGames");
		}
#else
		if (ApiSystem::getInstance()->isScriptingSupported(ApiSystem::GAMESETTINGS))
			addEntry(_("GAME SETTINGS").c_str(), true, [this] { openGamesSettings_batocera(); }, "iconGames");

		addEntry(_("UI SETTINGS").c_str(), true, [this] { openUISettings(); }, "iconUI");

		if (ApiSystem::getInstance()->isScriptingSupported(ApiSystem::GAMESETTINGS))
			addEntry(controllers_settings_label.c_str(), true, [this] { openControllersSettings_batocera(); }, "iconControllers");
		else
			addEntry(_("CONFIGURE INPUT"), true, [this] { openConfigInput(); }, "iconControllers");

		addEntry(_("SOUND SETTINGS").c_str(), true, [this] { openSoundSettings(); }, "iconSound");
		addEntry(_("GAME COLLECTION SETTINGS").c_str(), true, [this] { openCollectionSystemSettings(); }, "iconAdvanced");

		if (!ApiSystem::getInstance()->isScriptingSupported(ApiSystem::GAMESETTINGS))
		{
			for (auto system : SystemData::sSystemVector)
			{
				if (system->isCollection() || system->getEmulators().size() == 0 || (system->getEmulators().size() == 1 && system->getEmulators().begin()->cores.size() <= 1))
					continue;

				addEntry(_("EMULATOR SETTINGS"), true, [this] { openEmulatorSettings(); }, "iconGames");
				break;
			}
		}
#endif

		addEntry(_("SCRAPER").c_str(), true, [this] { openScraperSettings(); }, "iconScraper");
//		addEntry(_("UPDATES & DOWNLOADS"), true, [this] { openUpdatesSettings(); }, "iconUpdates");
	}
	else
	{


		if (ApiSystem::getInstance()->isScriptingSupported(ApiSystem::RETROACHIVEMENTS) &&
			SystemConf::getInstance()->getBool("global.retroachievements") &&
			Settings::getInstance()->getBool("RetroachievementsMenuitem") &&
			SystemConf::getInstance()->get("global.retroachievements.username") != "")
			addEntry(_("RETROACHIEVEMENTS").c_str(), true, [this] { GuiRetroAchievements::show(mWindow); }, "iconRetroachievements");

		addEntry(_("INFORMATION").c_str(), true, [this] { openSystemInformations_batocera(); }, "iconSystem");
		addEntry(_("UNLOCK UI MODE").c_str(), true, [this] { exitKidMode(); }, "iconAdvanced");
	}

#ifdef WIN32
	addEntry(_("QUIT").c_str(), !Settings::getInstance()->getBool("ShowOnlyExit"), [this] {openQuitMenu_batocera(); }, "iconQuit");
#else
#ifdef _ENABLEEMUELEC
if (!isKidUI) {
	addEntry(_("QUIT").c_str(), true, [this] { openQuitMenu_batocera(); }, "iconQuit");
}
#else
	addEntry(_("QUIT").c_str(), true, [this] { openQuitMenu_batocera(); }, "iconQuit");
#endif
#endif

	addChild(&mMenu);
	addVersionInfo(); // batocera
	setSize(mMenu.getSize());

	if (animate)
	{
		if (Renderer::isSmallScreen())
			animateTo(Vector2f((Renderer::getScreenWidth() - getSize().x()) / 2, (Renderer::getScreenHeight() - getSize().y()) / 2));
		else
			animateTo(Vector2f((Renderer::getScreenWidth() - mSize.x()) / 2, Renderer::getScreenHeight() * 0.15f));
	}
	else
	{
		if (Renderer::isSmallScreen())
			setPosition((Renderer::getScreenWidth() - mSize.x()) / 2, (Renderer::getScreenHeight() - mSize.y()) / 2);
		else
			setPosition((Renderer::getScreenWidth() - mSize.x()) / 2, Renderer::getScreenHeight() * 0.15f);
	}
}
#ifdef _ENABLEEMUELEC
/* < emuelec */
void GuiMenu::openEmuELECSettings()
{
	auto s = new GuiSettings(mWindow, "EmulationStation Settings");

	Window* window = mWindow;
	std::string a;
        auto bluetoothd_enabled = std::make_shared<SwitchComponent>(mWindow);
		bool btbaseEnabled = SystemConf::getInstance()->get("bluetooth.enabled") == "1";
		bluetoothd_enabled->setState(btbaseEnabled);
		s->addWithLabel(_("ENABLE BLUETOOTH"), bluetoothd_enabled);
		s->addSaveFunc([bluetoothd_enabled] {
			if (bluetoothd_enabled->changed()) {
			if (bluetoothd_enabled->getState() == false) {
				runSystemCommand("systemctl stop bluealsa", "", nullptr);
				runSystemCommand("systemctl stop bluetooth", "", nullptr);
				runSystemCommand("systemctl stop bluetooth-agent", "", nullptr);
				runSystemCommand("rm /storage/.cache/services/bluez.conf", "", nullptr);
				runSystemCommand("rfkill block bluetooth", "", nullptr);
			} else {
				runSystemCommand("mkdir -p /storage/.cache/services/", "", nullptr);
				runSystemCommand("touch /storage/.cache/services/bluez.conf", "", nullptr);
				runSystemCommand("systemctl start bluetooth", "", nullptr);
				runSystemCommand("systemctl start bluetooth-agent", "", nullptr);
                                runSystemCommand("systemctl start bluealsa", "", nullptr);
                                runSystemCommand("rfkill unblock bluetooth", "", nullptr);
			}
                bool bluetoothenabled = bluetoothd_enabled->getState();
                SystemConf::getInstance()->set("bluetooth.enabled", bluetoothenabled ? "1" : "0");
				SystemConf::getInstance()->saveSystemConf();
			}
		});

       auto sshd_enabled = std::make_shared<SwitchComponent>(mWindow);
		bool baseEnabled = SystemConf::getInstance()->get("ssh.enabled") == "1";
		sshd_enabled->setState(baseEnabled);
		s->addWithLabel(_("ENABLE SSH"), sshd_enabled);
		s->addSaveFunc([sshd_enabled] {
			if (sshd_enabled->changed()) {
			if (sshd_enabled->getState() == false) {
				runSystemCommand("systemctl stop sshd", "", nullptr);
				runSystemCommand("rm /storage/.cache/services/sshd.conf", "", nullptr);
			} else {
				runSystemCommand("mkdir -p /storage/.cache/services/", "", nullptr);
				runSystemCommand("touch /storage/.cache/services/sshd.conf", "", nullptr);
				runSystemCommand("systemctl start sshd", "", nullptr);
			}
                bool sshenabled = sshd_enabled->getState();
                SystemConf::getInstance()->set("ssh.enabled", sshenabled ? "1" : "0");
				SystemConf::getInstance()->saveSystemConf();
			}
		});

		auto emuelec_boot_def = std::make_shared< OptionListComponent<std::string> >(mWindow, "START AT BOOT", false);
		std::vector<std::string> devices;
		devices.push_back("Emulationstation");
		devices.push_back("Retroarch");
		for (auto it = devices.cbegin(); it != devices.cend(); it++)
		emuelec_boot_def->add(*it, *it, SystemConf::getInstance()->get("boot") == *it);
		s->addWithLabel(_("START AT BOOT"), emuelec_boot_def);
		s->addSaveFunc([emuelec_boot_def] {
			if (emuelec_boot_def->changed()) {
				std::string selectedBootMode = emuelec_boot_def->getSelected();
				SystemConf::getInstance()->set("boot", selectedBootMode);
				SystemConf::getInstance()->saveSystemConf();
			}
		});

       auto fps_enabled = std::make_shared<SwitchComponent>(mWindow);
		bool fpsEnabled = SystemConf::getInstance()->get("global.showFPS") == "1";
		fps_enabled->setState(fpsEnabled);
		s->addWithLabel(_("SHOW RETROARCH FPS"), fps_enabled);
		s->addSaveFunc([fps_enabled] {
			bool fpsenabled = fps_enabled->getState();
                SystemConf::getInstance()->set("global.showFPS", fpsenabled ? "1" : "0");
				SystemConf::getInstance()->saveSystemConf();
			});
/*
       auto bezels_enabled = std::make_shared<SwitchComponent>(mWindow);
		bool bezelsEnabled = SystemConf::getInstance()->get("global.bezel") == "1";
		bezels_enabled->setState(bezelsEnabled);
		s->addWithLabel(_("ENABLE RA BEZELS"), bezels_enabled);
		s->addSaveFunc([bezels_enabled] {
			bool bezelsenabled = bezels_enabled->getState();
                SystemConf::getInstance()->set("global.bezel", bezelsenabled ? "1" : "0");
				SystemConf::getInstance()->saveSystemConf();
			});
*/
       auto splash_enabled = std::make_shared<SwitchComponent>(mWindow);
		bool splashEnabled = SystemConf::getInstance()->get("splash.enabled") == "1";
		splash_enabled->setState(splashEnabled);
		s->addWithLabel(_("ENABLE RA SPLASH"), splash_enabled);
		s->addSaveFunc([splash_enabled] {
                bool splashenabled = splash_enabled->getState();
                SystemConf::getInstance()->set("splash.enabled", splashenabled ? "1" : "0");
				SystemConf::getInstance()->saveSystemConf();
			});

	auto enable_bootvideo = std::make_shared<SwitchComponent>(mWindow);
	bool bootEnabled = SystemConf::getInstance()->get("bootvideo.enabled") == "1";
	enable_bootvideo->setState(bootEnabled);
	s->addWithLabel(_("ALWAYS SHOW BOOT VIDEO"), enable_bootvideo);

	s->addSaveFunc([enable_bootvideo, window] {
		bool bootvideoenabled = enable_bootvideo->getState();
		SystemConf::getInstance()->set("bootvideo.enabled", bootvideoenabled ? "1" : "0");
		SystemConf::getInstance()->saveSystemConf();
	});

	auto enable_randombootvideo = std::make_shared<SwitchComponent>(mWindow);
	bool randombootEnabled = SystemConf::getInstance()->get("randombootvideo.enabled") == "1";
	enable_randombootvideo->setState(randombootEnabled);
	s->addWithLabel(_("RANDOMIZE BOOT VIDEO"), enable_randombootvideo);

	s->addSaveFunc([enable_randombootvideo, window] {
		bool randombootvideoenabled = enable_randombootvideo->getState();
		SystemConf::getInstance()->set("randombootvideo.enabled", randombootvideoenabled ? "1" : "0");
        if (randombootvideoenabled)
        SystemConf::getInstance()->set("bootvideo.enabled", "1");
		SystemConf::getInstance()->saveSystemConf();
	});

	s->addInputTextRow(_("DEFAULT YOUTUBE SEARCH WORD"), "youtube.searchword", false);

	auto enable_advmamegp = std::make_shared<SwitchComponent>(mWindow);
	bool advgpEnabled = SystemConf::getInstance()->get("advmame_auto_gamepad") == "1";
	enable_advmamegp->setState(advgpEnabled);
	s->addWithLabel(_("AUTO CONFIG ADVANCEMAME GAMEPAD"), enable_advmamegp);

	s->addSaveFunc([enable_advmamegp, window] {
		bool advmamegpenabled = enable_advmamegp->getState();
		SystemConf::getInstance()->set("advmame_auto_gamepad", advmamegpenabled ? "1" : "0");
		SystemConf::getInstance()->saveSystemConf();
	});

		auto emuelec_retroarch_menu_def = std::make_shared< OptionListComponent<std::string> >(mWindow, "RETROARCH MENU", false);
		std::vector<std::string> ramenuoptions;
		ramenuoptions.push_back("default");
		ramenuoptions.push_back("ozone");
		ramenuoptions.push_back("xmb");
		ramenuoptions.push_back("rgui");

		auto ramenuoptionsS = SystemConf::getInstance()->get("global.retroarch.menu_driver");
		if (ramenuoptionsS.empty())
		ramenuoptionsS = "default";

		for (auto it = ramenuoptions.cbegin(); it != ramenuoptions.cend(); it++)
		emuelec_retroarch_menu_def->add(*it, *it, ramenuoptionsS == *it);

		s->addWithLabel(_("RETROARCH MENU"), emuelec_retroarch_menu_def);
		s->addSaveFunc([emuelec_retroarch_menu_def] {
			if (emuelec_retroarch_menu_def->changed()) {
				std::string selectedretroarch_menu = emuelec_retroarch_menu_def->getSelected();
				SystemConf::getInstance()->set("global.retroarch.menu_driver", selectedretroarch_menu);
				SystemConf::getInstance()->saveSystemConf();
			}
		});

if (UIModeController::getInstance()->isUIModeFull())
	{
        //Danger zone options
        s->addEntry(_("DANGER ZONE"), true, [this] { openDangerZone(mWindow, "global"); });
    }

    mWindow->pushGui(s);
}

void GuiMenu::openDangerZone(Window* mWindow, std::string configName)
{

	GuiSettings* dangerZone = new GuiSettings(mWindow, _("DANGER ZONE").c_str());

    dangerZone->addEntry(_("BACKUP CONFIGURATIONS"), true, [mWindow] {
    mWindow->pushGui(new GuiMsgBox(mWindow, _("WARNING THIS WILL RESTART EMULATIONSTATION!\n\nAFTER THE SCRIPT IS DONE REMEMBER TO COPY THE FILE /storage/roms/backup/JELOS_BACKUP.zip TO SOME PLACE SAFE OR IT WILL BE DELETED ON NEXT REBOOT!\n\nBACKUP CURRENT CONFIG AND RESTART?"), _("YES"),
				[] {
				runSystemCommand("/usr/bin/run \"/usr/bin/backuptool backup\"", "", nullptr);
				}, _("NO"), nullptr));
     });

    dangerZone->addEntry(_("RESTORE FROM BACKUP"), true, [mWindow] {
    mWindow->pushGui(new GuiMsgBox(mWindow, _("WARNING THIS WILL RESTART EMULATIONSTATION AND REBOOT!\n\nYOUR EXISTING CONFIGURATION WILL BE OVERWRITTEN!\n\nRESTORE FROM BACKUP AND RESTART?"), _("YES"),
				[] {
				runSystemCommand("/usr/bin/run \"/usr/bin/backuptool restore\"", "", nullptr);
				}, _("NO"), nullptr));
     });

    dangerZone->addEntry(_("SPLIT ROM SYSTEMS"), true, [mWindow] {
    mWindow->pushGui(new GuiMsgBox(mWindow, _("WARNING THIS WILL RESTART EMULATIONSTATION!\n\nTHIS SCRIPT WILL LOOK FOR SYSTEMS IN '/STORAGE/ROMS_LOCAL' AND '/STORAGE/ROMS' AND UPDATE EMULATIONSTATION ROM LOCATIONS\n\nSPLIT SYSTEM FOLDERS AND RESTART?"), _("YES"),
				[] {
				runSystemCommand("/usr/bin/run \"/usr/bin/rom_system_split\"", "", nullptr);
				}, _("NO"), nullptr));
     });

    dangerZone->addEntry(_("RESET RETROARCH CONFIG TO DEFAULT"), true, [mWindow] {
    mWindow->pushGui(new GuiMsgBox(mWindow, _("WARNING: RETROARCH CONFIG WILL RESET TO DEFAULT\n\nPER-CORE CONFIGURATIONS WILL NOT BE AFFECTED BUT NO BACKUP WILL BE CREATED!\n\nRESET RETROARCH CONFIG TO DEFAULT?"), _("YES"),
				[] {
				runSystemCommand("/usr/bin/run \"/usr/bin/factoryreset retroarch\"", "", nullptr);
				}, _("NO"), nullptr));
     });

mWindow->pushGui(dangerZone);
}


/*  emuelec >*/
#endif

void GuiMenu::openScraperSettings()
{
	// scrape now
	ComponentListRow row;
	auto openScrapeNow = [this]
	{
		if (!checkNetwork())
			return;

		if (ThreadedScraper::isRunning())
		{
			mWindow->pushGui(new GuiMsgBox(mWindow, _("SCRAPER IS RUNNING. DO YOU WANT TO STOP IT?"), _("YES"), [this]
			{
				ThreadedScraper::stop();
			}, _("NO"), nullptr));

			return;
		}

		mWindow->pushGui(new GuiScraperStart(mWindow));
	};

	auto s = new GuiSettings(mWindow,
		_("SCRAPER"),
		_("NEXT"), [openScrapeNow](GuiSettings* settings)
	{
		settings->save();
		openScrapeNow();
	});

	std::string scraper = Settings::getInstance()->getString("Scraper");

	// scrape from
	auto scraper_list = std::make_shared< OptionListComponent< std::string > >(mWindow, _("SCRAPING DATABASE"), false);
	std::vector<std::string> scrapers = Scraper::getScraperList();

	// Select either the first entry of the one read from the settings, just in case the scraper from settings has vanished.
	for(auto it = scrapers.cbegin(); it != scrapers.cend(); it++)
		scraper_list->add(*it, *it, *it == scraper);

	s->addWithLabel(_("SCRAPING DATABASE"), scraper_list); // batocera
	s->addSaveFunc([scraper_list] { Settings::getInstance()->setString("Scraper", scraper_list->getSelected()); });

	if (!scraper_list->hasSelection())
	{
		scraper_list->selectFirstItem();
		scraper = scraper_list->getSelected();
	}

	if (scraper == "ScreenScraper")
	{
		// Image source : <image> tag
		std::string imageSourceName = Settings::getInstance()->getString("ScrapperImageSrc");
		auto imageSource = std::make_shared< OptionListComponent<std::string> >(mWindow, _("IMAGE SOURCE"), false);
		imageSource->add(_("SCREENSHOT"), "ss", imageSourceName == "ss");
		imageSource->add(_("TITLE SCREENSHOT"), "sstitle", imageSourceName == "sstitle");
		imageSource->add(_("MIX V1"), "mixrbv1", imageSourceName == "mixrbv1");
		imageSource->add(_("MIX V2"), "mixrbv2", imageSourceName == "mixrbv2");
		imageSource->add(_("BOX 2D"), "box-2D", imageSourceName == "box-2D");
		imageSource->add(_("BOX 3D"), "box-3D", imageSourceName == "box-3D");
		imageSource->add(_("FAN ART"), "fanart", imageSourceName == "fanart");
		imageSource->add(_("NONE"), "", imageSourceName.empty());

		if (!imageSource->hasSelection())
			imageSource->selectFirstItem();

		s->addWithLabel(_("IMAGE SOURCE"), imageSource);
		s->addSaveFunc([imageSource] { Settings::getInstance()->setString("ScrapperImageSrc", imageSource->getSelected()); });

		// Box source : <thumbnail> tag
		std::string thumbSourceName = Settings::getInstance()->getString("ScrapperThumbSrc");
		auto thumbSource = std::make_shared< OptionListComponent<std::string> >(mWindow, _("BOX SOURCE"), false);
		thumbSource->add(_("NONE"), "", thumbSourceName.empty());
		thumbSource->add(_("BOX 2D"), "box-2D", thumbSourceName == "box-2D");
		thumbSource->add(_("BOX 3D"), "box-3D", thumbSourceName == "box-3D");

		if (!thumbSource->hasSelection())
			thumbSource->selectFirstItem();

		s->addWithLabel(_("BOX SOURCE"), thumbSource);
		s->addSaveFunc([thumbSource] { Settings::getInstance()->setString("ScrapperThumbSrc", thumbSource->getSelected()); });

		imageSource->setSelectedChangedCallback([this, thumbSource](std::string value)
		{
			if (value == "box-2D")
				thumbSource->remove(_("BOX 2D"));
			else
				thumbSource->add(_("BOX 2D"), "box-2D", false);

			if (value == "box-3D")
				thumbSource->remove(_("BOX 3D"));
			else
				thumbSource->add(_("BOX 3D"), "box-3D", false);
		});

		// Logo source : <marquee> tag
		std::string logoSourceName = Settings::getInstance()->getString("ScrapperLogoSrc");
		auto logoSource = std::make_shared< OptionListComponent<std::string> >(mWindow, _("LOGO SOURCE"), false);
		logoSource->add(_("NONE"), "", logoSourceName.empty());
		logoSource->add(_("WHEEL"), "wheel", logoSourceName == "wheel");
		logoSource->add(_("MARQUEE"), "marquee", logoSourceName == "marquee");

		if (!logoSource->hasSelection())
			logoSource->selectFirstItem();

		s->addWithLabel(_("LOGO SOURCE"), logoSource);
		s->addSaveFunc([logoSource] { Settings::getInstance()->setString("ScrapperLogoSrc", logoSource->getSelected()); });

		// scrape ratings
		auto scrape_ratings = std::make_shared<SwitchComponent>(mWindow);
		scrape_ratings->setState(Settings::getInstance()->getBool("ScrapeRatings"));
		s->addWithLabel(_("SCRAPE RATINGS"), scrape_ratings); // batocera
		s->addSaveFunc([scrape_ratings] { Settings::getInstance()->setBool("ScrapeRatings", scrape_ratings->getState()); });

		// scrape video
		auto scrape_video = std::make_shared<SwitchComponent>(mWindow);
		scrape_video->setState(Settings::getInstance()->getBool("ScrapeVideos"));
		s->addWithLabel(_("SCRAPE VIDEOS"), scrape_video);
		s->addSaveFunc([scrape_video] { Settings::getInstance()->setBool("ScrapeVideos", scrape_video->getState()); });

		// SCRAPE FANART
		auto scrape_fanart = std::make_shared<SwitchComponent>(mWindow);
		scrape_fanart->setState(Settings::getInstance()->getBool("ScrapeFanart"));
		s->addWithLabel(_("SCRAPE FANART"), scrape_fanart);
		s->addSaveFunc([scrape_fanart] { Settings::getInstance()->setBool("ScrapeFanart", scrape_fanart->getState()); });

		// SCRAPE BOX BACKSIDE
		auto scrape_boxBack = std::make_shared<SwitchComponent>(mWindow);
		scrape_boxBack->setState(Settings::getInstance()->getBool("ScrapeBoxBack"));
		s->addWithLabel(_("SCRAPE BOX BACKSIDE"), scrape_boxBack);
		s->addSaveFunc([scrape_boxBack] { Settings::getInstance()->setBool("ScrapeBoxBack", scrape_boxBack->getState()); });

		// SCRAPE MAP
		auto scrape_map = std::make_shared<SwitchComponent>(mWindow);
		scrape_map->setState(Settings::getInstance()->getBool("ScrapeMap"));
		s->addWithLabel(_("SCRAPE MAP"), scrape_map);
		s->addSaveFunc([scrape_map] { Settings::getInstance()->setBool("ScrapeMap", scrape_map->getState()); });

		// SCRAPE TITLESHOT
		auto scrape_titleshot = std::make_shared<SwitchComponent>(mWindow);
		scrape_titleshot->setState(Settings::getInstance()->getBool("ScrapeTitleShot"));
		s->addWithLabel(_("SCRAPE TITLESHOT"), scrape_titleshot);
		s->addSaveFunc([scrape_titleshot] { Settings::getInstance()->setBool("ScrapeTitleShot", scrape_titleshot->getState()); });

		// SCRAPE CARTRIDGE
		auto scrape_cartridge = std::make_shared<SwitchComponent>(mWindow);
		scrape_cartridge->setState(Settings::getInstance()->getBool("ScrapeCartridge"));
		s->addWithLabel(_("SCRAPE CARTRIDGE"), scrape_cartridge);
		s->addSaveFunc([scrape_cartridge] { Settings::getInstance()->setBool("ScrapeCartridge", scrape_cartridge->getState()); });

		// SCRAPE MANUAL
		auto scrape_manual = std::make_shared<SwitchComponent>(mWindow);
		scrape_manual->setState(Settings::getInstance()->getBool("ScrapeManual"));
		s->addWithLabel(_("SCRAPE MANUAL"), scrape_manual);
		s->addSaveFunc([scrape_manual] { Settings::getInstance()->setBool("ScrapeManual", scrape_manual->getState()); });

		// SCRAPE PAD TO KEYBOARD
		//auto scrapePadToKey = std::make_shared<SwitchComponent>(mWindow);
		//scrapePadToKey->setState(Settings::getInstance()->getBool("ScrapePadToKey"));
		//s->addWithLabel(_("SCRAPE PADTOKEY SETTINGS"), scrapePadToKey);
		//s->addSaveFunc([scrapePadToKey] { Settings::getInstance()->setBool("ScrapePadToKey", scrapePadToKey->getState()); });

		// Account
		s->addInputTextRow(_("USERNAME"), "ScreenScraperUser", false, true);
		s->addInputTextRow(_("PASSWORD"), "ScreenScraperPass", true, true);
	}
	else
	{
		std::string imageSourceName = Settings::getInstance()->getString("ScrapperImageSrc");
		auto imageSource = std::make_shared< OptionListComponent<std::string> >(mWindow, _("IMAGE SOURCE"), false);

		// Image source : <image> tag

		imageSource->add(_("SCREENSHOT"), "ss", imageSourceName == "ss");
		imageSource->add(_("TITLE SCREENSHOT"), "sstitle", imageSourceName == "sstitle");
		imageSource->add(_("BOX 2D"), "box-2D", imageSourceName == "box-2D");
		imageSource->add(_("FAN ART"), "fanart", imageSourceName == "fanart");

		if (!imageSource->hasSelection())
			imageSource->selectFirstItem();

		s->addWithLabel(_("IMAGE SOURCE"), imageSource);
		s->addSaveFunc([imageSource] { Settings::getInstance()->setString("ScrapperImageSrc", imageSource->getSelected()); });

		// Box source : <thumbnail> tag
		std::string thumbSourceName = Settings::getInstance()->getString("ScrapperThumbSrc");
		auto thumbSource = std::make_shared< OptionListComponent<std::string> >(mWindow, _("BOX SOURCE"), false);
		thumbSource->add(_("NONE"), "", thumbSourceName.empty());
		thumbSource->add(_("BOX 2D"), "box-2D", thumbSourceName == "box-2D");

		if (scraper == "HfsDB")
			thumbSource->add(_("BOX 3D"), "box-3D", thumbSourceName == "box-3D");

		if (!thumbSource->hasSelection())
			thumbSource->selectFirstItem();

		s->addWithLabel(_("BOX SOURCE"), thumbSource);
		s->addSaveFunc([thumbSource] { Settings::getInstance()->setString("ScrapperThumbSrc", thumbSource->getSelected()); });

		imageSource->setSelectedChangedCallback([this, thumbSource](std::string value)
		{
			if (value == "box-2D")
				thumbSource->remove(_("BOX 2D"));
			else
				thumbSource->add(_("BOX 2D"), "box-2D", false);
		});

		// Logo source : <marquee> tag
		std::string logoSourceName = Settings::getInstance()->getString("ScrapperLogoSrc");
		auto logoSource = std::make_shared< OptionListComponent<std::string> >(mWindow, _("LOGO SOURCE"), false);
		logoSource->add(_("NONE"), "", logoSourceName.empty());
		logoSource->add(_("WHEEL"), "wheel", logoSourceName == "wheel");

		if (scraper == "HfsDB")
			logoSource->add(_("MARQUEE"), "marquee", logoSourceName == "marquee");

		if (!logoSource->hasSelection())
			logoSource->selectFirstItem();

		s->addWithLabel(_("LOGO SOURCE"), logoSource);
		s->addSaveFunc([logoSource] { Settings::getInstance()->setString("ScrapperLogoSrc", logoSource->getSelected()); });

		if (scraper == "TheGamesDB")
		{
			// SCRAPE FANART
			auto scrape_fanart = std::make_shared<SwitchComponent>(mWindow);
			scrape_fanart->setState(Settings::getInstance()->getBool("ScrapeFanart"));
			s->addWithLabel(_("SCRAPE FANART"), scrape_fanart);
			s->addSaveFunc([scrape_fanart] { Settings::getInstance()->setBool("ScrapeFanart", scrape_fanart->getState()); });

			// SCRAPE BOX BACKSIDE
			auto scrape_boxBack = std::make_shared<SwitchComponent>(mWindow);
			scrape_boxBack->setState(Settings::getInstance()->getBool("ScrapeBoxBack"));
			s->addWithLabel(_("SCRAPE BOX BACKSIDE"), scrape_boxBack);
			s->addSaveFunc([scrape_boxBack] { Settings::getInstance()->setBool("ScrapeBoxBack", scrape_boxBack->getState()); });
		}
		else if (scraper == "HfsDB")
		{
			// SCRAPE FANART
			auto scrape_fanart = std::make_shared<SwitchComponent>(mWindow);
			scrape_fanart->setState(Settings::getInstance()->getBool("ScrapeFanart"));
			s->addWithLabel(_("SCRAPE FANART"), scrape_fanart);
			s->addSaveFunc([scrape_fanart] { Settings::getInstance()->setBool("ScrapeFanart", scrape_fanart->getState()); });

			// scrape video
			auto scrape_video = std::make_shared<SwitchComponent>(mWindow);
			scrape_video->setState(Settings::getInstance()->getBool("ScrapeVideos"));
			s->addWithLabel(_("SCRAPE VIDEOS"), scrape_video);
			s->addSaveFunc([scrape_video] { Settings::getInstance()->setBool("ScrapeVideos", scrape_video->getState()); });

			// SCRAPE BOX BACKSIDE
			auto scrape_boxBack = std::make_shared<SwitchComponent>(mWindow);
			scrape_boxBack->setState(Settings::getInstance()->getBool("ScrapeBoxBack"));
			s->addWithLabel(_("SCRAPE BOX BACKSIDE"), scrape_boxBack);
			s->addSaveFunc([scrape_boxBack] { Settings::getInstance()->setBool("ScrapeBoxBack", scrape_boxBack->getState()); });

			// SCRAPE MANUAL
			auto scrape_manual = std::make_shared<SwitchComponent>(mWindow);
			scrape_manual->setState(Settings::getInstance()->getBool("ScrapeManual"));
			s->addWithLabel(_("SCRAPE MANUAL"), scrape_manual);
			s->addSaveFunc([scrape_manual] { Settings::getInstance()->setBool("ScrapeManual", scrape_manual->getState()); });
		}
		else
		{		// scrape video
			auto scrape_video = std::make_shared<SwitchComponent>(mWindow);
			scrape_video->setState(Settings::getInstance()->getBool("ScrapeVideos"));
			s->addWithLabel(_("SCRAPE VIDEOS"), scrape_video);
			s->addSaveFunc([scrape_video] { Settings::getInstance()->setBool("ScrapeVideos", scrape_video->getState()); });
		}
	}

	scraper_list->setSelectedChangedCallback([this, s, scraper, scraper_list](std::string value)
	{
		if (value != scraper)
		{
			Settings::getInstance()->setString("Scraper", value);
			delete s;
			openScraperSettings();
		}
	});

	mWindow->pushGui(s);
}

void GuiMenu::openConfigInput()
{
	Window* window = mWindow;
	window->pushGui(new GuiMsgBox(window, _("ARE YOU SURE YOU WANT TO CONFIGURE THE INPUT?"),
		_("YES"), [window] { window->pushGui(new GuiDetectDevice(window, false, nullptr)); },
		_("NO"), nullptr)
	);
}

void GuiMenu::addVersionInfo()
{
	std::string  buildDate = (Settings::getInstance()->getBool("Debug") ? std::string( "   (" + Utils::String::toUpper(PROGRAM_BUILT_STRING) + ")") : (""));

	auto theme = ThemeData::getMenuTheme();

	mVersion.setFont(theme->Footer.font);
	mVersion.setColor(theme->Footer.color);

	mVersion.setLineSpacing(0);

	if (!ApiSystem::getInstance()->getVersion().empty())
	{
#if WIN32
		std::string aboutInfo = ApiSystem::getInstance()->getApplicationName()+ " V"+ ApiSystem::getInstance()->getVersion();
		if (!aboutInfo.empty())
			mVersion.setText(aboutInfo + buildDate);
		else
#endif
		mVersion.setText(ApiSystem::getInstance()->getApplicationName());
	}

	mVersion.setHorizontalAlignment(ALIGN_CENTER);
	mVersion.setVerticalAlignment(ALIGN_CENTER);
	addChild(&mVersion);
}

void GuiMenu::openScreensaverOptions()
{
	mWindow->pushGui(new GuiGeneralScreensaverOptions(mWindow));
}
void GuiMenu::openCollectionSystemSettings()
{
	if (ThreadedScraper::isRunning() || ThreadedHasher::isRunning())
	{
		mWindow->pushGui(new GuiMsgBox(mWindow, _("THIS FUNCTION IS DISABLED WHILE THE SCRAPER IS RUNNING")));
		return;
	}

	mWindow->pushGui(new GuiCollectionSystemsOptions(mWindow));
}

void GuiMenu::onSizeChanged()
{
	float h = mMenu.getButtonGridHeight();

	mVersion.setSize(mSize.x(), h);
	mVersion.setPosition(0, mSize.y() - h); //  mVersion.getSize().y()
}

void GuiMenu::addEntry(std::string name, bool add_arrow, const std::function<void()>& func, const std::string iconName)
{
	auto theme = ThemeData::getMenuTheme();
	std::shared_ptr<Font> font = theme->Text.font;
	unsigned int color = theme->Text.color;

	// populate the list
	ComponentListRow row;

	if (!iconName.empty())
	{
		std::string iconPath = theme->getMenuIcon(iconName);
		if (!iconPath.empty())
		{
			// icon
			auto icon = std::make_shared<ImageComponent>(mWindow, true);
			icon->setImage(iconPath);
			icon->setColorShift(theme->Text.color);
			icon->setResize(0, theme->Text.font->getLetterHeight() * 1.25f);
			row.addElement(icon, false);

			// spacer between icon and text
			auto spacer = std::make_shared<GuiComponent>(mWindow);
			spacer->setSize(10, 0);
			row.addElement(spacer, false);
		}
	}

	auto text = std::make_shared<TextComponent>(mWindow, name, font, color);
	row.addElement(text, true);

	if (EsLocale::isRTL())
		text->setHorizontalAlignment(Alignment::ALIGN_RIGHT);

	if (add_arrow)
	{
		std::shared_ptr<ImageComponent> bracket = makeArrow(mWindow);

		if (EsLocale::isRTL())
			bracket->setFlipX(true);

		row.addElement(bracket, false);
	}

	row.makeAcceptInputHandler(func);
	mMenu.addRow(row);
}

bool GuiMenu::input(InputConfig* config, Input input)
{
	if(GuiComponent::input(config, input))
		return true;

	if((config->isMappedTo(BUTTON_BACK, input) || config->isMappedTo("start", input)) && input.value != 0)
	{
		delete this;
		return true;
	}

	return false;
}

std::vector<HelpPrompt> GuiMenu::getHelpPrompts()
{
	std::vector<HelpPrompt> prompts;
	prompts.push_back(HelpPrompt("up/down", _("CHOOSE"))); // batocera
	prompts.push_back(HelpPrompt(BUTTON_OK, _("SELECT"))); // batocera
	prompts.push_back(HelpPrompt("start", _("CLOSE"))); // batocera
	return prompts;
}

class ExitKidModeMsgBox : public GuiSettings
{
	public: ExitKidModeMsgBox(Window* window, const std::string& title, const std::string& text) : GuiSettings(window, title) { addEntry(text); }

	bool input(InputConfig* config, Input input) override
	{
		if (UIModeController::getInstance()->listen(config, input))
		{
			mWindow->pushGui(new GuiMsgBox(mWindow, _("THE UI MODE IS NOW UNLOCKED"),
				_("OK"), [this]
				{
					Window* window = mWindow;
					while (window->peekGui() && window->peekGui() != ViewController::get())
						delete window->peekGui();
				}));


			return true;
		}

		return GuiComponent::input(config, input);
	}
};

void GuiMenu::exitKidMode()
{
	mWindow->pushGui(new ExitKidModeMsgBox(mWindow, _("UNLOCK UI MODE"), _("ENTER THE CODE TO UNLOCK THE CURRENT UI MODE")));
}

void GuiMenu::openSystemInformations_batocera()
{
	auto theme = ThemeData::getMenuTheme();
	std::shared_ptr<Font> font = theme->Text.font;
	unsigned int color = theme->Text.color;

	Window *window = mWindow;
	bool isFullUI = UIModeController::getInstance()->isUIModeFull();
	GuiSettings *informationsGui = new GuiSettings(window, _("INFORMATION").c_str());

	// various informations
	std::vector<std::string> infos = ApiSystem::getInstance()->getSystemInformations();
	for (auto it = infos.begin(); it != infos.end(); it++) {
		std::vector<std::string> tokens = Utils::String::split(*it, ':');

		if (tokens.size() >= 2) {
			// concatenat the ending words
			std::string vname = "";
			for (unsigned int i = 1; i < tokens.size(); i++) {
				if (i > 1) vname += " ";
				vname += tokens.at(i);
			}

			auto space = std::make_shared<TextComponent>(window,
				vname,
				font,
				color);
			informationsGui->addWithLabel(_(tokens.at(0).c_str()), space);
		}
	}

	informationsGui->addGroup(_("VIDEO DRIVER"));
	for (auto info : Renderer::getDriverInformation())
	{
		auto glversion = std::make_shared<TextComponent>(window, info.second, font, color);
		informationsGui->addWithLabel(_(info.first.c_str()), glversion);
	}

	window->pushGui(informationsGui);
}
void GuiMenu::openDecorationConfiguration(Window *mWindow, std::string configName, std::vector<DecorationSetInfo> sets)
{
	//Using a shared pointer to ensure the memory doesn't cause issues in the other class
	std::map<std::string, std::string> decorationSetNameToPath;
	for (auto set : sets)
	{
		decorationSetNameToPath.insert(std::make_pair(set.name, set.path));
	}

	auto decorationOptions = new GuiDecorationOptions(mWindow, configName, decorationSetNameToPath);
	mWindow->pushGui(decorationOptions);
}

bool GuiMenu::checkNetwork()
{
	if (ApiSystem::getInstance()->getIpAdress() == "NOT CONNECTED")
	{
		mWindow->pushGui(new GuiMsgBox(mWindow, _("YOU ARE NOT CONNECTED TO A NETWORK"), _("OK"), nullptr));
		return false;
	}

	return true;
}

void GuiMenu::openSystemSettings_batocera()
{
	Window *window = mWindow;

	auto s = new GuiSettings(mWindow, _("SYSTEM SETTINGS").c_str());
	bool isFullUI = UIModeController::getInstance()->isUIModeFull();

	s->addGroup(_("SYSTEM"));

	// System informations
	s->addEntry(_("INFORMATION"), true, [this] { openSystemInformations_batocera(); });

	auto emuelec_timezones = std::make_shared<OptionListComponent<std::string> >(mWindow, _("TIMEZONE"), false);
	std::string currentTimezone = SystemConf::getInstance()->get("system.timezone");
	if (currentTimezone.empty())
		currentTimezone = std::string(getShOutput(R"(/usr/bin/timeinfo current_timezone)"));
	std::string a;
	for(std::stringstream ss(getShOutput(R"(/usr/bin/timeinfo timezones)")); getline(ss, a, ','); ) {
		emuelec_timezones->add(a, a, currentTimezone == a); // emuelec
	}
	s->addWithLabel(_("TIMEZONE"), emuelec_timezones);
	s->addSaveFunc([emuelec_timezones] {
		if (emuelec_timezones->changed()) {
			std::string selectedTimezone = emuelec_timezones->getSelected();
			runSystemCommand("ln -sf /usr/share/zoneinfo/" + selectedTimezone + " $(readlink /etc/localtime)", "", nullptr);
		}
		SystemConf::getInstance()->set("system.timezone", emuelec_timezones->getSelected());
	});

	// language choice
	/*
	auto language_choice = std::make_shared<OptionListComponent<std::string> >(window, _("LANGUAGE"), false);

	std::string language = SystemConf::getInstance()->get("system.language");
	if (language.empty())
		language = "en_US";

	language_choice->add("ARABIC",               "ar_YE", language == "ar_YE");
	language_choice->add("CATALÀ",               "ca_ES", language == "ca_ES");
	language_choice->add("CYMRAEG",              "cy_GB", language == "cy_GB");
	language_choice->add("DEUTSCH", 	     "de_DE", language == "de_DE");
	language_choice->add("GREEK",                "el_GR", language == "el_GR");
	language_choice->add("ENGLISH", 	     "en_US", language == "en_US" || language == "en");
	language_choice->add("ESPAÑOL", 	     "es_ES", language == "es_ES" || language == "es");
	language_choice->add("ESPAÑOL MEXICANO",     "es_MX", language == "es_MX");
	language_choice->add("BASQUE",               "eu_ES", language == "eu_ES");
	language_choice->add("FRANÇAIS",             "fr_FR", language == "fr_FR" || language == "fr");
	language_choice->add("עברית",                "he_IL", language == "he_IL");
	language_choice->add("HUNGARIAN",            "hu_HU", language == "hu_HU");
	language_choice->add("ITALIANO",             "it_IT", language == "it_IT");
	language_choice->add("JAPANESE", 	     "ja_JP", language == "ja_JP");
	language_choice->add("KOREAN",   	     "ko_KR", language == "ko_KR" || language == "ko");
	language_choice->add("NORWEGIAN BOKMAL",     "nb_NO", language == "nb_NO");
	language_choice->add("DUTCH",                "nl_NL", language == "nl_NL");
	language_choice->add("NORWEGIAN",            "nn_NO", language == "nn_NO");
	language_choice->add("OCCITAN",              "oc_FR", language == "oc_FR");
	language_choice->add("POLISH",               "pl_PL", language == "pl_PL");
	language_choice->add("PORTUGUES BRASILEIRO", "pt_BR", language == "pt_BR");
	language_choice->add("PORTUGUES PORTUGAL",   "pt_PT", language == "pt_PT");
	language_choice->add("РУССКИЙ",              "ru_RU", language == "ru_RU");
	language_choice->add("SVENSKA", 	     "sv_SE", language == "sv_SE");
	language_choice->add("TÜRKÇE",  	     "tr_TR", language == "tr_TR");
	language_choice->add("Українська",           "uk_UA", language == "uk_UA");
	language_choice->add("简体中文", 	     "zh_CN", language == "zh_CN");
	language_choice->add("正體中文", 	     "zh_TW", language == "zh_TW");
	s->addWithLabel(_("LANGUAGE"), language_choice);
	*/

#if !defined(_ENABLEEMUELEC)
	// Timezone
#if !defined(WIN32) || defined(_DEBUG)
	auto availableTimezones = ApiSystem::getInstance()->getTimezones();
	if (availableTimezones.size() > 0)
	{
		std::string currentTZ = ApiSystem::getInstance()->getCurrentTimezone();

		bool valid_tz = false;
		for (auto list_tz : availableTimezones){
			if (currentTZ == list_tz) {
				valid_tz = true;
			}
		}
		if (!valid_tz)
			currentTZ = "Europe/Paris";

		auto tzChoices= std::make_shared<OptionListComponent<std::string> >(mWindow, _("SELECT YOUR TIME ZONE"), false);

		for (auto tz : availableTimezones)
			tzChoices->add(_(Utils::String::toUpper(tz).c_str()), tz, currentTZ == tz);

		s->addWithLabel(_("TIME ZONE"), tzChoices);
		s->addSaveFunc([tzChoices] {
				SystemConf::getInstance()->set("system.timezone", tzChoices->getSelected());
				ApiSystem::getInstance()->setTimezone(tzChoices->getSelected());
				});
	}
#endif
#endif
	// Clock time format (14:42 or 2:42 pm)
	auto tmFormat = std::make_shared<SwitchComponent>(mWindow);
	tmFormat->setState(Settings::getInstance()->getBool("ClockMode12"));
	s->addWithLabel(_("SHOW CLOCK IN 12-HOUR FORMAT"), tmFormat);
	s->addSaveFunc([tmFormat] { Settings::getInstance()->setBool("ClockMode12", tmFormat->getState()); });

	// power saver
	/*
	auto power_saver = std::make_shared< OptionListComponent<std::string> >(mWindow, _("POWER SAVER MODES"), false);
	power_saver->addRange({ { _("DISABLED"), "disabled" }, { _("DEFAULT"), "default" }, { _("ENHANCED"), "enhanced" }, { _("INSTANT"), "instant" }, }, Settings::PowerSaverMode());
	s->addWithLabel(_("POWER SAVER MODES"), power_saver);
	s->addSaveFunc([this, power_saver]
	{
		if (Settings::PowerSaverMode() != "instant" && power_saver->getSelected() == "instant")
			Settings::getInstance()->setBool("EnableSounds", false);

		Settings::setPowerSaverMode(power_saver->getSelected());
		PowerSaver::init();
	});
	*/

#if defined(_ENABLE_TTS_) || defined(WIN32)
	if (TextToSpeech::getInstance()->isAvailable())
	{
			// tts
		auto tts = std::make_shared<SwitchComponent>(mWindow);
		tts->setState(Settings::getInstance()->getBool("TTS"));
		s->addWithLabel(_("SCREEN READER (TEXT TO SPEECH)"), tts);
		s->addSaveFunc([tts] {
			 if(TextToSpeech::getInstance()->isEnabled() != tts->getState()) {
			   TextToSpeech::getInstance()->enable(tts->getState());
			   Settings::getInstance()->setBool("TTS", tts->getState());
			 }});
	}
#endif

	// UI RESTRICTIONS
	/*
	auto UImodeSelection = std::make_shared< OptionListComponent<std::string> >(mWindow, _("UI MODE"), false);
	std::vector<std::string> UImodes = UIModeController::getInstance()->getUIModes();
	for (auto it = UImodes.cbegin(); it != UImodes.cend(); it++)
		UImodeSelection->add(_(it->c_str()), *it, Settings::getInstance()->getString("UIMode") == *it);

	s->addWithLabel(_("UI MODE"), UImodeSelection);
	s->addSaveFunc([UImodeSelection, window]
	{
		std::string selectedMode = UImodeSelection->getSelected();
		if (selectedMode != "Full")
		{
			std::string msg = _("You are changing the UI to a restricted mode:\nThis will hide most menu options to prevent changes to the system.\nTo unlock and return to the full UI, enter this code:") + "\n";
			msg += "\"" + UIModeController::getInstance()->getFormattedPassKeyStr() + "\"\n\n";
			msg += _("Do you want to proceed?");
			window->pushGui(new GuiMsgBox(window, msg,
				_("YES"), [selectedMode] {
				LOG(LogDebug) << "Setting UI mode to " << selectedMode;
				Settings::getInstance()->setString("UIMode", selectedMode);
				Settings::getInstance()->saveFile();
			}, _("NO"), nullptr));
		}
	});
	*/

	// KODI SETTINGS
#ifdef _ENABLE_KODI_
	if (ApiSystem::getInstance()->isScriptingSupported(ApiSystem::KODI))
	{
		s->addEntry(_("KODI SETTINGS"), true, [this]
		{
			GuiSettings* kodiGui = new GuiSettings(mWindow, _("KODI SETTINGS").c_str());

			auto kodiEnabled = std::make_shared<SwitchComponent>(mWindow);
			kodiEnabled->setState(SystemConf::getInstance()->getBool("kodi.enabled", true));
			kodiGui->addWithLabel(_("ENABLE KODI"), kodiEnabled);

			auto kodiAtStart = std::make_shared<SwitchComponent>(mWindow);
			kodiAtStart->setState(SystemConf::getInstance()->getBool("kodi.atstartup"));
			kodiGui->addWithLabel(_("LAUNCH KODI AT BOOT"), kodiAtStart);

			kodiGui->addSaveFunc([kodiEnabled, kodiAtStart]
			{
				SystemConf::getInstance()->setBool("kodi.enabled", kodiEnabled->getState());
				SystemConf::getInstance()->setBool("kodi.atstartup", kodiAtStart->getState());
			});

			mWindow->pushGui(kodiGui);
		});
	}
#endif

        s->addGroup(_("AUTHENTICATION"));
        bool rotateRootPassEnabled = SystemConf::getInstance()->getBool("rotate.root.password");
        auto rotate_root_pass = std::make_shared<SwitchComponent>(mWindow);
        rotate_root_pass->setState(rotateRootPassEnabled);
        s->addWithLabel(_("ROTATE ROOT PASSWORD"), rotate_root_pass);

        s->addSaveFunc([this, rotate_root_pass]
        {
                SystemConf::getInstance()->setBool("rotate.root.password", rotate_root_pass->getState());
                SystemConf::getInstance()->saveSystemConf();
        });

        auto root_password = std::make_shared<TextComponent>(mWindow, SystemConf::getInstance()->get("root.password"), ThemeData::getMenuTheme()->Text.font, ThemeData::getMenuTheme()->Text.color);
        s->addInputTextRow(_("ROOT PASSWORD"), "root.password", false);

        s->addSaveFunc([this, root_password] {
                SystemConf::getInstance()->saveSystemConf();
                const std::string rootpass = SystemConf::getInstance()->get("root.password");
                runSystemCommand("setrootpass " + rootpass, "", nullptr);
        });

	s->addGroup(_("DISPLAY"));


	if (BrightnessControl::getInstance()->isAvailable())
	{
		// brightness
		int brightness = BrightnessControl::getInstance()->getBrightness();
		auto brightnessComponent = std::make_shared<SliderComponent>(mWindow, 1.f, 100.f, 1.f, "%");
		brightnessComponent->setValue(brightness);
		brightnessComponent->setOnValueChanged([](const float &newVal) { BrightnessControl::getInstance()->setBrightness((int)Math::round(newVal)); });

		s->addWithLabel(_("BRIGHTNESS"), brightnessComponent);

		auto brightnessPopup = std::make_shared<SwitchComponent>(mWindow);
		brightnessPopup->setState(Settings::getInstance()->getBool("BrightnessPopup"));
		s->addWithLabel(_("SHOW OVERLAY WHEN BRIGHTNESS CHANGES"), brightnessPopup);
		s->addSaveFunc([brightnessPopup]
			{
				bool old_value = Settings::getInstance()->getBool("BrightnessPopup");
				if (old_value != brightnessPopup->getState())
					Settings::getInstance()->setBool("BrightnessPopup", brightnessPopup->getState());
			}
		);

#if defined(RK3566) || defined(RK3566_X55)

                // gamma
                auto gamma = std::make_shared<SliderComponent>(mWindow, 1.f, 100.f, 1.f, "%");
                gamma->setValue(std::stof(SystemConf::getInstance()->get("display.brightness")));
                gamma->setOnValueChanged([](const float &newVal) {
                        runSystemCommand("paneladj brightness " + std::to_string((int)round(newVal)),"", nullptr);
                });
                s->addWithLabel(_("GAMMA"), gamma);
                s->addSaveFunc([this, gamma]
                {
                        SystemConf::getInstance()->set("display.brightness", std::to_string((int)round(gamma->getValue())));
                        SystemConf::getInstance()->saveSystemConf();
                });


                // contrast
                auto contrast = std::make_shared<SliderComponent>(mWindow, 1.f, 100.f, 1.f, "%");
                contrast->setValue(std::stof(SystemConf::getInstance()->get("display.contrast")));
                contrast->setOnValueChanged([](const float &newVal) {
			runSystemCommand("paneladj contrast " + std::to_string((int)round(newVal)),"", nullptr);
		});
                s->addWithLabel(_("CONTRAST"), contrast);
                s->addSaveFunc([this, contrast]
                {
                        SystemConf::getInstance()->set("display.contrast", std::to_string((int)round(contrast->getValue())));
                        SystemConf::getInstance()->saveSystemConf();
                });

                // saturation
                auto saturation = std::make_shared<SliderComponent>(mWindow, 1.f, 100.f, 1.f, "%");
                saturation->setValue(std::stof(SystemConf::getInstance()->get("display.saturation")));
                saturation->setOnValueChanged([](const float &newVal) { 
			runSystemCommand("paneladj saturation " + std::to_string((int)round(newVal)),"", nullptr);
		});
                s->addWithLabel(_("SATURATION"), saturation);
                s->addSaveFunc([this, saturation]
                {
                        SystemConf::getInstance()->set("display.saturation", std::to_string((int)round(saturation->getValue())));
                        SystemConf::getInstance()->saveSystemConf();
                });

                // hue
                auto hue = std::make_shared<SliderComponent>(mWindow, 1.f, 100.f, 1.f, "%");
                hue->setValue(std::stof(SystemConf::getInstance()->get("display.hue")));
                hue->setOnValueChanged([](const float &newVal) { 
			runSystemCommand("paneladj hue " + std::to_string((int)round(newVal)),"", nullptr);
		});
                s->addWithLabel(_("HUE"), hue);
                s->addSaveFunc([this, hue]
                {
                        SystemConf::getInstance()->set("display.hue", std::to_string((int)round(hue->getValue())));
                        SystemConf::getInstance()->saveSystemConf();
                });
#endif
	}

	bool deviceDisablePLed = getenv("DEVICE_PWR_LED_CONTROL");
	if (deviceDisablePLed == true) {

		s->addGroup(_("DEVICE LEDS"));
		// Disable Power LED
		auto pwr_led_disabled = std::make_shared<SwitchComponent>(mWindow);
		bool pwrleddisabled = SystemConf::getInstance()->get("powerled.disabled") == "1";
		pwr_led_disabled->setState(SystemConf::getInstance()->getBool("powerled.disabled"));
		s->addWithLabel(_("DISABLE POWER LED"), pwr_led_disabled);
		s->addSaveFunc([pwr_led_disabled] {
        	bool pwrleddisabled = pwr_led_disabled->getState();
			SystemConf::getInstance()->set("powerled.disabled", pwrleddisabled ? "1" : "0");
			SystemConf::getInstance()->saveSystemConf(); });
	}
	
#if defined(AMD64)

        char* deviceLedControl = getenv("DEVICE_LED_CONTROL");
        if (deviceLedControl) {
		s->addGroup(_("DEVICE LEDS"));
		// Provides LED management
		auto optionsLEDProfile = std::make_shared<OptionListComponent<std::string> >(mWindow, _("LED COLOR"), false);
		std::string selectedLEDProfile = SystemConf::getInstance()->get("led.color");
		if (selectedLEDProfile.empty())
			selectedLEDProfile = "default";

		optionsLEDProfile->add(_("DEFAULT (REBOOT)"),"default", selectedLEDProfile == "default");
		optionsLEDProfile->add(_("OFF"),"off", selectedLEDProfile == "off");
		optionsLEDProfile->add(_("RED"),"red", selectedLEDProfile == "red");
		optionsLEDProfile->add(_("GREEN"),"green", selectedLEDProfile == "green");
		optionsLEDProfile->add(_("BLUE"),"blue", selectedLEDProfile == "blue");
		optionsLEDProfile->add(_("TEAL"),"teal", selectedLEDProfile == "teal");
		optionsLEDProfile->add(_("PURPLE"),"purple", selectedLEDProfile == "purple");
		s->addWithLabel(_("LED COLOR"), optionsLEDProfile);

		s->addSaveFunc([this, optionsLEDProfile, selectedLEDProfile]
		{
			if (optionsLEDProfile->changed()) {
				SystemConf::getInstance()->set("led.color", optionsLEDProfile->getSelected());
				SystemConf::getInstance()->saveSystemConf();
				runSystemCommand("/usr/bin/ledcontrol " + optionsLEDProfile->getSelected(), "", nullptr);
			}
		});

	        // Sets LED brightness
	        auto optionsLEDBrightness = std::make_shared<OptionListComponent<std::string> >(mWindow, _("LED BRIGHTNESS"), false);
	        std::string selectedLEDBrightness = SystemConf::getInstance()->get("led.brightness");
	        if (selectedLEDBrightness.empty())
	                selectedLEDBrightness = "max";

	        optionsLEDBrightness->add(_("MAX"),"max", selectedLEDBrightness == "max");
	        optionsLEDBrightness->add(_("MID"),"mid", selectedLEDBrightness == "mid");
	        optionsLEDBrightness->add(_("MIN"),"min", selectedLEDBrightness == "min");
	        s->addWithLabel(_("LED BRIGHTNESS"), optionsLEDBrightness);
	
	        s->addSaveFunc([this, optionsLEDBrightness, selectedLEDBrightness]
	        {
	                if (optionsLEDBrightness->changed()) {
	                        SystemConf::getInstance()->set("led.brightness", optionsLEDBrightness->getSelected());
	                        SystemConf::getInstance()->saveSystemConf();
	                        runSystemCommand("/usr/bin/ledcontrol brightness " + optionsLEDBrightness->getSelected(), "", nullptr);
	                }
	        });
	}

#endif

	s->addGroup(_("HARDWARE / AUDIO"));

	// audio device
	auto optionsAudio = std::make_shared<OptionListComponent<std::string> >(mWindow, _("AUDIO DEVICE"), false);

	std::vector<std::string> availableAudio = ApiSystem::getInstance()->getAvailableAudioOutputDevices();
	std::string selectedAudio = ApiSystem::getInstance()->getCurrentAudioOutputDevice();

	bool vfound = false;
	for (auto it = availableAudio.begin(); it != availableAudio.end(); it++)
	{
		optionsAudio->add((*it), (*it), selectedAudio == (*it));
		if (selectedAudio == (*it))
			vfound = true;
	}
	if (!vfound)
		optionsAudio->add(selectedAudio, selectedAudio, true);

	s->addWithLabel(_("AUDIO DEVICE"), optionsAudio);

	s->addSaveFunc([this, optionsAudio, selectedAudio]
	{
		if (optionsAudio->changed()) {
			std::string msg = _("Emulationstation will restart")+"\n";
			msg += _("Do you want to proceed ?");
			mWindow->pushGui(new GuiMsgBox(mWindow, msg, _("YES"), [optionsAudio] {
				ApiSystem::getInstance()->setAudioOutputDevice(optionsAudio->getSelected());
				quitES(QuitMode::QUIT);
			}, "NO",nullptr));
		}
		SystemConf::getInstance()->saveSystemConf();
	});

        // audio path
        auto AudioPath = std::make_shared<OptionListComponent<std::string> >(mWindow, _("AUDIO PATH"), false);

        std::vector<std::string> availablePaths = ApiSystem::getInstance()->getAvailableAudioOutputPaths();
        std::string selectedPath = ApiSystem::getInstance()->getCurrentAudioOutputPath();

        bool afound = false;
        for (auto it = availablePaths.begin(); it != availablePaths.end(); it++)
        {
                AudioPath->add((*it), (*it), selectedPath == (*it));
                if (selectedPath == (*it))
                        afound = true;
        }
        if (!afound)
                AudioPath->add(selectedPath, selectedPath, true);

        s->addWithLabel(_("AUDIO PATH"), AudioPath);

        s->addSaveFunc([this, AudioPath, selectedPath]
        {
                if (AudioPath->changed()) {
                        std::string msg = _("Emulationstation will restart")+"\n";
                        msg += _("Do you want to proceed ?");
                        mWindow->pushGui(new GuiMsgBox(mWindow, msg, _("YES"), [AudioPath] {
                                ApiSystem::getInstance()->setAudioOutputPath(AudioPath->getSelected());
                                quitES(QuitMode::QUIT);
                        }, "NO",nullptr));
                }
                SystemConf::getInstance()->saveSystemConf();
        });


	s->addGroup(_("HARDWARE / CPU"));
	
#if defined(AMD64)
        // Allow offlining all but n threads
	auto optionsThreads = std::make_shared<OptionListComponent<std::string> >(mWindow, _("AVAILABLE THREADS"), false);

	std::vector<std::string> availableThreads = ApiSystem::getInstance()->getAvailableThreads();
	std::string selectedThreads = SystemConf::getInstance()->get("system.threads");
	if (selectedThreads.empty())
		selectedThreads = "all";

	bool wfound = false;
	for (auto it = availableThreads.begin(); it != availableThreads.end(); it++)
	{
		optionsThreads->add((*it), (*it), selectedThreads == (*it));
		if (selectedThreads == (*it))
		wfound = true;
	}
	if (!wfound)
		optionsThreads->add(selectedThreads, selectedThreads, true);

	s->addWithLabel(_("AVAILABLE THREADS"), optionsThreads);

	s->addSaveFunc([this, optionsThreads, selectedThreads]
	{
		if (optionsThreads->changed()) {
			SystemConf::getInstance()->set("system.threads", optionsThreads->getSelected());
			runSystemCommand("/usr/bin/bash -lc \". /etc/profile; onlinethreads " + optionsThreads->getSelected() + " 0" + "\"" , "", nullptr);
			SystemConf::getInstance()->saveSystemConf();
		}
	});

#endif

	bool deviceHasFan = getenv("DEVICE_HAS_FAN");
	if (deviceHasFan == true) {
	  // Provides cooling profile switching
	  auto optionsFanProfile = std::make_shared<OptionListComponent<std::string> >(mWindow, _("COOLING PROFILE"), false);
	  std::string selectedFanProfile = SystemConf::getInstance()->get("cooling.profile");
	  if (selectedFanProfile.empty())
		selectedFanProfile = "quiet";

	  optionsFanProfile->add(_("QUIET"),"quiet", selectedFanProfile == "quiet");
	  optionsFanProfile->add(_("MODERATE"),"moderate", selectedFanProfile == "moderate");
	  optionsFanProfile->add(_("AGGRESSIVE"),"aggressive", selectedFanProfile == "aggressive");
	  optionsFanProfile->add(_("CUSTOM"),"custom", selectedFanProfile == "custom");

	  s->addWithLabel(_("COOLING PROFILE"), optionsFanProfile);

	  s->addSaveFunc([this, optionsFanProfile, selectedFanProfile]
	  {
	    if (optionsFanProfile->changed()) {
	      SystemConf::getInstance()->set("cooling.profile", optionsFanProfile->getSelected());
	      SystemConf::getInstance()->saveSystemConf();
	      runSystemCommand("systemctl restart fancontrol", "", nullptr);
	    }
	  });
	}

// Prep for additional device support.
#if defined(AMD64)
	// Provides overclock profile switching
	auto optionsOCProfile = std::make_shared<OptionListComponent<std::string> >(mWindow, _("CPU Max TDP (AMD Only)"), false);
	std::string selectedOCProfile = SystemConf::getInstance()->get("system.overclock");
	if (selectedOCProfile.empty())
		selectedOCProfile = "off";

	optionsOCProfile->add(_("OFF"),    "off", selectedOCProfile == "off");
        optionsOCProfile->add(_("2w"),"2w", selectedOCProfile == "2w");
        optionsOCProfile->add(_("4w"),"4w", selectedOCProfile == "4w");
        optionsOCProfile->add(_("6w"),"6w", selectedOCProfile == "6w");
        optionsOCProfile->add(_("8w"),"8w", selectedOCProfile == "8w");
        optionsOCProfile->add(_("10w"),"10w", selectedOCProfile == "10w");
        optionsOCProfile->add(_("12w"),"12w", selectedOCProfile == "12w");
        optionsOCProfile->add(_("14w"),"14w", selectedOCProfile == "14w");
        optionsOCProfile->add(_("16w"),"16w", selectedOCProfile == "16w");
        optionsOCProfile->add(_("18w"),"18w", selectedOCProfile == "18w");
        optionsOCProfile->add(_("20w"),"20w", selectedOCProfile == "20w");
        optionsOCProfile->add(_("22w"),"22w", selectedOCProfile == "22w");
        optionsOCProfile->add(_("24w"),"24w", selectedOCProfile == "24w");
 	s->addWithLabel(_("CPU Max TDP (AMD Only)"), optionsOCProfile);

	s->addSaveFunc([this, optionsOCProfile, selectedOCProfile]
	{
		if (optionsOCProfile->changed()) {
			mWindow->pushGui(new GuiMsgBox(mWindow, _("WARNING: OVERCLOCKING YOUR DEVICE MAY RESULT IN STABILITY PROBLEMS OR CAUSE HARDWARE DAMAGE!\n\nUSING THE QUIET COOLING PROFILE WHILE USING CERTAIN OVERCLOCKS MAY CAUSE PANIC REBOOTS!\n\nJELOS IS NOT RESPONSIBLE FOR ANY DAMAGE THAT MAY OCCUR USING THESE SETTINGS!\n\nCLICK YES THAT YOU AGREE, OR NO TO CANCEL."), _("YES"),
                                [this,optionsOCProfile] {
					SystemConf::getInstance()->set("system.overclock", optionsOCProfile->getSelected());
					SystemConf::getInstance()->saveSystemConf();
					runSystemCommand("/usr/bin/overclock", "", nullptr);
                                }, _("NO"), nullptr));
		}
	});
#endif

        // Default CPU governor

        auto cpuGovUpdate = std::make_shared<OptionListComponent<std::string> >(mWindow, _("DEFAULT CPU GOVERNOR"), false);

        std::string cpu_governor = SystemConf::getInstance()->get("system.cpugovernor");
        if (cpu_governor.empty())
                cpu_governor = "schedutil";

        cpuGovUpdate->add(_("SCHEDUTIL"), "schedutil", cpu_governor == "schedutil");
        cpuGovUpdate->add(_("ONDEMAND"), "ondemand", cpu_governor == "ondemand");
        cpuGovUpdate->add(_("PERFORMANCE"), "performance", cpu_governor == "performance");
        cpuGovUpdate->add(_("POWERSAVE"), "powersave", cpu_governor == "powersave");

        s->addWithLabel(_("DEFAULT CPU GOVERNOR"), cpuGovUpdate);

        s->addSaveFunc([this, cpuGovUpdate, cpu_governor]
        {
          if (cpuGovUpdate->changed()) {
            SystemConf::getInstance()->set("system.cpugovernor", cpuGovUpdate->getSelected());
            SystemConf::getInstance()->saveSystemConf();
          }
	  runSystemCommand("/usr/bin/bash -lc \". /etc/profile; "+ cpuGovUpdate->getSelected() + "\"", "", nullptr);
        });

	s->addGroup(_("HARDWARE / GPU"));
        // Automatically enable or disable enhanced power saving mode
        auto enh_powersave = std::make_shared<SwitchComponent>(mWindow);
        bool enhpowersaveEnabled = SystemConf::getInstance()->get("system.powersave") == "1";
        enh_powersave->setState(enhpowersaveEnabled);
        s->addWithLabel(_("ENHANCED POWER SAVING"), enh_powersave);
        s->addSaveFunc([enh_powersave] {
                bool enhpowersaveEnabled = enh_powersave->getState();
                SystemConf::getInstance()->set("system.powersave", enhpowersaveEnabled ? "1" : "0");
                SystemConf::getInstance()->saveSystemConf();
        });

        if (SystemConf::getInstance()->getBool("system.powersave", true)) {
        	// Options for enhanced power savings mode
        	auto enh_cpupowersave = std::make_shared<SwitchComponent>(mWindow);
        	bool enhcpupowersaveEnabled = SystemConf::getInstance()->get("system.power.cpu") == "1";
        	enh_cpupowersave->setState(enhcpupowersaveEnabled);
        	s->addWithLabel(_("CPU POWER SAVING"), enh_cpupowersave);
        	s->addSaveFunc([enh_cpupowersave] {
        	        bool enhcpupowersaveEnabled = enh_cpupowersave->getState();
               	 SystemConf::getInstance()->set("system.power.cpu", enhcpupowersaveEnabled ? "1" : "0");
               	 SystemConf::getInstance()->saveSystemConf();
        	});

#if defined(AMD64)
	          // GPU performance mode with enhanced power savings
	          auto gpuPerformance = std::make_shared<OptionListComponent<std::string> >(mWindow, _("GPU POWER SAVINGS MODE (AMD ONLY)"), false);
	          std::string gpu_performance = SystemConf::getInstance()->get("system.gpuperf");
	          if (gpu_performance.empty())
	                  gpu_performance = "auto";
	
	          gpuPerformance->add(_("AUTO"), "default", gpu_performance == "auto");
	          gpuPerformance->add(_("LOW"), "low", gpu_performance == "low");
	          gpuPerformance->add(_("STANDARD"), "profile_standard", gpu_performance == "profile_standard");
	          gpuPerformance->add(_("PEAK"), "profile_peak", gpu_performance == "profile_peak");

	          s->addWithLabel(_("GPU POWER SAVINGS MODE (AMD ONLY)"), gpuPerformance);
	          s->addSaveFunc([this, gpuPerformance, gpu_performance]
	          {
	            if (gpuPerformance->changed()) {
	              SystemConf::getInstance()->set("system.gpuperf", gpuPerformance->getSelected());
	              SystemConf::getInstance()->saveSystemConf();
	              runSystemCommand("/usr/bin/bash -lc \". /etc/profile; gpu_performance_level "+ gpuPerformance->getSelected() + "\"", "", nullptr);
	            }
	          });
#endif
	        auto enh_audiopowersave = std::make_shared<SwitchComponent>(mWindow);
	        bool enhaudiopowersaveEnabled = SystemConf::getInstance()->get("system.power.audio") == "1";
	        enh_audiopowersave->setState(enhaudiopowersaveEnabled);
	        s->addWithLabel(_("AUDIO POWER SAVING"), enh_audiopowersave);
	        s->addSaveFunc([enh_audiopowersave] {
	                bool enhaudiopowersaveEnabled = enh_audiopowersave->getState();
	                SystemConf::getInstance()->set("system.power.audio", enhaudiopowersaveEnabled ? "1" : "0");
	                SystemConf::getInstance()->saveSystemConf();
	        });
	        auto enh_pciepowersave = std::make_shared<SwitchComponent>(mWindow);
	        bool enhpciepowersaveEnabled = SystemConf::getInstance()->get("system.power.pcie") == "1";
	        enh_pciepowersave->setState(enhpciepowersaveEnabled);
	        s->addWithLabel(_("PCIE ACTIVE STATE POWER MANAGEMENT"), enh_pciepowersave);
	        s->addSaveFunc([enh_pciepowersave] {
	                bool enhpciepowersaveEnabled = enh_pciepowersave->getState();
	                SystemConf::getInstance()->set("system.power.pcie", enhpciepowersaveEnabled ? "1" : "0");
	                SystemConf::getInstance()->saveSystemConf();
	        });
	        auto wakeevents = std::make_shared<SwitchComponent>(mWindow);
	        bool wakeeventsEnabled = SystemConf::getInstance()->get("system.power.wakeevents") == "1";
	        wakeevents->setState(wakeeventsEnabled);
	        s->addWithLabel(_("ENABLE WAKE EVENTS"), wakeevents);
	        s->addSaveFunc([wakeevents] {
	                bool wakeeventsEnabled = wakeevents->getState();
	                SystemConf::getInstance()->set("system.power.wakeevents", wakeeventsEnabled ? "1" : "0");
	                SystemConf::getInstance()->saveSystemConf();
	        });
	        auto rtpm = std::make_shared<SwitchComponent>(mWindow);
	        bool rtpmEnabled = SystemConf::getInstance()->get("system.power.rtpm") == "1";
	        rtpm->setState(rtpmEnabled);
	        s->addWithLabel(_("RUNTIME POWER MANAGEMENT"), rtpm);
	        s->addSaveFunc([rtpm] {
	                bool rtpmEnabled = rtpm->getState();
	                SystemConf::getInstance()->set("system.power.rtpm", rtpmEnabled ? "1" : "0");
	                SystemConf::getInstance()->saveSystemConf();
	        });
	}

// Do not show on S922X devices yet.
#if defined(AMD64) || defined(RK3326) || defined(RK3566) || defined(RK3566_X55) || defined(RK3588) || defined(RK3399)
        // Allow user control over how the device sleeps
        s->addGroup(_("HARDWARE / SUSPEND"));
        auto systemSuspend = std::make_shared<OptionListComponent<std::string> >(mWindow, _("DEVICE SUSPEND MODE"), false);
        std::string sys_suspend = SystemConf::getInstance()->get("system.suspendmode");
        if (sys_suspend.empty())
                sys_suspend = "default";

        systemSuspend->add(_("FREEZE (S0)"), "freeze", sys_suspend == "freeze");
        systemSuspend->add(_("DEEP (S3)"), "mem", sys_suspend == "mem");
        s->addWithLabel(_("DEVICE SUSPEND MODE"), systemSuspend);

        s->addSaveFunc([this, systemSuspend, sys_suspend]
        {
          if (systemSuspend->changed()) {
            SystemConf::getInstance()->set("system.suspendmode", systemSuspend->getSelected());
            runSystemCommand("/usr/bin/setsuspendmode " + systemSuspend->getSelected(), "", nullptr);
            SystemConf::getInstance()->saveSystemConf();
          }
        });
#endif

	s->addGroup(_("HARDWARE / WIFI"));

        // Automatically enable or disable WIFI power saving mode
        auto wifi_powersave = std::make_shared<SwitchComponent>(mWindow);
        bool wifipowersaveEnabled = SystemConf::getInstance()->get("system.power.wifi") == "1";
        wifi_powersave->setState(wifipowersaveEnabled);
        s->addWithLabel(_("ENABLE WIFI POWER SAVING"), wifi_powersave);
        s->addSaveFunc([wifi_powersave] {
                bool wifipowersaveEnabled = wifi_powersave->getState();
                SystemConf::getInstance()->set("system.power.wifi", wifipowersaveEnabled ? "1" : "0");
		SystemConf::getInstance()->saveSystemConf();
                runSystemCommand("/usr/bin/wifictl setpowersave", "", nullptr);
        });

        s->addGroup(_("PREFERENCES"));

        // Provides a mechanism to disable use of the second device
        bool MountGamesEnabled = SystemConf::getInstance()->getBool("system.automount");
        auto mount_games = std::make_shared<SwitchComponent>(mWindow);
        mount_games->setState(MountGamesEnabled);
        s->addWithLabel(_("AUTODETECT GAMES CARD"), mount_games);
        s->addSaveFunc([mount_games] {
          SystemConf::getInstance()->setBool("system.automount", mount_games->getState());
        });

        // Provides a mechanism to disable automatic hotkey assignment
        bool HotKeysEnabled = SystemConf::getInstance()->getBool("system.autohotkeys");
        auto autohotkeys = std::make_shared<SwitchComponent>(mWindow);
        autohotkeys->setState(HotKeysEnabled);
        s->addWithLabel(_("AUTOCONFIGURE RETROARCH HOTKEYS"), autohotkeys);
        s->addSaveFunc([autohotkeys] {
          SystemConf::getInstance()->setBool("system.autohotkeys", autohotkeys->getState());
        });

#ifdef _ENABLEUPDATES
	s->addGroup(_("SYSTEM UPDATE"));

        // Enable updates
        auto updates_enabled = std::make_shared<SwitchComponent>(mWindow);
        updates_enabled->setState(SystemConf::getInstance()->getBool("updates.enabled"));
        s->addSaveFunc([updates_enabled] {
                bool updatesenabled = updates_enabled->getState();
		SystemConf::getInstance()->set("updates.enabled", updatesenabled ? "1" : "0");
	});

        auto optionsUpdates = std::make_shared<OptionListComponent<std::string> >(mWindow, _("UPDATE BRANCH"), false);

        std::string selectedBranch = SystemConf::getInstance()->get("updates.branch");
        if (selectedBranch.empty())
                selectedBranch = "stable";

        optionsUpdates->add(_("STABLE"), "stable", selectedBranch == "stable");
//        optionsUpdates->add(_("DEVELOPMENT"), "dev", selectedBranch == "dev");

        s->addWithLabel(_("UPDATE BRANCH"), optionsUpdates);

        s->addSaveFunc([this, optionsUpdates, selectedBranch]
        {
          if (optionsUpdates->changed()) {
            SystemConf::getInstance()->set("updates.branch", optionsUpdates->getSelected());
            SystemConf::getInstance()->saveSystemConf();
          }
        });

        bool ForceUpdateEnabled = SystemConf::getInstance()->getBool("updates.force");
        auto force_update = std::make_shared<SwitchComponent>(mWindow);
        force_update->setState(ForceUpdateEnabled);
        s->addWithLabel(_("FORCE NEXT UPDATE"), force_update);
        s->addSaveFunc([force_update] {
                bool forceupdate = force_update->getState();
                SystemConf::getInstance()->set("updates.force", forceupdate ? "1" : "0");
        });

        s->addWithLabel(_("CHECK FOR UPDATES"), updates_enabled);
        s->addSaveFunc([updates_enabled]
        {
                SystemConf::getInstance()->setBool("updates.enabled", updates_enabled->getState());
        });

                // Start update
        s->addEntry(GuiUpdate::state == GuiUpdateState::State::UPDATE_READY ? _("APPLY UPDATE") : _("START UPDATE"), true, [this]
        {
                if (GuiUpdate::state == GuiUpdateState::State::UPDATE_READY)
                        quitES(QuitMode::RESTART);
                else if (GuiUpdate::state == GuiUpdateState::State::UPDATER_RUNNING)
                        mWindow->pushGui(new GuiMsgBox(mWindow, _("UPDATER IS ALREADY RUNNING")));
                else
                {
                        if (!checkNetwork())
                                return;

                        mWindow->pushGui(new GuiUpdate(mWindow));
                }
        });
#endif

	if (!ApiSystem::getInstance()->isScriptingSupported(ApiSystem::GAMESETTINGS))
	{
		// Retroachievements
		if (ApiSystem::getInstance()->isScriptingSupported(ApiSystem::RETROACHIVEMENTS))
			s->addEntry(_("RETROACHIEVEMENT SETTINGS"), true, [this] { openRetroachievementsSettings(); });

		if (SystemData::isNetplayActivated() && ApiSystem::getInstance()->isScriptingSupported(ApiSystem::NETPLAY))
			s->addEntry(_("NETPLAY SETTINGS"), true, [this] { openNetplaySettings(); }, "iconNetplay");

//		if (ApiSystem::getInstance()->isScriptingSupported(ApiSystem::BIOSINFORMATION))
//		{
//			s->addEntry(_("MISSING BIOS CHECK"), true, [this, s] { openMissingBiosSettings(); });
//#ifndef _ENABLEEMUELEC
//			auto checkBiosesAtLaunch = std::make_shared<SwitchComponent>(mWindow);
//			checkBiosesAtLaunch->setState(Settings::getInstance()->getBool("CheckBiosesAtLaunch"));
//			s->addWithLabel(_("CHECK BIOS FILES BEFORE RUNNING A GAME"), checkBiosesAtLaunch);
//			s->addSaveFunc([checkBiosesAtLaunch] { Settings::getInstance()->setBool("CheckBiosesAtLaunch", checkBiosesAtLaunch->getState()); });
//#endif
//		}

	}

	std::shared_ptr<OptionListComponent<std::string>> overclock_choice;

#if defined(ODROIDGOA) && !defined(_ENABLEEMUELEC)
	// multimedia keys
	auto multimediakeys_enabled = std::make_shared<OptionListComponent<std::string>>(mWindow, _("MULTIMEDIA KEYS"));
	multimediakeys_enabled->add(_("DEFAULT"), "default", SystemConf::getInstance()->get("system.multimediakeys.enabled") != "0" && SystemConf::getInstance()->get("system.multimediakeys.enabled") != "1");
	multimediakeys_enabled->add(_("ON"), "1", SystemConf::getInstance()->get("system.multimediakeys.enabled") == "1");
	multimediakeys_enabled->add(_("OFF"), "0", SystemConf::getInstance()->get("system.multimediakeys.enabled") == "0");
	s->addWithLabel(_("MULTIMEDIA KEYS"), multimediakeys_enabled);
	s->addSaveFunc([this, multimediakeys_enabled]
	{
	  if (multimediakeys_enabled->changed()) {
	    SystemConf::getInstance()->set("system.multimediakeys.enabled", multimediakeys_enabled->getSelected());
	    this->mWindow->displayNotificationMessage(_U("\uF011  ") + _("REBOOT REQUIRED TO APPLY THE NEW CONFIGURATION"));
	  }
	});
#endif

#ifdef GAMEFORCE
	auto buttonColor_GameForce = std::make_shared< OptionListComponent<std::string> >(mWindow, _("BUTTON LED COLOR"));
	buttonColor_GameForce->add(_("off"), "off", SystemConf::getInstance()->get("color_rgb") == "off" || SystemConf::getInstance()->get("color_rgb") == "");
	buttonColor_GameForce->add(_("red"), "red", SystemConf::getInstance()->get("color_rgb") == "red");
	buttonColor_GameForce->add(_("green"), "green", SystemConf::getInstance()->get("color_rgb") == "green");
	buttonColor_GameForce->add(_("blue"), "blue", SystemConf::getInstance()->get("color_rgb") == "blue");
	buttonColor_GameForce->add(_("white"), "white", SystemConf::getInstance()->get("color_rgb") == "white");
	buttonColor_GameForce->add(_("purple"), "purple", SystemConf::getInstance()->get("color_rgb") == "purple");
	buttonColor_GameForce->add(_("yellow"), "yellow", SystemConf::getInstance()->get("color_rgb") == "yellow");
	buttonColor_GameForce->add(_("cyan"), "cyan", SystemConf::getInstance()->get("color_rgb") == "cyan");
	s->addWithLabel(_("BUTTON LED COLOR"), buttonColor_GameForce);
	s->addSaveFunc([buttonColor_GameForce]
	{
		if (buttonColor_GameForce->changed()) {
			ApiSystem::getInstance()->setButtonColorGameForce(buttonColor_GameForce->getSelected());
			SystemConf::getInstance()->set("color_rgb", buttonColor_GameForce->getSelected());
		}
	});

	auto powerled_GameForce = std::make_shared< OptionListComponent<std::string> >(mWindow, _("POWER LED COLOR"));
	powerled_GameForce->add(_("heartbeat"), "heartbeat", SystemConf::getInstance()->get("option_powerled") == "heartbeat" || SystemConf::getInstance()->get("option_powerled") == "");
	powerled_GameForce->add(_("off"), "off", SystemConf::getInstance()->get("option_powerled") == "off");
	powerled_GameForce->add(_("on"), "on", SystemConf::getInstance()->get("option_powerled") == "on");
	s->addWithLabel(_("POWER LED COLOR"), powerled_GameForce);
	s->addSaveFunc([powerled_GameForce]
	{
		if (powerled_GameForce->changed()) {
			ApiSystem::getInstance()->setPowerLedGameForce(powerled_GameForce->getSelected());
			SystemConf::getInstance()->set("option_powerled", powerled_GameForce->getSelected());
		}
	});
#endif

	// Overclock choice
	if (ApiSystem::getInstance()->isScriptingSupported(ApiSystem::OVERCLOCK))
	{
		overclock_choice = std::make_shared<OptionListComponent<std::string> >(window, _("OVERCLOCK"), false);

		std::string currentOverclock = Settings::getInstance()->getString("Overclock");
		if (currentOverclock == "")
			currentOverclock = "none";

		std::vector<std::string> availableOverclocking = ApiSystem::getInstance()->getAvailableOverclocking();

		// Overclocking device
		bool isOneSet = false;
		for (auto it = availableOverclocking.begin(); it != availableOverclocking.end(); it++)
		{
			std::vector<std::string> tokens = Utils::String::split(*it, ' ');
			if (tokens.size() >= 2)
			{
				// concatenat the ending words
				std::string vname;
				for (unsigned int i = 1; i < tokens.size(); i++)
				{
					if (i > 1) vname += " ";
					vname += tokens.at(i);
				}
				bool isSet = currentOverclock == std::string(tokens.at(0));
				if (isSet)
					isOneSet = true;

				if (vname == "NONE" || vname == "none")
					vname = _("NONE");

				overclock_choice->add(vname, tokens.at(0), isSet);
			}
		}

		if (isOneSet == false)
		{
			if (currentOverclock == "none")
				overclock_choice->add(_("NONE"), currentOverclock, true);
			else
				overclock_choice->add(currentOverclock, currentOverclock, true);
		}

		// overclocking
		s->addWithLabel(_("OVERCLOCK"), overclock_choice);
	}

#if !defined(WIN32) && !defined _ENABLEEMUELEC || defined(_DEBUG)
	s->addGroup(_("STORAGE"));
#endif

	// Storage device
	std::vector<std::string> availableStorage = ApiSystem::getInstance()->getAvailableStorageDevices();
	std::string selectedStorage = ApiSystem::getInstance()->getCurrentStorage();

	auto optionsStorage = std::make_shared<OptionListComponent<std::string> >(window, _("STORAGE DEVICE"), false);
	for (auto it = availableStorage.begin(); it != availableStorage.end(); it++)
	{
		if ((*it) != "RAM")
		{
			if (Utils::String::startsWith(*it, "DEV"))
			{
				std::vector<std::string> tokens = Utils::String::split(*it, ' ');

				if (tokens.size() >= 3) {
					// concatenat the ending words
					std::string vname = "";
					for (unsigned int i = 2; i < tokens.size(); i++) {
						if (i > 2) vname += " ";
						vname += tokens.at(i);
					}
					optionsStorage->add(vname, (*it), selectedStorage == std::string("DEV " + tokens.at(1)));
				}
			}
			else {
				optionsStorage->add((*it), (*it), selectedStorage == (*it));
			}
		}
	}
#if !defined(WIN32) && !defined _ENABLEEMUELEC || defined(_DEBUG)
	s->addWithLabel(_("STORAGE DEVICE"), optionsStorage);
#endif

#if !defined(WIN32) && !defined _ENABLEEMUELEC || defined(_DEBUG)
	// backup
	s->addEntry(_("BACKUP USER DATA"), true, [this] { mWindow->pushGui(new GuiBackupStart(mWindow)); });
#endif

#if !defined(WIN32) && !defined _ENABLEEMUELEC || defined(_DEBUG)
	// Install
	s->addEntry(_("INSTALL BATOCERA ON A NEW DISK"), true, [this] { mWindow->pushGui(new GuiInstallStart(mWindow)); });

	s->addGroup(_("ADVANCED"));

	// Security
	s->addEntry(_("SECURITY"), true, [this] {
		GuiSettings *securityGui = new GuiSettings(mWindow, _("SECURITY").c_str());
		auto securityEnabled = std::make_shared<SwitchComponent>(mWindow);
		securityEnabled->setState(SystemConf::getInstance()->get("system.security.enabled") == "1");
		securityGui->addWithLabel(_("ENFORCE SECURITY"), securityEnabled);

		auto rootpassword = std::make_shared<TextComponent>(mWindow,
			ApiSystem::getInstance()->getRootPassword(),
			ThemeData::getMenuTheme()->Text.font, ThemeData::getMenuTheme()->Text.color);
		securityGui->addWithLabel(_("ROOT PASSWORD"), rootpassword);

		securityGui->addSaveFunc([this, securityEnabled] {
			Window* window = this->mWindow;
			bool reboot = false;

			if (securityEnabled->changed()) {
				SystemConf::getInstance()->set("system.security.enabled",
					securityEnabled->getState() ? "1" : "0");
				SystemConf::getInstance()->saveSystemConf();
				reboot = true;
			}

			if (reboot) {
			  window->displayNotificationMessage(_U("\uF011  ") + _("REBOOT REQUIRED TO APPLY THE NEW CONFIGURATION"));
			}
		});
		mWindow->pushGui(securityGui);
	});
#else
	if (isFullUI)
		s->addGroup(_("ADVANCED"));
#endif

	s->addSaveFunc([overclock_choice, window, optionsStorage, selectedStorage, s]
	{
		bool reboot = false;
		if (optionsStorage->changed())
		{
			ApiSystem::getInstance()->setStorage(optionsStorage->getSelected());
			reboot = true;
		}

		if (overclock_choice && overclock_choice->changed() && Settings::getInstance()->setString("Overclock", overclock_choice->getSelected()))
		{
			ApiSystem::getInstance()->setOverclock(overclock_choice->getSelected());
			reboot = true;
		}

		/*
		if (language_choice->changed())
		{
#ifdef _ENABLEEMUELEC
			std::string selectedLanguage = language_choice->getSelected();
			std::string msg = _("You are about to set EmuELEC Language to:") +"\n" +  selectedLanguage + "\n";
			msg += _("Emulationstation will restart")+"\n";
			msg += _("Do you want to proceed ?");
			window->pushGui(new GuiMsgBox(window, msg, _("YES"), [selectedLanguage] {
				SystemConf::getInstance()->set("system.language", selectedLanguage);
				SystemConf::getInstance()->saveSystemConf();
				quitES(QuitMode::QUIT);
			}, "NO",nullptr));
#else
			if (SystemConf::getInstance()->set("system.language", language_choice->getSelected()))
			{
				FileSorts::reset();
				MetaDataList::initMetadata();

				s->setVariable("reloadGuiMenu", true);
#ifdef HAVE_INTL
				reboot = true;
				rebootForLanguage = true;
#endif
			}
#endif
		}

		if (reboot) {
		  window->displayNotificationMessage(_U("\uF011  ") + _("REBOOT REQUIRED TO APPLY THE NEW CONFIGURATION"));

		  /*
		  if(rebootForLanguage) {
		    if (Settings::getInstance()->getBool("ExitOnRebootRequired")) {
		      quitES(QuitMode::QUIT);
		    }
		  }
		  */

	});

	if (isFullUI){
		//Danger zone options
		s->addEntry(_("DANGER ZONE"), true, [this] { openDangerZone(mWindow, "global"); });
	}

	auto pthis = this;
	s->onFinalize([s, pthis, window]
	{
		if (s->getVariable("reloadGuiMenu"))
		{
			delete pthis;
			window->pushGui(new GuiMenu(window, false));
		}
	});

	mWindow->pushGui(s);
}

void GuiMenu::openLatencyReductionConfiguration(Window* mWindow, std::string configName)
{
	GuiSettings* guiLatency = new GuiSettings(mWindow, _("LATENCY REDUCTION").c_str());

	// run-ahead
	auto runahead_enabled = std::make_shared<OptionListComponent<std::string>>(mWindow, _("RUN-AHEAD FRAMES"));
	runahead_enabled->addRange({ { _("DEFAULT"), "" }, { _("NONE"), "0" }, { "1", "1" }, { "2", "2" }, { "3", "3" }, { "4", "4" }, { "5", "5" }, { "6", "6" } }, SystemConf::getInstance()->get(configName + ".runahead"));
	guiLatency->addWithLabel(_("USE RUN-AHEAD FRAMES"), runahead_enabled);
	guiLatency->addSaveFunc([configName, runahead_enabled] { SystemConf::getInstance()->set(configName + ".runahead", runahead_enabled->getSelected()); });

	// second instance
	auto secondinstance = std::make_shared<OptionListComponent<std::string>>(mWindow, _("RUN-AHEAD USE SECOND INSTANCE"));
	secondinstance->addRange({ { _("DEFAULT"), "" }, { _("ON"), "1" }, { _("OFF"), "0" } }, SystemConf::getInstance()->get(configName + ".secondinstance"));
	guiLatency->addWithLabel(_("RUN-AHEAD USE SECOND INSTANCE"), secondinstance);
	guiLatency->addSaveFunc([configName, secondinstance] { SystemConf::getInstance()->set(configName + ".secondinstance", secondinstance->getSelected()); });

	// audio-latency
	auto audio_latency = std::make_shared<OptionListComponent<std::string>>(mWindow, _("AUDIO LATENCY MILLISECONDS"));
	audio_latency->addRange({ { _("DEFAULT"), "" }, { "2", "2" }, { "4", "4" }, { "8", "8" }, { "16", "16" }, { "24", "24" }, { "32", "32" }, { "40", "40" }, { "48", "48" }, { "56", "56" }, { "64", "64" }, { "72", "72" }, { "80", "80" }, { "88", "88" }, { "96", "96" }, { "104", "104" }, { "112", "112" }, { "120", "120" }, { "128", "128" } }, SystemConf::getInstance()->get(configName + ".audiolatency"));
	guiLatency->addWithLabel(_("AUDIO LATENCY MILLISECONDS"), audio_latency);
	guiLatency->addSaveFunc([configName, audio_latency] { SystemConf::getInstance()->set(configName + ".audiolatency", audio_latency->getSelected()); });
	
	mWindow->pushGui(guiLatency);
}

void GuiMenu::openRetroachievementsSettings()
{
	Window* window = mWindow;
	GuiSettings* retroachievements = new GuiSettings(mWindow, _("RETROACHIEVEMENT SETTINGS").c_str());

	retroachievements->addGroup(_("SETTINGS"));

	bool retroachievementsEnabled = SystemConf::getInstance()->getBool("global.retroachievements");
	std::string username = SystemConf::getInstance()->get("global.retroachievements.username");
	std::string password = SystemConf::getInstance()->get("global.retroachievements.password");

	// retroachievements_enable
	auto retroachievements_enabled = std::make_shared<SwitchComponent>(mWindow);
	retroachievements_enabled->setState(retroachievementsEnabled);
	retroachievements->addWithLabel(_("RETROACHIEVEMENTS"), retroachievements_enabled);

	// retroachievements_hardcore_mode
	auto retroachievements_hardcore_enabled = std::make_shared<SwitchComponent>(mWindow);
	retroachievements_hardcore_enabled->setState(SystemConf::getInstance()->getBool("global.retroachievements.hardcore"));
	retroachievements->addWithLabel(_("HARDCORE MODE"), retroachievements_hardcore_enabled);
	retroachievements->addSaveFunc([retroachievements_hardcore_enabled] { SystemConf::getInstance()->setBool("global.retroachievements.hardcore", retroachievements_hardcore_enabled->getState()); });

	//// retroachievements_leaderboards
	//auto retroachievements_leaderboards_enabled = std::make_shared<SwitchComponent>(mWindow);
	//retroachievements_leaderboards_enabled->setState(SystemConf::getInstance()->getBool("global.retroachievements.leaderboards"));
	//retroachievements->addWithLabel(_("LEADERBOARDS"), retroachievements_leaderboards_enabled);
	//retroachievements->addSaveFunc([retroachievements_leaderboards_enabled] { SystemConf::getInstance()->setBool("global.retroachievements.///leaderboards", retroachievements_leaderboards_enabled->getState()); });

	// retroachievements_leaderboards list
	auto retroachievements_leaderboards_list = std::make_shared< OptionListComponent<std::string> >(mWindow, _("LEADERBOARDS"), false);
	std::vector<std::string> leader;
	leader.push_back("disabled");
	leader.push_back("enabled");
	leader.push_back("trackers only");
	leader.push_back("notifications only");

	auto currentLeader = SystemConf::getInstance()->get("global.retroachievements.leaderboards");
	if (currentLeader.empty())
		currentLeader = "disabled";

	for (auto it = leader.cbegin(); it != leader.cend(); it++)
		retroachievements_leaderboards_list->add(_(it->c_str()), *it, currentLeader == *it);

		retroachievements->addWithLabel(_("LEADERBOARDS"), retroachievements_leaderboards_list);
		retroachievements->addSaveFunc([retroachievements_leaderboards_list]
	{
		SystemConf::getInstance()->set("global.retroachievements.leaderboards", retroachievements_leaderboards_list->getSelected());
		SystemConf::getInstance()->saveSystemConf();
	});

	// retroachievements_challenge_indicators
	auto retroachievements_challenge_indicators = std::make_shared<SwitchComponent>(mWindow);
	retroachievements_challenge_indicators->setState(SystemConf::getInstance()->getBool("global.retroachievements.challengeindicators"));
	retroachievements->addWithLabel(_("CHALLENGE INDICATORS"), retroachievements_challenge_indicators);
	retroachievements->addSaveFunc([retroachievements_challenge_indicators] { SystemConf::getInstance()->setBool("global.retroachievements.challengeindicators", retroachievements_challenge_indicators->getState()); });

	// retroachievements_richpresence_enable
	auto retroachievements_richpresence_enable = std::make_shared<SwitchComponent>(mWindow);
	retroachievements_richpresence_enable->setState(SystemConf::getInstance()->getBool("global.retroachievements.richpresence"));
	retroachievements->addWithLabel(_("RICH PRESENCE"), retroachievements_richpresence_enable);
	retroachievements->addSaveFunc([retroachievements_richpresence_enable] { SystemConf::getInstance()->setBool("global.retroachievements.richpresence", retroachievements_richpresence_enable->getState()); });

	// retroachievements_badges_enable
	auto retroachievements_badges_enable = std::make_shared<SwitchComponent>(mWindow);
	retroachievements_badges_enable->setState(SystemConf::getInstance()->getBool("global.retroachievements.badges"));
	retroachievements->addWithLabel(_("BADGES"), retroachievements_badges_enable);
	retroachievements->addSaveFunc([retroachievements_badges_enable] { SystemConf::getInstance()->setBool("global.retroachievements.badges", retroachievements_badges_enable->getState()); });

	// retroachievements_test_unofficial
	auto retroachievements_test_unofficial = std::make_shared<SwitchComponent>(mWindow);
	retroachievements_test_unofficial->setState(SystemConf::getInstance()->getBool("global.retroachievements.testunofficial"));
	retroachievements->addWithLabel(_("TEST UNOFFICIAL ACHIEVEMENTS"), retroachievements_test_unofficial);
	retroachievements->addSaveFunc([retroachievements_test_unofficial] { SystemConf::getInstance()->setBool("global.retroachievements.testunofficial", retroachievements_test_unofficial->getState()); });

	// retroachievements_unlock_sound_enable
	auto retroachievements_unlock_sound_enable = std::make_shared<SwitchComponent>(mWindow);
	retroachievements_unlock_sound_enable->setState(SystemConf::getInstance()->getBool("global.retroachievements.soundenable"));
	retroachievements->addWithLabel(_("UNLOCK SOUND"), retroachievements_unlock_sound_enable);
	retroachievements->addSaveFunc([retroachievements_unlock_sound_enable] { SystemConf::getInstance()->setBool("global.retroachievements.soundenable", retroachievements_unlock_sound_enable->getState()); });

	// retroachievements_verbose_mode
	auto retroachievements_verbose_enabled = std::make_shared<SwitchComponent>(mWindow);
	retroachievements_verbose_enabled->setState(SystemConf::getInstance()->getBool("global.retroachievements.verbose"));
	retroachievements->addWithLabel(_("VERBOSE MODE"), retroachievements_verbose_enabled);
	retroachievements->addSaveFunc([retroachievements_verbose_enabled] { SystemConf::getInstance()->setBool("global.retroachievements.verbose", retroachievements_verbose_enabled->getState()); });

	// retroachievements_automatic_screenshot
	auto retroachievements_screenshot_enabled = std::make_shared<SwitchComponent>(mWindow);
	retroachievements_screenshot_enabled->setState(SystemConf::getInstance()->getBool("global.retroachievements.screenshot"));
	retroachievements->addWithLabel(_("AUTOMATIC SCREENSHOT"), retroachievements_screenshot_enabled);
	retroachievements->addSaveFunc([retroachievements_screenshot_enabled] { SystemConf::getInstance()->setBool("global.retroachievements.screenshot", retroachievements_screenshot_enabled->getState()); });

	// retroachievements_start_active
	auto retroachievements_start_active = std::make_shared<SwitchComponent>(mWindow);
	retroachievements_start_active->setState(SystemConf::getInstance()->getBool("global.retroachievements.active"));
	retroachievements->addWithLabel(_("ENCORE MODE (LOCAL RESET OF ACHIEVEMENTS)"), retroachievements_start_active);
	retroachievements->addSaveFunc([retroachievements_start_active] { SystemConf::getInstance()->setBool("global.retroachievements.active", retroachievements_start_active->getState()); });

	// Unlock sound
	auto installedRSounds = ApiSystem::getInstance()->getRetroachievementsSoundsList();
	if (installedRSounds.size() > 0)
	{
		std::string currentSound = SystemConf::getInstance()->get("global.retroachievements.sound");

		auto rsounds_choices = std::make_shared<OptionListComponent<std::string> >(mWindow, _("RETROACHIEVEMENT UNLOCK SOUND"), false);
		rsounds_choices->add(_("none"), "none", currentSound.empty() || currentSound == "none");

		for (auto snd : installedRSounds)
			rsounds_choices->add(_(Utils::String::toUpper(snd).c_str()), snd, currentSound == snd);

		if (!rsounds_choices->hasSelection())
			rsounds_choices->selectFirstItem();

		retroachievements->addWithLabel(_("UNLOCK SOUND"), rsounds_choices);
		retroachievements->addSaveFunc([rsounds_choices] { SystemConf::getInstance()->set("global.retroachievements.sound", rsounds_choices->getSelected()); });
	}

	// retroachievements, username, password
	retroachievements->addInputTextRow(_("USERNAME"), "global.retroachievements.username", false);
	retroachievements->addInputTextRow(_("PASSWORD"), "global.retroachievements.password", true);

	// retroachievements_hardcore_mode
	auto retroachievements_menuitem = std::make_shared<SwitchComponent>(mWindow);
	retroachievements_menuitem->setState(Settings::getInstance()->getBool("RetroachievementsMenuitem"));
	retroachievements->addWithLabel(_("SHOW RETROACHIEVEMENTS ENTRY IN MAIN MENU"), retroachievements_menuitem);
	retroachievements->addSaveFunc([retroachievements_menuitem] { Settings::getInstance()->setBool("RetroachievementsMenuitem", retroachievements_menuitem->getState()); });

	retroachievements->addGroup(_("GAME INDEXES"));

	// CheckOnStart
	auto checkOnStart = std::make_shared<SwitchComponent>(mWindow);
	checkOnStart->setState(Settings::getInstance()->getBool("CheevosCheckIndexesAtStart"));
	retroachievements->addWithLabel(_("INDEX NEW GAMES AT STARTUP"), checkOnStart);
	retroachievements->addSaveFunc([checkOnStart] { Settings::getInstance()->setBool("CheevosCheckIndexesAtStart", checkOnStart->getState()); });

	// Index games
	retroachievements->addEntry(_("INDEX GAMES"), true, [this]
	{
		if (ThreadedHasher::checkCloseIfRunning(mWindow))
			mWindow->pushGui(new GuiHashStart(mWindow, ThreadedHasher::HASH_CHEEVOS_MD5));
	});


	//retroachievements->addEntry(_("FIND ALL GAMES"), false, [this] { ThreadedHasher::start(mWindow, ThreadedHasher::HASH_CHEEVOS_MD5, true); });
	//retroachievements->addEntry(_("FIND NEW GAMES"), false, [this] { ThreadedHasher::start(mWindow, ThreadedHasher::HASH_CHEEVOS_MD5); });

	retroachievements->addSaveFunc([retroachievementsEnabled, retroachievements_enabled, username, password, window]
	{
		bool newState = retroachievements_enabled->getState();
		std::string newUsername = SystemConf::getInstance()->get("global.retroachievements.username");
		std::string newPassword = SystemConf::getInstance()->get("global.retroachievements.password");

		if (newState && (!retroachievementsEnabled || username != newUsername || password != newPassword))
		{
			std::string error;
			if (!RetroAchievements::testAccount(newUsername, newPassword, error))
			{
				window->pushGui(new GuiMsgBox(window, _("UNABLE TO ACTIVATE RETROACHIEVEMENTS:") + "\n" + error, _("OK"), nullptr, GuiMsgBoxIcon::ICON_ERROR));
				retroachievements_enabled->setState(false);
				newState = false;
			}
		}

		if (SystemConf::getInstance()->setBool("global.retroachievements", newState))
			if (!ThreadedHasher::isRunning() && newState)
				ThreadedHasher::start(window, ThreadedHasher::HASH_CHEEVOS_MD5, false, true);
	});

	mWindow->pushGui(retroachievements);
}

template <typename StructType, typename FieldSelectorUnaryFn>
static auto groupBy(const std::vector<StructType>& instances, const FieldSelectorUnaryFn& fieldChooser) // -> std::map<decltype(forward<FieldSelectorUnaryFn>(fieldChooser)), std::vector<StructType>>
{
	StructType _;
	using FieldType = decltype(fieldChooser(_));
	std::map<FieldType, std::vector<StructType>> instancesByField;
	for (auto& instance : instances)
	{
		instancesByField[fieldChooser(instance)].push_back(instance);
	}
	return instancesByField;
}

void GuiMenu::openNetplaySettings()
{
	GuiSettings* settings = new GuiSettings(mWindow, _("NETPLAY SETTINGS").c_str());

	settings->addGroup(_("SETTINGS"));

	// Enable
	auto enableNetplay = std::make_shared<SwitchComponent>(mWindow);
	enableNetplay->setState(SystemConf::getInstance()->getBool("global.netplay"));
	settings->addWithLabel(_("ENABLE NETPLAY"), enableNetplay);

	std::string port = SystemConf::getInstance()->get("global.netplay.port");
	if (port.empty())
		SystemConf::getInstance()->set("global.netplay.port", "55435");

	settings->addInputTextRow(_("NICKNAME"), "global.netplay.nickname", false);
	settings->addInputTextRow(_("PORT"), "global.netplay.port", false);

	// RELAY SERVER
	std::string mitm = SystemConf::getInstance()->get("global.netplay.relay");

	auto mitms = std::make_shared<OptionListComponent<std::string> >(mWindow, _("USE RELAY SERVER"), false);
	mitms->add(_("NONE"), "", mitm.empty() || mitm == "none");
	mitms->add("NEW YORK", "nyc", mitm == "nyc");
	mitms->add("MADRID", "madrid", mitm == "madrid");
	mitms->add("MONTREAL", "montreal", mitm == "montreal");
	mitms->add("SAO PAULO", "saopaulo", mitm == "saopaulo");

	if (!mitms->hasSelection())
		mitms->selectFirstItem();

	settings->addWithLabel(_("USE RELAY SERVER"), mitms);

	settings->addGroup(_("GAME INDEXES"));

	// CheckOnStart
	auto checkOnStart = std::make_shared<SwitchComponent>(mWindow);
	checkOnStart->setState(Settings::getInstance()->getBool("NetPlayCheckIndexesAtStart"));
	settings->addWithLabel(_("INDEX NEW GAMES AT STARTUP"), checkOnStart);

	Window* window = mWindow;
	settings->addSaveFunc([enableNetplay, checkOnStart, mitms, window]
	{
		Settings::getInstance()->setBool("NetPlayCheckIndexesAtStart", checkOnStart->getState());
		SystemConf::getInstance()->set("global.netplay.relay", mitms->getSelected());

		if (SystemConf::getInstance()->setBool("global.netplay", enableNetplay->getState()))
		{
			if (!ThreadedHasher::isRunning() && enableNetplay->getState())
			{
				ThreadedHasher::start(window, ThreadedHasher::HASH_NETPLAY_CRC, false, true);
			}
		}
	});

	settings->addEntry(_("INDEX GAMES"), true, [this]
	{
		if (ThreadedHasher::checkCloseIfRunning(mWindow))
			mWindow->pushGui(new GuiHashStart(mWindow, ThreadedHasher::HASH_NETPLAY_CRC));
	});

	//settings->addEntry(_("FIND ALL GAMES"), false, [this] { ThreadedHasher::start(mWindow, ThreadedHasher::HASH_NETPLAY_CRC, true); });
	//settings->addEntry(_("FIND NEW GAMES"), false, [this] { ThreadedHasher::start(mWindow, ThreadedHasher::HASH_NETPLAY_CRC); });

	mWindow->pushGui(settings);
}

void GuiMenu::addDecorationSetOptionListComponent(Window* window, GuiSettings* parentWindow, const std::vector<DecorationSetInfo>& sets, const std::string& configName)
{
	auto decorations = std::make_shared<OptionListComponent<std::string> >(window, _("DECORATION SET"), false);
	decorations->setRowTemplate([window, sets](std::string data, ComponentListRow& row) { createDecorationItemTemplate(window, sets, data, row); });

	std::vector<std::string> items = { _("DEFAULT"), _("NONE") };
	for (auto set : sets)
		items.push_back(set.name);

	std::string bezel = SystemConf::getInstance()->get(configName + ".bezel");

	for (auto item : items)
		decorations->add(item, item, (bezel == item) || (bezel == "none" && item == _("NONE")) || (bezel == "" && item == _("DEFAULT")));

	if (!decorations->hasSelection())
		decorations->selectFirstItem();

	parentWindow->addWithLabel(_("DECORATION SET"), decorations);
	parentWindow->addSaveFunc([decorations, configName]
	{
		SystemConf::getInstance()->set(configName + ".bezel", decorations->getSelected() == _("NONE") ? "none" : decorations->getSelected() == _("DEFAULT") ? "" : decorations->getSelected());
	});
};

void GuiMenu::openGamesSettings_batocera()
{
	Window* window = mWindow;

	auto s = new GuiSettings(mWindow, _("GAME SETTINGS").c_str());

	if (SystemConf::getInstance()->get("system.es.menu") != "bartop")
	{
		s->addGroup(_("TOOLS"));

		// Game List Update
		s->addEntry(_("UPDATE GAMELISTS"), false, [this, window] { updateGameLists(window); });

		if (SystemConf::getInstance()->getBool("global.retroachievements") &&
			!Settings::getInstance()->getBool("RetroachievementsMenuitem") &&
			SystemConf::getInstance()->get("global.retroachievements.username") != "")
		{
			s->addEntry(_("RETROACHIEVEMENTS").c_str(), true, [this]
			{
				if (!checkNetwork())
					return;

				GuiRetroAchievements::show(mWindow);
			}/*, "iconRetroachievements"*/);
		}
	}

	s->addGroup(_("DEFAULT GLOBAL SETTINGS"));

	// Screen ratio choice
	if (SystemConf::getInstance()->get("system.es.menu") != "bartop")
	{
		auto ratio_choice = createRatioOptionList(mWindow, "global");
		s->addWithLabel(_("GAME ASPECT RATIO"), ratio_choice);
		s->addSaveFunc([ratio_choice] { SystemConf::getInstance()->set("global.ratio", ratio_choice->getSelected()); });
	}
#ifndef _ENABLEEMUELEC
	// video resolution mode
	if (ApiSystem::getInstance()->isScriptingSupported(ApiSystem::RESOLUTION))
	{
		auto videoModeOptionList = createVideoResolutionModeOptionList(mWindow, "global");
		s->addWithLabel(_("VIDEO MODE"), videoModeOptionList);
		s->addSaveFunc([this, videoModeOptionList] { SystemConf::getInstance()->set("global.videomode", videoModeOptionList->getSelected()); });
	}
#endif

	// bilinear filtering
	auto smoothing_enabled = std::make_shared<OptionListComponent<std::string>>(mWindow, _("BILINEAR FILTERING"));
	smoothing_enabled->addRange({ { _("DEFAULT"), "default" },{ _("ON") , "1" },{ _("OFF") , "0" } }, SystemConf::getInstance()->get("global.smooth"));
	s->addWithLabel(_("BILINEAR FILTERING"), smoothing_enabled);
	s->addSaveFunc([smoothing_enabled] { SystemConf::getInstance()->set("global.smooth", smoothing_enabled->getSelected()); });

#ifdef _ENABLEEMUELEC
	// bezel
	/*
	auto bezel_enabled = std::make_shared<OptionListComponent<std::string>>(mWindow, _("ENABLE RA BEZELS"));
	bezel_enabled->add(_("DEFAULT"), "default", SystemConf::getInstance()->get("global.bezel") != "0" && SystemConf::getInstance()->get("global.bezel") != "1");
	bezel_enabled->add(_("ON"), "1", SystemConf::getInstance()->get("global.bezel") == "1");
	bezel_enabled->add(_("OFF"), "0", SystemConf::getInstance()->get("global.bezel") == "0");
	s->addWithLabel(_("ENABLE RA BEZELS"), bezel_enabled);
	s->addSaveFunc([bezel_enabled] { SystemConf::getInstance()->set("global.bezel", bezel_enabled->getSelected()); });
	*/

#endif

#if defined(S922X) || defined(RK3588)  || defined(RK3399)
        // Core chooser
        auto cores_used = std::make_shared<OptionListComponent<std::string>>(mWindow, _("CORES USED"));
        cores_used->addRange({ { _("ALL"), "all" },{ _("BIG") , "big" },{ _("LITTLE") , "little" } }, SystemConf::getInstance()->get("global.cores"));
        s->addWithLabel(_("CORES USED"), cores_used);
        s->addSaveFunc([cores_used] { SystemConf::getInstance()->set("global.cores", cores_used->getSelected()); });
#endif
	
	// rewind
	auto rewind_enabled = std::make_shared<OptionListComponent<std::string>>(mWindow, _("REWIND"));
	rewind_enabled->addRange({ { _("DEFAULT"), "default" },{ _("ON") , "1" },{ _("OFF") , "0" } }, SystemConf::getInstance()->get("global.rewind"));
	s->addWithLabel(_("REWIND"), rewind_enabled);
	s->addSaveFunc([rewind_enabled] { SystemConf::getInstance()->set("global.rewind", rewind_enabled->getSelected()); });

	// Integer scale
	auto integerscale_enabled = std::make_shared<OptionListComponent<std::string>>(mWindow, _("INTEGER SCALING (PIXEL PERFECT)"));
	integerscale_enabled->addRange({ { _("DEFAULT"), "default" },{ _("ON") , "1" },{ _("OFF") , "0" } }, SystemConf::getInstance()->get("global.integerscale"));
	s->addWithLabel(_("INTEGER SCALING (PIXEL PERFECT)"), integerscale_enabled);
	s->addSaveFunc([integerscale_enabled] { SystemConf::getInstance()->set("global.integerscale", integerscale_enabled->getSelected()); });

	// autosave/load
	auto autosave_enabled = std::make_shared<OptionListComponent<std::string>>(mWindow, _("AUTO SAVE/LOAD ON GAME LAUNCH"));
	autosave_enabled->addRange({ { _("OFF"), "auto" },{ _("ON") , "1" },{ _("SHOW SAVE STATES") , "2" },{ _("SHOW SAVE STATES IF NOT EMPTY") , "3" } }, SystemConf::getInstance()->get("global.autosave"));
	s->addWithLabel(_("AUTO SAVE/LOAD ON GAME LAUNCH"), autosave_enabled);
	s->addSaveFunc([autosave_enabled] { SystemConf::getInstance()->set("global.autosave", autosave_enabled->getSelected()); });

	// Incremental savestates
	auto incrementalSaveStates = std::make_shared<SwitchComponent>(mWindow);
	incrementalSaveStates->setState(SystemConf::getInstance()->get("global.incrementalsavestates") == "1");
	s->addWithLabel(_("INCREMENTAL SAVESTATES"), incrementalSaveStates);
	s->addSaveFunc([incrementalSaveStates] { SystemConf::getInstance()->set("global.incrementalsavestates", incrementalSaveStates->getState() ? "1" : "0"); });

        // Automated Cloud Backup
        auto cloudBackup = std::make_shared<SwitchComponent>(mWindow);
        cloudBackup->setState(SystemConf::getInstance()->get("cloud.backup") == "1");
        s->addWithLabel(_("BACKUP TO CLOUD ON GAME EXIT"), cloudBackup);
        s->addSaveFunc([cloudBackup] { SystemConf::getInstance()->set("cloud.backup", cloudBackup->getState() ? "1" : "0"); });

	// Shaders preset
#ifndef _ENABLEEMUELEC
	if (ApiSystem::getInstance()->isScriptingSupported(ApiSystem::SHADERS))
	{
		auto installedShaders = ApiSystem::getInstance()->getShaderList();
		if (installedShaders.size() > 0)
		{
#endif
			std::string currentShader = SystemConf::getInstance()->get("global.shaderset");

			auto shaders_choices = std::make_shared<OptionListComponent<std::string> >(mWindow, _("SHADER SET"), false);
			shaders_choices->add(_("DEFAULT"), "default", currentShader.empty() || currentShader == "auto");
			shaders_choices->add(_("NONE"), "none", currentShader == "none");

#ifdef _ENABLEEMUELEC
	std::string a;
	for(std::stringstream ss(getShOutput(R"(/usr/bin/getshaders)")); getline(ss, a, ','); )
		shaders_choices->add(a, a, currentShader == a); // emuelec
#else
			for (auto shader : installedShaders)
				shaders_choices->add(_(Utils::String::toUpper(shader).c_str()), shader, currentShader == shader);

			if (!shaders_choices->hasSelection())
				shaders_choices->selectFirstItem();

#endif
			s->addWithLabel(_("SHADER SET"), shaders_choices);
			s->addSaveFunc([shaders_choices] { SystemConf::getInstance()->set("global.shaderset", shaders_choices->getSelected()); });
#ifndef _ENABLEEMUELEC
	}
	}
#endif

	// Filters preset
	std::string currentFilter = SystemConf::getInstance()->get("global.filterset");
	auto filters_choices = std::make_shared<OptionListComponent<std::string> >(mWindow, _("FILTER SET"), false);
	filters_choices->add(_("DEFAULT"), "default", currentFilter.empty() || currentFilter == "auto");
	filters_choices->add(_("NONE"), "none", currentFilter == "none");
	std::string filterList;
	for(std::stringstream ss(getShOutput(R"(/usr/bin/getfilters)")); getline(ss, filterList, ','); )
		filters_choices->add(filterList, filterList, currentFilter == filterList); // emuelec
	s->addWithLabel(_("FILTER SET"), filters_choices);
	s->addSaveFunc([filters_choices] { SystemConf::getInstance()->set("global.filterset", filters_choices->getSelected()); });

	// decorations
	if (ApiSystem::getInstance()->isScriptingSupported(ApiSystem::DECORATIONS))
	{
		auto sets = GuiMenu::getDecorationsSets(ViewController::get()->getState().getSystem());
		if (sets.size() > 0)
		{
				auto decorations = std::make_shared<OptionListComponent<std::string> >(mWindow, _("DECORATION SET"), false);
				decorations->setRowTemplate([window, sets](std::string data, ComponentListRow& row)
				{
					createDecorationItemTemplate(window, sets, data, row);
				});

				std::vector<std::string> decorations_item;
				decorations_item.push_back(_("DEFAULT"));
				decorations_item.push_back(_("NONE"));
				for (auto set : sets)
					decorations_item.push_back(set.name);

				for (auto it = decorations_item.begin(); it != decorations_item.end(); it++)
					decorations->add(*it, *it,
					(SystemConf::getInstance()->get("global.bezel") == *it) ||
						(SystemConf::getInstance()->get("global.bezel") == "none" && *it == _("NONE")) ||
						(SystemConf::getInstance()->get("global.bezel") == "" && *it == _("DEFAULT")));

			s->addWithLabel(_("DECORATION SET"), decorations);

			decorations->setSelectedChangedCallback([decorations](std::string value)
			{
				if (Utils::String::toLower(value) == "auto") {
					value = "";
				}
				LOG(LogDebug) << "Setting bezel on change to: " << value;

				SystemConf::getInstance()->set("global.bezel", value);
			});

			if (decorations->getSelectedName() == "")
			{
				decorations->selectFirstItem();
			}

#ifndef _ENABLEEMUELEC
			// stretch bezels
			auto bezel_stretch_enabled = std::make_shared<OptionListComponent<std::string>>(mWindow, _("STRETCH BEZELS (4K & ULTRAWIDE)"));
			bezel_stretch_enabled->add(_("DEFAULT"), "default", SystemConf::getInstance()->get("global.bezel_stretch") != "0" && SystemConf::getInstance()->get("global.bezel_stretch") != "1");
			bezel_stretch_enabled->add(_("ON"), "1", SystemConf::getInstance()->get("global.bezel_stretch") == "1");
			bezel_stretch_enabled->add(_("OFF"), "0", SystemConf::getInstance()->get("global.bezel_stretch") == "0");
				decorations_window->addWithLabel(_("STRETCH BEZELS (4K & ULTRAWIDE)"), bezel_stretch_enabled);
				decorations_window->addSaveFunc([bezel_stretch_enabled] {
					if (bezel_stretch_enabled->changed()) {
					SystemConf::getInstance()->set("global.bezel_stretch", bezel_stretch_enabled->getSelected());
					SystemConf::getInstance()->saveSystemConf();
					}
					});

				// tattoo and controller overlays
				auto bezel_tattoo = std::make_shared<OptionListComponent<std::string>>(mWindow, _("SHOW CONTROLLER OVERLAYS"));
				bezel_tattoo->add(_("DEFAULT"), "default", SystemConf::getInstance()->get("global.bezel.tattoo") != "0"
						&& SystemConf::getInstance()->get("global.bezel.tattoo") != "system"
						&& SystemConf::getInstance()->get("global.bezel.tattoo") != "custom");
				bezel_tattoo->add(_("NO"), "0", SystemConf::getInstance()->get("global.bezel.tattoo") == "0");
				bezel_tattoo->add(_("SYSTEM CONTROLLERS"), "system", SystemConf::getInstance()->get("global.bezel.tattoo") == "system");
				bezel_tattoo->add(_("CUSTOM .PNG IMAGE"), "custom", SystemConf::getInstance()->get("global.bezel.tattoo") == "custom");
				decorations_window->addWithLabel(_("SHOW CONTROLLER OVERLAYS"), bezel_tattoo);
				decorations_window->addSaveFunc([bezel_tattoo] {
						if (bezel_tattoo->changed()) {
						SystemConf::getInstance()->set("global.bezel.tattoo", bezel_tattoo->getSelected());
						SystemConf::getInstance()->saveSystemConf();
						}
						});

				auto bezel_tattoo_corner = std::make_shared<OptionListComponent<std::string>>(mWindow, _("OVERLAY CORNER"));
				bezel_tattoo_corner->add(_("DEFAULT"), "default", SystemConf::getInstance()->get("global.bezel.tattoo_corner") != "NW"
						&& SystemConf::getInstance()->get("global.bezel.tattoo_corner") != "NE"
						&& SystemConf::getInstance()->get("global.bezel.tattoo_corner") != "SE"
						&& SystemConf::getInstance()->get("global.bezel.tattoo_corner") != "SW");
				bezel_tattoo_corner->add(_("NORTH WEST"), "NW", SystemConf::getInstance()->get("global.bezel.tattoo_corner") == "NW");
				bezel_tattoo_corner->add(_("NORTH EAST"), "NE", SystemConf::getInstance()->get("global.bezel.tattoo_corner") == "NE");
				bezel_tattoo_corner->add(_("SOUTH EAST"), "SE", SystemConf::getInstance()->get("global.bezel.tattoo_corner") == "SE");
				bezel_tattoo_corner->add(_("SOUTH WEST"), "SW", SystemConf::getInstance()->get("global.bezel.tattoo_corner") == "SW");
				decorations_window->addWithLabel(_("OVERLAY CORNER"), bezel_tattoo_corner);
				decorations_window->addSaveFunc([bezel_tattoo_corner] {
						if (bezel_tattoo_corner->changed()) {
						SystemConf::getInstance()->set("global.bezel.tattoo_corner", bezel_tattoo_corner->getSelected());
						SystemConf::getInstance()->saveSystemConf();
						}
						});
				decorations_window->addInputTextRow(_("CUSTOM .PNG IMAGE PATH"), "global.bezel.tattoo_file", false);

				mWindow->pushGui(decorations_window);
			});
#endif
			}
	}

	// latency reduction
	s->addEntry(_("LATENCY REDUCTION"), true, [this] { openLatencyReductionConfiguration(mWindow, "global"); });

	//AI-enabled translations
	s->addEntry(_("AI GAME TRANSLATION"), true, [this]
	{
		GuiSettings *ai_service = new GuiSettings(mWindow, _("AI GAME TRANSLATION").c_str());

		// AI service enabled?
		auto ai_service_enabled = std::make_shared<SwitchComponent>(mWindow);
		ai_service_enabled->setState(
			SystemConf::getInstance()->get("global.ai_service_enabled") == "1");
		ai_service->addWithLabel(_("ENABLE AI TRANSLATION SERVICE"), ai_service_enabled);

		// Target language - order is: popular languages in the Batocera community first
		// then alphabetical order of the 2-char lang code (because the strings are localized)
		auto lang_choices = std::make_shared<OptionListComponent<std::string> >(mWindow,
			_("TARGET LANGUAGE"), false);
		std::string currentLang = SystemConf::getInstance()->get("global.ai_target_lang");
		if (currentLang.empty())
			currentLang = std::string("En");
		lang_choices->add("ENGLISH", "En", currentLang == "En");
		lang_choices->add("FRANÇAIS", "Fr", currentLang == "Fr");
		lang_choices->add("PORTUGUES", "Pt", currentLang == "Pt");
		lang_choices->add("DEUTSCH", "De", currentLang == "De");
		lang_choices->add("GREEK", "El", currentLang == "El");
		lang_choices->add("ESPAÑOL", "Es", currentLang == "Es");
		lang_choices->add("CZECH", "Cs", currentLang == "Cs");
		lang_choices->add("DANISH", "Da", currentLang == "Da");
		lang_choices->add("CROATIAN", "Hr", currentLang == "Hr");
		lang_choices->add("HUNGARIAN", "Hu", currentLang == "Hu");
		lang_choices->add("ITALIANO", "It", currentLang == "It");
		lang_choices->add("JAPANESE", "Ja", currentLang == "Ja");
		lang_choices->add("KOREAN", "Ko", currentLang == "Ko");
		lang_choices->add("DUTCH", "Nl", currentLang == "Nl");
		lang_choices->add("NORWEGIAN", "Nn", currentLang == "Nn");
		lang_choices->add("POLISH", "Po", currentLang == "Po");
		lang_choices->add("ROMANIAN", "Ro", currentLang == "Ro");
		lang_choices->add("РУССКИЙ", "Ru", currentLang == "Ru");
		lang_choices->add("SVENSKA", "Sv", currentLang == "Sv");
		lang_choices->add("TÜRKÇE", "Tr", currentLang == "Tr");
		lang_choices->add("简体中文", "Zh", currentLang == "Zh");
		ai_service->addWithLabel(_("TARGET LANGUAGE"), lang_choices);

		// Service  URL
		ai_service->addInputTextRow(_("AI TRANSLATION SERVICE URL"), "global.ai_service_url", false);

		// Pause game for translation?
		auto ai_service_pause = std::make_shared<SwitchComponent>(mWindow);
		ai_service_pause->setState(
			SystemConf::getInstance()->get("global.ai_service_pause") == "1");
		ai_service->addWithLabel(_("PAUSE ON TRANSLATED SCREEN"), ai_service_pause);

		ai_service->addSaveFunc([ai_service_enabled, lang_choices, ai_service_pause] {
			if (ai_service_enabled->changed())
				SystemConf::getInstance()->set("global.ai_service_enabled",
					ai_service_enabled->getState() ? "1" : "0");
			if (lang_choices->changed())
				SystemConf::getInstance()->set("global.ai_target_lang",
					lang_choices->getSelected());
			if (ai_service_pause->changed())
				SystemConf::getInstance()->set("global.ai_service_pause",
					ai_service_pause->getState() ? "1" : "0");
			SystemConf::getInstance()->saveSystemConf();
		});

		mWindow->pushGui(ai_service);
	});

	auto groups = groupBy(SystemData::mGlobalFeatures, [](const CustomFeature& item) { return item.submenu; });
	for (auto group : groups)
	{
		if (!group.first.empty())
		{
			s->addEntry(group.first, true, [this, group]
			{
				GuiSettings* groupSettings = new GuiSettings(mWindow, _(group.first.c_str()));

				for (auto feat : group.second)
				{
					std::string storageName = "global." + feat.value;
					std::string storedValue = SystemConf::getInstance()->get(storageName);

					auto cf = std::make_shared<OptionListComponent<std::string>>(mWindow, _(feat.name.c_str()));
					cf->add(_("DEFAULT"), "", storedValue.empty() || storedValue == "default");

					for (auto fval : feat.choices)
						cf->add(_(fval.name.c_str()), fval.value, storedValue == fval.value);

					if (!cf->hasSelection())
						cf->selectFirstItem();

					if (!feat.description.empty())
						groupSettings->addWithDescription(_(feat.name.c_str()), _(feat.description.c_str()), cf);
					else
						groupSettings->addWithLabel(_(feat.name.c_str()), cf);

					groupSettings->addSaveFunc([cf, storageName] { SystemConf::getInstance()->set(storageName, cf->getSelected()); });
				}

				mWindow->pushGui(groupSettings);
			});
		}
		else
		{
			// Load global custom features
			for (auto feat : group.second)
			{
				std::string storageName = "global." + feat.value;
				std::string storedValue = SystemConf::getInstance()->get(storageName);

				auto cf = std::make_shared<OptionListComponent<std::string>>(mWindow, _(feat.name.c_str()));
				cf->add(_("DEFAULT"), "", storedValue.empty() || storedValue == "default");

				for (auto fval : feat.choices)
					cf->add(_(fval.name.c_str()), fval.value, storedValue == fval.value);

				if (!cf->hasSelection())
					cf->selectFirstItem();

				if (!feat.description.empty())
					s->addWithDescription(_(feat.name.c_str()), _(feat.description.c_str()), cf);
				else
					s->addWithLabel(_(feat.name.c_str()), cf);

				s->addSaveFunc([cf, storageName] { SystemConf::getInstance()->set(storageName, cf->getSelected()); });
			}
		}
	}

	// Custom config for systems
	s->addGroup(_("SETTINGS"));

	s->addEntry(_("PER SYSTEM ADVANCED CONFIGURATION"), true, [this, s, window]
	{
		s->save();
		GuiSettings* configuration = new GuiSettings(window, _("PER SYSTEM ADVANCED CONFIGURATION").c_str());

		// For each activated system
		std::vector<SystemData *> systems = SystemData::sSystemVector;
		for (auto system : systems)
		{
			if (system->isCollection() || !system->isGameSystem())
				continue;

			if (system->hasPlatformId(PlatformIds::PLATFORM_IGNORE))
				continue;

			if (!system->hasFeatures() && !system->hasEmulatorSelection())
				continue;

			configuration->addEntry(system->getFullName(), true, [this, system, window] {
				popSystemConfigurationGui(window, system);
			});
		}

		window->pushGui(configuration);
	});

	if (SystemConf::getInstance()->get("system.es.menu") != "bartop")
	{
		s->addGroup(_("SYSTEM SETTINGS"));

		// Retroachievements
		if (ApiSystem::getInstance()->isScriptingSupported(ApiSystem::RETROACHIVEMENTS))
		{
			/*
			if (SystemConf::getInstance()->getBool("global.retroachievements") &&
				!Settings::getInstance()->getBool("RetroachievementsMenuitem") &&
				SystemConf::getInstance()->get("global.retroachievements.username") != "")
				s->addEntry(_("RETROACHIEVEMENTS").c_str(), true, [this] { GuiRetroAchievements::show(mWindow); }, "iconRetroachievements");
				*/
			s->addEntry(_("RETROACHIEVEMENT SETTINGS"), true, [this] { openRetroachievementsSettings(); });
		}

		// Netplay
		if (SystemData::isNetplayActivated() && ApiSystem::getInstance()->isScriptingSupported(ApiSystem::NETPLAY))
			s->addEntry(_("NETPLAY SETTINGS"), true, [this] { openNetplaySettings(); }, "iconNetplay");

//		// Missing Bios
//		if (ApiSystem::getInstance()->isScriptingSupported(ApiSystem::BIOSINFORMATION))
//		{
//			s->addEntry(_("MISSING BIOS CHECK"), true, [this, s] { openMissingBiosSettings(); });
//#ifndef _ENABLEEMUELEC
//			auto checkBiosesAtLaunch = std::make_shared<SwitchComponent>(mWindow);
//			checkBiosesAtLaunch->setState(Settings::getInstance()->getBool("CheckBiosesAtLaunch"));
//			s->addWithLabel(_("CHECK BIOS FILES BEFORE RUNNING A GAME"), checkBiosesAtLaunch);
//			s->addSaveFunc([checkBiosesAtLaunch] { Settings::getInstance()->setBool("CheckBiosesAtLaunch", checkBiosesAtLaunch->getState()); });
//#endif
//		}

		// Game List Update
		// s->addEntry(_("UPDATE GAME LISTS"), false, [this, window] { updateGameLists(window); });
	}

	mWindow->pushGui(s);
}

void GuiMenu::openMissingBiosSettings()
{
	GuiBios::show(mWindow);
}

void GuiMenu::updateGameLists(Window* window, bool confirm)
{
	if (ThreadedScraper::isRunning())
	{
		window->pushGui(new GuiMsgBox(window, _("SCRAPER IS RUNNING. DO YOU WANT TO STOP IT?"),
			_("YES"), [] { ThreadedScraper::stop(); },
			_("NO"), nullptr));

		return;
	}

	if (ThreadedHasher::isRunning())
	{
		window->pushGui(new GuiMsgBox(window, _("GAME HASHING IS RUNNING. DO YOU WANT TO STOP IT?"),
			_("YES"), [] { ThreadedHasher::stop(); },
			_("NO"), nullptr));

		return;
	}

	if (!confirm)
	{
		ViewController::reloadAllGames(window, true, true);
		return;
	}

	window->pushGui(new GuiMsgBox(window, _("REALLY UPDATE GAMELISTS?"), _("YES"), [window]
		{
		ViewController::reloadAllGames(window, true, true);
		},
		_("NO"), nullptr));
}

void GuiMenu::openSystemEmulatorSettings(SystemData* system)
{
	auto theme = ThemeData::getMenuTheme();

	GuiSettings* s = new GuiSettings(mWindow, system->getFullName().c_str());

	auto emul_choice = std::make_shared<OptionListComponent<std::string>>(mWindow, _("Emulator"), false);
	auto core_choice = std::make_shared<OptionListComponent<std::string>>(mWindow, _("Core"), false);

	std::string currentEmul = system->getEmulator(false);
	std::string defaultEmul = system->getDefaultEmulator();

	emul_choice->add(_("DEFAULT"), "", false);

	bool found = false;
	for (auto emul : system->getEmulators())
	{
		if (emul.name == currentEmul)
			found = true;

		emul_choice->add(emul.name, emul.name, emul.name == currentEmul);
	}

	if (!found)
		emul_choice->selectFirstItem();

	ComponentListRow row;
	row.addElement(std::make_shared<TextComponent>(mWindow, Utils::String::toUpper(_("Emulator")), theme->Text.font, theme->Text.color), true);
	row.addElement(emul_choice, false);

	s->addRow(row);

	emul_choice->setSelectedChangedCallback([this, system, core_choice](std::string emulatorName)
	{
		std::string currentCore = system->getCore(false);
		std::string defaultCore = system->getDefaultCore(emulatorName);

		core_choice->clear();
		core_choice->add(_("DEFAULT"), "", false);

		bool found = false;

		for (auto& emulator : system->getEmulators())
		{
			if (emulatorName != emulator.name)
				continue;

			for (auto core : emulator.cores)
			{
				core_choice->add(core.name, core.name, currentCore == core.name);
				if (currentCore == core.name)
					found = true;
			}
		}

		if (!found)
			core_choice->selectFirstItem();
		else
			core_choice->invalidate();
	});

	row.elements.clear();
	row.addElement(std::make_shared<TextComponent>(mWindow, Utils::String::toUpper(_("Core")), theme->Text.font, theme->Text.color), true);
	row.addElement(core_choice, false);
	s->addRow(row);

	// force change event to load core list
	emul_choice->invalidate();


	s->addSaveFunc([system, emul_choice, core_choice]
	{
		Settings::getInstance()->setString(system->getName() + ".emulator", emul_choice->getSelected());
		Settings::getInstance()->setString(system->getName() + ".core", core_choice->getSelected());
	});

	mWindow->pushGui(s);
}

void GuiMenu::openEmulatorSettings()
{
	GuiSettings* configuration = new GuiSettings(mWindow, _("EMULATOR SETTINGS").c_str());

	Window* window = mWindow;

	// For each activated system
	for (auto system : SystemData::sSystemVector)
	{
		if (system->isCollection())
			continue;

		if (system->getEmulators().size() == 0)
			continue;

		if (system->getEmulators().size() == 1 && system->getEmulators().cbegin()->cores.size() <= 1)
			continue;

		configuration->addEntry(system->getFullName(), true, [this, system] { openSystemEmulatorSettings(system); });
	}

	window->pushGui(configuration);
}

void GuiMenu::openControllersSettings_batocera(int autoSel)
{
	GuiSettings* s = new GuiSettings(mWindow, controllers_settings_label.c_str());

	Window *window = mWindow;

	// CONTROLLER CONFIGURATION
	s->addEntry(_("CONTROLLER MAPPING"), false, [window, this, s]
	{
		window->pushGui(new GuiMsgBox(window,
			_("YOU ARE GOING TO MAP A CONTROLLER. MAP BASED ON THE BUTTON'S POSITION "
				"RELATIVE TO ITS EQUIVALENT ON A SNES CONTROLLER, NOT ITS PHYSICAL LABEL. "
				"IF YOU DO NOT HAVE A SPECIAL KEY FOR HOTKEY, USE THE SELECT BUTTON. SKIP "
				"ALL BUTTONS/STICKS YOU DO NOT HAVE BY HOLDING ANY KEY. PRESS THE "
				"SOUTH BUTTON TO CONFIRM WHEN DONE."), _("OK"),
			[window, this, s] {
			window->pushGui(new GuiDetectDevice(window, false, [this, s] {
				s->setSave(false);
				delete s;
				this->openControllersSettings_batocera();
			}));
		}));
	});

	if (ApiSystem::getInstance()->isScriptingSupported(ApiSystem::BLUETOOTH))
	{
		// BLUETOOTH TOGGLE
		auto bluetoothd_enabled = std::make_shared<SwitchComponent>(mWindow);
		bool btbaseEnabled = SystemConf::getInstance()->get("bluetooth.enabled") == "1";
		bluetoothd_enabled->setState(btbaseEnabled);
		s->addWithLabel(_("ENABLE BLUETOOTH"), bluetoothd_enabled);
		bluetoothd_enabled->setOnChangedCallback([this, s, bluetoothd_enabled]() {
			if (bluetoothd_enabled->getState() == false) {
                                runSystemCommand("systemctl stop bluealsa", "", nullptr);
                                runSystemCommand("systemctl stop bluetooth", "", nullptr);
                                runSystemCommand("systemctl stop bluetoothsense", "", nullptr);
                                runSystemCommand("systemctl stop bluetooth-agent", "", nullptr);
                                runSystemCommand("rm /storage/.cache/services/bluez.conf", "", nullptr);
                                runSystemCommand("rfkill block bluetooth", "", nullptr);
			} else {
                                runSystemCommand("mkdir -p /storage/.cache/services/", "", nullptr);
                                runSystemCommand("touch /storage/.cache/services/bluez.conf", "", nullptr);
                                runSystemCommand("systemctl start bluetooth", "", nullptr);
                                runSystemCommand("systemctl start bluetooth-agent", "", nullptr);
                                runSystemCommand("systemctl start bluetoothsense", "", nullptr);
                                runSystemCommand("systemctl start bluealsa", "", nullptr);
                                runSystemCommand("rfkill unblock bluetooth", "", nullptr);
				mWindow->pushGui(new GuiLoading<bool>(mWindow, _("ENABLING BLUETOOTH"),
					[this] {
						// batocera-bluetooth-agent sleeps 2000 to ensure hardware is
						// initialised, extra second gives it time to initialise itself.
						std::this_thread::sleep_for(std::chrono::milliseconds(3000));
						return true;
					},
					[this](bool ret) {}));
			}
			bool bluetoothenabled = bluetoothd_enabled->getState();
			SystemConf::getInstance()->set("bluetooth.enabled", bluetoothenabled ? "1" : "0");
			SystemConf::getInstance()->saveSystemConf();
		});

		// PAIR A BLUETOOTH CONTROLLER OR BT AUDIO DEVICE
		s->addEntry(_("PAIR A BLUETOOTH DEVICE"), false, [window, bluetoothd_enabled] {
			if (bluetoothd_enabled->getState() == false) {
				window->pushGui(new GuiMsgBox(window, _("BLUETOOTH IS DISABLED")));
			} else {
				ThreadedBluetooth::start(window);
			}
		});

		// FORGET BLUETOOTH CONTROLLERS OR BT AUDIO DEVICES
		s->addEntry(_("FORGET A BLUETOOTH DEVICE"), false, [window, this, s]
		{
			window->pushGui(new GuiBluetooth(window));
		});
	}

	ComponentListRow row;

	// Here we go; for each player
	std::list<int> alreadyTaken = std::list<int>();

	// clear the current loaded inputs
	clearLoadedInput();

	std::vector<std::shared_ptr<OptionListComponent<StrInputConfig *>>> options;
	//char strbuf[256];

	auto configList = InputManager::getInstance()->getInputConfigs();

	for (int player = 0; player < MAX_PLAYERS; player++)
	{
		std::string label = Utils::String::format(_("P%i'S CONTROLLER").c_str(), player + 1);
		std::string confName = Utils::String::format("INPUT P%iNAME", player + 1);
		std::string confGuid = Utils::String::format("INPUT P%iGUID", player + 1);

		LOG(LogInfo) << player + 1 << " " << confName << " " << confGuid;
		auto inputOptionList = std::make_shared<OptionListComponent<StrInputConfig *> >(mWindow, label, false);
		inputOptionList->add(_("default"), nullptr, false);
		options.push_back(inputOptionList);

		// Checking if a setting has been saved, else setting to default
		std::string configuratedName = Settings::getInstance()->getString(confName);
		std::string configuratedGuid = Settings::getInstance()->getString(confGuid);
		bool found = false;

		// For each available and configured input
		for (auto config : configList)
		{
			// create name
			std::stringstream dispNameSS;
			dispNameSS << "#" << config->getDeviceIndex() << " ";

			std::string deviceName = config->getDeviceName();
			if (deviceName.size() > 25)
				dispNameSS << deviceName.substr(0, 16) << "..." << deviceName.substr(deviceName.size() - 5, deviceName.size() - 1);
			else
				dispNameSS << deviceName;

			std::string displayName = dispNameSS.str();

			bool foundFromConfig = configuratedName == config->getDeviceName() && configuratedGuid == config->getDeviceGUIDString();
			int deviceID = config->getDeviceId();
			// Si la manette est configurée, qu'elle correspond a la configuration, et qu'elle n'est pas
			// deja selectionnée on l'ajoute en séléctionnée
			StrInputConfig* newInputConfig = new StrInputConfig(config->getDeviceName(), config->getDeviceGUIDString());
			mLoadedInput.push_back(newInputConfig);

			if (foundFromConfig && std::find(alreadyTaken.begin(), alreadyTaken.end(), deviceID) == alreadyTaken.end() && !found)
			{
				found = true;
				alreadyTaken.push_back(deviceID);

				LOG(LogWarning) << "adding entry for player" << player << " (selected): " << config->getDeviceName() << "  " << config->getDeviceGUIDString();
				inputOptionList->add(displayName, newInputConfig, true);
			}
			else
			{
				LOG(LogInfo) << "adding entry for player" << player << " (not selected): " << config->getDeviceName() << "  " << config->getDeviceGUIDString();
				inputOptionList->add(displayName, newInputConfig, false);
			}
		}

		if (!inputOptionList->hasSelection())
			inputOptionList->selectFirstItem();

		// Populate controllers list
		s->addWithLabel(label, inputOptionList);
	}

	s->addSaveFunc([this, options, window]
	{
		bool changed = false;

		for (int player = 0; player < MAX_PLAYERS; player++)
		{
			std::stringstream sstm;
			sstm << "INPUT P" << player + 1;
			std::string confName = sstm.str() + "NAME";
			std::string confGuid = sstm.str() + "GUID";

			auto input = options.at(player);

			StrInputConfig* selected = input->getSelected();
			if (selected == nullptr)
			{
				changed |= Settings::getInstance()->setString(confName, "DEFAULT");
				changed |= Settings::getInstance()->setString(confGuid, "");
			}
			else if (input->changed())
			{
				LOG(LogWarning) << "Found the selected controller ! : name in list  = " << input->getSelectedName();
				LOG(LogWarning) << "Found the selected controller ! : guid  = " << selected->deviceGUIDString;

				changed |= Settings::getInstance()->setString(confName, selected->deviceName);
				changed |= Settings::getInstance()->setString(confGuid, selected->deviceGUIDString);
			}
		}

		if (changed)
			Settings::getInstance()->saveFile();

		// this is dependant of this configuration, thus update it
		InputManager::getInstance()->computeLastKnownPlayersDeviceIndexes();
	});

	// CONTROLLER ACTIVITY
	auto activity = std::make_shared<SwitchComponent>(mWindow);
	activity->setState(Settings::getInstance()->getBool("ShowControllerActivity"));
	s->addWithLabel(_("SHOW CONTROLLER ACTIVITY"), activity, autoSel == 1);
	activity->setOnChangedCallback([this, s, activity]
	{
		if (Settings::getInstance()->setBool("ShowControllerActivity", activity->getState()))
		{
			delete s;
			openControllersSettings_batocera(1);
		}
	});

	if (Settings::getInstance()->getBool("ShowControllerActivity"))
	{
		// CONTROLLER BATTERY
		auto battery = std::make_shared<SwitchComponent>(mWindow);
		battery->setState(Settings::getInstance()->getBool("ShowControllerBattery"));
		s->addWithLabel(_("SHOW CONTROLLER BATTERY LEVEL"), battery);
		s->addSaveFunc([battery] { Settings::getInstance()->setBool("ShowControllerBattery", battery->getState()); });
	}

	window->pushGui(s);
}

struct ThemeConfigOption
{
	std::string defaultSettingName;
	std::string subset;
	std::shared_ptr<OptionListComponent<std::string>> component;
};

void GuiMenu::openThemeConfiguration(Window* mWindow, GuiComponent* s, std::shared_ptr<OptionListComponent<std::string>> theme_set, const std::string systemTheme)
{
	if (theme_set != nullptr && Settings::getInstance()->getString("ThemeSet") != theme_set->getSelected())
	{
		mWindow->pushGui(new GuiMsgBox(mWindow, _("YOU MUST APPLY THE THEME BEFORE EDITING ITS CONFIGURATION"), _("OK")));
		return;
	}

	Window* window = mWindow;

	auto system = ViewController::get()->getState().getSystem();
	auto theme = system->getTheme();

	auto themeconfig = new GuiSettings(mWindow, (systemTheme.empty() ? _("THEME CONFIGURATION") : _("VIEW CUSTOMIZATION")).c_str());

	auto themeSubSets = theme->getSubSets();

	std::string viewName;
	bool showGridFeatures = true;
	if (!systemTheme.empty())
	{
		auto glv = ViewController::get()->getGameListView(system);
		viewName = glv->getName();
		std::string baseType = theme->getCustomViewBaseType(viewName);

		showGridFeatures = (viewName == "grid" || baseType == "grid");
	}

	// gamelist_style
	std::shared_ptr<OptionListComponent<std::string>> gamelist_style = nullptr;

	if (systemTheme.empty() || showGridFeatures && system != NULL && theme->hasView("grid"))
		themeconfig->addGroup(_("GAMELIST STYLE"));

	if (systemTheme.empty())
	{
		gamelist_style = std::make_shared< OptionListComponent<std::string> >(mWindow, _("GAMELIST VIEW STYLE"), false);

		std::vector<std::pair<std::string, std::string>> styles;
		styles.push_back(std::pair<std::string, std::string>("automatic", _("automatic")));

		bool showViewStyle = true;

		if (system != NULL)
		{
			auto mViews = theme->getViewsOfTheme();

			showViewStyle = mViews.size() > 1;

			for (auto it = mViews.cbegin(); it != mViews.cend(); ++it)
			{
				if (it->first == "basic" || it->first == "detailed" || it->first == "grid" || it->first == "video" || it->first == "gamecarousel")
					styles.push_back(std::pair<std::string, std::string>(it->first, _(it->first.c_str())));
				else
					styles.push_back(*it);
			}
		}
		else
		{
			styles.push_back(std::pair<std::string, std::string>("basic", _("basic")));
			styles.push_back(std::pair<std::string, std::string>("detailed", _("detailed")));
		}

		auto viewPreference = systemTheme.empty() ? Settings::getInstance()->getString("GamelistViewStyle") : system->getSystemViewMode();
		if (!theme->hasView(viewPreference))
			viewPreference = "automatic";

		for (auto it = styles.cbegin(); it != styles.cend(); it++)
			gamelist_style->add(it->second, it->first, viewPreference == it->first);

		if (!gamelist_style->hasSelection())
			gamelist_style->selectFirstItem();

		if (showViewStyle)
			themeconfig->addWithLabel(_("GAMELIST VIEW STYLE"), gamelist_style);
	}

	// Default grid size
	std::shared_ptr<OptionListComponent<std::string>> mGridSize = nullptr;
	if (showGridFeatures && system != NULL && theme->hasView("grid"))
	{
		Vector2f gridOverride =
			systemTheme.empty() ? Vector2f::parseString(Settings::getInstance()->getString("DefaultGridSize")) :
			system->getGridSizeOverride();

		auto ovv = std::to_string((int)gridOverride.x()) + "x" + std::to_string((int)gridOverride.y());

		mGridSize = std::make_shared<OptionListComponent<std::string>>(mWindow, _("DEFAULT GRID SIZE"), false);

		bool found = false;
		for (auto it = GuiGamelistOptions::gridSizes.cbegin(); it != GuiGamelistOptions::gridSizes.cend(); it++)
		{
			bool sel = (gridOverride == Vector2f(0, 0) && *it == "automatic") || ovv == *it;
			if (sel)
				found = true;

			mGridSize->add(_(it->c_str()), *it, sel);
		}

		if (!found)
			mGridSize->selectFirstItem();

		themeconfig->addWithLabel(_("DEFAULT GRID SIZE"), mGridSize);
	}



	std::map<std::string, ThemeConfigOption> options;

	Utils::String::stringVector subsetNames = theme->getSubSetNames(viewName);

	// push appliesTo at end of list
	std::sort(subsetNames.begin(), subsetNames.end(), [themeSubSets](const std::string& a, const std::string& b) -> bool
	{
		auto sa = ThemeData::getSubSet(themeSubSets, a);
		auto sb = ThemeData::getSubSet(themeSubSets, b);

		bool aHasApplies = sa.size() > 0 && !sa.cbegin()->appliesTo.empty();
		bool bHasApplies = sb.size() > 0 && !sb.cbegin()->appliesTo.empty();

		return aHasApplies < bHasApplies;
	});

	bool hasThemeOptionGroup = false;
	bool hasApplyToGroup = false;
	for (std::string subset : subsetNames) // theme->getSubSetNames(viewName)
	{
		std::string settingName = "subset." + subset;
		std::string perSystemSettingName = systemTheme.empty() ? "" : "subset." + systemTheme + "." + subset;

		if (subset == "colorset") settingName = "ThemeColorSet";
		else if (subset == "iconset") settingName = "ThemeIconSet";
		else if (subset == "menu") settingName = "ThemeMenu";
		else if (subset == "systemview") settingName = "ThemeSystemView";
		else if (subset == "gamelistview") settingName = "ThemeGamelistView";
		else if (subset == "region") settingName = "ThemeRegionName";

		auto themeColorSets = ThemeData::getSubSet(themeSubSets, subset);

		if (themeColorSets.size() > 0)
		{
			auto selectedColorSet = themeColorSets.end();
			auto selectedName = !perSystemSettingName.empty() ? Settings::getInstance()->getString(perSystemSettingName) : Settings::getInstance()->getString(settingName);

			if (!perSystemSettingName.empty() && selectedName.empty())
				selectedName = Settings::getInstance()->getString(settingName);

			for (auto it = themeColorSets.begin(); it != themeColorSets.end() && selectedColorSet == themeColorSets.end(); it++)
				if (it->name == selectedName)
					selectedColorSet = it;

			std::shared_ptr<OptionListComponent<std::string>> item = std::make_shared<OptionListComponent<std::string> >(mWindow, _(("THEME " + Utils::String::toUpper(subset)).c_str()), false);
			item->setTag(!perSystemSettingName.empty() ? perSystemSettingName : settingName);

			std::string defaultName;
			for (auto it = themeColorSets.begin(); it != themeColorSets.end(); it++)
			{
				std::string displayName = it->displayName;

				if (!systemTheme.empty())
				{
					std::string defaultValue = Settings::getInstance()->getString(settingName);
					if (defaultValue.empty())
						defaultValue = system->getTheme()->getDefaultSubSetValue(subset);

					if (it->name == defaultValue)
					{
						defaultName = Utils::String::toUpper(displayName);
						// displayName = displayName + " (" + _("DEFAULT") + ")";
					}
				}

				item->add(displayName, it->name, it == selectedColorSet);
			}

			if (selectedColorSet == themeColorSets.end())
				item->selectFirstItem();

			if (!themeColorSets.empty())
			{
				std::string displayName = themeColorSets.cbegin()->subSetDisplayName;
				if (!displayName.empty())
				{
					bool hasApplyToSubset = themeColorSets.cbegin()->appliesTo.size() > 0;

					std::string prefix;

					if (systemTheme.empty())
					{
						for (auto subsetName : themeColorSets.cbegin()->appliesTo)
						{
							std::string pfx = theme->getViewDisplayName(subsetName);
							if (!pfx.empty())
							{
								if (prefix.empty())
									prefix = pfx;
								else
									prefix = prefix + ", " + pfx;
							}
						}

						prefix = Utils::String::toUpper(prefix);
					}

					if (hasApplyToSubset && !hasApplyToGroup)
					{
						hasApplyToGroup = true;
						themeconfig->addGroup(_("GAMELIST THEME OPTIONS"));
					}
					else if (!hasApplyToSubset && !hasThemeOptionGroup)
					{
						hasThemeOptionGroup = true;
						themeconfig->addGroup(_("THEME OPTIONS"));
					}

					if (!prefix.empty())
						themeconfig->addWithDescription(displayName, prefix, item);
					else if (!defaultName.empty())
						themeconfig->addWithDescription(displayName, _("DEFAULT VALUE") + " : " + defaultName, item);
					else
						themeconfig->addWithLabel(displayName + prefix, item);
				}
				else
				{
					if (!hasThemeOptionGroup)
					{
						hasThemeOptionGroup = true;
						themeconfig->addGroup(_("THEME OPTIONS"));
					}

					themeconfig->addWithLabel(_(("THEME " + Utils::String::toUpper(subset)).c_str()), item);
				}
			}

			ThemeConfigOption opt;
			opt.component = item;
			opt.subset = subset;
			opt.defaultSettingName = settingName;
			options[!perSystemSettingName.empty() ? perSystemSettingName : settingName] = opt;
		}
		else
		{
			ThemeConfigOption opt;
			opt.component = nullptr;
			options[!perSystemSettingName.empty() ? perSystemSettingName : settingName] = opt;
		}
	}


	if (!systemTheme.empty())
	{
		themeconfig->addGroup(_("GAMELIST OPTIONS"));

		// Show favorites first in gamelists
		auto fav = Settings::getInstance()->getString(system->getName() + ".FavoritesFirst");
		auto favoritesFirst = std::make_shared<OptionListComponent<std::string>>(mWindow, _("SHOW FAVORITES ON TOP"), false);
		std::string defFav = Settings::getInstance()->getBool("FavoritesFirst") ? _("YES") : _("NO");
		favoritesFirst->add(_("DEFAULT"), "", fav == "" || fav == "default");
		favoritesFirst->add(_("YES"), "1", fav == "1");
		favoritesFirst->add(_("NO"), "0", fav == "0");
		themeconfig->addWithDescription(_("SHOW FAVORITES ON TOP"), _("DEFAULT VALUE") + " : " + defFav, favoritesFirst);
		themeconfig->addSaveFunc([themeconfig, favoritesFirst, system]
		{
			if (Settings::getInstance()->setString(system->getName() + ".FavoritesFirst", favoritesFirst->getSelected()))
				themeconfig->setVariable("reloadAll", true);
		});

		// Show favorites first in gamelists
		auto defHid = Settings::ShowHiddenFiles() ? _("YES") : _("NO");
		auto curhid = Settings::getInstance()->getString(system->getName() + ".ShowHiddenFiles");
		auto hiddenFiles = std::make_shared<OptionListComponent<std::string>>(mWindow, _("SHOW HIDDEN FILES"), false);
		hiddenFiles->add(_("DEFAULT"), "", curhid == "" || curhid == "default");
		hiddenFiles->add(_("YES"), "1", curhid == "1");
		hiddenFiles->add(_("NO"), "0", curhid == "0");
		themeconfig->addWithDescription(_("SHOW HIDDEN FILES"), _("DEFAULT VALUE") + " : " + defHid, hiddenFiles);
		themeconfig->addSaveFunc([themeconfig, hiddenFiles, system]
		{
			if (Settings::getInstance()->setString(system->getName() + ".ShowHiddenFiles", hiddenFiles->getSelected()))
				themeconfig->setVariable("reloadAll", true);
		});

		// Folder View Mode
		auto folderView = Settings::getInstance()->getString("FolderViewMode");
		auto defFol = folderView.empty() ? "" : Utils::String::toUpper(_(folderView.c_str()));
		auto curFol = Settings::getInstance()->getString(system->getName() + ".FolderViewMode");

		auto foldersBehavior = std::make_shared<OptionListComponent<std::string>>(mWindow, _("SHOW FOLDERS"), false);
		foldersBehavior->add(_("DEFAULT"), "", curFol == "" || curFol == "default"); //  + " (" + defFol + ")"
		foldersBehavior->add(_("always"), "always", curFol == "always");
		foldersBehavior->add(_("never"), "never", curFol == "never");
		foldersBehavior->add(_("having multiple games"), "having multiple games", curFol == "having multiple games");

		themeconfig->addWithDescription(_("SHOW FOLDERS"), _("DEFAULT VALUE") + " : " + defFol, foldersBehavior);
		themeconfig->addSaveFunc([themeconfig, foldersBehavior, system]
		{
			if (Settings::getInstance()->setString(system->getName() + ".FolderViewMode", foldersBehavior->getSelected()))
				themeconfig->setVariable("reloadAll", true);
		});

		// Show parent folder in gamelists
		auto defPf = Settings::getInstance()->getBool("ShowParentFolder") ? _("YES") : _("NO");
		auto curPf = Settings::getInstance()->getString(system->getName() + ".ShowParentFolder");
		auto parentFolder = std::make_shared<OptionListComponent<std::string>>(mWindow, _("SHOW '..' PARENT FOLDER"), false);
		parentFolder->add(_("DEFAULT"), "", curPf == "" || curPf == "default");
		parentFolder->add(_("YES"), "1", curPf == "1");
		parentFolder->add(_("NO"), "0", curPf == "0");
		themeconfig->addWithDescription(_("SHOW '..' PARENT FOLDER"), _("DEFAULT VALUE") + " : " + defPf, parentFolder);
		themeconfig->addSaveFunc([themeconfig, parentFolder, system]
		{
			if (Settings::getInstance()->setString(system->getName() + ".ShowParentFolder", parentFolder->getSelected()))
				themeconfig->setVariable("reloadAll", true);
		});

		// Show flags

		auto defSF = Settings::getInstance()->getString("ShowFlags");
		if (defSF == "1")
			defSF = _("BEFORE NAME");
		else if (defSF == "2")
			defSF = _("AFTER NAME");
		else
			defSF = _("NO");

		auto curSF = Settings::getInstance()->getString(system->getName() + ".ShowFlags");
		auto showRegionFlags = std::make_shared<OptionListComponent<std::string>>(mWindow, _("SHOW REGION FLAG"), false);

		showRegionFlags->addRange({
			{ _("DEFAULT"), "default" },
			{ _("NO"), "0" },
			{ _("BEFORE NAME") , "1" },
			{ _("AFTER NAME"), "2" } },
			curSF);

		themeconfig->addWithDescription(_("SHOW REGION FLAG"), _("DEFAULT VALUE") + " : " + defSF, showRegionFlags);
		themeconfig->addSaveFunc([themeconfig, showRegionFlags, system]
		{
			if (Settings::getInstance()->setString(system->getName() + ".ShowFlags", showRegionFlags->getSelected()))
				themeconfig->setVariable("reloadAll", true);
		});

		// Show SaveStates
		auto defSS = Settings::getInstance()->getBool("ShowSaveStates") ? _("YES") : _("NO");
		auto curSS = Settings::getInstance()->getString(system->getName() + ".ShowSaveStates");
		auto showSaveStates = std::make_shared<OptionListComponent<std::string>>(mWindow, _("SHOW SAVESTATE ICON"), false);
		showSaveStates->add(_("DEFAULT"), "", curSS == "" || curSS == "default");
		showSaveStates->add(_("YES"), "1", curSS == "1");
		showSaveStates->add(_("NO"), "0", curSS == "0");
		themeconfig->addWithDescription(_("SHOW SAVESTATE ICON"), _("DEFAULT VALUE") + " : " + defSS, showSaveStates);
		themeconfig->addSaveFunc([themeconfig, showSaveStates, system]
		{
			if (Settings::getInstance()->setString(system->getName() + ".ShowSaveStates", showSaveStates->getSelected()))
				themeconfig->setVariable("reloadAll", true);
		});

		// Show Manual
		auto defMM = Settings::getInstance()->getBool("ShowManualIcon") ? _("YES") : _("NO");
		auto curMM = Settings::getInstance()->getString(system->getName() + ".ShowManualIcon");
		auto showManual = std::make_shared<OptionListComponent<std::string>>(mWindow, _("SHOW MANUAL ICON"), false);
		showManual->add(_("DEFAULT"), "", curMM == "" || curMM == "default");
		showManual->add(_("YES"), "1", curMM == "1");
		showManual->add(_("NO"), "0", curMM == "0");
		themeconfig->addWithDescription(_("SHOW MANUAL ICON"), _("DEFAULT VALUE") + " : " + defMM, showManual);
		themeconfig->addSaveFunc([themeconfig, showManual, system]
		{
			if (Settings::getInstance()->setString(system->getName() + ".ShowManualIcon", showManual->getSelected()))
				themeconfig->setVariable("reloadAll", true);
		});

		// Show filenames
		auto defFn = Settings::getInstance()->getBool("ShowFilenames") ? _("YES") : _("NO");
		auto curFn = Settings::getInstance()->getString(system->getName() + ".ShowFilenames");

		auto showFilenames = std::make_shared<OptionListComponent<std::string>>(mWindow, _("SHOW FILENAMES INSTEAD"), false);
		showFilenames->add(_("DEFAULT"), "", curFn == "");
		showFilenames->add(_("YES"), "1", curFn == "1");
		showFilenames->add(_("NO"), "0", curFn == "0");
		themeconfig->addWithDescription(_("SHOW FILENAMES INSTEAD"), _("DEFAULT VALUE") + " : " + defFn, showFilenames);
		themeconfig->addSaveFunc([themeconfig, showFilenames, system]
		{
			if (Settings::getInstance()->setString(system->getName() + ".ShowFilenames", showFilenames->getSelected()))
			{
				SystemData::resetSettings();
				FileData::resetSettings();

		//		themeconfig->setVariable("reloadCollections", true);
				themeconfig->setVariable("reloadAll", true);
			}
		});


		// File extensions
		if (!system->isCollection() && system->isGameSystem())
		{
			auto hiddenExts = Utils::String::split(Settings::getInstance()->getString(system->getName() + ".HiddenExt"), ';');

			auto hiddenCtrl = std::make_shared<OptionListComponent<std::string>>(mWindow, _("FILE EXTENSIONS"), true);

			for (auto ext : system->getExtensions())
			{
				std::string extid = Utils::String::toLower(Utils::String::replace(ext, ".", ""));
				hiddenCtrl->add(ext, extid, std::find(hiddenExts.cbegin(), hiddenExts.cend(), extid) == hiddenExts.cend());
			}

			themeconfig->addWithLabel(_("FILE EXTENSIONS"), hiddenCtrl);
			themeconfig->addSaveFunc([themeconfig, system, hiddenCtrl]
			{
				std::string hiddenSystems;

				std::vector<std::string> sel = hiddenCtrl->getSelectedObjects();

				for (auto ext : system->getExtensions())
				{
					std::string extid = Utils::String::toLower(Utils::String::replace(ext, ".", ""));
					if (std::find(sel.cbegin(), sel.cend(), extid) == sel.cend())
					{
						if (hiddenSystems.empty())
							hiddenSystems = extid;
						else
							hiddenSystems = hiddenSystems + ";" + extid;
					}
				}

				if (Settings::getInstance()->setString(system->getName() + ".HiddenExt", hiddenSystems))
				{
					Settings::getInstance()->saveFile();

					themeconfig->setVariable("reloadAll", true);
					themeconfig->setVariable("forceReloadGames", true);
				}
			});
		}
	}

	if (systemTheme.empty())
	{
		themeconfig->addGroup(_("TOOLS"));

		themeconfig->addEntry(_("RESET CUSTOMIZATIONS"), false, [s, themeconfig, window]
		{
			themeconfig->setVariable("resetTheme", true);
			themeconfig->setVariable("reloadAll", true);
			themeconfig->close();
		});
	}

	//  theme_colorset, theme_iconset, theme_menu, theme_systemview, theme_gamelistview, theme_region,
	themeconfig->addSaveFunc([systemTheme, system, themeconfig, options, gamelist_style, mGridSize, window]
	{
		bool reloadAll = false;

		for (auto option : options)
		{
			ThemeConfigOption& opt = option.second;

			std::string value;

			if (opt.component != nullptr)
			{
				value = opt.component->getSelected();

				if (!systemTheme.empty() && !value.empty())
				{
					std::string defaultValue = Settings::getInstance()->getString(opt.defaultSettingName);
					if (defaultValue.empty())
						defaultValue = system->getTheme()->getDefaultSubSetValue(opt.subset);

					if (value == defaultValue)
						value = "";
				}
				else if (systemTheme.empty() && value == system->getTheme()->getDefaultSubSetValue(opt.subset))
					value = "";
			}

			if (value != Settings::getInstance()->getString(option.first))
				reloadAll |= Settings::getInstance()->setString(option.first, value);
		}

		Vector2f gridSizeOverride(0, 0);

		if (mGridSize != nullptr)
		{
			std::string str = mGridSize->getSelected();
			std::string value = "";

			size_t divider = str.find('x');
			if (divider != std::string::npos)
			{
				std::string first = str.substr(0, divider);
				std::string second = str.substr(divider + 1, std::string::npos);

				gridSizeOverride = Vector2f((float)atof(first.c_str()), (float)atof(second.c_str()));
				value = Utils::String::replace(Utils::String::replace(gridSizeOverride.toString(), ".000000", ""), "0 0", "");
			}

			if (systemTheme.empty())
				reloadAll |= Settings::getInstance()->setString("DefaultGridSize", value);
		}
		else if (systemTheme.empty())
			reloadAll |= Settings::getInstance()->setString("DefaultGridSize", "");

		if (systemTheme.empty())
			reloadAll |= Settings::getInstance()->setString("GamelistViewStyle", gamelist_style == nullptr ? "" : gamelist_style->getSelected());
		else
		{
			std::string viewMode = gamelist_style == nullptr ? system->getSystemViewMode() : gamelist_style->getSelected();
			reloadAll |= system->setSystemViewMode(viewMode, gridSizeOverride);
		}

		if (themeconfig->getVariable("resetTheme"))
		{
			Settings::getInstance()->setString("GamelistViewStyle", "");
			Settings::getInstance()->setString("DefaultGridSize", "");
			Settings::getInstance()->setString("ThemeRegionName", "");
			Settings::getInstance()->setString("ThemeColorSet", "");
			Settings::getInstance()->setString("ThemeIconSet", "");
			Settings::getInstance()->setString("ThemeMenu", "");
			Settings::getInstance()->setString("ThemeSystemView", "");
			Settings::getInstance()->setString("ThemeGamelistView", "");
			Settings::getInstance()->setString("GamelistViewStyle", "");
			Settings::getInstance()->setString("DefaultGridSize", "");

			for (auto sm : Settings::getInstance()->getStringMap())
				if (Utils::String::startsWith(sm.first, "subset."))
					Settings::getInstance()->setString(sm.first, "");

			for (auto system : SystemData::sSystemVector)
			{
				system->setSystemViewMode("automatic", Vector2f(0, 0));

				Settings::getInstance()->setString(system->getName() + ".FavoritesFirst", "");
				Settings::getInstance()->setString(system->getName() + ".ShowHiddenFiles", "");
				Settings::getInstance()->setString(system->getName() + ".FolderViewMode", "");
				Settings::getInstance()->setString(system->getName() + ".ShowFilenames", "");
				Settings::getInstance()->setString(system->getName() + ".ShowParentFolder", "");
			}

			Settings::getInstance()->saveFile();
			std::string path = Utils::FileSystem::getEsConfigPath() + "/themesettings/" + Settings::getInstance()->getString("ThemeSet") + ".cfg";
			if (Utils::FileSystem::exists(path))
				Utils::FileSystem::removeFile(path);
		}

		if (reloadAll || themeconfig->getVariable("reloadAll"))
		{
			if (themeconfig->getVariable("forceReloadGames"))
			{
				ViewController::reloadAllGames(window, false);
			}
			else if (systemTheme.empty())
			{
				CollectionSystemManager::get()->updateSystemsList();
				ViewController::get()->reloadAll(window);
				window->closeSplashScreen();
			}
			else
			{
				system->loadTheme();
				system->resetFilters();

				ViewController::get()->reloadSystemListViewTheme(system);
				ViewController::get()->reloadGameListView(system);
			}
		}
	});

	mWindow->pushGui(themeconfig);
}

void GuiMenu::openUISettings()
{
	auto pthis = this;
	Window* window = mWindow;

	auto s = new GuiSettings(mWindow, _("UI SETTINGS").c_str());

	// theme set
	auto theme = ThemeData::getMenuTheme();
	auto themeSets = ThemeData::getThemeSets();
	auto system = ViewController::get()->getState().getSystem();

	s->addGroup(_("APPEARANCE"));

	if (system != nullptr && !themeSets.empty())
	{
		auto selectedSet = themeSets.find(Settings::getInstance()->getString("ThemeSet"));
		if (selectedSet == themeSets.end())
			selectedSet = themeSets.begin();

		auto theme_set = std::make_shared<OptionListComponent<std::string> >(mWindow, _("THEME SET"), false);

		std::vector<std::string> themeList;
		for (auto it = themeSets.begin(); it != themeSets.end(); it++)
			themeList.push_back(it->first);

		std::sort(themeList.begin(), themeList.end(), [](const std::string& a, const std::string& b) -> bool { return Utils::String::toLower(a).compare(Utils::String::toLower(b)) < 0; });

		for (auto themeName : themeList)
			theme_set->add(themeName, themeName, themeName == selectedSet->first);

		//for (auto it = themeSets.begin(); it != themeSets.end(); it++)
		//	theme_set->add(it->first, it->first, it == selectedSet);

		s->addWithLabel(_("THEME SET"), theme_set);
		s->addSaveFunc([s, theme_set, pthis, window, system]
		{
			std::string oldTheme = Settings::getInstance()->getString("ThemeSet");
			if (oldTheme != theme_set->getSelected())
			{
				saveSubsetSettings();

				Settings::getInstance()->setString("ThemeSet", theme_set->getSelected());

				// theme changed without setting options, forcing options to avoid crash/blank theme
				Settings::getInstance()->setString("ThemeRegionName", "");
				Settings::getInstance()->setString("ThemeColorSet", "");
				Settings::getInstance()->setString("ThemeIconSet", "");
				Settings::getInstance()->setString("ThemeMenu", "");
				Settings::getInstance()->setString("ThemeSystemView", "");
				Settings::getInstance()->setString("ThemeGamelistView", "");
				Settings::getInstance()->setString("GamelistViewStyle", "");
				Settings::getInstance()->setString("DefaultGridSize", "");

				for(auto sm : Settings::getInstance()->getStringMap())
					if (Utils::String::startsWith(sm.first, "subset."))
						Settings::getInstance()->setString(sm.first, "");

				for (auto sysIt = SystemData::sSystemVector.cbegin(); sysIt != SystemData::sSystemVector.cend(); sysIt++)
					(*sysIt)->setSystemViewMode("automatic", Vector2f(0,0));

				loadSubsetSettings(theme_set->getSelected());

				s->setVariable("reloadCollections", true);
				s->setVariable("reloadAll", true);
				s->setVariable("reloadGuiMenu", true);

				Scripting::fireEvent("theme-changed", theme_set->getSelected(), oldTheme);
			}
		});

		bool showThemeConfiguration = system->getTheme()->hasSubsets() || system->getTheme()->hasView("grid");
		if (showThemeConfiguration)
		{
			s->addSubMenu(_("THEME CONFIGURATION"), [this, s, theme_set]() { openThemeConfiguration(mWindow, s, theme_set); });
		}
		else // GameList view style only, acts like Retropie for simple themes
		{
			auto gamelist_style = std::make_shared< OptionListComponent<std::string> >(mWindow, _("GAMELIST VIEW STYLE"), false);
			std::vector<std::pair<std::string, std::string>> styles;
			styles.push_back(std::pair<std::string, std::string>("automatic", _("automatic")));

			auto system = ViewController::get()->getState().getSystem();
			if (system != NULL)
			{
				auto mViews = system->getTheme()->getViewsOfTheme();
				for (auto it = mViews.cbegin(); it != mViews.cend(); ++it)
					styles.push_back(*it);
			}
			else
			{
				styles.push_back(std::pair<std::string, std::string>("basic", _("basic")));
				styles.push_back(std::pair<std::string, std::string>("detailed", _("detailed")));
				styles.push_back(std::pair<std::string, std::string>("video", _("video")));
				styles.push_back(std::pair<std::string, std::string>("grid", _("grid")));
			}

			auto viewPreference = Settings::getInstance()->getString("GamelistViewStyle");
			if (!system->getTheme()->hasView(viewPreference))
				viewPreference = "automatic";

			for (auto it = styles.cbegin(); it != styles.cend(); it++)
				gamelist_style->add(it->second, it->first, viewPreference == it->first);

			s->addWithLabel(_("GAMELIST VIEW STYLE"), gamelist_style);
			s->addSaveFunc([s, gamelist_style, window] {
				if (Settings::getInstance()->setString("GamelistViewStyle", gamelist_style->getSelected()))
				{
					s->setVariable("reloadAll", true);
					s->setVariable("reloadGuiMenu", true);
				}
			});
		}
	}

	// language choice
	auto language_choice = std::make_shared<OptionListComponent<std::string> >(window, _("LANGUAGE"), false);

	std::string language = SystemConf::getInstance()->get("system.language");
	if (language.empty())
		language = "en_US";

	language_choice->add("ARABIC",               "ar_YE", language == "ar_YE");
	language_choice->add("CATALÀ",               "ca_ES", language == "ca_ES");
	language_choice->add("CYMRAEG",              "cy_GB", language == "cy_GB");
	language_choice->add("DEUTSCH", 	     "de_DE", language == "de_DE");
	language_choice->add("GREEK",                "el_GR", language == "el_GR");
	language_choice->add("ENGLISH", 	     "en_US", language == "en_US" || language == "en");
	language_choice->add("ESPAÑOL", 	     "es_ES", language == "es_ES" || language == "es");
	language_choice->add("ESPAÑOL MEXICANO",     "es_MX", language == "es_MX");
	language_choice->add("BASQUE",               "eu_ES", language == "eu_ES");
	language_choice->add("FRANÇAIS",             "fr_FR", language == "fr_FR" || language == "fr");
	language_choice->add("עברית",                "he_IL", language == "he_IL");
	language_choice->add("HUNGARIAN",            "hu_HU", language == "hu_HU");
	language_choice->add("ITALIANO",             "it_IT", language == "it_IT");
	language_choice->add("JAPANESE", 	     "ja_JP", language == "ja_JP");
	language_choice->add("KOREAN",   	     "ko_KR", language == "ko_KR" || language == "ko");
	language_choice->add("NORWEGIAN BOKMAL",     "nb_NO", language == "nb_NO");
	language_choice->add("DUTCH",                "nl_NL", language == "nl_NL");
	language_choice->add("NORWEGIAN",            "nn_NO", language == "nn_NO");
	language_choice->add("OCCITAN",              "oc_FR", language == "oc_FR");
	language_choice->add("POLISH",               "pl_PL", language == "pl_PL");
	language_choice->add("PORTUGUES BRASILEIRO", "pt_BR", language == "pt_BR");
	language_choice->add("PORTUGUES PORTUGAL",   "pt_PT", language == "pt_PT");
	language_choice->add("РУССКИЙ",              "ru_RU", language == "ru_RU");
	language_choice->add("SVENSKA", 	     "sv_SE", language == "sv_SE");
	language_choice->add("TÜRKÇE",  	     "tr_TR", language == "tr_TR");
	language_choice->add("Українська",           "uk_UA", language == "uk_UA");
	language_choice->add("简体中文", 	     "zh_CN", language == "zh_CN");
	language_choice->add("正體中文", 	     "zh_TW", language == "zh_TW");
	s->addWithLabel(_("LANGUAGE"), language_choice);

	s->addSaveFunc([window, language_choice, language, s]
	{
		bool reboot = false;

		if (language_choice->changed())
		{
#ifdef _ENABLEEMUELEC
			std::string selectedLanguage = language_choice->getSelected();
			std::string msg = _("You are about to set your language to:") +"\n" +  selectedLanguage + "\n";
			msg += _("Emulationstation will restart")+"\n";
			msg += _("Do you want to proceed ?");
			window->pushGui(new GuiMsgBox(window, msg, _("YES"), [selectedLanguage] {
				SystemConf::getInstance()->set("system.language", selectedLanguage);
				SystemConf::getInstance()->saveSystemConf();
				quitES(QuitMode::QUIT);
			}, "NO",nullptr));
#else
			if (SystemConf::getInstance()->set("system.language", language_choice->getSelected()))
			{
				FileSorts::reset();
				MetaDataList::initMetadata();

				s->setVariable("reloadGuiMenu", true);
#ifdef HAVE_INTL
				reboot = true;
#endif
			}
#endif
		}

		if (reboot)
			window->displayNotificationMessage(_U("\uF011  ") + _("A REBOOT OF THE SYSTEM IS REQUIRED TO APPLY THE NEW CONFIGURATION"));

	});

	// UI RESTRICTIONS
	auto UImodeSelection = std::make_shared< OptionListComponent<std::string> >(mWindow, _("UI MODE"), false);
	std::vector<std::string> UImodes = UIModeController::getInstance()->getUIModes();
	for (auto it = UImodes.cbegin(); it != UImodes.cend(); it++)
		UImodeSelection->add(_(it->c_str()), *it, Settings::getInstance()->getString("UIMode") == *it);

	s->addWithLabel(_("UI MODE"), UImodeSelection);
	s->addSaveFunc([UImodeSelection, window]
	{
		std::string selectedMode = UImodeSelection->getSelected();
		if (selectedMode != "Full")
		{
			std::string msg = _("You are changing the UI to a restricted mode:\nThis will hide most menu-options to prevent changes to the system.\nTo unlock and return to the full UI, enter this code:") + "\n";
			msg += "\"" + UIModeController::getInstance()->getFormattedPassKeyStr() + "\"\n\n";
			msg += _("Do you want to proceed ?");
			window->pushGui(new GuiMsgBox(window, msg,
				_("YES"), [selectedMode] {
				LOG(LogDebug) << "Setting UI mode to " << selectedMode;
				Settings::getInstance()->setString("UIMode", selectedMode);
				Settings::getInstance()->saveFile();
			}, _("NO"), nullptr));
		}
	});

	// retroarch.menu_driver choose from 'auto' (default), 'xmb', 'rgui', 'ozone', 'glui'
	auto retroarchRgui = std::make_shared< OptionListComponent<std::string> >(mWindow, _("RETROARCH MENU DRIVER"), false);
	std::vector<std::string> driver;
	driver.push_back("default");
	driver.push_back("xmb");
	driver.push_back("rgui");
	driver.push_back("ozone");
	driver.push_back("glui");

	auto currentDriver = SystemConf::getInstance()->get("global.retroarch.menu_driver");
	if (currentDriver.empty())
		currentDriver = "default";

	for (auto it = driver.cbegin(); it != driver.cend(); it++)
		retroarchRgui->add(_(it->c_str()), *it, currentDriver == *it);

	s->addWithLabel(_("RETROARCH MENU DRIVER"), retroarchRgui);
	s->addSaveFunc([retroarchRgui]
	{
		SystemConf::getInstance()->set("global.retroarch.menu_driver", retroarchRgui->getSelected());
		SystemConf::getInstance()->saveSystemConf();
	});

        auto invertJoy = std::make_shared<SwitchComponent>(mWindow);
        invertJoy->setState(Settings::getInstance()->getBool("InvertButtons"));
        s->addWithLabel(_("SWITCH A & B BUTTONS IN EMULATIONSTATION"), invertJoy);
        s->addSaveFunc([this, s, invertJoy]
        {
                if (Settings::getInstance()->setBool("InvertButtons", invertJoy->getState()))
                {
                        InputConfig::AssignActionButtons();
                        s->setVariable("reloadAll", true);
                }
        });

	auto fps_enabled = std::make_shared<SwitchComponent>(mWindow);
	bool fpsEnabled = SystemConf::getInstance()->get("global.showFPS") == "1";
	fps_enabled->setState(fpsEnabled);
	s->addWithLabel(_("SHOW RETROARCH FPS"), fps_enabled);
	s->addSaveFunc([fps_enabled] {
		bool fpsenabled = fps_enabled->getState();
	SystemConf::getInstance()->set("global.showFPS", fpsenabled ? "1" : "0");
			SystemConf::getInstance()->saveSystemConf();
		});

	auto desktop_enabled = std::make_shared<SwitchComponent>(mWindow);
	bool desktopEnabled = SystemConf::getInstance()->get("desktop.enabled") == "1";
	desktop_enabled->setState(desktopEnabled);
	s->addWithLabel(_("DESKTOP MODE"), desktop_enabled);
	s->addSaveFunc([this,desktop_enabled] {
		if (desktop_enabled->changed()) {
	                std::string msg = _("The system will restart")+"\n";
	                msg += _("Do you want to continue?");
			mWindow->pushGui(new GuiMsgBox(mWindow,msg, _("YES"), 
				[this,desktop_enabled] {
					bool desktopenabled = desktop_enabled->getState();
					SystemConf::getInstance()->set("desktop.enabled", desktopenabled ? "1" : "0");
					SystemConf::getInstance()->saveSystemConf();
					quitES(QuitMode::REBOOT);
			}, "NO",nullptr));
		}
	});


	s->addGroup(_("DISPLAY OPTIONS"));

	s->addEntry(_("SCREENSAVER SETTINGS"), true, std::bind(&GuiMenu::openScreensaverOptions, this));

	// transition style
	auto transition_style = std::make_shared<OptionListComponent<std::string> >(mWindow, _("LIST TRANSITION STYLE"), false);
	transition_style->addRange({ "default", "fade", "slide", "fade & slide", "instant" }, Settings::TransitionStyle());
	s->addWithLabel(_("LIST TRANSITION STYLE"), transition_style);
	s->addSaveFunc([transition_style] { Settings::setTransitionStyle(transition_style->getSelected()); });

	// game transition style
	auto transitionOfGames_style = std::make_shared< OptionListComponent<std::string> >(mWindow, _("GAME LAUNCH TRANSITION"), false);
	transitionOfGames_style->addRange({ "default", "fade", "slide", "instant" }, Settings::GameTransitionStyle());
	s->addWithLabel(_("GAME LAUNCH TRANSITION"), transitionOfGames_style);
	s->addSaveFunc([transitionOfGames_style] { Settings::setGameTransitionStyle(transitionOfGames_style->getSelected()); });

	// clock
	auto clock = std::make_shared<SwitchComponent>(mWindow);
	clock->setState(Settings::getInstance()->getBool("DrawClock"));
	s->addWithLabel(_("SHOW CLOCK"), clock);
	s->addSaveFunc(
		[clock] { Settings::getInstance()->setBool("DrawClock", clock->getState()); });

	// show help
	auto show_help = std::make_shared<SwitchComponent>(mWindow);
	show_help->setState(Settings::getInstance()->getBool("ShowHelpPrompts"));
	s->addWithLabel(_("ON-SCREEN HELP"), show_help);
	s->addSaveFunc([s, show_help]
	{
		if (Settings::getInstance()->setBool("ShowHelpPrompts", show_help->getState()))
			s->setVariable("reloadAll", true);
	});

	// Battery indicator
	if (queryBatteryInformation().hasBattery)
	{
		auto batteryStatus = std::make_shared<OptionListComponent<std::string> >(mWindow, _("SHOW BATTERY STATUS"), false);
		batteryStatus->addRange({ { _("NO"), "" },{ _("ICON"), "icon" },{ _("ICON AND TEXT"), "text" } }, Settings::getInstance()->getString("ShowBattery"));
		s->addWithLabel(_("SHOW BATTERY STATUS"), batteryStatus);
		s->addSaveFunc([batteryStatus] { Settings::getInstance()->setString("ShowBattery", batteryStatus->getSelected()); });
	}

	s->addGroup(_("GAMELIST OPTIONS"));

	// Show favorites first in gamelists
	auto favoritesFirstSwitch = std::make_shared<SwitchComponent>(mWindow);
	favoritesFirstSwitch->setState(Settings::getInstance()->getBool("FavoritesFirst"));
	s->addWithLabel(_("SHOW FAVORITES ON TOP"), favoritesFirstSwitch);
	s->addSaveFunc([s, favoritesFirstSwitch]
	{
		if (Settings::getInstance()->setBool("FavoritesFirst", favoritesFirstSwitch->getState()))
			s->setVariable("reloadAll", true);
	});

	// hidden files
	auto hidden_files = std::make_shared<SwitchComponent>(mWindow);
	hidden_files->setState(Settings::ShowHiddenFiles());
	s->addWithLabel(_("SHOW HIDDEN FILES"), hidden_files);
	s->addSaveFunc([s, hidden_files]
	{
		if (Settings::setShowHiddenFiles(hidden_files->getState()))
			s->setVariable("reloadAll", true);
	});

	// Folder View Mode
	auto foldersBehavior = std::make_shared< OptionListComponent<std::string> >(mWindow, _("SHOW FOLDERS"), false);

	foldersBehavior->add(_("always"), "always", Settings::getInstance()->getString("FolderViewMode") == "always");
	foldersBehavior->add(_("never"), "never", Settings::getInstance()->getString("FolderViewMode") == "never");
	foldersBehavior->add(_("having multiple games"), "having multiple games", Settings::getInstance()->getString("FolderViewMode") == "having multiple games");

	s->addWithLabel(_("SHOW FOLDERS"), foldersBehavior);
	s->addSaveFunc([s, foldersBehavior]
	{
		if (Settings::getInstance()->setString("FolderViewMode", foldersBehavior->getSelected()))
			s->setVariable("reloadAll", true);
	});

	// Show parent folder
	auto parentFolder = std::make_shared<SwitchComponent>(mWindow);
	parentFolder->setState(Settings::getInstance()->getBool("ShowParentFolder"));
	s->addWithLabel(_("SHOW '..' PARENT FOLDER"), parentFolder);
	s->addSaveFunc([s, parentFolder]
	{
		if (Settings::getInstance()->setBool("ShowParentFolder", parentFolder->getState()))
			s->setVariable("reloadAll", true);
	});

	// Show flags
	auto showRegionFlags = std::make_shared<OptionListComponent<std::string>>(mWindow, _("SHOW REGION FLAG"), false);
	showRegionFlags->addRange({ { _("NO"), "auto" },{ _("BEFORE NAME") , "1" },{ _("AFTER NAME"), "2" } }, Settings::getInstance()->getString("ShowFlags"));
	s->addWithLabel(_("SHOW REGION FLAG"), showRegionFlags);
	s->addSaveFunc([s, showRegionFlags]
	{
		if (Settings::getInstance()->setString("ShowFlags", showRegionFlags->getSelected()))
			s->setVariable("reloadAll", true);
	});

	// Show SaveStates
	auto showSaveStates = std::make_shared<SwitchComponent>(mWindow);
	showSaveStates->setState(Settings::getInstance()->getBool("ShowSaveStates"));
	s->addWithLabel(_("SHOW SAVESTATE ICON"), showSaveStates);
	s->addSaveFunc([s, showSaveStates]
	{
		if (Settings::getInstance()->setBool("ShowSaveStates", showSaveStates->getState()))
			s->setVariable("reloadAll", true);
	});

	// Show Manual
	auto showManual = std::make_shared<SwitchComponent>(mWindow);
	showManual->setState(Settings::getInstance()->getBool("ShowManualIcon"));
	s->addWithLabel(_("SHOW MANUAL ICON"), showManual);
	s->addSaveFunc([s, showManual]
	{
		if (Settings::getInstance()->setBool("ShowManualIcon", showManual->getState()))
			s->setVariable("reloadAll", true);
	});

	// filenames
	auto showFilesnames = std::make_shared<SwitchComponent>(mWindow);
	showFilesnames->setState(Settings::getInstance()->getBool("ShowFilenames"));
	s->addWithLabel(_("SHOW FILENAMES INSTEAD"), showFilesnames);
	s->addSaveFunc([showFilesnames, s]
	{
		if (Settings::getInstance()->setBool("ShowFilenames", showFilesnames->getState()))
		{
			SystemData::resetSettings();
			FileData::resetSettings();

			s->setVariable("reloadCollections", true);
			s->setVariable("reloadAll", true);
		}
	});

	auto ignoreArticles = std::make_shared<SwitchComponent>(mWindow);
	ignoreArticles->setState(Settings::getInstance()->getBool("IgnoreLeadingArticles"));
	s->addWithLabel(_("IGNORE LEADING ARTICLES WHEN SORTING"), ignoreArticles);
	s->addSaveFunc([s, ignoreArticles]
	{
		if (Settings::getInstance()->setBool("IgnoreLeadingArticles", ignoreArticles->getState()))
		{
			s->setVariable("reloadAll", true);
		}
	});

	s->onFinalize([s, pthis, window]
	{
		if (s->getVariable("reloadCollections"))
			CollectionSystemManager::get()->updateSystemsList();

		if (s->getVariable("reloadAll"))
		{
			ViewController::get()->reloadAll(window);
			window->closeSplashScreen();
		}

		if (s->getVariable("reloadGuiMenu"))
		{
			delete pthis;
			window->pushGui(new GuiMenu(window));
		}
	});

	mWindow->pushGui(s);
}

void GuiMenu::openSoundSettings()
{
	auto s = new GuiSettings(mWindow, _("SOUND SETTINGS").c_str());

	bool deviceSWHP = getenv("DEVICE_SW_HP_SWITCH");
	if (deviceSWHP == true) {
		s->addGroup(_("OUTPUT"));

		// sw headphone enable
		auto sw_hp_enabled = std::make_shared<SwitchComponent>(mWindow);
		bool hpbaseEnabled = SystemConf::getInstance()->get("headphone.enabled") == "1";
		sw_hp_enabled->setState(hpbaseEnabled);
		s->addWithLabel(_("ENABLE HEADPHONE JACK"), sw_hp_enabled);
		s->addSaveFunc([sw_hp_enabled]
			{
				if (sw_hp_enabled->getState() == false) {
					runSystemCommand("amixer -c0 sset \"Playback Mux\" \"SPK\"", "", nullptr);
				} else {
					runSystemCommand("amixer -c0 sset \"Playback Mux\" \"HP\"", "", nullptr);
				}
				bool swhpenabled = sw_hp_enabled->getState();
				SystemConf::getInstance()->set("headphone.enabled", swhpenabled ? "1" : "0");
				SystemConf::getInstance()->saveSystemConf();
			});
	}

	if (VolumeControl::getInstance()->isAvailable())
	{
		s->addGroup(_("VOLUME"));

		// volume
		auto volume = std::make_shared<SliderComponent>(mWindow, 0.f, 100.f, 1.f, "%");
		volume->setValue((float)VolumeControl::getInstance()->getVolume());
		volume->setOnValueChanged([](const float &newVal) { VolumeControl::getInstance()->setVolume((int)Math::round(newVal)); });
		s->addWithLabel(_("SYSTEM VOLUME"), volume);
		s->addSaveFunc([this, volume]
		{
			VolumeControl::getInstance()->setVolume((int)Math::round(volume->getValue()));
#if !WIN32
			SystemConf::getInstance()->set("audio.volume", std::to_string((int)round(volume->getValue())));
#endif
		});

		// preamp
		auto preamp = std::make_shared<SliderComponent>(mWindow, 0.f, 100.f, 1.f, "%");
		preamp->setValue(std::stof(SystemConf::getInstance()->get("audio.preamp")));
		preamp->setOnValueChanged([](const float &newVal) { SystemConf::getInstance()->set("audio.preamp", std::to_string((int)round(newVal))); });
		s->addWithLabel(_("VOLUME PREAMP"), preamp);
		s->addSaveFunc([this, preamp]
		{
			runSystemCommand("amixer -M set Pre-Amp -- " + std::to_string((int)round(preamp->getValue())) + "%","", nullptr);
		});

		// Music Volume
		auto musicVolume = std::make_shared<SliderComponent>(mWindow, 0.f, 100.f, 1.f, "%");
		musicVolume->setValue(Settings::getInstance()->getInt("MusicVolume"));
		musicVolume->setOnValueChanged([](const float &newVal) { Settings::getInstance()->setInt("MusicVolume", (int)round(newVal)); });
		s->addWithLabel(_("MUSIC VOLUME"), musicVolume);
		//s->addSaveFunc([this, musicVolume] { Settings::getInstance()->setInt("MusicVolume", (int)round(musicVolume->getValue())); });

		auto volumePopup = std::make_shared<SwitchComponent>(mWindow);
		volumePopup->setState(Settings::getInstance()->getBool("VolumePopup"));
		s->addWithLabel(_("SHOW OVERLAY WHEN VOLUME CHANGES"), volumePopup);
		s->addSaveFunc([volumePopup] { Settings::getInstance()->setBool("VolumePopup", volumePopup->getState()); });
	}

	s->addGroup(_("MUSIC"));

	// disable sounds
	auto music_enabled = std::make_shared<SwitchComponent>(mWindow);
	music_enabled->setState(Settings::getInstance()->getBool("audio.bgmusic"));
	s->addWithLabel(_("FRONTEND MUSIC"), music_enabled);
	s->addSaveFunc([music_enabled]
	{
		if (Settings::getInstance()->setBool("audio.bgmusic", music_enabled->getState()))
		{
			if (music_enabled->getState())
				AudioManager::getInstance()->playRandomMusic();
			else
				AudioManager::getInstance()->stopMusic();
		}
	});

	// batocera - display music titles
	auto display_titles = std::make_shared<SwitchComponent>(mWindow);
	display_titles->setState(Settings::getInstance()->getBool("audio.display_titles"));
	s->addWithLabel(_("DISPLAY SONG TITLES"), display_titles);
	s->addSaveFunc([display_titles] {
		Settings::getInstance()->setBool("audio.display_titles", display_titles->getState());
	});

	// batocera - how long to display the song titles?
	auto titles_time = std::make_shared<SliderComponent>(mWindow, 2.f, 120.f, 2.f, "s");
	titles_time->setValue(Settings::getInstance()->getInt("audio.display_titles_time"));
	s->addWithLabel(_("SONG TITLE DISPLAY DURATION"), titles_time);
	s->addSaveFunc([titles_time] {
		Settings::getInstance()->setInt("audio.display_titles_time", (int)Math::round(titles_time->getValue()));
	});

	// batocera - music per system
	auto music_per_system = std::make_shared<SwitchComponent>(mWindow);
	music_per_system->setState(Settings::getInstance()->getBool("audio.persystem"));
	s->addWithLabel(_("ONLY PLAY SYSTEM-SPECIFIC MUSIC FOLDER"), music_per_system);
	s->addSaveFunc([music_per_system] {
		if (Settings::getInstance()->setBool("audio.persystem", music_per_system->getState()))
			AudioManager::getInstance()->changePlaylist(ViewController::get()->getState().getSystem()->getTheme(), true);
	});

	// batocera - music per system
	auto enableThemeMusics = std::make_shared<SwitchComponent>(mWindow);
	enableThemeMusics->setState(Settings::getInstance()->getBool("audio.thememusics"));
	s->addWithLabel(_("PLAY SYSTEM-SPECIFIC MUSIC"), enableThemeMusics);
	s->addSaveFunc([enableThemeMusics] {
		if (Settings::getInstance()->setBool("audio.thememusics", enableThemeMusics->getState()))
			AudioManager::getInstance()->changePlaylist(ViewController::get()->getState().getSystem()->getTheme(), true);
	});

	auto videolowermusic = std::make_shared<SwitchComponent>(mWindow);
	videolowermusic->setState(Settings::getInstance()->getBool("VideoLowersMusic"));
	s->addWithLabel(_("LOWER MUSIC WHEN PLAYING VIDEO"), videolowermusic);
	s->addSaveFunc([videolowermusic] { Settings::getInstance()->setBool("VideoLowersMusic", videolowermusic->getState()); });

	s->addGroup(_("SOUNDS"));

	// disable sounds
	auto sounds_enabled = std::make_shared<SwitchComponent>(mWindow);
	sounds_enabled->setState(Settings::getInstance()->getBool("EnableSounds"));
	s->addWithLabel(_("ENABLE NAVIGATION SOUNDS"), sounds_enabled);
	s->addSaveFunc([sounds_enabled]
	{
	    if (sounds_enabled->getState() && !Settings::getInstance()->getBool("EnableSounds") && PowerSaver::getMode() == PowerSaver::INSTANT)
		{
			Settings::getInstance()->setPowerSaverMode("default");
			PowerSaver::init();
		}
	    Settings::getInstance()->setBool("EnableSounds", sounds_enabled->getState());
	  });

        auto batteryWarning = std::make_shared<SwitchComponent>(mWindow);
        bool batteryWarningEnabled = SystemConf::getInstance()->get("system.battery.warning") == "1";
        batteryWarning->setState(batteryWarningEnabled);
        s->addWithLabel(_("ENABLE AUDIBLE BATTERY WARNING"), batteryWarning);
        s->addSaveFunc([batteryWarning] {
                bool batteryWarningEnabled = batteryWarning->getState();
                SystemConf::getInstance()->set("system.battery.warning", batteryWarningEnabled ? "1" : "0");
                SystemConf::getInstance()->saveSystemConf();
        });

	auto video_audio = std::make_shared<SwitchComponent>(mWindow);
	video_audio->setState(Settings::getInstance()->getBool("VideoAudio"));
	s->addWithLabel(_("ENABLE VIDEO PREVIEW AUDIO"), video_audio);
	s->addSaveFunc([video_audio] { Settings::getInstance()->setBool("VideoAudio", video_audio->getState()); });



	mWindow->pushGui(s);
}

void GuiMenu::openWifiSettings(Window* win, std::string title, std::string data, const std::function<void(std::string)>& onsave)
{
	win->pushGui(new GuiWifi(win, title, data, onsave));
}

void GuiMenu::openNetworkSettings_batocera(bool selectWifiEnable)
{
	bool baseNetworkEnabled = SystemConf::getInstance()->getBool("network.enabled");

	auto theme = ThemeData::getMenuTheme();
	std::shared_ptr<Font> font = theme->Text.font;
	unsigned int color = theme->Text.color;

	Window *window = mWindow;

	auto s = new GuiSettings(mWindow, _("NETWORK SETTINGS").c_str());
	s->addGroup(_("INFORMATION"));

	auto ip = std::make_shared<TextComponent>(mWindow, ApiSystem::getInstance()->getIpAdress(), font, color);
	s->addWithLabel(_("IP ADDRESS"), ip);

	auto status = std::make_shared<TextComponent>(mWindow, ApiSystem::getInstance()->ping() ? _("CONNECTED") : _("NOT CONNECTED"), font, color);
	s->addWithLabel(_("INTERNET STATUS"), status);

	// Network Indicator
	auto networkIndicator = std::make_shared<SwitchComponent>(mWindow);
	networkIndicator->setState(Settings::getInstance()->getBool("ShowNetworkIndicator"));
	s->addWithLabel(_("SHOW NETWORK INDICATOR"), networkIndicator);
	s->addSaveFunc([networkIndicator] { Settings::getInstance()->setBool("ShowNetworkIndicator", networkIndicator->getState()); });

	s->addGroup(_("NETWORK SETTINGS"));

#if !WIN32
	// Hostname
	s->addInputTextRow(_("HOSTNAME"), "system.hostname", false);
#endif

//        std::string a;
//        auto bluetoothd_enabled = std::make_shared<SwitchComponent>(mWindow);
//                bool btbaseEnabled = SystemConf::getInstance()->get("bluetooth.enabled") == "1";
//                bluetoothd_enabled->setState(btbaseEnabled);
//                s->addWithLabel(_("ENABLE BLUETOOTH"), bluetoothd_enabled);
//                s->addSaveFunc([bluetoothd_enabled] {
//                        if (bluetoothd_enabled->changed()) {
//                        if (bluetoothd_enabled->getState() == false) {
//                                runSystemCommand("systemctl stop bluetooth", "", nullptr);
//                                runSystemCommand("rm /storage/.cache/services/bluez.conf", "", nullptr);
//                        } else {
//                                runSystemCommand("mkdir -p /storage/.cache/services/", "", nullptr);
//                                runSystemCommand("touch /storage/.cache/services/bluez.conf", "", nullptr);
//                                runSystemCommand("systemctl start bluetooth", "", nullptr);
//                        }
//                bool bluetoothenabled = bluetoothd_enabled->getState();
//                SystemConf::getInstance()->set("bluetooth.enabled", bluetoothenabled ? "1" : "0");
//                                SystemConf::getInstance()->saveSystemConf();
//                        }
//                });

        // Wifi enable
        auto enable_net = std::make_shared<SwitchComponent>(mWindow);
        enable_net->setState(baseNetworkEnabled);
        s->addWithLabel(_("ENABLE NETWORK"), enable_net, selectWifiEnable);

	// window, title, settingstring,
	const std::string baseSSID = SystemConf::getInstance()->get("wifi.ssid");
	const std::string baseKEY = SystemConf::getInstance()->get("wifi.key");

	if (baseNetworkEnabled)
	{
		s->addInputTextRow(_("WIFI SSID"), "wifi.ssid", false, false, &openWifiSettings);
		s->addInputTextRow(_("WIFI KEY"), "wifi.key", true);
	}

	s->addSaveFunc([baseNetworkEnabled, baseSSID, baseKEY, enable_net, window]
	{
		bool networkenabled = enable_net->getState();

		SystemConf::getInstance()->setBool("network.enabled", networkenabled);

		if (networkenabled)
		{
			std::string newSSID = SystemConf::getInstance()->get("wifi.ssid");
			std::string newKey = SystemConf::getInstance()->get("wifi.key");

			if (baseSSID != newSSID || baseKEY != newKey || !baseNetworkEnabled)
			{
				if (ApiSystem::getInstance()->enableWifi(newSSID, newKey))
					window->pushGui(new GuiMsgBox(window, _("WIFI ENABLED")));
				else
					window->pushGui(new GuiMsgBox(window, _("WIFI CONFIGURATION ERROR")));
			}
		}
		else if (baseNetworkEnabled)
			ApiSystem::getInstance()->disableWifi();
	});

	enable_net->setOnChangedCallback([this, s, baseNetworkEnabled, enable_net]()
	{
		bool networkenabled = enable_net->getState();
		if (baseNetworkEnabled != networkenabled)
		{
			SystemConf::getInstance()->setBool("network.enabled", networkenabled);

			if (networkenabled)
				ApiSystem::getInstance()->enableWifi(SystemConf::getInstance()->get("wifi.ssid"), SystemConf::getInstance()->get("wifi.key"));
			else
				ApiSystem::getInstance()->disableWifi();

			delete s;
			openNetworkSettings_batocera(true);
		}
	});

        // Enable or disable ipv6
        auto ipv6_enable = std::make_shared<SwitchComponent>(mWindow);
        bool ipv6Enabled = SystemConf::getInstance()->get("ipv6.enabled") == "1";
        ipv6_enable->setState(ipv6Enabled);
        s->addWithLabel(_("ENABLE IPV6"), ipv6_enable);
        s->addSaveFunc([ipv6_enable] {
                bool ipv6Enabled = ipv6_enable->getState();
                SystemConf::getInstance()->set("ipv6.enabled", ipv6Enabled ? "1" : "0");
                SystemConf::getInstance()->saveSystemConf();
                runSystemCommand("/usr/bin/toggle-ipv6", "", nullptr);
        });

	s->addGroup(_("NETWORK SERVICES"));

       auto sshd_enabled = std::make_shared<SwitchComponent>(mWindow);
                bool sshbaseEnabled = SystemConf::getInstance()->get("ssh.enabled") == "1";
                sshd_enabled->setState(sshbaseEnabled);
                s->addWithLabel(_("ENABLE SSH"), sshd_enabled);
                s->addSaveFunc([sshd_enabled] {
                        if (sshd_enabled->getState() == false) {
                                runSystemCommand("systemctl stop sshd", "", nullptr);
                                runSystemCommand("systemctl disable sshd", "", nullptr);
                                runSystemCommand("rm /storage/.cache/services/sshd.conf", "", nullptr);
                        } else {
                                runSystemCommand("mkdir -p /storage/.cache/services/", "", nullptr);
                                runSystemCommand("touch /storage/.cache/services/sshd.conf", "", nullptr);
                                runSystemCommand("systemctl enable sshd", "", nullptr);
                                runSystemCommand("systemctl start sshd", "", nullptr);
                        }
                bool sshenabled = sshd_enabled->getState();
                SystemConf::getInstance()->set("ssh.enabled", sshenabled ? "1" : "0");
                                SystemConf::getInstance()->saveSystemConf();
                });

       auto samba_enabled = std::make_shared<SwitchComponent>(mWindow);
                bool smbbaseEnabled = SystemConf::getInstance()->get("samba.enabled") == "1";
                samba_enabled->setState(smbbaseEnabled);
                s->addWithLabel(_("ENABLE SAMBA"), samba_enabled);
                s->addSaveFunc([samba_enabled] {
                        if (samba_enabled->getState() == false) {
                                runSystemCommand("systemctl stop nmbd", "", nullptr);
                                runSystemCommand("systemctl stop smbd", "", nullptr);
                                runSystemCommand("rm /storage/.cache/services/smb.conf", "", nullptr);
                        } else {
                                runSystemCommand("mkdir -p /storage/.cache/services/", "", nullptr);
                                runSystemCommand("touch /storage/.cache/services/smb.conf", "", nullptr);
                                runSystemCommand("systemctl start nmbd", "", nullptr);
                                runSystemCommand("systemctl start smbd", "", nullptr);
                        }
                bool sambaenabled = samba_enabled->getState();
                SystemConf::getInstance()->set("samba.enabled", sambaenabled ? "1" : "0");
                                SystemConf::getInstance()->saveSystemConf();
                });


	s->addGroup(_("CLOUD SERVICES"));

       auto enable_syncthing = std::make_shared<SwitchComponent>(mWindow);
                bool syncthingEnabled = SystemConf::getInstance()->get("syncthing.enabled") == "1";
                enable_syncthing->setState(syncthingEnabled);
                s->addWithLabel(_("ENABLE SYNCTHING"), enable_syncthing);
                s->addSaveFunc([enable_syncthing] {
                        if (enable_syncthing->getState() == false) {
                                runSystemCommand("systemctl stop syncthing", "", nullptr);
                        } else {
                                runSystemCommand("systemctl start syncthing", "", nullptr);
                        }
                bool syncthingenabled = enable_syncthing->getState();
                SystemConf::getInstance()->set("syncthing.enabled", syncthingenabled ? "1" : "0");
                                SystemConf::getInstance()->saveSystemConf();
                });

       auto mount_cloud = std::make_shared<SwitchComponent>(mWindow);
                bool mntcloudEnabled = SystemConf::getInstance()->get("clouddrive.mounted") == "1";
                mount_cloud->setState(mntcloudEnabled);
                s->addWithLabel(_("MOUNT CLOUD DRIVE"), mount_cloud);
                s->addSaveFunc([mount_cloud] {
                        if (mount_cloud->getState() == false) {
                                runSystemCommand("rclonectl unmount", "", nullptr);
                        } else {
                                runSystemCommand("rclonectl mount", "", nullptr);
                        }
                bool cloudenabled = mount_cloud->getState();
                SystemConf::getInstance()->set("clouddrive.mounted", cloudenabled ? "1" : "0");
                                SystemConf::getInstance()->saveSystemConf();
                });

	s->addGroup(_("VPN SERVICES"));

	const std::string wireguardConfigFile = "/storage/.config/wireguard/wg0.conf";
	if (Utils::FileSystem::exists(wireguardConfigFile)) {
		auto wireguard = std::make_shared<SwitchComponent>(mWindow);
		bool wgUp = SystemConf::getInstance()->get("wireguard.up") == "1";
		wireguard->setState(wgUp);
		s->addWithLabel(_("WIREGUARD VPN"), wireguard);
		s->addSaveFunc([wireguard, wireguardConfigFile] {
			if (wireguard->getState() == false) {
				runSystemCommand("wg-quick down " + wireguardConfigFile, "", nullptr);
				runSystemCommand("systemctl stop connman-vpn", "", nullptr);
			} else {
				runSystemCommand("systemctl start connman-vpn", "", nullptr);
				runSystemCommand("wg-quick up " + wireguardConfigFile, "", nullptr);
			}
      SystemConf::getInstance()->set("wireguard.up", wireguard->getState() ? "1" : "0");
      SystemConf::getInstance()->saveSystemConf();
		});
	}

	auto tailscale = std::make_shared<SwitchComponent>(mWindow);
	bool tsUp = SystemConf::getInstance()->get("tailscale.up") == "1";
	tailscale->setState(tsUp);
	s->addWithLabel(_("TAILSCALE VPN"), tailscale);
	s->addSaveFunc([tailscale] {
  	bool tsEnabled = tailscale->getState();
		if (tsEnabled) {
			runSystemCommand("systemctl start tailscaled", "", nullptr);
			runSystemCommand("tailscale up --timeout=7s", "", nullptr);
			tsEnabled = IsTailscaleUp();
		} else {
			runSystemCommand("tailscale down", "", nullptr);
			runSystemCommand("systemctl stop tailscaled", "", nullptr);
		}
		SystemConf::getInstance()->set("tailscale.up", tsEnabled ? "1" : "0");
		SystemConf::getInstance()->saveSystemConf();
	});

	std::string tsUrl;
	if (!IsTailscaleUp(&tsUrl) && !tsUrl.empty()) {
		s->addGroup("TAILSCALE REAUTHENTICATE:");
		s->addGroup(tsUrl);
	}

    auto zerotier = std::make_shared<SwitchComponent>(mWindow);
	bool ztUp = SystemConf::getInstance()->get("zerotier.up") == "1";
	zerotier->setState(ztUp);
	s->addWithLabel(_("ZeroTier One"), zerotier);
	s->addSaveFunc([zerotier] {
	bool ztEnabled = zerotier->getState();
	    if(ztEnabled) {
			runSystemCommand("systemctl start zerotier-one", "", nullptr);
			ztEnabled = IsZeroTierUp();
		} else {
			runSystemCommand("systemctl stop zerotier-one", "", nullptr);
		}
		SystemConf::getInstance()->set("zerotier.up", ztEnabled ? "1" : "0");
		SystemConf::getInstance()->saveSystemConf();
	});

	mWindow->pushGui(s);
}

bool GuiMenu::IsTailscaleUp(std::string* loginUrl) {
  bool loggedOut = false;
	ApiSystem::executeScript("tailscale status", [loginUrl, &loggedOut](std::string line) {
		 const std::string prompt = "Log in at: ";
		 if (loginUrl && line.find(prompt) == 0)
		 	 *loginUrl = line.substr(prompt.length());

		 if (line.find("Logged out.") != std::string::npos) loggedOut = true;
	});
	return !loggedOut;
}

bool GuiMenu::IsZeroTierUp(std::string* networkId) {
	bool running = false;
      ApiSystem::executeScript("zerotier-cli -D/storage/.config/zerotier/ info", [networkId, &running](std::string line) {
          if (line.find("Error connecting to the ZeroTier") != std::string::npos ) running = false;
          else running = true;
      });
	 return running;
}

void GuiMenu::openQuitMenu_batocera()
{
  GuiMenu::openQuitMenu_batocera_static(mWindow);
}

void GuiMenu::openQuitMenu_batocera_static(Window *window, bool quickAccessMenu, bool animate)
{
#ifdef WIN32
	if (!quickAccessMenu && Settings::getInstance()->getBool("ShowOnlyExit"))
	{
		quitES(QuitMode::QUIT);
		return;
	}
#endif

	auto s = new GuiSettings(window, (quickAccessMenu ? _("QUICK ACCESS") : _("QUIT")).c_str());
	s->setCloseButton("select");

	if (quickAccessMenu)
	{
		s->addGroup(_("QUICK ACCESS"));

		// Don't like one of the songs? Press next
		if (AudioManager::getInstance()->isSongPlaying())
		{
			auto sname = AudioManager::getInstance()->getSongName();
			if (!sname.empty())
			{
				s->addWithDescription(_("SKIP TO NEXT SONG"), _("LISTENING NOW") + " : " + sname, nullptr, [s, window]
				{
					Window* w = window;
					AudioManager::getInstance()->playRandomMusic(false);
					delete s;
					openQuitMenu_batocera_static(w, true, false);
				}, "iconSound");
			}
		}

		s->addEntry(_("LAUNCH SCREENSAVER"), false, [s, window]
		{
			Window* w = window;
			window->postToUiThread([w]()
			{
				w->startScreenSaver();
				w->renderScreenSaver();
			});
			delete s;

		}, "iconScraper", true);

#if WIN32
#define BATOCERA_MANUAL_FILE Utils::FileSystem::getEsConfigPath() + "/notice.pdf"
#else
#define BATOCERA_MANUAL_FILE "/usr/share/batocera/doc/notice.pdf"
#endif

		if (ApiSystem::getInstance()->isScriptingSupported(ApiSystem::ScriptId::PDFEXTRACTION) && Utils::FileSystem::exists(BATOCERA_MANUAL_FILE))
		{
#if defined(WIN32)
			s->addEntry(_("VIEW USER'S MANUAL"), false, [s, window]
#else
			s->addEntry(_("VIEW BATOCERA MANUAL"), false, [s, window]
#endif
			{
				GuiImageViewer::showPdf(window, BATOCERA_MANUAL_FILE);
				delete s;
			}, "iconManual");
		}
	}

	if (quickAccessMenu)
		s->addGroup(_("QUIT"));

#ifdef _ENABLEEMUELEC
	s->addEntry(_("RESTART EMULATIONSTATION"), false, [window] {
		window->pushGui(new GuiMsgBox(window, _("REALLY RESTART EMULATIONSTATION?"), _("YES"),
			[] {
    		   /*runSystemCommand("systemctl restart emustation.service", "", nullptr);*/
    		   Scripting::fireEvent("quit", "restart");
			   quitES(QuitMode::QUIT);
		}, _("NO"), nullptr));
	}, "iconRestart");

	/*
	s->addEntry(_("START RETROARCH"), false, [window] {
		window->pushGui(new GuiMsgBox(window, _("REALLY START RETROARCH?"), _("YES"),
			[] {
			remove("/var/lock/start.games");
			runSystemCommand("touch /var/lock/start.retro", "", nullptr);
			runSystemCommand("systemctl start retroarch.service", "", nullptr);
			Scripting::fireEvent("quit", "retroarch");
			quitES(QuitMode::QUIT);
		}, _("NO"), nullptr));
	}, "iconControllers");

	s->addEntry(_("REBOOT FROM NAND"), false, [window] {
		window->pushGui(new GuiMsgBox(window, _("REALLY REBOOT FROM NAND?"), _("YES"),
			[] {
			Scripting::fireEvent("quit", "nand");
			runSystemCommand("rebootfromnand", "", nullptr);
			runSystemCommand("sync", "", nullptr);
			runSystemCommand("systemctl reboot", "", nullptr);
			quitES(QuitMode::QUIT);
		}, _("NO"), nullptr));
	}, "iconAdvanced");
	*/

#endif

	s->addEntry(_("RESTART SYSTEM"), false, [window] {
		window->pushGui(new GuiMsgBox(window, _("REALLY RESTART?"),
			_("YES"), [] { quitES(QuitMode::REBOOT); },
			_("NO"), nullptr));
	}, "iconRestart");


	s->addEntry(_("SHUTDOWN SYSTEM"), false, [window] {
		window->pushGui(new GuiMsgBox(window, _("REALLY SHUTDOWN?"),
			_("YES"), [] { quitES(QuitMode::SHUTDOWN); },
			_("NO"), nullptr));
	}, "iconShutdown");

#ifndef _ENABLEEMUELEC
	s->addEntry(_("FAST SHUTDOWN SYSTEM"), false, [window] {
		window->pushGui(new GuiMsgBox(window, _("REALLY SHUTDOWN WITHOUT SAVING METADATA?"),
			_("YES"), [] { quitES(QuitMode::FAST_SHUTDOWN); },
			_("NO"), nullptr));
	}, "iconFastShutdown");
#endif

#ifdef WIN32
	if (Settings::getInstance()->getBool("ShowExit"))
	{
		s->addEntry(_("QUIT EMULATIONSTATION"), false, [window] {
			window->pushGui(new GuiMsgBox(window, _("REALLY QUIT?"),
				_("YES"), [] { quitES(QuitMode::QUIT); },
				_("NO"), nullptr));
		}, "iconQuit");
	}
#endif

	if (quickAccessMenu && animate)
		s->getMenu().animateTo(Vector2f((Renderer::getScreenWidth() - s->getMenu().getSize().x()) / 2, (Renderer::getScreenHeight() - s->getMenu().getSize().y()) / 2));
	else if (quickAccessMenu)
		s->getMenu().setPosition((Renderer::getScreenWidth() - s->getMenu().getSize().x()) / 2, (Renderer::getScreenHeight() - s->getMenu().getSize().y()) / 2);

	window->pushGui(s);
}

void GuiMenu::createDecorationItemTemplate(Window* window, std::vector<DecorationSetInfo> sets, std::string data, ComponentListRow& row)
{
	Vector2f maxSize(Renderer::getScreenWidth() * 0.14, Renderer::getScreenHeight() * 0.14);

	int IMGPADDING = Renderer::getScreenHeight()*0.01f;

	auto theme = ThemeData::getMenuTheme();
	std::shared_ptr<Font> font = theme->Text.font;
	unsigned int color = theme->Text.color;

	// spacer between icon and text
	auto spacer = std::make_shared<GuiComponent>(window);
	spacer->setSize(IMGPADDING, 0);
	row.addElement(spacer, false);
	row.addElement(std::make_shared<TextComponent>(window, Utils::String::toUpper(Utils::String::replace(data, "_", " ")), font, color, ALIGN_LEFT), true, true);

	std::string imageUrl;

	for (auto set : sets)
		if (set.name == data)
			imageUrl = set.imageUrl;

	// image
	if (!imageUrl.empty())
	{
		auto icon = std::make_shared<ImageComponent>(window);
		icon->setImage(imageUrl, false, maxSize);
		icon->setMaxSize(maxSize);
		icon->setColorShift(theme->Text.color);
		icon->setPadding(IMGPADDING);
		row.addElement(icon, false);
	}
}

void GuiMenu::popSystemConfigurationGui(Window* mWindow, SystemData* systemData)
{
	popSpecificConfigurationGui(mWindow,
		systemData->getFullName(),
		systemData->getName(),
		systemData,
		nullptr);
}

void GuiMenu::popGameConfigurationGui(Window* mWindow, FileData* fileData)
{
	popSpecificConfigurationGui(mWindow,
		fileData->getName(),
		fileData->getConfigurationName(),
		fileData->getSourceFileData()->getSystem(),
		fileData);
}

void GuiMenu::popSpecificConfigurationGui(Window* mWindow, std::string title, std::string configName, SystemData *systemData, FileData* fileData, bool selectCoreLine)
{
	// The system configuration
	GuiSettings* systemConfiguration = new GuiSettings(mWindow, title.c_str());

	if (fileData != nullptr)
		systemConfiguration->setSubTitle(systemData->getFullName());

	std::string currentEmulator = fileData != nullptr ? fileData->getEmulator(false) : systemData->getEmulator(false);
	std::string currentCore = fileData != nullptr ? fileData->getCore(false) : systemData->getCore(false);

	if (systemData->hasEmulatorSelection())
	{
		auto emulChoice = std::make_shared<OptionListComponent<std::string>>(mWindow, _("Emulator"), false);
		emulChoice->add(_("DEFAULT"), "", false);
		for (auto& emul : systemData->getEmulators())
		{
			if (emul.cores.size() == 0)
				emulChoice->add(emul.name, emul.name, emul.name == currentEmulator);
			else
			{
				for (auto& core : emul.cores)
				{
					bool selected = (emul.name == currentEmulator && core.name == currentCore);

					if (emul.name == core.name)
						emulChoice->add(emul.name, emul.name + "/" + core.name, selected);
					else
						emulChoice->add(emul.name + ": " + Utils::String::replace(core.name, "_", " "), emul.name + "/" + core.name, selected);
				}
			}
		}

		if (!emulChoice->hasSelection())
			emulChoice->selectFirstItem();

		emulChoice->setSelectedChangedCallback([mWindow, title, systemConfiguration, systemData, fileData, configName, emulChoice](std::string s)
		{
			std::string newEmul;
			std::string newCore;

			auto values = Utils::String::split(emulChoice->getSelected(), '/');
			if (values.size() > 0)
				newEmul = values[0];

			if (values.size() > 1)
				newCore = values[1];

			if (fileData != nullptr)
			{
				fileData->setEmulator(newEmul);
				fileData->setCore(newCore);
			}
			else
			{
				SystemConf::getInstance()->set(configName + ".emulator", newEmul);
				SystemConf::getInstance()->set(configName + ".core", newCore);
			}

			popSpecificConfigurationGui(mWindow, title, configName, systemData, fileData);
			delete systemConfiguration;

		});

		systemConfiguration->addWithLabel(_("Emulator"), emulChoice);
	}

#ifdef _ENABLEEMUELEC
	/*
	if (systemData->isFeatureSupported(currentEmulator, currentCore, EmulatorFeatures::nativevideo))
	{
		auto videoNativeResolutionMode_choice = createNativeVideoResolutionModeOptionList(mWindow, configName);
		systemConfiguration->addWithLabel(_("NATIVE VIDEO"), videoNativeResolutionMode_choice);
		systemConfiguration->addSaveFunc([configName, videoNativeResolutionMode_choice] {
			SystemConf::getInstance()->set(configName + ".nativevideo", videoNativeResolutionMode_choice->getSelected());
			SystemConf::getInstance()->saveSystemConf();
		});
	}
	*/
#endif
	// Screen ratio choice
	if (systemData->isFeatureSupported(currentEmulator, currentCore, EmulatorFeatures::ratio))
	{
		auto ratio_choice = createRatioOptionList(mWindow, configName);
		systemConfiguration->addWithLabel(_("GAME ASPECT RATIO"), ratio_choice);
		systemConfiguration->addSaveFunc([configName, ratio_choice] { SystemConf::getInstance()->set(configName + ".ratio", ratio_choice->getSelected()); });
	}

	// video resolution mode
	if (systemData->isFeatureSupported(currentEmulator, currentCore, EmulatorFeatures::videomode))
	{
		auto videoResolutionMode_choice = createVideoResolutionModeOptionList(mWindow, configName);
		systemConfiguration->addWithLabel(_("VIDEO MODE"), videoResolutionMode_choice);
		systemConfiguration->addSaveFunc([configName, videoResolutionMode_choice] { SystemConf::getInstance()->set(configName + ".videomode", videoResolutionMode_choice->getSelected()); });
	}

	// smoothing
	if (systemData->isFeatureSupported(currentEmulator, currentCore, EmulatorFeatures::smooth))
	{
		auto smoothing_enabled = std::make_shared<OptionListComponent<std::string>>(mWindow, _("BILINEAR FILTERING"));
		smoothing_enabled->addRange({ { _("DEFAULT"), "default" },{ _("ON") , "1" },{ _("OFF"), "0" } }, SystemConf::getInstance()->get(configName + ".smooth"));
		systemConfiguration->addWithLabel(_("BILINEAR FILTERING"), smoothing_enabled);
		systemConfiguration->addSaveFunc([configName, smoothing_enabled] { SystemConf::getInstance()->set(configName + ".smooth", smoothing_enabled->getSelected()); });
	}

	// rewind
	if (systemData->isFeatureSupported(currentEmulator, currentCore, EmulatorFeatures::rewind))
	{
		auto rewind_enabled = std::make_shared<OptionListComponent<std::string>>(mWindow, _("REWIND"));
		rewind_enabled->addRange({ { _("DEFAULT"), "default" }, { _("ON") , "1" }, { _("OFF"), "0" } }, SystemConf::getInstance()->get(configName + ".rewind"));
		systemConfiguration->addWithLabel(_("REWIND"), rewind_enabled);
		systemConfiguration->addSaveFunc([configName, rewind_enabled] { SystemConf::getInstance()->set(configName + ".rewind", rewind_enabled->getSelected()); });
	}

	// autosave
	if (systemData->isFeatureSupported(currentEmulator, currentCore, EmulatorFeatures::autosave))
	{
		auto autosave_enabled = std::make_shared<OptionListComponent<std::string>>(mWindow, _("AUTO SAVE/LOAD ON GAME LAUNCH"));
		autosave_enabled->addRange({ { _("DEFAULT"), "default" }, { _("ON") , "1" }, { _("OFF"), "0" }, { _("SHOW SAVE STATES") , "2" }, { _("SHOW SAVE STATES IF NOT EMPTY") , "3" } }, SystemConf::getInstance()->get(configName + ".autosave"));
		systemConfiguration->addWithLabel(_("AUTO SAVE/LOAD ON GAME LAUNCH"), autosave_enabled);
		systemConfiguration->addSaveFunc([configName, autosave_enabled] { SystemConf::getInstance()->set(configName + ".autosave", autosave_enabled->getSelected()); });
	}
#ifdef _ENABLEEMUELEC
	// Shaders preset
	if (ApiSystem::getInstance()->isScriptingSupported(ApiSystem::SHADERS) &&
		systemData->isFeatureSupported(currentEmulator, currentCore, EmulatorFeatures::shaders))
	{
        std::string a;
		auto shaders_choices = std::make_shared<OptionListComponent<std::string> >(mWindow, _("SHADER SET"),false);
		std::string currentShader = SystemConf::getInstance()->get(configName + ".shaderset");
		if (currentShader.empty()) {
			currentShader = std::string("auto");
		}

		shaders_choices->add(_("DEFAULT"), "default", currentShader == "auto");
		shaders_choices->add(_("NONE"), "none", currentShader == "none");
		for(std::stringstream ss(getShOutput(R"(/usr/bin/getshaders)")); getline(ss, a, ','); )
		shaders_choices->add(a, a, currentShader == a); // emuelec
		systemConfiguration->addWithLabel(_("SHADER SET"), shaders_choices);
		systemConfiguration->addSaveFunc([shaders_choices, configName] { SystemConf::getInstance()->set(configName + ".shaderset", shaders_choices->getSelected()); });
	}

	// Filters preset
	if (ApiSystem::getInstance()->isScriptingSupported(ApiSystem::SHADERS) &&
		systemData->isFeatureSupported(currentEmulator, currentCore, EmulatorFeatures::shaders))
	{
		std::string a;
		auto filters_choices = std::make_shared<OptionListComponent<std::string> >(mWindow, _("FILTER SET"),false);
		std::string currentFilter = SystemConf::getInstance()->get(configName + ".filterset");
		if (currentFilter.empty()) {
			currentFilter = std::string("auto");
		}

		filters_choices->add(_("DEFAULT"), "default", currentFilter == "auto");
		filters_choices->add(_("NONE"), "none", currentFilter == "none");
		for(std::stringstream ss(getShOutput(R"(/usr/bin/getfilters)")); getline(ss, a, ','); )
		filters_choices->add(a, a, currentFilter == a); // emuelec
		systemConfiguration->addWithLabel(_("FILTER SET"), filters_choices);
		systemConfiguration->addSaveFunc([filters_choices, configName] { SystemConf::getInstance()->set(configName + ".filterset", filters_choices->getSelected()); });
	}

	// Vertical Game
	if (systemData->isFeatureSupported(currentEmulator, currentCore, EmulatorFeatures::vertical))
	{
		auto vertical_enabled = std::make_shared<OptionListComponent<std::string>>(mWindow, _("ENABLE VERTICAL"));
		vertical_enabled->add(_("OFF"), "default", SystemConf::getInstance()->get(configName + ".vertical") != "1");
		vertical_enabled->add(_("ON"), "1", SystemConf::getInstance()->get(configName + ".vertical") == "1");
		systemConfiguration->addWithLabel(_("ENABLE VERTICAL"), vertical_enabled);
		systemConfiguration->addSaveFunc([configName, vertical_enabled] { SystemConf::getInstance()->set(configName + ".vertical", vertical_enabled->getSelected()); });

        auto vert_aspect_enabled = std::make_shared<OptionListComponent<std::string>>(mWindow, _("VERTICAL ASPECT RATIO"));
		vert_aspect_enabled->addRange({ { _("16:9") , "1" }, { _("3:2") , "7" }, { _("21:9"), "4" }, { _("4:3") , "0" } }, SystemConf::getInstance()->get(configName + ".vert_aspect"));
		systemConfiguration->addWithLabel(_("VERTICAL ASPECT RATIO"), vert_aspect_enabled);
		systemConfiguration->addSaveFunc([configName, vert_aspect_enabled] { SystemConf::getInstance()->set(configName + ".vert_aspect", vert_aspect_enabled->getSelected()); });
	}
#else
	// Shaders preset
	if (ApiSystem::getInstance()->isScriptingSupported(ApiSystem::SHADERS) &&
		systemData->isFeatureSupported(currentEmulator, currentCore, EmulatorFeatures::shaders))
	{
		auto installedShaders = ApiSystem::getInstance()->getShaderList(systemData->getName());
		if (installedShaders.size() > 0)
		{
			std::string currentShader = SystemConf::getInstance()->get(configName + ".shaderset");

			auto shaders_choices = std::make_shared<OptionListComponent<std::string> >(mWindow, _("SHADER SET"), false);
			shaders_choices->add(_("DEFAULT"), "default", currentShader.empty() || currentShader == "auto");
			shaders_choices->add(_("NONE"), "none", currentShader == "none");

			for (auto shader : installedShaders)
				shaders_choices->add(_(Utils::String::toUpper(shader).c_str()), shader, currentShader == shader);

			if (!shaders_choices->hasSelection())
				shaders_choices->selectFirstItem();

			systemConfiguration->addWithLabel(_("SHADER SET"), shaders_choices);
			systemConfiguration->addSaveFunc([configName, shaders_choices] { SystemConf::getInstance()->set(configName + ".shaderset", shaders_choices->getSelected()); });
		}
	}
#endif
	// Integer scale
	if (systemData->isFeatureSupported(currentEmulator, currentCore, EmulatorFeatures::pixel_perfect))
	{
		auto integerscale_enabled = std::make_shared<OptionListComponent<std::string>>(mWindow, _("INTEGER SCALING (PIXEL PERFECT)"));
		integerscale_enabled->addRange({ { _("DEFAULT"), "default" },{ _("ON") , "1" },{ _("OFF"), "0" } }, SystemConf::getInstance()->get(configName + ".integerscale"));
		systemConfiguration->addWithLabel(_("INTEGER SCALING (PIXEL PERFECT)"), integerscale_enabled);
		systemConfiguration->addSaveFunc([integerscale_enabled, configName] { SystemConf::getInstance()->set(configName + ".integerscale", integerscale_enabled->getSelected()); });
	}
#ifdef _ENABLEEMUELEC
	// bezel
	/*
	if (systemData->isFeatureSupported(currentEmulator, currentCore, EmulatorFeatures::decoration))
	{
		auto bezel_enabled = std::make_shared<OptionListComponent<std::string>>(mWindow, _("BEZEL"));
		bezel_enabled->add(_("DEFAULT"), "default", SystemConf::getInstance()->get(configName + ".bezel") != "0" && SystemConf::getInstance()->get(configName + ".bezel") != "1");
		bezel_enabled->add(_("YES"), "1", SystemConf::getInstance()->get(configName + ".bezel") == "1");
		bezel_enabled->add(_("NO"), "0", SystemConf::getInstance()->get(configName + ".bezel") == "0");
		systemConfiguration->addWithLabel(_("BEZEL"), bezel_enabled);
		systemConfiguration->addSaveFunc([bezel_enabled, configName] { SystemConf::getInstance()->set(configName + ".bezel", bezel_enabled->getSelected()); });
	}
	*/

#if defined(S922X) || defined(RK3588)  || defined(RK3399)
        // Core chooser
        auto cores_used = std::make_shared<OptionListComponent<std::string>>(mWindow, _("CORES USED"));
        cores_used->addRange({ { _("ALL"), "all" },{ _("BIG") , "big" },{ _("LITTLE") , "little" } }, SystemConf::getInstance()->get(configName + ".cores"));
        systemConfiguration->addWithLabel(_("CORES USED"), cores_used);
        systemConfiguration->addSaveFunc([cores_used, configName] { SystemConf::getInstance()->set(configName + ".cores", cores_used->getSelected()); });
#endif
	
#if defined(AMD64)

        // Allow offlining all but n threads
        auto optionsThreads = std::make_shared<OptionListComponent<std::string> >(mWindow, _("AVAILABLE THREADS"), false);

        std::vector<std::string> availableThreads = ApiSystem::getInstance()->getAvailableThreads();
        std::string selectedThreads = SystemConf::getInstance()->get(configName +".threads");
        if (selectedThreads.empty())
                selectedThreads = "default";

        bool wfound = false;
        for (auto it = availableThreads.begin(); it != availableThreads.end(); it++)
        {
                optionsThreads->add((*it), (*it), selectedThreads == (*it));
                if (selectedThreads == (*it))
                wfound = true;
        }
        if (!wfound)
                optionsThreads->add(selectedThreads, selectedThreads, true);

        systemConfiguration->addWithLabel(_("AVAILABLE THREADS"), optionsThreads);

        systemConfiguration->addSaveFunc([configName, optionsThreads, selectedThreads]
        {
                if (optionsThreads->changed()) {
                        SystemConf::getInstance()->set(configName +".threads", optionsThreads->getSelected());
                        runSystemCommand("/usr/bin/bash -lc \". /etc/profile; onlinethreads " + optionsThreads->getSelected() + " 0" + "\"" , "", nullptr);
                        SystemConf::getInstance()->saveSystemConf();
                }
        });

#endif

        bool deviceHasFan = getenv("DEVICE_HAS_FAN");
        if (deviceHasFan == true) {
          // Provides cooling profile switching
          auto optionsFanProfile = std::make_shared<OptionListComponent<std::string> >(mWindow, _("COOLING PROFILE"), false);
          std::string selectedFanProfile = SystemConf::getInstance()->get(configName + ".cooling.profile");
          if (selectedFanProfile.empty())
                selectedFanProfile = "quiet";

          optionsFanProfile->add(_("QUIET"),"quiet", selectedFanProfile == "quiet");
          optionsFanProfile->add(_("MODERATE"),"moderate", selectedFanProfile == "moderate");
          optionsFanProfile->add(_("AGGRESSIVE"),"aggressive", selectedFanProfile == "aggressive");
          optionsFanProfile->add(_("CUSTOM"),"custom", selectedFanProfile == "custom");

          systemConfiguration->addWithLabel(_("COOLING PROFILE"), optionsFanProfile);

          systemConfiguration->addSaveFunc([optionsFanProfile, selectedFanProfile, configName]
          {
            if (optionsFanProfile->changed()) {
              SystemConf::getInstance()->set(configName + ".cooling.profile", optionsFanProfile->getSelected());
              SystemConf::getInstance()->saveSystemConf();
            }
          });
	}

// Prep for additional device support.
#if defined(AMD64)
        // Provides overclock profile switching
        auto optionsOCProfile = std::make_shared<OptionListComponent<std::string> >(mWindow, _("CPU Max TDP (AMD Only)"), false);
        std::string selectedOCProfile = SystemConf::getInstance()->get(configName + ".overclock");
        if (selectedOCProfile.empty())
                selectedOCProfile = "system";

        optionsOCProfile->add(_("DEFAULT"), "system", selectedOCProfile == "system");
        optionsOCProfile->add(_("OFF"), "off", selectedOCProfile == "off");
        optionsOCProfile->add(_("2w"),"2w", selectedOCProfile == "2w");
        optionsOCProfile->add(_("4w"),"4w", selectedOCProfile == "4w");
        optionsOCProfile->add(_("6w"),"6w", selectedOCProfile == "6w");
        optionsOCProfile->add(_("8w"),"8w", selectedOCProfile == "8w");
        optionsOCProfile->add(_("10w"),"10w", selectedOCProfile == "10w");
        optionsOCProfile->add(_("12w"),"12w", selectedOCProfile == "12w");
        optionsOCProfile->add(_("14w"),"14w", selectedOCProfile == "14w");
        optionsOCProfile->add(_("16w"),"16w", selectedOCProfile == "16w");
        optionsOCProfile->add(_("18w"),"18w", selectedOCProfile == "18w");
        optionsOCProfile->add(_("20w"),"20w", selectedOCProfile == "20w");
        optionsOCProfile->add(_("22w"),"22w", selectedOCProfile == "22w");
        optionsOCProfile->add(_("24w"),"24w", selectedOCProfile == "24w");
#endif
#if defined(AMD64)
        systemConfiguration->addWithLabel(_("CPU Max TDP (AMD Only)"), optionsOCProfile);

        systemConfiguration->addSaveFunc([optionsOCProfile, selectedOCProfile, configName, mWindow]
        {
                if (optionsOCProfile->changed()) {
                        mWindow->pushGui(new GuiMsgBox(mWindow, _("WARNING: OVERCLOCKING YOUR DEVICE MAY RESULT IN STABILITY PROBLEMS OR CAUSE HARDWARE DAMAGE!\n\nUSING THE QUIET COOLING PROFILE WHILE USING CERTAIN OVERCLOCKS MAY CAUSE PANIC REBOOTS!\n\nJELOS IS NOT RESPONSIBLE FOR ANY DAMAGE THAT MAY OCCUR USING THESE SETTINGS!\n\nCLICK YES THAT YOU AGREE, OR NO TO CANCEL."), _("YES"),
			[optionsOCProfile,configName] {
                                SystemConf::getInstance()->set(configName + ".overclock", optionsOCProfile->getSelected());
                                SystemConf::getInstance()->saveSystemConf();
                        }, _("NO"), nullptr));
                }
        });
#endif

        // Per game/core/emu CPU governor

        auto cpuGovUpdate = std::make_shared<OptionListComponent<std::string> >(mWindow, _("CPU GOVERNOR"), false);

        std::string cpu_governor = SystemConf::getInstance()->get(configName + ".cpugovernor");
        if (cpu_governor.empty())
                cpu_governor = "default";

        cpuGovUpdate->add(_("DEFAULT"), "default", cpu_governor == "default");
        cpuGovUpdate->add(_("SCHEDUTIL"), "schedutil", cpu_governor == "schedutil");
        cpuGovUpdate->add(_("ONDEMAND"), "ondemand", cpu_governor == "ondemand");
        cpuGovUpdate->add(_("PERFORMANCE"), "performance", cpu_governor == "performance");
        cpuGovUpdate->add(_("powersave"), "powersave", cpu_governor == "powersave");

        systemConfiguration->addWithLabel(_("DEFAULT CPU GOVERNOR"), cpuGovUpdate);

        systemConfiguration->addSaveFunc([configName, cpuGovUpdate, cpu_governor]
        {
          if (cpuGovUpdate->changed()) {
            SystemConf::getInstance()->set(configName + ".cpugovernor", cpuGovUpdate->getSelected());
            SystemConf::getInstance()->saveSystemConf();
          }
        });

        if (SystemConf::getInstance()->getBool("system.powersave", true)) {
          // GPU performance mode with enhanced power savings
          auto gpuPerformance = std::make_shared<OptionListComponent<std::string> >(mWindow, _("GPU POWER SAVINGS MODE (AMD ONLY)"), false);
          std::string gpu_performance = SystemConf::getInstance()->get(configName + ".gpuperf");
          if (gpu_performance.empty())
                  gpu_performance = "default";

          gpuPerformance->add(_("DEFAULT"), "default", gpu_performance == "default");
          gpuPerformance->add(_("AUTO"), "default", gpu_performance == "auto");
          gpuPerformance->add(_("LOW"), "low", gpu_performance == "low");
          gpuPerformance->add(_("STANDARD"), "profile_standard", gpu_performance == "profile_standard");
          gpuPerformance->add(_("PEAK"), "profile_peak", gpu_performance == "profile_peak");

          systemConfiguration->addWithLabel(_("GPU POWER SAVINGS MODE (AMD ONLY)"), gpuPerformance);

          systemConfiguration->addSaveFunc([configName, gpuPerformance, gpu_performance]
          {
            if (gpuPerformance->changed()) {
              SystemConf::getInstance()->set(configName + ".gpuperf", gpuPerformance->getSelected());
              SystemConf::getInstance()->saveSystemConf();
            }
          });
        }

#endif
	if (systemData->isFeatureSupported(currentEmulator, currentCore, EmulatorFeatures::latency_reduction))
		systemConfiguration->addEntry(_("LATENCY REDUCTION"), true, [mWindow, configName] { openLatencyReductionConfiguration(mWindow, configName); });

	if (systemData->isFeatureSupported(currentEmulator, currentCore, EmulatorFeatures::colorization))
	{
		// gameboy colorize
		auto colorizations_choices = std::make_shared<OptionListComponent<std::string> >(mWindow, _("COLORIZATION"), false);
		std::string currentColorization = SystemConf::getInstance()->get(configName + ".renderer.colorization");
		if (currentColorization.empty())
			currentColorization = std::string("default");

		colorizations_choices->add(_("DEFAULT"), "default", currentColorization == "default");
		colorizations_choices->add(_("NONE"), "none", currentColorization == "none");
#ifdef _ENABLEEMUELEC
        colorizations_choices->add(_("GBC"), "GBC", currentColorization == "GBC");
		colorizations_choices->add(_("SGB"), "SGB", currentColorization == "SGB");
#endif
		colorizations_choices->add(_("Best Guess"), "Best Guess", currentColorization == "Best Guess");

		const char* all_gambate_gc_colors_modes[] = { "GB - DMG",
								 "GB - Light",
								 "GB - Pocket",
								 "GBC - Blue",
								 "GBC - Brown",
								 "GBC - Dark Blue",
								 "GBC - Dark Brown",
								 "GBC - Dark Green",
								 "GBC - Grayscale",
								 "GBC - Green",
								 "GBC - Inverted",
								 "GBC - Orange",
								 "GBC - Pastel Mix",
								 "GBC - Red",
								 "GBC - Yellow",
								 "SGB - 1A",
								 "SGB - 1B",
								 "SGB - 1C",
								 "SGB - 1D",
								 "SGB - 1E",
								 "SGB - 1F",
								 "SGB - 1G",
								 "SGB - 1H",
								 "SGB - 2A",
								 "SGB - 2B",
								 "SGB - 2C",
								 "SGB - 2D",
								 "SGB - 2E",
								 "SGB - 2F",
								 "SGB - 2G",
								 "SGB - 2H",
								 "SGB - 3A",
								 "SGB - 3B",
								 "SGB - 3C",
								 "SGB - 3D",
								 "SGB - 3E",
								 "SGB - 3F",
								 "SGB - 3G",
								 "SGB - 3H",
								 "SGB - 4A",
								 "SGB - 4B",
								 "SGB - 4C",
								 "SGB - 4D",
								 "SGB - 4E",
								 "SGB - 4F",
								 "SGB - 4G",
								 "SGB - 4H",
								 "Special 1",
								 "Special 2",
								 "Special 3",
								 "TWB01 - 756 Production",
								 "TWB02 - AKB48 Pink",
								 "TWB03 - Angry Volcano",
								 "TWB04 - Anime Expo",
								 "TWB05 - Aqours Blue",
								 "TWB06 - Aquatic Iro",
								 "TWB07 - Bandai Namco",
								 "TWB08 - Blossom Pink",
								 "TWB09 - Bubbles Blue",
								 "TWB10 - Builder Yellow",
								 "TWB11 - Buttercup Green",
								 "TWB12 - Camouflage",
								 "TWB13 - Cardcaptor Pink",
								 "TWB14 - Christmas",
								 "TWB15 - Crunchyroll Orange",
								 "TWB16 - Digivice",
								 "TWB17 - Do The Dew",
								 "TWB18 - Eevee Brown",
								 "TWB19 - Fruity Orange",
								 "TWB20 - Game.com",
								 "TWB21 - Game Grump Orange",
								 "TWB22 - GameKing",
								 "TWB23 - Game Master",
								 "TWB24 - Ghostly Aoi",
								 "TWB25 - Golden Wild",
								 "TWB26 - Green Banana",
								 "TWB27 - Greenscale",
								 "TWB28 - Halloween",
								 "TWB29 - Hero Yellow",
								 "TWB30 - Hokage Orange",
								 "TWB31 - Labo Fawn",
								 "TWB32 - Legendary Super Saiyan",
								 "TWB33 - Lemon Lime Green",
								 "TWB34 - Lime Midori",
								 "TWB35 - Mania Plus Green",
								 "TWB36 - Microvision",
								 "TWB37 - Million Live Gold",
								 "TWB38 - Miraitowa Blue",
								 "TWB39 - NASCAR",
								 "TWB40 - Neo Geo Pocket",
								 "TWB41 - Neon Blue",
								 "TWB42 - Neon Green",
								 "TWB43 - Neon Pink",
								 "TWB44 - Neon Red",
								 "TWB45 - Neon Yellow",
								 "TWB46 - Nick Orange",
								 "TWB47 - Nijigasaki Orange",
								 "TWB48 - Odyssey Gold",
								 "TWB49 - Patrick Star Pink",
								 "TWB50 - Pikachu Yellow",
								 "TWB51 - Pocket Tales",
								 "TWB52 - Pokemon mini",
								 "TWB53 - Pretty Guardian Gold",
								 "TWB54 - S.E.E.S. Blue",
								 "TWB55 - Saint Snow Red",
								 "TWB56 - Scooby-Doo Mystery",
								 "TWB57 - Shiny Sky Blue",
								 "TWB58 - Sidem Green",
								 "TWB59 - Slime Blue",
								 "TWB60 - Spongebob Yellow",
								 "TWB61 - Stone Orange",
								 "TWB62 - Straw Hat Red",
								 "TWB63 - Superball Ivory",
								 "TWB64 - Super Saiyan Blue",
								 "TWB65 - Super Saiyan Rose",
								 "TWB66 - Supervision",
								 "TWB67 - Survey Corps Brown",
								 "TWB68 - Tea Midori",
								 "TWB69 - TI-83",
								 "TWB70 - Tokyo Midtown",
								 "TWB71 - Travel Wood",
								 "TWB72 - Virtual Boy",
								 "TWB73 - VMU",
								 "TWB74 - Wisteria Murasaki",
								 "TWB75 - WonderSwan",
								 "TWB76 - Yellow Banana" };

		int n_all_gambate_gc_colors_modes = 126;
		for (int i = 0; i < n_all_gambate_gc_colors_modes; i++)
			colorizations_choices->add(all_gambate_gc_colors_modes[i], all_gambate_gc_colors_modes[i], currentColorization == std::string(all_gambate_gc_colors_modes[i]));
#ifdef _ENABLEEMUELEC
        if (SystemData::es_features_loaded || (!SystemData::es_features_loaded && (systemData->getName() == "gb" || systemData->getName() == "gbc" || systemData->getName() == "gb2players" || systemData->getName() == "gbc2players" || systemData->getName() == "gbh" || systemData->getName() == "gbch"))) // only for gb, gbc and gb2players gbh gbch
#else
        if (SystemData::es_features_loaded || (!SystemData::es_features_loaded && (systemData->getName() == "gb" || systemData->getName() == "gbc" || systemData->getName() == "gb2players" || systemData->getName() == "gbc2players")))  // only for gb, gbc and gb2players
#endif
		{
			systemConfiguration->addWithLabel(_("COLORIZATION"), colorizations_choices);
			systemConfiguration->addSaveFunc([colorizations_choices, configName] { SystemConf::getInstance()->set(configName + ".renderer.colorization", colorizations_choices->getSelected()); });
		}
	}
#ifndef _ENABLEEMUELEC
	// ps2 full boot
	if (systemData->isFeatureSupported(currentEmulator, currentCore, EmulatorFeatures::fullboot))
	{
		if (SystemData::es_features_loaded || (!SystemData::es_features_loaded && systemData->getName() == "ps2")) // only for ps2
		{
			auto fullboot_enabled = std::make_shared<OptionListComponent<std::string>>(mWindow, _("FULL BOOT"));
			fullboot_enabled->addRange({ { _("DEFAULT"), "default" },{ _("ON") , "1" },{ _("OFF"), "0" } }, SystemConf::getInstance()->get(configName + ".fullboot"));
			systemConfiguration->addWithLabel(_("FULL BOOT"), fullboot_enabled);
			systemConfiguration->addSaveFunc([fullboot_enabled, configName] { SystemConf::getInstance()->set(configName + ".fullboot", fullboot_enabled->getSelected()); });
		}
	}

	// wii emulated wiimotes
	if (systemData->isFeatureSupported(currentEmulator, currentCore, EmulatorFeatures::emulated_wiimotes))
	{
		if (SystemData::es_features_loaded || (!SystemData::es_features_loaded && systemData->getName() == "wii"))  // only for wii
		{
			auto emulatedwiimotes_enabled = std::make_shared<OptionListComponent<std::string>>(mWindow, _("EMULATED WIIMOTES"));
			emulatedwiimotes_enabled->addRange({ { _("DEFAULT"), "default" },{ _("ON") , "1" },{ _("OFF"), "0" } }, SystemConf::getInstance()->get(configName + ".emulatedwiimotes"));
			systemConfiguration->addWithLabel(_("EMULATED WIIMOTES"), emulatedwiimotes_enabled);
			systemConfiguration->addSaveFunc([emulatedwiimotes_enabled, configName] { SystemConf::getInstance()->set(configName + ".emulatedwiimotes", emulatedwiimotes_enabled->getSelected()); });
		}
	}

	// citra change screen layout
	if (systemData->isFeatureSupported(currentEmulator, currentCore, EmulatorFeatures::screen_layout))
	{
		if (SystemData::es_features_loaded || (!SystemData::es_features_loaded && systemData->getName() == "3ds"))  // only for 3ds
		{
			auto changescreen_layout = std::make_shared<OptionListComponent<std::string>>(mWindow, _("CHANGE SCREEN LAYOUT"));
			changescreen_layout->addRange({ { _("DEFAULT"), "default" },{ _("LARGE SCREEN") , "2" },{ _("SIDE BY SIDE"), "3" } }, SystemConf::getInstance()->get(configName + ".layout_option"));
			systemConfiguration->addWithLabel(_("CHANGE SCREEN LAYOUT"), changescreen_layout);
			systemConfiguration->addSaveFunc([changescreen_layout, configName] { SystemConf::getInstance()->set(configName + ".layout_option", changescreen_layout->getSelected()); });
		}
	}

	// psp internal resolution
	if (systemData->isFeatureSupported(currentEmulator, currentCore, EmulatorFeatures::internal_resolution))
	{
		std::string curResol = SystemConf::getInstance()->get(configName + ".internalresolution");

		auto internalresolution = std::make_shared<OptionListComponent<std::string>>(mWindow, _("INTERNAL RESOLUTION"));
		internalresolution->add(_("DEFAULT"), "default", curResol.empty() || curResol == "default");
		internalresolution->add("1:1", "0", curResol == "0");
		internalresolution->add("x1", "1", curResol == "1");
		internalresolution->add("x2", "2", curResol == "2");
		internalresolution->add("x3", "3", curResol == "3");
		internalresolution->add("x4", "4", curResol == "4");
		internalresolution->add("x5", "5", curResol == "5");
		internalresolution->add("x8", "8", curResol == "8");
		internalresolution->add("x10", "10", curResol == "10");

		if (!internalresolution->hasSelection())
			internalresolution->selectFirstItem();

		if (SystemData::es_features_loaded || (!SystemData::es_features_loaded && (systemData->getName() == "psp" || systemData->getName() == "wii" || systemData->getName() == "gamecube"))) // only for psp, wii, gamecube
		{
			systemConfiguration->addWithLabel(_("INTERNAL RESOLUTION"), internalresolution);
			systemConfiguration->addSaveFunc([internalresolution, configName] { SystemConf::getInstance()->set(configName + ".internalresolution", internalresolution->getSelected()); });
		}
	}
#endif
	// Load per-game / per-emulator / per-system custom features
	std::vector<CustomFeature> customFeatures = systemData->getCustomFeatures(currentEmulator, currentCore);

	auto groups = groupBy(customFeatures, [](const CustomFeature& item) { return item.submenu; });
	for (auto group : groups)
	{
		if (!group.first.empty())
		{
			systemConfiguration->addEntry(group.first, true, [configName, mWindow, group]
			{
				GuiSettings* groupSettings = new GuiSettings(mWindow, _(group.first.c_str()));

				for (auto feat : group.second)
				{
					std::string storageName = configName + "." + feat.value;
					std::string storedValue = SystemConf::getInstance()->get(storageName);

					auto cf = std::make_shared<OptionListComponent<std::string>>(mWindow, _(feat.name.c_str()));
					cf->add(_("DEFAULT"), "", storedValue.empty() || storedValue == "default");

					for (auto fval : feat.choices)
						cf->add(_(fval.name.c_str()), fval.value, storedValue == fval.value);

					if (!cf->hasSelection())
						cf->selectFirstItem();

					if (!feat.description.empty())
						groupSettings->addWithDescription(_(feat.name.c_str()), _(feat.description.c_str()), cf);
					else
						groupSettings->addWithLabel(_(feat.name.c_str()), cf);

					groupSettings->addSaveFunc([cf, storageName]
					{
						SystemConf::getInstance()->set(storageName, cf->getSelected());
					});
				}

				mWindow->pushGui(groupSettings);
			});
		}
		else
		{
			for (auto feat : group.second)
	{
		std::string storageName = configName + "." + feat.value;
		std::string storedValue = SystemConf::getInstance()->get(storageName);

		auto cf = std::make_shared<OptionListComponent<std::string>>(mWindow, _(feat.name.c_str()));
		cf->add(_("DEFAULT"), "", storedValue.empty() || storedValue == "default");

				for (auto fval : feat.choices)
			cf->add(_(fval.name.c_str()), fval.value, storedValue == fval.value);

		if (!cf->hasSelection())
			cf->selectFirstItem();

		if (!feat.description.empty())
			systemConfiguration->addWithDescription(_(feat.name.c_str()), _(feat.description.c_str()), cf);
		else
			systemConfiguration->addWithLabel(_(feat.name.c_str()), cf);

		systemConfiguration->addSaveFunc([cf, storageName]
		{
			SystemConf::getInstance()->set(storageName, cf->getSelected());
		});
	}
		}
	}

	// automatic controller configuration
	if (systemData->isFeatureSupported(currentEmulator, currentCore, EmulatorFeatures::autocontrollers))
	{
		auto autoControllers = std::make_shared<OptionListComponent<std::string>>(mWindow, _("AUTOCONFIGURE CONTROLLERS"));
		autoControllers->addRange({ { _("DEFAULT"), "" },{ _("ON"), "0" },{ _("OFF"), "1" } }, SystemConf::getInstance()->get(configName + ".disableautocontrollers"));
		systemConfiguration->addWithLabel(_("AUTOCONFIGURE CONTROLLERS"), autoControllers);
		systemConfiguration->addSaveFunc([configName, autoControllers] { SystemConf::getInstance()->set(configName + ".disableautocontrollers", autoControllers->getSelected()); });
	}

	if (fileData == nullptr && ApiSystem::getInstance()->isScriptingSupported(ApiSystem::ScriptId::EVMAPY) && systemData->isCurrentFeatureSupported(EmulatorFeatures::Features::padTokeyboard))
	{
		if (systemData->hasKeyboardMapping())
			systemConfiguration->addEntry(_("EDIT PADTOKEY PROFILE"), true, [mWindow, systemData] { editKeyboardMappings(mWindow, systemData, true); });
		else
			systemConfiguration->addEntry(_("CREATE PADTOKEY PROFILE"), true, [mWindow, systemData] { editKeyboardMappings(mWindow, systemData, true); });
	}

	// Set as boot game
	/*
	if (fileData != nullptr)
	{
		std::string gamePath = fileData->getFullPath();

		auto bootgame = std::make_shared<SwitchComponent>(mWindow);
		bootgame->setState(SystemConf::getInstance()->get("global.bootgame.path") == gamePath);
		systemConfiguration->addWithLabel(_("LAUNCH THIS GAME AT STARTUP"), bootgame);
		systemConfiguration->addSaveFunc([bootgame, fileData, gamePath]
		{
			if (bootgame->changed())
			{
				SystemConf::getInstance()->set("global.bootgame.path", bootgame->getState() ? gamePath : "");
				SystemConf::getInstance()->set("global.bootgame.cmd", bootgame->getState() ? fileData->getlaunchCommand(false) : "");
			}
		});
	}
	*/

	mWindow->pushGui(systemConfiguration);
}

std::shared_ptr<OptionListComponent<std::string>> GuiMenu::createRatioOptionList(Window *window, std::string configname)
{
	auto ratio_choice = std::make_shared<OptionListComponent<std::string> >(window, _("GAME ASPECT RATIO"), false);
	std::string currentRatio = SystemConf::getInstance()->get(configname + ".ratio");
	if (currentRatio.empty())
		currentRatio = std::string("default");

	std::map<std::string, std::string> *ratioMap = LibretroRatio::getInstance()->getRatio();
	for (auto ratio = ratioMap->begin(); ratio != ratioMap->end(); ratio++)
		ratio_choice->add(_(ratio->first.c_str()), ratio->second, currentRatio == ratio->second);

	if (!ratio_choice->hasSelection())
		ratio_choice->selectFirstItem();

	return ratio_choice;
}

std::shared_ptr<OptionListComponent<std::string>> GuiMenu::createVideoResolutionModeOptionList(Window *window, std::string configname)
{
	auto videoResolutionMode_choice = std::make_shared<OptionListComponent<std::string> >(window, _("VIDEO MODE"), false);

	std::string currentVideoMode = SystemConf::getInstance()->get(configname + ".videomode");
	if (currentVideoMode.empty())
		currentVideoMode = std::string("default");

	std::vector<std::string> videoResolutionModeMap = ApiSystem::getInstance()->getVideoModes();
	videoResolutionMode_choice->add(_("DEFAULT"), "default", currentVideoMode == "default");
	for (auto videoMode = videoResolutionModeMap.begin(); videoMode != videoResolutionModeMap.end(); videoMode++)
	{
		std::vector<std::string> tokens = Utils::String::split(*videoMode, ':');

		// concatenat the ending words
		std::string vname;
		for (unsigned int i = 1; i < tokens.size(); i++)
		{
			if (i > 1)
				vname += ":";

			vname += tokens.at(i);
		}

		videoResolutionMode_choice->add(vname, tokens.at(0), currentVideoMode == tokens.at(0));
	}

	if (!videoResolutionMode_choice->hasSelection())
		videoResolutionMode_choice->selectFirstItem();

	return videoResolutionMode_choice;
}

void GuiMenu::clearLoadedInput() {
  for(int i=0; i<mLoadedInput.size(); i++) {
    delete mLoadedInput[i];
  }
  mLoadedInput.clear();
}

GuiMenu::~GuiMenu() {
  clearLoadedInput();
}

std::vector<DecorationSetInfo> GuiMenu::getDecorationsSets(SystemData* system)
{
	std::vector<DecorationSetInfo> sets;
	if (system == nullptr)
		return sets;

	static const size_t pathCount = 3;


#if WIN32
	std::vector<std::string> paths =
	{
		Utils::FileSystem::getEsConfigPath() + "/decorations" // for win32 testings
	};

	std::string win32path = Win32ApiSystem::getEmulatorLauncherPath("system.decorations");
	if (!win32path.empty())
		paths[0] = win32path;

	win32path = Win32ApiSystem::getEmulatorLauncherPath("decorations");
	if (!win32path.empty())
		paths.push_back(win32path);


#else
	std::vector<std::string> paths = {
		"/storage/roms/bezels",
		"/tmp/overlays/bezels"
	};
#endif

	Utils::FileSystem::stringList dirContent;
	std::string folder;

	for (auto path : paths)
	{
		if (!Utils::FileSystem::isDirectory(path))
			continue;

		dirContent = Utils::FileSystem::getDirContent(path);
		for (Utils::FileSystem::stringList::const_iterator it = dirContent.cbegin(); it != dirContent.cend(); ++it)
		{
			if (Utils::FileSystem::isDirectory(*it))
			{
				folder = *it;

				DecorationSetInfo info;
				info.name = folder.substr(path.size() + 1);
				info.path = folder;

				if (system != nullptr && Utils::String::startsWith(info.name, "default"))
				{
					std::string systemImg = path + "/"+ info.name +"/systems/" + system->getName() + ".png";
					if (Utils::FileSystem::exists(systemImg))
						info.imageUrl = systemImg;
				}

				if (info.imageUrl.empty())
				{
					std::string img = folder + "/default.png";
					if (Utils::FileSystem::exists(img))
						info.imageUrl = img;
				}

				sets.push_back(info);
			}
		}
	}

	struct { bool operator()(DecorationSetInfo& a, DecorationSetInfo& b) const { return a.name < b.name; } } compareByName;
	struct { bool operator()(DecorationSetInfo& a, DecorationSetInfo& b) const { return a.name == b.name; } } nameEquals;

	// sort and remove duplicates
	std::sort(sets.begin(), sets.end(), compareByName);
	sets.erase(std::unique(sets.begin(), sets.end(), nameEquals), sets.end());

	return sets;
}


void GuiMenu::openFormatDriveSettings()
{
	Window *window = mWindow;

	auto s = new GuiSettings(mWindow, _("FORMAT DEVICE").c_str());

	// Drive
	auto optionsStorage = std::make_shared<OptionListComponent<std::string> >(window, _("DEVICE TO FORMAT"), false);

	std::vector<std::string> disks = ApiSystem::getInstance()->getFormatDiskList();
	if (disks.size() == 0)
		optionsStorage->add(_("NONE"), "", false);
	else
	{
		for (auto disk : disks)
		{
			auto idx = disk.find(" ");
			if (idx != std::string::npos)
				optionsStorage->add(disk.substr(idx + 1), disk.substr(0, idx), false);
		}
	}

	optionsStorage->selectFirstItem();
	s->addWithLabel(_("DEVICE TO FORMAT"), optionsStorage);

	// File system
	auto fileSystem = std::make_shared<OptionListComponent<std::string> >(window, _("FILE SYSTEM"), false);

	std::vector<std::string> fileSystems = ApiSystem::getInstance()->getFormatFileSystems();
	if (fileSystems.size() == 0)
		fileSystem->add(_("NONE"), "", false);
	else
	{
		for (auto fs : fileSystems)
			fileSystem->add(fs, fs, false);
	}

	fileSystem->selectFirstItem();
	s->addWithLabel(_("FILE SYSTEM"), fileSystem);

	s->addEntry(_("FORMAT NOW"), false, [s, optionsStorage, fileSystem, window]
		{
			std::string disk = optionsStorage->getSelected();
			std::string fs = fileSystem->getSelected();

			if (disk.empty() || fs.empty())
			{
				window->pushGui(new GuiMsgBox(window, _("SELECTED OPTIONS ARE INVALID")));
				return;
			}

			window->pushGui(new GuiMsgBox(window, _("ARE YOU SURE YOU WANT TO FORMAT THIS DRIVE?"), _("YES"), [s, window, disk, fs]
			{
				ThreadedFormatter::start(window, disk, fs);
				s->close();
			}, _("NO"), nullptr));

		});

	mWindow->pushGui(s);
}



void GuiMenu::saveSubsetSettings()
{
	auto currentSystem = ViewController::get()->getState().getSystem();
	if (currentSystem == nullptr || currentSystem->getTheme() == nullptr)
		return;

	std::string fileData;

	auto subsets = currentSystem->getTheme()->getSubSetNames();
	for (auto subset : subsets)
	{
		std::string name = subset;
		std::string value;

		if (name == "colorset")
			value = Settings::getInstance()->getString("ThemeColorSet");
		else if (name == "iconset")
			value = Settings::getInstance()->getString("ThemeIconSet");
		else if (name == "menu")
			value = Settings::getInstance()->getString("ThemeMenu");
		else if (name == "systemview")
			value = Settings::getInstance()->getString("ThemeSystemView");
		else if (name == "gamelistview")
			value = Settings::getInstance()->getString("ThemeGamelistView");
		else if (name == "region")
			value = Settings::getInstance()->getString("ThemeRegionName");
		else
		{
			value = Settings::getInstance()->getString("subset." + name);
			name = "subset." + name;
		}

		if (!value.empty())
			fileData += name + "=" + value + "\r";

		for (auto system : SystemData::sSystemVector)
		{
			value = Settings::getInstance()->getString("subset." + system->getThemeFolder() + "." + subset);
			if (!value.empty())
				fileData += "subset." + system->getThemeFolder() + "." + subset + "=" + value + "\r";
		}
	}

	if (!Settings::getInstance()->getString("GamelistViewStyle").empty() && Settings::getInstance()->getString("GamelistViewStyle") != "automatic")
		fileData += "GamelistViewStyle=" + Settings::getInstance()->getString("GamelistViewStyle") + "\r";

	if (!Settings::getInstance()->getString("DefaultGridSize").empty())
		fileData += "DefaultGridSize=" + Settings::getInstance()->getString("DefaultGridSize") + "\r";

	for (auto system : SystemData::sSystemVector)
	{
		auto defaultView = Settings::getInstance()->getString(system->getName() + ".defaultView");
		if (!defaultView.empty())
			fileData += system->getName() + ".defaultView=" + defaultView + "\r";

		auto gridSizeOverride = Settings::getInstance()->getString(system->getName() + ".gridSize");
		if (!gridSizeOverride.empty())
			fileData += system->getName() + ".gridSize=" + gridSizeOverride + "\r";
	}

	std::string path = Utils::FileSystem::getEsConfigPath() + "/themesettings";
	if (!Utils::FileSystem::exists(path))
		Utils::FileSystem::createDirectory(path);

	std::string themeSet = Settings::getInstance()->getString("ThemeSet");
	std::string fileName = path + "/" + themeSet + ".cfg";

	if (fileData.empty())
	{
		if (Utils::FileSystem::exists(fileName))
			Utils::FileSystem::removeFile(fileName);
	}
	else
		Utils::FileSystem::writeAllText(fileName, fileData);

}

void GuiMenu::loadSubsetSettings(const std::string themeName)
{
	std::string path = Utils::FileSystem::getEsConfigPath() + "/themesettings";
	if (!Utils::FileSystem::exists(path))
		Utils::FileSystem::createDirectory(path);

	std::string fileName = path + "/" + themeName + ".cfg";
	if (!Utils::FileSystem::exists(fileName))
		return;

	std::string line;
	std::ifstream systemConf(fileName);
	if (systemConf && systemConf.is_open())
	{
		while (std::getline(systemConf, line, '\r'))
		{
			int idx = line.find("=");
			if (idx == std::string::npos || line.find("#") == 0 || line.find(";") == 0)
				continue;

			std::string name = line.substr(0, idx);
			std::string value = line.substr(idx + 1);
			if (!name.empty() && !value.empty())
			{
				if (name == "colorset")
					Settings::getInstance()->setString("ThemeColorSet", value);
				else if (name == "iconset")
					Settings::getInstance()->setString("ThemeIconSet", value);
				else if (name == "menu")
					Settings::getInstance()->setString("ThemeMenu", value);
				else if (name == "systemview")
					Settings::getInstance()->setString("ThemeSystemView", value);
				else if (name == "gamelistview")
					Settings::getInstance()->setString("ThemeGamelistView", value);
				else if (name == "region")
					Settings::getInstance()->setString("ThemeRegionName", value);
				else if (name == "GamelistViewStyle")
					Settings::getInstance()->setString("GamelistViewStyle", value);
				else if (name == "DefaultGridSize")
					Settings::getInstance()->setString("DefaultGridSize", value);
				else if (name.find(".defaultView") != std::string::npos)
					Settings::getInstance()->setString(name, value);
				else if (name.find(".gridSize") != std::string::npos)
					Settings::getInstance()->setString(name, value);
				else if (Utils::String::startsWith(name, "subset."))
					Settings::getInstance()->setString(name, value);
			}
		}
		systemConf.close();

		for (auto system : SystemData::sSystemVector)
		{
			auto defaultView = Settings::getInstance()->getString(system->getName() + ".defaultView");
			auto gridSizeOverride = Vector2f::parseString(Settings::getInstance()->getString(system->getName() + ".gridSize"));
			system->setSystemViewMode(defaultView, gridSizeOverride, false);
		}
	}
	else
		LOG(LogError) << "Unable to open " << fileName;
}

void GuiMenu::editKeyboardMappings(Window *window, IKeyboardMapContainer* mapping, bool editable)
{
	window->pushGui(new GuiKeyMappingEditor(window, mapping, editable));
}
