#ifndef AFQAUTOCONFIGWIZARD_H
#define AFQAUTOCONFIGWIZARD_H

#include <QWidget>
#include <QWizard>

class AFQAutoConfigWizard : public QWizard
{
    Q_OBJECT

	friend class AFQAutoConfigStartPage;
	friend class AFQAutoConfigVideoPage;
	friend class AFQAutoConfigStreamPage;
	friend class AFQAutoConfigTestPage;

	enum class Type {
		Invalid,
		Streaming,
		Recording,
		VirtualCam,
	};

	enum class Service {
		Twitch,
		YouTube,
		Other,
	};

	enum class Encoder {
		x264,
		NVENC,
		QSV,
		AMD,
		Apple,
		Stream,
	};

	enum class Quality {
		Stream,
		High,
	};

	enum class FPSType : int {
		PreferHighFPS,
		PreferHighRes,
		UseCurrent,
		fps30,
		fps60,
	};

	//static inline const char* GetEncoderId(Encoder enc);

	//AutoConfigStreamPage* streamPage = nullptr;

	Service service = Service::Other;
	Quality recordingQuality = Quality::Stream;
	Encoder recordingEncoder = Encoder::Stream;
	Encoder streamingEncoder = Encoder::x264;
	Type type = Type::Streaming;
	FPSType fpsType = FPSType::PreferHighFPS;
	int idealBitrate = 2500;
	int baseResolutionCX = 1920;
	int baseResolutionCY = 1080;
	int idealResolutionCX = 1280;
	int idealResolutionCY = 720;
	int idealFPSNum = 60;
	int idealFPSDen = 1;
	std::string serviceName;
	std::string serverName;
	std::string server;
	std::string key;

	bool hardwareEncodingAvailable = false;
	bool nvencAvailable = false;
	bool qsvAvailable = false;
	bool vceAvailable = false;
	bool appleAvailable = false;

	int startingBitrate = 2500;
	bool customServer = false;
	bool bandwidthTest = false;
	bool testRegions = true;
	bool twitchAuto = false;
	bool regionUS = true;
	bool regionEU = true;
	bool regionAsia = true;
	bool regionOther = true;
	bool preferHighFPS = false;
	bool preferHardware = false;
	int specificFPSNum = 0;
	int specificFPSDen = 0;

	/*void TestHardwareEncoding();
	bool CanTestServer(const char* server);

	virtual void done(int result) override;

	void SaveStreamSettings();
	void SaveSettings();*/

public:
    explicit AFQAutoConfigWizard(QWidget *parent = nullptr);

	enum Page {
		StartPage,
		VideoPage,
		StreamPage,
		TestPage,
	};
};

#endif // AFQAUTOCONFIGWIZARD_H
