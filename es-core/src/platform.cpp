#include "platform.h"

#include <SDL_events.h>
#ifdef WIN32
#include <codecvt>
#else
#include <unistd.h>
#endif
#include <sys/wait.h>
#include <fcntl.h>
#include "Window.h"
#include "utils/FileSystemUtil.h"
#include "utils/StringUtil.h"
#include "Log.h"
#include "Scripting.h"

#if !defined(WIN32)
#include <ifaddrs.h>
#include <netinet/in.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <arpa/inet.h>
#endif

// #define DEVTEST

int runShutdownCommand()
{
	return system("/usr/sbin/shutdown -h now");
}

int runRestartCommand()
{
	return system("/usr/sbin/shutdown -r now");
}

int runSystemCommand(const std::string& cmd_utf8, const std::string& name, Window* window)
{

	std::string stderrFilename = " 2>/var/log/es_launch_stderr.log";
	std::string stdoutFilename = " >/var/log/es_launch_stdout.log";	

	// Parent fork
	pid_t ret = fork();
	if (ret == 0)
	{
		ret = fork();
		if (ret == 0)
		{
			execl("/usr/bin/sh", "sh", "-c", (cmd_utf8 + stderrFilename + stdoutFilename).c_str(), (char *) NULL);
			_exit(1); // execl failed
		}
		int status;
		waitpid(ret, &status, 0); // Wait for fork exit..
		_exit(0);
	}
	else
	{
		if (ret > 0)
		{
			int status;
			waitpid(ret, &status, 0); // keep calm and kill zombies
		}
	}
	return 0;
}

QuitMode quitMode = QuitMode::QUIT;

int quitES(QuitMode mode)
{
	quitMode = mode;

	switch (quitMode)
	{
		case QuitMode::QUIT:
			Scripting::fireEvent("quit");
			break;

		case QuitMode::REBOOT:
		case QuitMode::FAST_REBOOT:
			Scripting::fireEvent("quit", "reboot");
			Scripting::fireEvent("reboot");
			break;

		case QuitMode::SHUTDOWN:
		case QuitMode::FAST_SHUTDOWN:
			Scripting::fireEvent("quit", "shutdown");
			Scripting::fireEvent("shutdown");
			break;
	}


	SDL_Event* quit = new SDL_Event();
	quit->type = SDL_QUIT;
	SDL_PushEvent(quit);
	return 0;
}

void touch(const std::string& filename)
{
#ifndef WIN32
	int fd = open(filename.c_str(), O_CREAT | O_WRONLY, 0644);
	if (fd >= 0)
		close(fd);

	// system(("touch " + filename).c_str());
#endif	
}

void processQuitMode()
{

	switch (quitMode)
	{
	case QuitMode::RESTART:
		LOG(LogInfo) << "Restarting EmulationStation";
		touch("/tmp/restart.please");
		break;
	case QuitMode::REBOOT:
	case QuitMode::FAST_REBOOT:
		LOG(LogInfo) << "Rebooting system";
		touch("/tmp/reboot.please");
		runRestartCommand();
		break;
	case QuitMode::SHUTDOWN:
	case QuitMode::FAST_SHUTDOWN:
		LOG(LogInfo) << "Shutting system down";
		touch("/tmp/shutdown.please");
		runShutdownCommand();
		break;
	}
}

bool isFastShutdown()
{
	return quitMode == QuitMode::FAST_REBOOT || quitMode == QuitMode::FAST_SHUTDOWN;
}

std::string queryIPAdress()
{
#ifdef DEVTEST
	return "127.0.0.1";
#endif

	std::string result;

#if WIN32
	// Init WinSock
	WSADATA wsa_Data;
	int wsa_ReturnCode = WSAStartup(0x101, &wsa_Data);
	if (wsa_ReturnCode != 0)
		return "";

	char* szLocalIP = nullptr;

	// Get the local hostname
	char szHostName[255];
	if (gethostname(szHostName, 255) == 0)
	{
		struct hostent *host_entry;
		host_entry = gethostbyname(szHostName);
		if (host_entry != nullptr)
			szLocalIP = inet_ntoa(*(struct in_addr *)*host_entry->h_addr_list);
	}

	WSACleanup();

	if (szLocalIP == nullptr)
		return "";

	return std::string(szLocalIP); // "127.0.0.1"
#else
	struct ifaddrs *ifAddrStruct = NULL;
	struct ifaddrs *ifa = NULL;
	void *tmpAddrPtr = NULL;

	getifaddrs(&ifAddrStruct);

	for (ifa = ifAddrStruct; ifa != NULL; ifa = ifa->ifa_next) 
	{
		if (!ifa->ifa_addr)
			continue;
		
		// check it is IP4 is a valid IP4 Address
		if (ifa->ifa_addr->sa_family == AF_INET)
		{ 			
			tmpAddrPtr = &((struct sockaddr_in *) ifa->ifa_addr)->sin_addr;
			char addressBuffer[INET_ADDRSTRLEN];
			inet_ntop(AF_INET, tmpAddrPtr, addressBuffer, INET_ADDRSTRLEN);
			
			std::string ifName = ifa->ifa_name;
			if (ifName.find("eth") != std::string::npos || ifName.find("wlan") != std::string::npos || ifName.find("mlan") != std::string::npos || ifName.find("en") != std::string::npos || ifName.find("wl") != std::string::npos || ifName.find("p2p") != std::string::npos)
			{
				result = std::string(addressBuffer);
				break;
			}
		}
	}
	// Seeking for ipv6 if no IPV4
	if (result.empty()) 
	{
		for (ifa = ifAddrStruct; ifa != NULL; ifa = ifa->ifa_next) 
		{
			if (!ifa->ifa_addr)
				continue;
			
			// check it is IP6 is a valid IP6 Address
			if (ifa->ifa_addr->sa_family == AF_INET6) 
			{ 				
				tmpAddrPtr = &((struct sockaddr_in6 *) ifa->ifa_addr)->sin6_addr;
				char addressBuffer[INET6_ADDRSTRLEN];
				inet_ntop(AF_INET6, tmpAddrPtr, addressBuffer, INET6_ADDRSTRLEN);

				std::string ifName = ifa->ifa_name;
				if (ifName.find("eth") != std::string::npos || ifName.find("wlan") != std::string::npos || ifName.find("mlan") != std::string::npos || ifName.find("en") != std::string::npos || ifName.find("wl") != std::string::npos || ifName.find("p2p") != std::string::npos)
				{
					result = std::string(addressBuffer);
					break;
				}
			}
		}
	}

	if (ifAddrStruct != NULL)
		freeifaddrs(ifAddrStruct);
#endif

	return result;

}

