#include "BrightnessControl.h"
#include "Log.h"
#include "Settings.h"
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <SystemConf.h>
#include <unistd.h>
#include "platform.h"

std::string BACKLIGHT_DEVICE = std::string(getShOutput(R"(/usr/bin/brightness device)"));
std::string BACKLIGHT_BRIGHTNESS_NAME = "/sys/class/backlight/" + BACKLIGHT_DEVICE + "/brightness";
std::string BACKLIGHT_BRIGHTNESS_MAX_NAME = "/sys/class/backlight/" + BACKLIGHT_DEVICE + "/max_brightness";
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

    fd = open(BACKLIGHT_BRIGHTNESS_NAME.c_str(), O_RDONLY);
    if (fd < 0)
        return false;

    memset(buffer, 0, BACKLIGHT_BUFFER_SIZE + 1);

    count = read(fd, buffer, BACKLIGHT_BUFFER_SIZE);
    if (count > 0)
        value = atoi(buffer);

    close(fd);

    value = (uint32_t)((value / (float)max * 100.0f) + 0.5f);
    return value;
}

void BrightnessControl::setBrightness(int value)
{
#ifdef WIN32
#error TODO: Not implemented for Windows yet!!!
#endif

    if (value < 1)
        value = 1;

    if (value > 100)
        value = 100;

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

    float percent = (value / 100.0f * (float)max) + 0.5f;
    sprintf(buffer, "%d\n", (uint32_t)percent);

    count = write(fd, buffer, strlen(buffer));
    if (count < 0)
        LOG(LogError) << "BrightnessControl::setBrightness failed";
    close(fd);

    SystemConf::getInstance()->set("system.brightness", buffer);
    SystemConf::getInstance()->saveSystemConf();

}

bool BrightnessControl::isAvailable()
{
#if WIN32
    return false;
#else
    return true;
#endif
}
