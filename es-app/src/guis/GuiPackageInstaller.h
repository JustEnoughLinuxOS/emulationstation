#pragma once

#include <string>
#include "GuiComponent.h"
#include "components/MenuComponent.h"
#include "components/ComponentGrid.h"
#include "components/TextComponent.h"
#include "components/ImageComponent.h"
#include "ApiSystem.h"
#include "ContentInstaller.h"

template<typename T>
class OptionListComponent;

class GuiThreeFiftyOnePackageEntry : public ComponentGrid
{
public:
	GuiThreeFiftyOnePackageEntry(Window* window, ThreeFiftyOnePackage& entry);

	bool isInstallPending() { return mIsPending; }
	ThreeFiftyOnePackage& getEntry() { return mEntry; }
	virtual void setColor(unsigned int color);

	void update(int deltaTime) override;

private:
	std::shared_ptr<TextComponent>  mImage;
	std::shared_ptr<TextComponent>  mText;
	std::shared_ptr<TextComponent>  mSubstring;

	std::shared_ptr<ImageComponent>  mPreviewImage;

	ThreeFiftyOnePackage mEntry;
	bool mIsPending;
};


class GuiPackageInstallStart : public GuiComponent, IContentInstalledNotify
{
public:
	GuiPackageInstallStart(Window* window);
	~GuiPackageInstallStart();

	bool input(InputConfig* config, Input input) override;
	void update(int deltaTime) override;

	virtual std::vector<HelpPrompt> getHelpPrompts() override;

	void OnContentInstalled(int contentType, std::string contentName, bool success) override;

private:
	void loadPackagesAsync();
	void loadList();

	void centerWindow();
	void processPackage(ThreeFiftyOnePackage package);

	int				mReloadList;
	MenuComponent	mMenu;

	std::vector<ThreeFiftyOnePackage> mPackages;
};
