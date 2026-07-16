#ifndef STUDIODEFINE_H
#define STUDIODEFINE_H

#include <string>

enum class BuildMode {
    RELEASE,
    STAGING
};

struct StudioConfig {
    static bool isStaging;
    static std::string getPrefix() { return isStaging ? "" : "https://"; }
    static std::string getFolder() { return isStaging ? "" : "SOOPStudio"; }
};

#define URL_PREFIX           (StudioConfig::getPrefix())
#define LOCAL_FOLDER_NAME    (StudioConfig::getFolder())
#define SOOPLIVE_DOMAIN      std::string("sooplive.com")
#define SOOPLIVE_DOMAIN_STR  "sooplive.com"
#define URL_HTTPS            "https://"

#define LOG_URL             ""
#define LOG_DATA            ""
#define HASH_FORMAT         ""
#define UPDATE_SERVER_URL   ""
#define GETSERVTIME_URL     ""

// --- SOOP Service URL ---
#define SOOPLIVE_SET_KR_URL         ""
#define SOOPLIVE_KR_URL             ""
#define SOOP_SERVICE_NOTICE_URL     ""
#define SOOP_FREECSHOT_COMMNET_URL  ""
#define SOOP_STREAMER_SUPPORT_URL   ""
#define SOOP_STUDIO_UPDATE_LOG_PAGE ""

// --- OpenAPI & Auth ---
#define SOOP_AUTH_URL           ""
#define SOOP_TOKEN_URL          ""
#define SOOP_USER_INFO_URL      ""
#define SOOP_STREAMKEY_URL      ""
#define SOOP_OPENAPI_CF_URL     ""

//
#define SOOP_DASHBOARD_URL          ""
#define SOOP_CHANNEL_URL            ""
#define SOOP_CHAT_URL               ""
#define SOOP_SUBTITLE               ""
#define SOOP_DASHBOARD_INFO_URL     ""
#define SOOP_DASHBOARD_API_URL      ""
#define SOOP_CATEGORY_URL           ""
#define REFRESH_COOKIE_URL          ""
#define SOOP_DASHBOARD_OVERLAY_URL  ""

//
#define SOOPLIVE_KR_LOGIN           ""
#define SOOPLIVE_KR_LOGIN_GLOBAL    ""
#define SOOP_BROAD_STATISTICS_URL   ""

// 
#define SOOP_SIGNATURE_STATUS_URL   ""
#define SOOP_SIGNATURE_PC_URL       ""
#define SOOP_SIGNATURE_MOBILE_URL   ""
#define SOOP_PROFILE_URL            ""
#define SOOP_AQUA_URL               ""

//
#define URL_MISSION_MAIN                ""
#define URL_STUDIO_VOTETOOL             ""
#define URL_SAVVY_REACTION              ""
#define URL_EXTENSION_LIST              ""
#define URL_AQUA_REMOTE_CONTROL         ""
#define URL_AQUA_REMOTE_CONTROL_FORMAT  ""
#define URL_SAVVY_SIGNATURE_IMAGE       ""
#define TAG_SETTING_URL                 ""
#define ANIMATION_LIST_URL              ""
#define CATEGORY_SETTING_URL            ""
#define URL_STUDIO_STICKER_POPUP        ""
#define SIGNATURE_UPLOAD_PAGE           ""
#define MOBILE_ALARM_USE_PAGE           ""
#define MOBILE_ALARM_SEND_PAGE          ""

//
#define SOOP_FIND_SECURITY      ""
#define SOOP_SECONDLOGIN        ""
#define SOOP_MINOR_CHECK        ""
#define SOOP_LOGIN_BLOCK        ""
#define SOOP_BLACK_CLEAR        ""
#define REALNAME_AUTH_POPUP_URL ""
#define NEED_CERTIFY_POPUP_URL  ""
#define EMAIL_CERTIFY_POPUP_URL ""
#define ADMIN_BLACK_POPUP_URL   ""
#define SOOP_DOMESTIC_DORMANT   ""
#define VIDEOBALLON_LINK_URL    ""
#define MEMBERSHIP_PAGE_URL     ""
#define LINK_FIND_PHP           ""
#define LINK_JOIN               ""
#define LINK_EVENT              ""
#define GET_DIRECTBROAD_INFO    ""
#define LOGO_URL                ""
#define SOOP_KR_HELP_URL        ""

#define LINK_SECOND_PASSWORD    ""
#define LINK_PASSWORD_CHANGE    ""
#define LINK_LOGIN_BLOCK        ""
#define LINK_MINOR_CHECK            ""
#define LINK_FOREIGN_BLOCK_CHECK    ""

