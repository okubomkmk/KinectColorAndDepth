#include <iostream>
#include <sstream>
#include <map>

#include <Kinect.h>
#include <opencv2\opencv.hpp>
#include "C:\Users\mkuser\Documents\K4W2-Book-master\K4W2-Book-master\C++(Native)\02_Depth\KinectV2-Depth-01\KinectV2\ForEnglish.h"
#include <atlbase.h>
using namespace std;
// 次のように使います
// ERROR_CHECK( ::GetDefaultKinectSensor( &kinect ) );
// 書籍での解説のためにマクロにしています。実際には展開した形で使うことを検討してください。
#define ERROR_CHECK( ret )  \
    if ( (ret) != S_OK ) {    \
        std::stringstream ss;	\
        ss << "failed " #ret " " << std::hex << ret << std::endl;			\
        throw std::runtime_error( ss.str().c_str() );			\
    }

class KinectApp
{
private:

    // Kinect SDK
    CComPtr<IKinectSensor> kinect = nullptr;

    CComPtr<IColorFrameReader> colorFrameReader = nullptr;

    int colorWidth;
    int colorHeight;
    unsigned int colorBytesPerPixel;

    ColorImageFormat colorFormat = ColorImageFormat::ColorImageFormat_Bgra;

    // 表示部分
    std::vector<BYTE> colorBuffer;
	std::vector<std::vector<BYTE>> colorFrameSave;

	CComPtr<IDepthFrameReader> depthFrameReader = nullptr;

	// 表示部分
	int depthWidth;
	int depthHeight;

	int depthPointX;
	int depthPointY;

	int filenamecounter = 0;
	string fileName;

	bool screenshot = false;
	bool recording = false;
	int frameCounter = 0;

	vector<UINT16> depthBuffer;
	vector<UINT16> revercedBuffer;
	vector<UINT16> PreviousFrame;
	vector<vector<UINT16>> FrameBuffer;
	const char* DepthWindowName = "Depth Image";
	double fps = 30;

	double dWidth = 1920;
	double dHeight = 1080;


	//CvVideoWriter* VideoWriter = cvCreateVideoWriter("avifile.avi", -1, fps, cvSize(dWidth, dHeight), 1);

public:

    ~KinectApp()
    {
        // Kinectの動作を終了する
        if ( kinect != nullptr ){
            kinect->Close();
        }
    }

    // 初期化
    void initialize()
    {
        // デフォルトのKinectを取得する
        ERROR_CHECK( ::GetDefaultKinectSensor( &kinect ) );
        ERROR_CHECK( kinect->Open() );

        // カラーリーダーを取得する
        CComPtr<IColorFrameSource> colorFrameSource;
        ERROR_CHECK( kinect->get_ColorFrameSource( &colorFrameSource ) );
        ERROR_CHECK( colorFrameSource->OpenReader( &colorFrameReader ) );

        // デフォルトのカラー画像のサイズを取得する
        CComPtr<IFrameDescription> defaultColorFrameDescription;
        ERROR_CHECK( colorFrameSource->get_FrameDescription( &defaultColorFrameDescription ) );
        ERROR_CHECK( defaultColorFrameDescription->get_Width( &colorWidth ) );
        ERROR_CHECK( defaultColorFrameDescription->get_Height( &colorHeight ) );
        ERROR_CHECK( defaultColorFrameDescription->get_BytesPerPixel( &colorBytesPerPixel ) );
        std::cout << "default : " << colorWidth << ", " << colorHeight << ", " << colorBytesPerPixel << std::endl;

        // カラー画像のサイズを取得する
        CComPtr<IFrameDescription> colorFrameDescription;
        ERROR_CHECK( colorFrameSource->CreateFrameDescription(
            colorFormat, &colorFrameDescription ) );
        ERROR_CHECK( colorFrameDescription->get_Width( &colorWidth ) );
        ERROR_CHECK( colorFrameDescription->get_Height( &colorHeight ) );
        ERROR_CHECK( colorFrameDescription->get_BytesPerPixel( &colorBytesPerPixel ) );
        std::cout << "create  : " << colorWidth << ", " << colorHeight << ", " << colorBytesPerPixel << std::endl;

        // バッファーを作成する
        colorBuffer.resize( colorWidth * colorHeight * colorBytesPerPixel );
		colorFrameSave.resize(GETFRAMENUMBER);

		for (int i = 0; i < GETFRAMENUMBER; i++){
			colorFrameSave[i].resize(colorWidth * colorHeight * colorBytesPerPixel);
		}
		// Depthリーダーを取得する
		CComPtr<IDepthFrameSource> depthFrameSource;
		ERROR_CHECK(kinect->get_DepthFrameSource(&depthFrameSource));
		ERROR_CHECK(depthFrameSource->OpenReader(&depthFrameReader));

		// Depth画像のサイズを取得する
		CComPtr<IFrameDescription> depthFrameDescription;
		ERROR_CHECK(depthFrameSource->get_FrameDescription(
			&depthFrameDescription));
		ERROR_CHECK(depthFrameDescription->get_Width(&depthWidth));
		ERROR_CHECK(depthFrameDescription->get_Height(&depthHeight));

		depthPointX = depthWidth / 2;
		depthPointY = depthHeight / 2;

		// Depthの最大値、最小値を取得する
		UINT16 minDepthReliableDistance;
		UINT16 maxDepthReliableDistance;
		ERROR_CHECK(depthFrameSource->get_DepthMinReliableDistance(
			&minDepthReliableDistance));
		ERROR_CHECK(depthFrameSource->get_DepthMaxReliableDistance(
			&maxDepthReliableDistance));
		std::cout << "Depth最小値       : " << minDepthReliableDistance << endl;
		cout << "Depth最大値       : " << maxDepthReliableDistance << endl;

		// バッファーを作成する
		depthBuffer.resize(depthWidth * depthHeight);
		FrameBuffer.resize(GETFRAMENUMBER);
		for (int i = 0; i < GETFRAMENUMBER; i++)
		{
			FrameBuffer[i].resize(depthWidth * depthHeight);
		}
		
		//マウス関連
		cv::namedWindow(DepthWindowName);
		cv::setMouseCallback(DepthWindowName, &KinectApp::mouseCallback, this);

    }

