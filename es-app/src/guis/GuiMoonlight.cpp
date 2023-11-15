#include "guis/GuiMoonlight.h"

#include <algorithm>
#include <iostream>
#include <sstream>
#include <fstream>
#include <cstdio>

#include "ApiSystem.h"
#include "Scripting.h"
#include "Window.h"
#include "guis/GuiMsgBox.h"
#include "platform.h"
#include "Log.h"
#include "SystemConf.h"
#include "HttpReq.h"
#include <pugixml.hpp>

class MoonlightClient {
 public:
  MoonlightClient(const std::string& server_ip)
   : server_ip_(server_ip) {}
  
  // Requests to Moonlight server
  std::string GetAppListXml() { return MakeRequest(MakeUrl("applist")); }
  void QuitApp() { MakeRequest(MakeUrl("cancel")); }
  bool WriteBoxArtFile(const std::string& app_id, std::string filename);

  // Helpers
  bool UpdateMoonlightGames();

 private:
  std::string MakeUrl(const std::string& command, std::string args={});
  std::string MakeRequest(const std::string& url, std::string* filename = nullptr);
  static std::string CreateNewGuid();

  std::string server_ip_;
};

// Moonlight version check function
bool isEmbedded(void) {
  FILE* mlver = popen ("moonlight -v", "r");
  std::stringstream ss;
  ss << mlver;
  std::string output = ss.str(); 
  if (output.find("Embedded") != std::string::npos) {
    return true;
  } else {
    return false;
  }
}

// SSL extraction function (client.pem)
void extractClient() {
  std::ifstream confFile("/storage/.config/Moonlight Game Streaming Project/Moonlight.conf");
  std::ofstream clientFile("/storage/.cache/Moonlight Game Streaming Project/client.pem");
  std::string line;
  do{
    std::getline(confFile, line);
  } 
  while (line.find("certificate=") == std::string::npos);
  if (line.find("certificate=\"") != std::string::npos) {
    line.erase(0, 24);
    line.erase(line.size()-2);
  } else {
    line.erase(0, 23);
    line.erase(line.size()-1);
  }
  std::string::size_type pos = 0;
  do{
    pos = line.find("\\n", pos);
    line.replace(pos, 2, "\n");
  }
  while (line.find("\\n", pos) != std::string::npos);
  clientFile << line;
}

// SSL extraction function (key.pem)
void extractKey() {
  std::ifstream confFile("/storage/.config/Moonlight Game Streaming Project/Moonlight.conf");
  std::ofstream keyFile("/storage/.cache/Moonlight Game Streaming Project/key.pem");
  std::string line;
  do{
    std::getline(confFile, line);
  }
  while (line.find("key=") == std::string::npos);
  if (line.find("key=\"") != std::string::npos) {
    line.erase(0, 16);
    line.erase(line.size()-2);
  } else {
    line.erase(0, 15);
    line.erase(line.size()-1);
  }
  std::string::size_type pos = 0;
  do{
    pos = line.find("\\n", pos);
    line.replace(pos, 2, "\n");
  }
  while (line.find("\\n", pos) != std::string::npos);
  keyFile << line;
}

// File existence check function
bool fileExists (const std::string& name) {
  if (FILE *file = fopen(name.c_str(), "r")) {
      fclose(file);
      return true;
  } else {
      return false;
  }   
}

std::string MoonlightClient::CreateNewGuid() {
  srand(time(NULL));

  char strUuid[256];
  snprintf(strUuid, sizeof strUuid, "%x%x-%x-%x-%x-%x%x%x", 
      rand(), rand(),                 // Generates a 64-bit Hex number
      rand(),                         // Generates a 32-bit Hex number
      ((rand() & 0x0fff) | 0x4000),   // Generates a 32-bit Hex number of the form 4xxx (4 indicates the UUID version)
      rand() % 0x3fff + 0x8000,       // Generates a 32-bit Hex number in the range [0x8000, 0xbfff]
      rand(), rand(), rand());
  return strUuid;
}

std::string MoonlightClient::MakeUrl(
    const std::string& command, std::string args) {
  if (!args.empty()) args += "&";
  args += "uniqueid=0123456789ABCDEF&uuid=" + CreateNewGuid();

  char url[1024];
  snprintf(url, sizeof url, "https://%s:47984/%s?%s",
      server_ip_.c_str(), command.c_str(), args.c_str());
  return std::string(url);
}

