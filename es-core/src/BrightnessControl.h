#pragma once
#ifndef ES_APP_BRIGHTNESS_CONTROL_H
#define ES_APP_BRIGHTNESS_CONTROL_H

#include <memory>

/*!
Singleton pattern. Call getInstance() to get an object.
*/
class BrightnessControl
{
    int originalBrightness;
    int internalBrightness;

    static std::weak_ptr<BrightnessControl> sInstance;

    BrightnessControl();
    BrightnessControl(const BrightnessControl &right);
    BrightnessControl &operator=(const BrightnessControl &right);

public:
    static std::shared_ptr<BrightnessControl> &getInstance();

    bool getBrightness(int& value);
    void setBrightness(int Brightness);
};

#endif // ES_APP_BRIGHTNESS_CONTROL_H