	static void mouseCallback(int event, int x, int y, int flags, void* userdata)
	{
		// 引数に渡したthisポインタを経由してメンバ関数に渡す
		auto pThis = (KinectApp*)userdata;
		pThis->mouseCallback(event, x, y, flags);
	}

	// マウスイベントのコールバック(実処理)
	void mouseCallback(int event, int x, int y, int flags)
	{
		if (event == CV_EVENT_LBUTTONDOWN) {
			depthPointX = x;
			depthPointY = y;
		}
	}

    void run()
    {
		while (1) {
			update();
			
			//draw();

			auto key = cv::waitKey(10);
			if (key == 'q'){
				break;
			}
			if (key == 's'){
				screenshot = true;
			}
			if (key == 'r'){
				recording = true;
				cout << "recording\n";
			}
			if (screenshot){
				cout << "save start\n";
				string Filename = "V:\\EnglishPaperPresentation\\Mapper\\colorOriginal";
				for (int i = 0; i < GETFRAMENUMBER; i++){
					cv::imwrite(Filename + to_string(i) + ".bmp", colorFrameSave[i]);
					cout << "frame num " << i << "\n";
				}
				cout << "save end \n";
				screenshot = false;
				frameCounter = 0;
			}
        }
    }



private:

    // データの更新処理
    void update()
    {
        updateColorFrame();
		//updateDepthFrame();
    }

    // カラーフレームの更新
    void updateColorFrame()
    {
        // フレームを取得する
        CComPtr<IColorFrame> colorFrame;
        auto ret = colorFrameReader->AcquireLatestFrame( &colorFrame );
        if ( FAILED( ret ) ){
            return;
        }
		if (!recording){
			ERROR_CHECK(colorFrame->CopyConvertedFrameDataToArray(
				colorBuffer.size(), &colorBuffer[0], colorFormat));
		}

		else{
			ERROR_CHECK(colorFrame->CopyConvertedFrameDataToArray(
				colorFrameSave[frameCounter].size(), &colorFrameSave[frameCounter][0], colorFormat));
			frameCounter++;
			if (frameCounter == GETFRAMENUMBER - 1){
				recording = false;
				cout << "record end\n";
			}
		}
        // 指定の形式でデータを取得する

    }
	void updateDepthFrame()
	{
		// Depthフレームを取得
		CComPtr<IDepthFrame> depthFrame;
		auto ret = depthFrameReader->AcquireLatestFrame(&depthFrame);
		if (ret != S_OK){
			return;
		}

		// データを取得する
		ERROR_CHECK(depthFrame->CopyFrameDataToArray(
			depthBuffer.size(), &depthBuffer[0]));

		for (int i = 0; i < depthWidth * depthHeight; i++)
		{
			FrameBuffer[GETFRAMENUMBER - 1][i] = depthBuffer[i];
		}

	}
	
	void draw()
	{
		drawColorFrame();
		drawDepthFrame();
	}

	void drawDepthFrame()
	{
		// Depthデータを表示するかコレ?
		cv::Mat depthImage(depthHeight, depthWidth, CV_8UC1);
		// フィルタ後
		// Depthデータを0-255のグレーデータにする


		for (int i = 0; i < depthImage.total(); ++i){
			depthImage.data[i] = depthBuffer[i] * 255 / 8000;
		}


		// Depthデータのインデックスを取得して、その場所の距離を表示する
		int index = (depthPointY * depthWidth) + depthPointX;
		std::stringstream ss;
		ss << depthBuffer[index] << "mm X=" << depthPointX << " Y= " << depthPointY << " " <<to_string(filenamecounter);

		cv::circle(depthImage, cv::Point(depthPointX, depthPointY), 3,
			cv::Scalar(255, 255, 255), 2);
		cv::putText(depthImage, ss.str(), cv::Point(depthPointX, depthPointY),
			0, 0.5, cv::Scalar(255, 255, 255));


		//cv::imshow(DepthWindowName, depthImage);
	}
    // データの表示処理


    // カラーデータの表示処理
    void drawColorFrame()
    {

        cv::Mat colorImage( colorHeight, colorWidth, CV_8UC4, &colorBuffer[0] );
        cv::Mat harfImage;
        cv::resize( colorImage, harfImage, cv::Size(), 0.5, 0.5 );
        //cv::imshow( "Harf Image", harfImage );
		/*
		if (filenamecounter < 30){
			saveFile(harfImage,filenamecounter,"ColorData");
			filenamecounter++;
		}
		*/



    }
};

void main()
{
    try {
        KinectApp app;
        app.initialize();
        app.run();
    }
    catch ( std::exception& ex ){
        std::cout << ex.what() << std::endl;
    }


}
