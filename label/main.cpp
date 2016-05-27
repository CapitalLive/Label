// LabelingSample.cpp
#include <iostream>
#include <string>
#include "Labeling.h"
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>

#ifdef _DEBUG
#pragma comment(lib,"opencv_highgui242d.lib")
#pragma comment(lib,"opencv_imgproc242d.lib")
#pragma comment(lib,"opencv_core242d.lib")
#else
#pragma comment(lib,"opencv_highgui242.lib")
#pragma comment(lib,"opencv_imgproc242.lib")
#pragma comment(lib,"opencv_core242.lib")
#endif
using namespace std;

// 適当な色を出す用
cv::RNG rnd(1192);
cv::Scalar randomColor()
{
	return cv::Scalar(rnd.next() & 0xFF, rnd.next() & 0xFF, rnd.next() & 0xFF);
}

int main(int argc, char* argv[])
{
	// 入力画像を読み込み

  if(argc < 2){
    cerr << "usage: " << argv[0] << "image_file_path" << endl;
    exit(0);
  }
  
  for(int l=1; l<argc; l++){
	cv::Mat image = cv::imread(argv[l]);
	string result_name;
	
	// グレイスケール化(3チャンネル→1チャンネル化)
	cv::Mat grayimg;
	cv::cvtColor(image, grayimg, CV_BGR2GRAY);

	// 二値化
	cv::Mat binimg;
	cv::threshold(grayimg, binimg, 0, 255, CV_THRESH_BINARY);

	// Labelingの結果を受け取る
	cv::Mat label(image.size(), CV_16SC1);

	// ラベリングを実施 ２値化した画像に対して実行する。
	LabelingBS	labeling;
	labeling.Exec(binimg.data, (short *)label.data, image.cols, image.rows, false, 0);	
	
	// ラベリング結果を出力する、黒で初期化
	cv::Mat outimg(image.size(), CV_8UC3, cv::Scalar(0, 0, 0));
	
	// ラベルされた領域をひとつずつ描画
	for( int i = 0; i < labeling.GetNumOfRegions(); i++)
	{
		// ラベリング結果の領域を抽出する。
		cv::Mat labelarea;
		cv::compare(label, i + 1, labelarea, CV_CMP_EQ);
		// 抽出した領域にランダムな色を設定して出力画像に追加。
		cv::Mat color(image.size(), CV_8UC3, randomColor());
  		color.copyTo(outimg, labelarea);
	}
	// 入力画像とラベリング結果の画像を画面表示
	cv::imshow("label", outimg);
	cv::waitKey(0);

	// ラベルされた出力画像を保存
	for(int j=0; j < strlen(argv[l]); j++){
	  if(argv[l][j]=='.'){
	    result_name += "_result";
	  }
	  result_name += argv[l][j];
	}
	cout << result_name << endl;
	cv::imwrite(result_name, outimg);
  }
	return 0;
}
