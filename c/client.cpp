#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include<string.h>
#include<pthread.h>

#include <unistd.h>
#include <stdlib.h>
#include <opencv2/opencv.hpp>

#include <arpa/inet.h>

using namespace std;
using namespace cv;

//messege libraries: serialtion libraries, json 


void *client_thread();
string type2str(int type);
int sockfd,n;
struct sockaddr_in servaddr;




int main(int argc,char **argv)
{
	printf("Input argurments: \n"); 
	for (int i=0; i<argc;i++)
	{
		printf("%s \n",argv[i]);
	}
	printf("End of arguments. \n\n");
	
    char sendline[100];
    char recvline[100];
    
 
    sockfd=socket(AF_INET,SOCK_STREAM,0);
    bzero(&servaddr,sizeof servaddr);
 

    int socketNum;
    sscanf(argv[2],"%d",&socketNum);
    printf("Socket number: %d\n\n",socketNum);

    servaddr.sin_family=AF_INET;
    servaddr.sin_port=htons(socketNum);
 
    inet_pton(AF_INET,"127.0.0.1",&(servaddr.sin_addr));
    
	
	//printf("Creating Threads...\n");
	//pthread_t * thread=malloc(sizeof(pthread_t)*100);
/*
	for(int i=0;i<2;i++)
	{
			pthread_create(&thread[i],NULL,client_thread,NULL);
	}

*/

    printf("Client Started \n");
    connect(sockfd,(struct sockaddr *)&servaddr,sizeof(servaddr));
    printf("Client connected \n\n");


printf("Loading image... \n");
char* imageName = argv[1];
printf("image path: %s\n",argv[1]);
Mat image;
image = imread( imageName, 1 );
Mat displayImage=image;

if( argc != 3 || !image.data )
{
  printf( " No image data \n " );
  return -1;
}

int cols=image.cols;
int rows=image.rows;
int imgSize=image.total()*image.elemSize();
printf("Image Data... \n");
printf("Type: %s\n",type2str(image.type()).c_str());
printf("Cols: %d\n",cols);
printf("Rows: %d\n",rows); 
printf("Image Size: %d\n",imgSize);
printf("\n");

printf("Sending Image Data...\n");
char imageInfo[100];
sprintf(imageInfo,"%d\n%d\n%d\n%d\n",image.type(),cols,rows,imgSize);
printf("%s",imageInfo);
write(sockfd,imageInfo,strlen(imageInfo)+1);
printf("Image Data Sent\n\n");


//colored image format bgr,bgr,bgr,bgr... (3vec per pixel)
printf("Sending colored image... \n");
image=(image.reshape(0,1));
int bytes=send(sockfd,image.data,imgSize,0);
printf("Colored image sent...\n\n");


printf("Receiving image...\n");
imgSize=193380;
uchar sockData[imgSize];
bytes=0;
for (int i=0;i<imgSize; i+=bytes){
	if ((bytes = recv(sockfd, sockData +i, imgSize  - i, 0)) == -1)
		printf("recv failed \n");
}


//format of gray image is 1 g,g,g,g,g,g.... (1 vec per pixel
Mat gray_image=Mat::zeros(rows,cols,CV_8UC1);
int ptr=0;			
for(int i=0; i<gray_image.rows;i++){
	for(int j=0; j<gray_image.cols; j++){
		gray_image.at<uchar>(i,j) = (sockData[ptr+ 0]);
		ptr=ptr+1;
	}
}
printf("Image recieved \n\n");





printf("Displaying colored image... \n");
namedWindow( imageName, WINDOW_AUTOSIZE );
imshow( imageName, displayImage );
printf("Image Displayed \n\n");



printf("Displaying greyscale image...\n");      		
namedWindow( "Gray image", WINDOW_AUTOSIZE );
imshow( "Gray image", gray_image );
printf("Image Displayed \n\n");


waitKey(0);



//std::cout<<image<<std::endl;



 //   while(1)
  //  {
//        bzero( sendline, 100);
  //      bzero( recvline, 100);
    //    fgets(sendline,100,stdin); /*stdin = 0 , for standard input */
	/*
 	strcpy(sendline,"Connection Established, send kill signal \n");
        write(sockfd,sendline,strlen(sendline)+1);
	printf("Line sent \n");
	printf("Line recieved:");
        read(sockfd,recvline,100);
        printf("%s",recvline);	
	printf("Terminating... \n");
*/	

	
   // }


}





void *client_thread()
{
    char sendline[100];
    char recvline[100];
    printf("Client Started \n");
    connect(sockfd,(struct sockaddr *)&servaddr,sizeof(servaddr));
    printf("Client connected \n");
    bzero( sendline, 100);
    bzero( recvline, 100);
    strcpy(sendline,"Connection Established, send kill signal \n");
    write(sockfd,sendline,strlen(sendline)+1);
	printf("Line sent \n");
	printf("Line recieved:");
        read(sockfd,recvline,100);
        printf("%s",recvline);	
	printf("Terminating... \n");

}






string type2str(int type) {
  string r;

  uchar depth = type & CV_MAT_DEPTH_MASK;
  uchar chans = 1 + (type >> CV_CN_SHIFT);

  switch ( depth ) {
    case CV_8U:  r = "8U"; break;
    case CV_8S:  r = "8S"; break;
    case CV_16U: r = "16U"; break;
    case CV_16S: r = "16S"; break;
    case CV_32S: r = "32S"; break;
    case CV_32F: r = "32F"; break;
    case CV_64F: r = "64F"; break;
    default:     r = "User"; break;
  }

  r += "C";
  r += (chans+'0');

  return r;
}