std::string MoonlightClient::MakeRequest(const std::string& url, std::string* filename) {
  std::string cert_path;
  if (fileExists("/storage/.config/Moonlight Game Streaming Project/Moonlight.conf") == true) {
    cert_path = "/storage/.cache/Moonlight Game Streaming Project/";
  } else {
    cert_path = "/storage/.cache/moonlight/";
  }

  HttpReqOptions opts;
  opts.clientCert = cert_path + "client.pem";
  opts.clientKey = cert_path + "key.pem";

  if (filename != nullptr)
    opts.outputFilename = *filename;

	HttpReq req(url, &opts);
  std::cout << "MoonlightClient::MakeRequest : " << url << std::endl;
	req.wait();
	
	if (req.status() != HttpReq::REQ_SUCCESS) return "";
  if (filename) {
    std::cout << "MoonlightClient::MakeRequest saved to : " << *filename << std::endl;
    return {};
  } else {
    auto res = req.getContent();
    std::cout << "MoonlightClient::MakeRequest got : " << res << std::endl;
    return res;
  }
}

bool MoonlightClient::WriteBoxArtFile(const std::string& app_id, std::string filename) {
  char args[256];
  snprintf(args, sizeof args, "appid=%s&AssetType=2&AssetIdx=0", app_id.c_str());
  MakeRequest(MakeUrl("appasset", args), &filename);
  return true;
}

bool MoonlightClient::UpdateMoonlightGames() {
  ApiSystem::executeScript("rm /storage/roms/moonlight/images/*");
  ApiSystem::executeScript("rm /storage/roms/moonlight/*");
  ApiSystem::executeScript("mkdir -p /storage/roms/moonlight");

  auto xml = GetAppListXml();
  if (xml.empty()) return false;

	pugi::xml_document doc;
	auto result = doc.load_string(xml.c_str());
 	if (!result) return false;

  // list api returns the following xml
  // <?xml version="1.0" encoding="UTF-16"?>
  // <root protocol_version="0.1" query="applist" status_code="200" status_message="OK">
  //   <App>
  //     <AppInstallPath>C:\Program Files (x86)\Steam\</AppInstallPath>
  //     <AppTitle>Steam</AppTitle>
  //     <CmsId>100021711</CmsId>
  //     <Distributor>Steam</Distributor>
  //     <ID>1088017781</ID>
  //     <IsAppCollectorGame>0</IsAppCollectorGame>
  //     <IsHdrSupported>1</IsHdrSupported>
  //     <MaxControllersForSingleSession>1</MaxControllersForSingleSession>
  //     <ShortName>steam</ShortName>
  //     <SupportedSOPS>
  //       <SOPS>
  //         <Height>2160</Height>
  //         <RefreshRate>60</RefreshRate>
  //         <Width>3840</Width>
  //       </SOPS>
  //       ...
  //     </SupportedSOPS>
  //     <UniqueId>20225001</UniqueId>
  //     <simulateControllers>0</simulateControllers>
  //   </App>
  //   ...
  // </root>

	pugi::xml_node root = doc.child("root");
  if (!root) return false;

  pugi::xml_document gamelist_doc;
  auto gamelist = gamelist_doc.append_child("gameList");

	for (auto app : root.children())
	{
    if (std::string(app.name()) != "App") continue;

    std::string title = app.child("AppTitle").text().get();
    std::cout << "MoonlightClient::UpdateMoonlightGames: " << title << std::endl;
    auto filename = title;
    std::replace(filename.begin(), filename.end(), '/', ' ');

    // Write bash script
    if (fileExists("/storage/.config/Moonlight Game Streaming Project/Moonlight.conf") == true) {
      std::ofstream app_file("/storage/roms/moonlight/" + filename + ".sh");
      app_file << "#!/bin/sh" << std::endl;
      app_file << ". /etc/profile" << std::endl;
      app_file << "jslisten set \"moonlight\"" << std::endl;
      app_file << "QT_QPA_PLATFORM=wayland moonlight stream " << server_ip_ << " \"" << title << "\" --quit-after" << std::endl;
      app_file.close();
    } else {
      std::ofstream app_file("/storage/roms/moonlight/" + filename + ".sh");
      app_file << "#!/bin/sh" << std::endl;
      app_file << ". /etc/profile" << std::endl;
      app_file << "jslisten set \"moonlight\"" << std::endl;
      app_file << "moonlight stream -app \"" << title << "\" -platform sdl " << server_ip_ << std::endl;
      app_file.close();
    }

    // Write box art image
    std::string app_id = app.child("ID").text().get();
    if (!app_id.empty()) {
      WriteBoxArtFile(app_id, "/storage/roms/moonlight/images/" + filename + ".png");
    }

    // Update gamelist
    auto game = gamelist.append_child("game");
    game.append_child("path").text().set(std::string("./" + filename + ".sh").c_str());
    game.append_child("name").text().set(title.c_str());
    game.append_child("image").text().set(std::string("./images/" + filename + ".png").c_str());
  }

  gamelist_doc.save_file("/storage/roms/moonlight/gamelist.xml");
  return true;
}

