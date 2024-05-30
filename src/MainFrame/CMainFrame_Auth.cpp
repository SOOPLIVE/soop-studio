#include "CMainFrame.h"


#include <QPainterPath>


#include "Common/CURLMiscUtils.h"

#include "CoreModel/Auth/CAuthManager.h"

#include "ViewModel/Auth/YouTube/youtube-api-wrappers.hpp"

#include "PopupWindows/StreamChannelConfig/YouTube/CYouTubeSettingDialog.hpp"


bool AFMainFrame::ShowPopupPageYoutubeChannel(AFBasicAuth* pAFDataAuted)
{
    if (pAFDataAuted == nullptr)
        return false;
    
    
    AFAuth::Def rawAuthDef = { "YouTube - RTMP",
                                AFAuth::Type::OAuth_LinkedAccount,
                                true, true };
    
    std::shared_ptr<YoutubeApiWrappers> tmpApiYoutube = std::make_shared<YoutubeApiWrappers>(rawAuthDef);
    AFOAuth* rawAuth = reinterpret_cast<AFOAuth*>(tmpApiYoutube.get());
    if (rawAuth->ConnectAuthedAFBase(pAFDataAuted) == false)
        return false;
    
    
    AFQYouTubeSettingDialog modal(this, rawAuth, false);
    return modal.exec();
}

QPixmap* AFMainFrame::MakePixmapFromAuthData(AFBasicAuth* pAFDataAuted)
{
    if (pAFDataAuted->strPlatform == "")
        return nullptr;
    
    auto& authManager = AFAuthManager::GetSingletonInstance();
    
    AFChannelData tmpForMakeUrl;
    tmpForMakeUrl.pAuthData = pAFDataAuted;
    
    std::string urlIMG = authManager.CreateUrlProfileImg(&tmpForMakeUrl);
    std::string readBufferImgBuffer;

    bool resCURL = CURLWriteDataOutStringBuffer(urlIMG.c_str(), readBufferImgBuffer);
    
    if (resCURL)
    {
        QImage orgimage;
        orgimage.loadFromData((const unsigned char*)readBufferImgBuffer.c_str(),
                              readBufferImgBuffer.size());
        
        
        int minDimension = std::fmin(orgimage.width(), orgimage.height());
        int margin = minDimension * 0.1;
        int diameter = minDimension - 2 * margin;

        QImage circularImage(diameter, diameter, QImage::Format_ARGB32);
        circularImage.fill(Qt::transparent);
        
        QPainter painter(&circularImage);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setRenderHint(QPainter::SmoothPixmapTransform);
        
        QPainterPath path;
        path.addEllipse(0, 0, diameter, diameter);
        painter.setClipPath(path);

        int xOffset = (orgimage.width() - diameter) / 2;
        int yOffset = (orgimage.height() - diameter) / 2;
        painter.drawImage(-xOffset, -yOffset, orgimage);
        
        
        QPixmap pixmap = QPixmap::fromImage(circularImage);
        const uint scaledSize = 80;
        QPixmap* scaledPixmap = new QPixmap();
        *scaledPixmap = pixmap.scaled(scaledSize,
                                      scaledSize,
                                      Qt::IgnoreAspectRatio,
                                      Qt::SmoothTransformation);
        
        return scaledPixmap;
    }
    
    return nullptr;
}
