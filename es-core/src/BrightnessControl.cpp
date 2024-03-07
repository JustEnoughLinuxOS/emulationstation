#include "BrightnessControl.h"
#include "Log.h"
#include "Settings.h"
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <sstream>
#include <SystemConf.h>
#include <unistd.h>
#include "platform.h"
#include <math.h>

std::string BACKLIGHT_PATH = std::string(getShOutput(R"(/usr/bin/brightness path)"));
std::string BACKLIGHT_BRIGHTNESS_NAME = BACKLIGHT_PATH + "/brightness";
std::string BACKLIGHT_BRIGHTNESS_MAX_NAME = BACKLIGHT_PATH + "/max_brightness";
std::vector<float> brightness_table = {0, 1};
#define BACKLIGHT_BUFFER_SIZE 127

std::weak_ptr<BrightnessControl> BrightnessControl::sInstance;

std::shared_ptr<BrightnessControl> &BrightnessControl::getInstance()
{
    // check if an BrightnessControl instance is already created, if not create one
    static std::shared_ptr<BrightnessControl> sharedInstance = sInstance.lock();
    if (sharedInstance == nullptr)
    {
        sharedInstance.reset(new BrightnessControl);
        sInstance = sharedInstance;
    }
    return sharedInstance;
}

BrightnessControl::BrightnessControl() {}

int BrightnessControl::getNumBrightness() const
{
  return brightness_table.size();
}

int BrightnessControl::getBrightness() const
{
#ifdef WIN32
#error TODO: Not implemented for Windows yet!!!
#endif

    int value = 0;

    int fd;
    int max = 100;
    char buffer[BACKLIGHT_BUFFER_SIZE + 1];
    ssize_t count;

    fd = open(BACKLIGHT_BRIGHTNESS_MAX_NAME.c_str(), O_RDONLY);
    if (fd < 0)
        return false;

    memset(buffer, 0, BACKLIGHT_BUFFER_SIZE + 1);

    count = read(fd, buffer, BACKLIGHT_BUFFER_SIZE);
    if (count > 0)
        max = atoi(buffer);

    close(fd);

    if (max == 0)
        return 0;

    SystemConf::getInstance()->loadSystemConf();

    std::stringstream ss(SystemConf::getInstance()->get("brightness_table"));
    brightness_table.clear();
    float b_val;
    while (ss >> b_val) {
      brightness_table.push_back(b_val);
    }
    //TODO: need to make a rational initial condition -- this should always be populated by the brightness script on boot??
    if (brightness_table.empty()) {
      brightness_table = {0, 0.04, 0.08, 0.13, 0.19, 0.25, 0.33, 0.42, 0.54, 0.71, 1};
    }

		auto sysbright = SystemConf::getInstance()->get("system.brightness");
    value = stoi(sysbright);
    return value;
}

void BrightnessControl::setBrightness(int value)
{
#ifdef WIN32
#error TODO: Not implemented for Windows yet!!!
#endif

    if (value < 0)
        value = 0;

    if (value >= brightness_table.size())
        value = brightness_table.size() - 1;

    int fd;
    int max = 100;
    char buffer[BACKLIGHT_BUFFER_SIZE + 1];
    ssize_t count;

    fd = open(BACKLIGHT_BRIGHTNESS_MAX_NAME.c_str(), O_RDONLY);
    if (fd < 0)
        return;

    memset(buffer, 0, BACKLIGHT_BUFFER_SIZE + 1);

    count = read(fd, buffer, BACKLIGHT_BUFFER_SIZE);
    if (count > 0)
        max = atoi(buffer);

    close(fd);

    if (max == 0)
        return;

    fd = open(BACKLIGHT_BRIGHTNESS_NAME.c_str(), O_WRONLY);
    if (fd < 0)
        return;

    float val = max * brightness_table[value] ;
    if (val > max)
        val = max;
    sprintf(buffer, "%d\n", (uint32_t)val);

    count = write(fd, buffer, strlen(buffer));
    if (count < 0)
        LOG(LogError) << "BrightnessControl::setBrightness failed";
    close(fd);

}

bool BrightnessControl::isAvailable()
{
#if WIN32
    return false;
#else
    int fd;
    fd = open(BACKLIGHT_BRIGHTNESS_MAX_NAME.c_str(), O_RDONLY);
    if (fd < 0) {
        return false;
    } else {
        close(fd);
        return true;
    }
#endif
}

void BrightnessControl::init()
{
        int brightness = BrightnessControl::getInstance()->getBrightness();
        auto sysbright = SystemConf::getInstance()->get("system.brightness");
	if (sysbright.empty()) {
		return;
	}
	else
	{
	        BrightnessControl::getInstance()->setBrightness(stoi(sysbright));
	}
}