///////////////////////////////////////////////////////////////////////////////

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

  addEntry(_("QUIT CURRENT GAME"), false, [window] {
    std::string server_ip = SystemConf::getInstance()->get("moonlight.host");
    char cmd[1024];
    snprintf(cmd, sizeof cmd, "moonlight quit %s", server_ip.c_str());
    ApiSystem::executeScript(cmd);
  });

  addEntry(_("UPDATE MOONLIGHT GAMES"), false, [window] {
    std::string server_ip = SystemConf::getInstance()->get("moonlight.host");
    
    MoonlightClient client(server_ip);
    if (!server_ip.empty() && client.UpdateMoonlightGames()) {
      Scripting::fireEvent("quit", "restart");
			quitES(QuitMode::QUIT);
    } else {
      window->pushGui(new GuiMsgBox(window, _("Unable to connect to server")));
    }
  });

  addEntry(_("PAIR WITH SERVER"), false, [window, pin] {
    std::string server_ip = SystemConf::getInstance()->get("moonlight.host");

    char cmd[1024];
    if (isEmbedded() == false) {
      snprintf(cmd, sizeof cmd, "QT_QPA_PLATFORM=wayland moonlight pair -pin %s %s", pin, server_ip.c_str());
    } else {
      snprintf(cmd, sizeof cmd, "moonlight pair -pin %s %s", pin, server_ip.c_str());
    }
		ApiSystem::executeScript(cmd, [server_ip, window](std::string line) {
      std::string new_server_ip;
      if (ParseServerIp(line, &new_server_ip) && server_ip != new_server_ip) {
  			SystemConf::getInstance()->set("moonlight.host", new_server_ip);
	  		SystemConf::getInstance()->saveSystemConf();
      }
      const std::string pared_ok = "Succesfully paired";
      if (line == "Succesfully paired") {
        window->pushGui(new GuiMsgBox(window, _("Succesfully paired with server")));
      }
    if (fileExists("/storage/.config/Moonlight Game Streaming Project/Moonlight.conf") == true) {
      const std::string cert_path = "/storage/.cache/Moonlight Game Streaming Project/";
      if (fileExists(cert_path + "client.pem") == false) {
        extractClient();
      }
      if (fileExists(cert_path + "key.pem") == false) {
        extractKey();
      }
    }
		});
	});

  addEntry(_("UNPAIR WITH SERVER"), false, [this, window] {
		if (fileExists("/storage/.config/Moonlight Game Streaming Project/Moonlight.conf") == true) {
      runSystemCommand("rm -r \"~/.cache/Moonlight Game Streaming Project\"", "", nullptr);
      runSystemCommand("rm -r \"~/.config/Moonlight Game Streaming Project\"", "", nullptr);
      window->pushGui(new GuiMsgBox(window, _("Unpaired and settings cleared")));
    } else {
      runSystemCommand("rm -r ~/.cache/moonlight", "", nullptr);
      window->pushGui(new GuiMsgBox(window, _("Unpaired from server")));
    }
	});

	addGroup(_("SETTINGS"));
  addInputTextRow(_("SERVER IP"), "moonlight.host", false);
  addWithLabel(_("PAIRING PIN"), pinUI);
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
