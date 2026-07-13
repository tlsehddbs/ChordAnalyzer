#include "MainComponent.h"

class ChordAnalyzerApplication final : public juce::JUCEApplication
{
public:
    const juce::String getApplicationName() override { return "Chord Analyzer"; }
    const juce::String getApplicationVersion() override { return "0.1.0"; }
    bool moreThanOneInstanceAllowed() override { return true; }

    void initialise(const juce::String&) override
    {
        juce::PropertiesFile::Options options;
        options.applicationName = getApplicationName();
        options.filenameSuffix = "settings";
        options.folderName = getApplicationName();
        options.osxLibrarySubFolder = "Application Support";
        applicationProperties.setStorageParameters(options);

        mainWindow = std::make_unique<MainWindow>(getApplicationName(), applicationProperties);
    }

    void shutdown() override
    {
        mainWindow.reset();
        applicationProperties.closeFiles();
    }

    void systemRequestedQuit() override
    {
        quit();
    }

private:
    class MainWindow final : public juce::DocumentWindow
    {
    public:
        MainWindow(const juce::String& name, juce::ApplicationProperties& properties)
            : DocumentWindow(name, juce::Colour(0xfff7f6f2), DocumentWindow::allButtons)
        {
            setUsingNativeTitleBar(true);
            setContentOwned(new MainComponent(properties), true);
            setResizable(true, true);
            centreWithSize(1120, 780);
            setVisible(true);
        }

        void closeButtonPressed() override
        {
            juce::JUCEApplication::getInstance()->systemRequestedQuit();
        }
    };

    juce::ApplicationProperties applicationProperties;
    std::unique_ptr<MainWindow> mainWindow;
};

START_JUCE_APPLICATION(ChordAnalyzerApplication)
