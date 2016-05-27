#include <iostream>
#include <string>
#include <cstdio> // sprintf
#include <vector>
#include <cstdlib>
#include <opencv2/opencv.hpp>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
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

#define PORT (10000)

using namespace std;
using namespace cv;

// 適当な色を出す用
RNG rnd(1192);
Scalar randomColor()
{
  return Scalar(rnd.next() & 0xFF, rnd.next() & 0xFF, rnd.next() & 0xFF);
}

void my_labeling(Mat& image)
{
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
  cout << "ラベリングが正常に完了しました" << endl;

  outimg.copyTo(image);
}


int main(int argc, char *argv[]){
  int srcSocket;	// 自分
  int dstSocket;	// 相手
  long unsigned int data_buf_size;
  unsigned int count_faces;
  const unsigned int ms_buf_size = sizeof(data_buf_size);
  const unsigned int cf_buf_size = sizeof(count_faces);
  struct sockaddr_in srcAddr;
  struct sockaddr_in dstAddr;
  int dstAddrSize = sizeof(dstAddr);
  char ms_buf[ms_buf_size];
  char* ds_buf;
  char cf_buf[cf_buf_size];
  const unsigned int TRANSPORT_NUM = 1000;

  vector<uchar> img_buff;
  Mat image;
  CascadeClassifier cascade;
  // カスケードファイル読み込み
  cascade.load("../cascades/haarcascade_frontalface_alt.xml");
	
  memset((char*)&srcAddr, 0, sizeof(srcAddr));
  srcAddr.sin_family = AF_INET;
  srcAddr.sin_addr.s_addr = INADDR_ANY;
  srcAddr.sin_port = htons(PORT);


  // ソケットの生成
  if( (srcSocket = socket(AF_INET, SOCK_STREAM, 0)) <0 ){
    cerr << "socket";	exit(1);
  }
  // ソケットのバインド
  if( bind(srcSocket, (struct sockaddr*)&srcAddr, sizeof(srcAddr)) <0 ){
    cerr << "bind";		exit(1);
  }
  // 接続の許可
  if( listen(srcSocket, 1) <0 ){
    cerr << "listen";	exit(1);
  }
	

  //接続の受付
  cout << "接続を待っています" << endl;
  dstSocket = accept(srcSocket, (struct sockaddr *) &dstAddr, (socklen_t *) &dstAddrSize);
  cout << inet_ntoa(dstAddr.sin_addr) << "から接続を受けました" << endl;
  while(1){
    //INIT
    ds_buf = NULL;

    //受信するデータのデータサイズを受信
    


    if( recv(dstSocket, ms_buf, ms_buf_size, 0) <=0 ){ break; }
    cout << "画像サイズを受信しました : data_buf_size is " << ms_buf << endl;
    data_buf_size = (long unsigned int)atoi(ms_buf);
    cerr << data_buf_size << endl;
    //受信するデータの領域を確保
    ds_buf = (char *)malloc(data_buf_size);
    if(ds_buf == NULL){ cerr << "ds_bufのメモリが確保できません" << endl; exit(1); }
    //データの受信
    //if( recv(dstSocket, ds_buf, data_buf_size, 0) <0 ){ break; }
    // recv_size : ちゃんと受け取れたサイズ
    for(int recv_size=0; recv_size!=data_buf_size;){
      if(data_buf_size < recv_size + TRANSPORT_NUM){
	recv_size += recv(dstSocket, ds_buf, data_buf_size - recv_size, 0);
      }
      else{
	recv_size += recv(dstSocket, ds_buf, TRANSPORT_NUM, 0);
      }     
    }
    for(int i=0; i<data_buf_size; i++){
      img_buff.push_back((uchar)ds_buf[i]);
    }
    cout << "画像を受け取りました。デコードします" << endl;

    image = imdecode(Mat(img_buff), CV_LOAD_IMAGE_COLOR);
    cout << "ラベリングを行います" << endl;
    my_labeling(image);
		       
		        
    free(ds_buf);
    img_buff.clear();


    imshow("debug", image);
    waitKey(0);

    //返す
    //vector<int> param = vector<int>(2);
    //param[0] = CV_IMWRITE_PNG_COMPRESSION;
    // param[1] = 9;

    // pngに圧縮
    //imencode(".png", image, img_buff, param);
    vector<uchar> img_buff(image.rows*image.cols);
    if(image.isContinuous())
      for(int j=0;j < image.rows*image.cols;j++){
      img_buff[j] = image.data[j];
    }
    // 送信するデータのサイズを取得
    data_buf_size = img_buff.size();
    // データのサイズをchar型に変換
    sprintf(ms_buf, "%lu", data_buf_size);
    if( send(dstSocket, ms_buf, ms_buf_size, 0) <0 ) { break; }
    cout << "画像サイズを送信しました : data_buf_size is " << ms_buf << " byte" << endl;
    // 送信するデータの領域を動的確保
    ds_buf = (char *)malloc(data_buf_size);
    if(ds_buf == NULL){ cerr << "ds_bufのメモリが確保できません" << endl; exit(1); }
    // char型の１次元配列に変換	
    for(int j=0; j<data_buf_size; j++) ds_buf[j] = img_buff[j];
    if( send(dstSocket, ds_buf, data_buf_size, 0) <0 ) { break; }
    cout << "画像を送りました" << endl;
			
    free(ds_buf);
    img_buff.clear();
  }
  close(dstSocket);
  close(srcSocket);
  return 0;
}
