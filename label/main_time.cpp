// LabelingSample_time.cpp
#include <iostream>
#include <string>
#include <ctime>
#include <sys/time.h>
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
using namespace cv;

// 適当な色を出す用
RNG rnd(1192);
Scalar randomColor()
{
	return Scalar(rnd.next() & 0xFF, rnd.next() & 0xFF, rnd.next() & 0xFF);
}

void print_elapsed_time(struct timeval *s, struct timeval *e)
{
  long sec, usec;

  sec  = e->tv_sec - s->tv_sec;
  usec = e->tv_usec - s->tv_usec;
  if (usec < 0) {
    sec--; usec += 1000000;
  }
  cout << "elapsed time = " << sec <<"."<< usec <<" sec." << endl;

}

int main(int argc, char* argv[])
{
  struct timeval s_time, e_time;

  // 入力画像を読み込み

  if(argc < 2){
    cerr << "usage: " << argv[0] << "image_file_path" << endl;
    exit(0);
  }
  
  gettimeofday(&s_time, NULL);

  for(int l=1; l<argc; l++){
	Mat image = imread(argv[l]);
	string result_name;
	
	// グレイスケール化(3チャンネル→1チャンネル化)
	Mat grayimg;
	cvtColor(image, grayimg, CV_BGR2GRAY);

	// 二値化
	Mat binimg;
	threshold(grayimg, binimg, 0, 255, CV_THRESH_BINARY);

	// Labelingの結果を受け取る
	Mat label(image.size(), CV_16SC1);

	// ラベリングを実施 ２値化した画像に対して実行する。
	LabelingBS	labeling;
	labeling.Exec(binimg.data, (short *)label.data, image.cols, image.rows, false, 0);	
	
	// ラベリング結果を出力する、黒で初期化
	Mat outimg(image.size(), CV_8UC3, Scalar(0, 0, 0));
	
	// ラベルされた領域をひとつずつ描画
	for( int i = 0; i < labeling.GetNumOfRegions(); i++)
	{
		// ラベリング結果の領域を抽出する。
		Mat labelarea;
		compare(label, i + 1, labelarea, CV_CMP_EQ);
		// 抽出した領域にランダムな色を設定して出力画像に追加。
		Mat color(image.size(), CV_8UC3, randomColor());
  		color.copyTo(outimg, labelarea);
	}
	// 入力画像とラベリング結果の画像を画面表示
	//imshow("label", outimg);
	//waitKey(0);

	// ラベルされた出力画像を保存
	for(int j=0; j < strlen(argv[l]); j++){
	  if(argv[l][j]=='.'){
	    result_name += "_result";
	  }
	  result_name += argv[l][j];
	}
	cout << result_name << endl;
	imwrite(result_name, outimg);
  }
  gettimeofday(&e_time, NULL);
  
  print_elapsed_time(&s_time, &e_time);

  return 0;
}