BatteryInformation queryBatteryInformation()
{
	BatteryInformation ret; 
	ret.hasBattery = false;
	ret.isCharging = false;
	ret.level = 0;

#ifdef DEVTEST
	ret.hasBattery = true;
	ret.isCharging = true;
	ret.level = 33;

	time_t     clockNow = time(0);
	struct tm  clockTstruct = *localtime(&clockNow);
	ret.level = clockTstruct.tm_min;

	return ret;
#endif

#ifdef WIN32
	SYSTEM_POWER_STATUS systemPowerStatus;
	if (GetSystemPowerStatus(&systemPowerStatus))
	{
		if ((systemPowerStatus.BatteryFlag & 128) == 128)
			ret.hasBattery = false;
		else
		{
			ret.hasBattery = true;
			ret.isCharging = (systemPowerStatus.BatteryFlag & 8) == 8;
			ret.level = systemPowerStatus.BatteryLifePercent;
		}
	}
	else
		ret.hasBattery = false;

	return ret;

#endif

	static std::string batteryStatusPath;
	static std::string batteryCapacityPath;

	// Find battery path - only at the first call
	if (batteryStatusPath.empty())
	{
		std::string batteryRootPath;

		auto files = Utils::FileSystem::getDirContent("/sys/class/power_supply");
		for (auto file : files)
		{
			if (Utils::String::toLower(file).find("/bat") != std::string::npos)
			{
				batteryRootPath = file;
				break;
			}
		}

		if (batteryRootPath.empty())
			batteryStatusPath = ".";
		else
		{
			batteryStatusPath = batteryRootPath + "/status";
			batteryCapacityPath = batteryRootPath + "/capacity";
		}
	}

	if (batteryStatusPath.length() <= 1)
	{
		ret.hasBattery = false;
		ret.isCharging = false;
		ret.level = 0;
	}
	else
	{
		ret.hasBattery = true;
		ret.isCharging = (Utils::String::replace(Utils::FileSystem::readAllText(batteryStatusPath), "\n", "") != "Discharging");
		ret.level = atoi(Utils::FileSystem::readAllText(batteryCapacityPath).c_str());
		if ( ret.level >= 100 )
		{
			ret.level = 100;
		}
	}

	return ret;
}

#ifdef _ENABLEEMUELEC
std::string getShOutput(const std::string& mStr)
{
    std::string result, file;
    FILE* pipe{popen(mStr.c_str(), "r")};
    char buffer[256];

    while(fgets(buffer, sizeof(buffer), pipe) != NULL)
    {
        file = buffer;
        result += file.substr(0, file.size() - 1);
    }

    pclose(pipe);
    return result;
}
#endif

std::string getArchString()
{
#if WIN32
	return "windows";
#endif

#if X86
	return "x86";
#endif

#if X86_64
	return "x86_64";
#endif

#if RPI1
	return "rpi1";
#endif

#if RPI2
	return "rpi2";
#endif

#if RPI3
	return "rpi3";
#endif

#if RPI4
	return "rpi4";
#endif

#if RK3566
	return "rk3566";
#endif

#if RK3566_X55
	return "rk3566-x55";
#endif

#if RK3588
	return "rk3588";
#endif

#if RK3588_ACE
	return "rk3588-ace";
#endif
	
#if S922X
	return "s922x";
#endif

#if RK3326
        return "rk3326";
#endif

#if AMD64
	return "AMD64";
#endif

#if ODROIDGOA
	return "odroidgoa";
#endif

#if GAMEFORCE
	return "gameforce";
#endif

#if ODROIDXU4
	return "odroidxu4";
#endif

#if ODROIDC2
	return "odroidc2";
#endif

#if ODROIDC4
	return "odroidc4";
#endif

#if TINKERBOARD
	return "tinkerboard";
#endif

#if RK3288
	return "rk3288";
#endif

#if RK3399
	return "rk3399";
#endif

#if MIQI
	return "miqi";
#endif

#if TRITIUM_H5
	return "tritium_h5";
#endif

#if ORANGEPI_ZERO2
	return "orangepi_zero2";
#endif

#if ORANGEPI_PC
	return "orangepi_pc";
#endif

#if CHA
	return "cha";
#endif

#if S812
	return "s812";
#endif

#if S922X
	return "s922x";
#endif

#if S905
	return "s905";
#endif

#if S905GEN3
	return "s905gen3";
#endif

#if S912
	return "s912";
#endif

	return "";
}
