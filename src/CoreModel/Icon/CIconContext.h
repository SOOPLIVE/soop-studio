#pragma once

#include <QIcon>

class AFIconContext final
{
#pragma region QT Field, CTOR/DTOR
public:
    static AFIconContext& GetSingletonInstance()
    {
        static AFIconContext* instance = nullptr;
        if (instance == nullptr)
            instance = new AFIconContext;
        return *instance;
    };
    ~AFIconContext() {};
private:
    // singleton constructor
    AFIconContext() = default;
    AFIconContext(const AFIconContext&) = delete;
    AFIconContext(/* rValue */AFIconContext&& other) noexcept = delete;
    AFIconContext& operator=(const AFIconContext&) = delete;
    //
#pragma endregion QT Field, CTOR/DTOR

#pragma region public func
public:
    void    InitContext();
    QIcon   GetSourceIcon(const char* id);
    QIcon   GetSceneIcon() const;
    QIcon   GetGroupIcon() const;
    QIcon   GetHotkeyConflictIcon() const;

#pragma endregion public func


#pragma region private func
private:
    void _LoadStudioIcon();

    // For Source List
    void _LoadGameCaptureIcon();
    void _LoadVideoCaptureIcon();
    void _LoadDesktopCaptureIcon();
    void _LoadWindowCaptureIcon();
    void _LoadAudioInputCaptureIcon();
    void _LoadAudioOutputCaptureIcon();
    void _LoadAudioProcessCaptureIcon();
    void _LoadBrowserCaptureIcon();
    void _LoadImageSourceIcon();
    void _LoadImageSliderIcon();
    void _LoadMediaSourceIcon();
    void _LoadTextSourceIcon();
    void _LoadColorPaletteIcon();
    void _LoadSceneIcon();
    void _LoadGroupIcon();


    QIcon _GetGameCapIcon() const;
    QIcon _GetVideoCapIcon() const;
    QIcon _GetDesktopCapIcon() const;
    QIcon _GetWindowCapIcon() const;
    QIcon _GetAudioInputCaptureIcon() const;
    QIcon _GetAudioOutputCaptureIcon() const;
    QIcon _GetAudioProcessCaptureIcon() const;
    QIcon _GetBrowserCaptureIcon() const;
    QIcon _GetImageSourceIcon() const;
    QIcon _GetImageSliderIcon() const;
    QIcon _GetMediaSourceIcon() const;
    QIcon _GetTextSourceIcon() const;
    QIcon _GetColorPaletteIcon() const;

    // 
    void _LoadRenameSceneIcon();
    void _LoadHotkeyConflictIcon();

#pragma endregion private func

#pragma region private var
private:
    QIcon imageIcon;
    QIcon colorIcon;
    QIcon slideshowIcon;
    QIcon audioInputIcon;
    QIcon audioOutputIcon;
    QIcon desktopCapIcon;
    QIcon windowCapIcon;
    QIcon gameCapIcon;
    QIcon cameraIcon;
    QIcon textIcon;
    QIcon mediaIcon;
    QIcon browserIcon;
    QIcon groupIcon;
    QIcon sceneIcon;
    QIcon defaultIcon;
    QIcon audioProcessOutputIcon;

    QIcon hotkeyConflictIcon;

#pragma endregion private var
};