#include <iostream>
#include <string>
#include <sstream>
#include <cstdio> // sprintf
#include <vector>
#include <cstdlib>
#include <opencv2/opencv.hpp>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <netdb.h>

#define PORT (10000)

using namespace std;
using namespace cv;

int main(int argc, char* argv[]){
  int srcSocket;	// 自分
  int dstSocket;	// 相手
  unsigned long int data_buf_size;		// 転送画像のサイズ
  // struct sockaddr_in srcAddr;
  struct hostent *hp;
  struct sockaddr_in dstAddr;
  const unsigned int ms_buf_size = sizeof(data_buf_size);

  char ms_buf[ms_buf_size]; // 画像のサイズ用のバッファ
  char* ds_buf;	// 画像用のバッファ
  // ラベリング画像数用のバッファ
  //vector<uchar> img_buff;
  string filename;
  stringstream ss;
  const unsigned int TRANSPORT_NUM = 1000;
	 
  Mat image;
  //vector<int> param = vector<int>(2);
  //param[0] = CV_IMWRITE_PNG_COMPRESSION;
  //param[1] = 9;


  if(argc < 3){
    cerr << "usage: " << argv[0] << "<Server_IP> <Image_path>" << endl;
    exit(1);
  }


  // sockaddr_in 構造体のセット
  bzero((char *)&dstAddr, sizeof(dstAddr));
  dstAddr.sin_family = AF_INET;
  dstAddr.sin_port = htons(PORT);

  hp = gethostbyname(argv[1]);
  bcopy(hp->h_addr, &dstAddr.sin_addr, hp->h_length);

  // ソケットの生成
  dstSocket = socket(AF_INET, SOCK_STREAM, 0);

  // 接続
  if (connect(dstSocket, (struct sockaddr *) &dstAddr, sizeof(dstAddr)) <0 ){
    cerr << argv[1] << "に接続できませんでした" << endl;
    exit(1);
  }
  cout << argv[1] << "に接続しました" << endl;

  for(int i=2; i<argc; i++){
    //init
    ds_buf = NULL;

    cout << argv[i] << "を読み込みます" << endl;
    image = imread(argv[i]);
    //imshow("debug", image);
    //waitKey(0);
    cout << argv[i] << "を読み込みました。送信します" << endl;
    // pngに圧縮
    //imencode(".png", image, img_buff, param);
    vector<uchar> img_buff(image.step*image.cols);
    if(image.isContinuous())
      for(int j=0;j < image.step*image.cols;j++){
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

    // ラベリングの結果を受信
    //if( recv(dstSocket, ds_buf, ds_buf_size, 0) <0 ){ break; }
    //imwrite()
    // 送信したデータの領域の解放
    free(ds_buf);
    img_buff.clear();


    //受け取る
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
    for(int recv_size=0; recv_size!=data_buf_size;){
      if(data_buf_size < recv_size + TRANSPORT_NUM){
	recv_size += recv(dstSocket, ds_buf, data_buf_size - recv_size, 0);
      }
      else{
	recv_size += recv(dstSocket, ds_buf, TRANSPORT_NUM, 0);
      }     
    }
    for(int k=0; k<data_buf_size; k++){
      img_buff.push_back((uchar)ds_buf[k]);
    }
    cout << "画像を受け取りました。デコードします" << endl;

    image = imdecode(Mat(img_buff), CV_LOAD_IMAGE_COLOR);
     
    ss.str(""); //バッファのクリア
    ss.clear(stringstream::goodbit); //ストリーム状態のクリア
    ss << "img_" << i-2 << "_result.png";
    filename =  ss.str();
	       
    imwrite(filename, image);
    cout << "出力が完了しました" << endl;
    free(ds_buf);
    img_buff.clear();

  }

  close(dstSocket);

  return 0;
}