// --- 권한 및 관리 ---
#define SOOP_PERMISSION_URL         ""
#define SOOP_FANCLUB_URL            ""
#define SOOP_SUBSCRIPTION_URL       ""
#define SOOP_BLACKLIST_URL          ""
#define SOOP_VODAUTH_URL            ""
#define SOOP_EDITOR_URL             ""
#define SOOP_USERCLIP_BLACKLIST_URL ""
#define SOOP_SUBSCRIBE_SETTING_URL  ""

#define SOOP_AIMANAGER_URL          ""

#define SOOP_RTMP_URL               ""

//TWITCH
#define TWITCH_URL					"https://www.twitch.tv/"
#define TWITCH_AUTH_URL				"https://id.twitch.tv/oauth2/authorize"
#define TWITCH_REDIRECT_URL			"http://localhost"
#define TWITCH_TOKEN_URL			"https://id.twitch.tv/oauth2/token"
#define TWITCH_DASHBOARD_URL		"https://dashboard.twitch.tv/u/" // ex) https://dashboard.twitch.tv/u/twitchid/home
#define TWITCH_POPUP_URL			"https://www.twitch.tv/popout/"


/* ------------------------------------------------------------------------- */
#define TWITCH_CLIENTID				""
#define TWITCH_CLIENT_SECRET        ""
#define YOUTUBE_API_TOKEN			""
//
#define TWITCH_HASH					0x0
#define TWITCH_SCOPE_VERSION		1


#define TWITCH_RTM_URL				"rtmp://live.twitch.tv/app"


#define TWITCH_CHAT_DOCK_NAME		"twitchChat"
#define TWITCH_INFO_DOCK_NAME		"twitchInfo"
#define TWITCH_STATS_DOCK_NAME		"twitchStats"
#define TWITCH_FEED_DOCK_NAME		"twitchFeed"

// YOUTUBE
#define YOUTUBE_AUTH_URL "https://accounts.google.com/o/oauth2/v2/auth"
#define YOUTUBE_URL "https://www.youtube.com/"
#define YOUTUBE_CHAT_POPOUT_URL \
	"https://www.youtube.com/live_chat?is_popout=1&dark_theme=1&v="

/* ------------------------------------------------------------------------- */
#define YOUTUBE_RTMP_URL	"rtmps://a.rtmps.youtube.com:443/live2"
#define YOUTUBE_LIVE_API_URL "https://www.googleapis.com/youtube/v3"
#define YOUTUBE_BLANK_CHAT "https://dashboard." SOOPLIVE_DOMAIN_STR "/page/settings/studio/chat/external_channel"

#define YOUTUBE_SEARCH_API  "https://youtube.googleapis.com/youtube/v3/search?part=snippet&type=video&maxResult=1&q=%1&key=%2"
#define YOUTUBE_VIDEO_URL	"https://www.youtube.com/watch?v=%1"

#define YOUTUBE_LIVE_STREAM_URL YOUTUBE_LIVE_API_URL "/liveStreams"
#define YOUTUBE_LIVE_BROADCAST_URL YOUTUBE_LIVE_API_URL "/liveBroadcasts"
#define YOUTUBE_LIVE_BROADCAST_TRANSITION_URL \
	YOUTUBE_LIVE_BROADCAST_URL "/transition"
#define YOUTUBE_LIVE_BROADCAST_BIND_URL YOUTUBE_LIVE_BROADCAST_URL "/bind"

#define YOUTUBE_LIVE_CHANNEL_URL YOUTUBE_LIVE_API_URL           "/channels"
#define YOUTUBE_LIVE_TOKEN_URL                                  "https://oauth2.googleapis.com/token"
#define YOUTUBE_LIVE_VIDEOCATEGORIES_URL YOUTUBE_LIVE_API_URL   "/videoCategories"
#define YOUTUBE_LIVE_VIDEOS_URL YOUTUBE_LIVE_API_URL            "/videos"
#define YOUTUBE_LIVE_CHAT_MESSAGES_URL YOUTUBE_LIVE_API_URL     "/liveChat/messages"
#define YOUTUBE_LIVE_THUMBNAIL_URL                              "https://www.googleapis.com/upload/youtube/v3/thumbnails/set"

#define DEFAULT_BROADCASTS_PER_QUERY \
	"50" // acceptable values are 0 to 50, inclusive

// --- SARSA (AI) ---
#define SARSA_URL                   ""
#define SARSA_TOKEN_API             ""
#define SARSA_SOCKET_DOMAIN         ""
#define SARSA_SOCKET_NSP            ""
#define SARSA_AUDIO_NSP             ""
#define SARSA_STREAMER_START_NSP    ""
#define SARSA_DEV_SOCKET_DOMAIN     ""

#endif // STUDIODEFINE_H