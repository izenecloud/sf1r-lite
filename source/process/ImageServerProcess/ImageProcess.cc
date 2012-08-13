#include "ImageProcess.h"
#include <string.h>
#include <stdio.h>
#include <iostream>
#include <exception>
#include <math.h>
#include <glog/logging.h>
#include <opencv/cv.h>
#include <opencv/highgui.h>
#include <opencv/cvaux.h>
#include <opencv/cxcore.h>

static RGB *gColorTable;
static int (*gDistTable)[256][256];
static double gMinDistance;
static char **gColorText;

//using namespace sf1r;
using namespace std;

bool getImageColor(IplImage* srcGray, IplImage* srcOrig, int* indexCollection, int count);

void getHistogram(unsigned char *imageData,int width,int height,int widthStep,int blockSize,int *histogram)
{
    int i=0,j=0;
    unsigned char *line=NULL;
    memset(histogram,0,sizeof(int)*blockSize);
    for(line=imageData,j=0;j<height;++j,line+=widthStep){
        for(i=0;i<width;++i){
            ////fprintf(stderr,"[DEBUG] line[i]=%d\n",line[i]);
            ++histogram[line[i]];
        }
    }
}

int quantizeColor(const RGB &rgb)
{
    int index=gDistTable[rgb.B][rgb.G][rgb.R];
    return index;
}

double rgb_distance(const RGB &rgb1,const RGB &rgb2)
{
    return sqrt((double)(abs(rgb1.R-rgb2.R)*abs(rgb1.R-rgb2.R)+
                abs(rgb1.G-rgb2.G)*abs(rgb1.G-rgb2.G)+
                abs(rgb1.B-rgb2.B)*abs(rgb1.B-rgb2.B)));
}

void initColorTable()
{
    gColorTable = new RGB[COLOR_TABLE_SIZE]{
        {140,   0,      236},
        {0,     145,    255},
        {232,   243,    252},
        {0,     242,    255},
        {155,   223,    196},
        {255,   245,    165},
        {255,   200,    0  },
        {255,   255,    255},
        {138,   166,    255},

        {175,   0,      180},
        {0,     72,     255},
        {187,   212,    235},
        {0,     203,    233},
        {63,    198,    141},
        {164,   169,    0  },
        {205,   114,    46 },
        {207,   207,    209},
        {211,   190,    255},

        {116,   46,     135},
        {0,     0,      220},
        {109,   156,    198},
        {18,    153,    199},
        {81,    166,    0  },
        {127,   111,    52 },
        {255,   60,     0  },
        {147,   147,    149},
        {188,   168,    191},

        {104,   0,      119},
        {5,     0,      140},
        {30,    70,     105},
        {0,     98,     104},
        {24,    102,    64 },
        {127,   88,     10 },
        {110,   28,     1  },
        {81,    81,     81 },
        {141,   98,     233},

        {96,    0,      45 },
        {5,     0,      90 },
        {5,     30,     55 },
        {0,     54,     57 },
        {0,     53,     27 },
        {69,    62,     23 },
        {60,    15,     0  },
        {1,     1,      1  },
        {124,   88,     255}
    };
    gColorText=new char*[COLOR_TABLE_SIZE]{
        "玫瑰红","橙色","象牙白","黄色","浅绿","浅蓝","天蓝","白色","浅橙",
        "紫红","朱红","裸色","中黄","黄绿","蓝绿","灰蓝","银灰","分红",
        "紫色","正红","卡其","金色","绿色","茶绿","蓝色","灰色","紫粉",
        "深紫红","酒红","咖啡","军绿","深绿","古兰","深蓝","深灰","藕红",
        "暗紫","暗红","深咖","棕绿","墨绿","古绿","墨蓝","黑色","西瓜红"
    };
/*
    gColorTable=new RGB[COLOR_TABLE_SIZE]{
        {0,0,0},
        {0,182,0},
        {0,255,170},
        {36,73,0},
        {36,146,170},
        {36,255,0},
        {73,36,170},
        {73,146,0},
        {73,219,170},
        {109,36,0},
        {109,109,170},
        {109,219,0},
        {146,0,170},
        {146,109,0},
        {146,182,170},
        {182,0,0},
        {182,73,170},
        {182,182,0},
        {182,255,170},
        {219,73,0},
        {219,146,170},
        {219,255,0},
        {255,36,170},
        {255,146,0},
        {255,255,255}
    };
    gColorText=new char*[COLOR_TABLE_SIZE]{
        "BLACK",
        "SEA GREEN",
        "LIGHT GREEN",
        "OLIVE GREEN",
        "AQUA",
        "BRIGHT GREEN",
        "BLUE",
        "GREEN",
        "TURQUOISE",
        "BROWN",
        "BLUE GRAY",
        "LIME",
        "LAVENDER",
        "PLUM",
        "TEAL",
        "DARK RED",
        "MAGENTA",
        "YELLOW GREEN",
        "FLOURO GREEN",
        "RED",
        "ROSE",
        "YELLOW",
        "PINK",
        "ORANGE",
        "WHITE"
    };
*/
}

void initDistTable()
{
    gMinDistance=std::sqrt(256*256*3)+1;
    gDistTable=new int[256][256][256];
    for(int b=0;b<256;b++){
        for(int g=0;g<256;g++){
            for(int r=0;r<256;r++){
                gDistTable[b][g][r]=-1;
            }
        }
    }
    //gDistTableHelper=new int[25][256][256][256];
    for(int b=0;b<256;b++){
        for(int g=0;g<256;g++){
            for(int r=0;r<256;r++){
                RGB rgb(b,g,r);
                double minDist=gMinDistance;
                for(int i=0;i<COLOR_TABLE_SIZE;++i){
                    RGB t = gColorTable[i];
                    double dist= rgb_distance(rgb, t);
                    if(minDist>dist){
                        minDist=dist;
                        gDistTable[b][g][r]=i;
                    }
                }
            }
        }
    }
}

void getEntityHistogram(IplImage *imageSrc,unsigned char *contourData,int width,int height,int widthStep,int blockSize,int *entityHist,unsigned char mask)
{
    assert(imageSrc->width==width);
    assert(imageSrc->height==height);
    //assert(imageSrc->widthStep==widthStep);
    //memset(entityHist,0,sizeof(int)*blockSize);
    int i=0,j=0;
    unsigned char *line=NULL;
    for(line=contourData,j=0;j<height;++j,line+=widthStep){
        for(i=0;i<width;++i){
            if(line[i]==0xff){
                CvScalar pixel=cvGet2D(imageSrc,j,i);
                RGB pixelRGB(pixel.val[0],pixel.val[1],pixel.val[2]);
                int index=quantizeColor(pixelRGB);
                if(index!=-1){
                    //fprintf(stdout,"index=%d(%f,%f,%f)[%s]\n",index,pixelRGB.B,pixelRGB.G,pixelRGB.R,gColorText[index]);
                    ++entityHist[index];
                }
            }else{
            }
        }
    }
}

int otsu(unsigned char *imageData, int width, int height, int widthStep)
{
    static const int blockSize=256;
    int i=0,j=0,total=0,sum=0,A=0,B=0,threshold=0,histogram[blockSize];
    double u=0,v=0,variance=0,maximum=0;
    getHistogram(imageData,width,height,widthStep,blockSize,histogram);
    for(i=0;i<blockSize;++i){
        total+=histogram[i];
        sum+=histogram[i]*i;
    }
    for(j=0;j<blockSize;++j){
        A=0;B=0;
        for(i=0;i<j;++i){
            A+=histogram[i];
            B+=histogram[i]*i;
        }
        if(A>0){
            u=B/A;
        }else{
            u=0;
        }
        if(total-A>0){
            v=(sum-B)/(total-A);
        }else{
            v=0;
        }
        variance=A*(total-A)*(u-v)*(u-v);
        if(variance>maximum){
            maximum=variance;
            threshold=j;
        }
    }
    return threshold;
}

void thresholdImage(unsigned char *imageData,int width, int height, int widthStep, unsigned threshold)
{
    int i=0,j=0;
    unsigned char *line=NULL;
    int black=0,white=0;
    for(line=imageData,j=0;j<height;++j,line+=widthStep){
        for(i=0;i<width;++i){
            if(line[i]>=threshold){
                line[i]=0x00;
                ++black;
            }else{
                line[i]=0xff;
                ++white;
            }
        }
    }
    //LOG(LM_DEBUG,"black=%d,white=%d\n",black,white);
    float entityPercent=(float)white/(float)(black+white);
    if(entityPercent<ENTITY_PERCENT_THRESHOLD){
        //LOG(LM_DEBUG,"tune the entity area,entityPercent=%f\n",entityPercent);
        int startX=width/3,endX=width/3*2;
        int startY=height/3,endY=height/3*2;
        //LOG(LM_DEBUG,"startX=%d endX=%d startY=%d endY=%d width=%d height=%d widthStep=%d",startX,endX,startY,endY,width,height,widthStep);
        for(line=imageData+widthStep*startY,j=startY;j<endY;++j,line+=widthStep){
            for(i=startX;i<endX;++i){
                line[i]=0xff;
            }
        }
    }
}

void notImage(IplImage *edge)
{
    int i,j;
    for(i=0;i<edge->height;++i){
        for(j=0;j<edge->width;++j){
            if(edge->imageData[edge->height*i+j]==0){
                edge->imageData[edge->height*i+j]=1;
            }else{
                edge->imageData[edge->height*i+j]=0;
            }
        }
    }
}

void findContours(unsigned char *imageData, int width, int height, int widthStep)
{
    int i=0,j=0;
    unsigned char *line[3]={NULL,NULL,NULL};
    for(j=1;j<height-1;++j){
        line[0]=imageData+widthStep*(j-1);
        line[1]=imageData+widthStep*j;
        line[2]=imageData+widthStep*(j+1);
        for(i=1;i<width-1;++i){
            if(line[0][i-1]==0xff && line[0][i]==0xff && line[0][i+1]==0xff &&
                    line[1][i-1]==0xff && line[1][i]==0xff && line[1][i+1]==0xff &&
                    line[2][i-1]==0xff && line[2][i]==0xff && line[2][i+1]==0xff){
                line[0][i-1]=0;
            }else{
                line[0][i-1]=line[1][i];
            }
        }
    }
}

void testGetEntityImage(const char * const imagePath)
{
    float factor=0.8;
    IplImage *srcGray=cvLoadImage(imagePath,CV_LOAD_IMAGE_GRAYSCALE);
    //cvNamedWindow("srcGray0",CV_WINDOW_AUTOSIZE);
    //cvShowImage("srcGray0",srcGray);
    IplImage *srcGrayTmp=cvCreateImage(cvSize(srcGray->width*factor,srcGray->height*factor),srcGray->depth,srcGray->nChannels);
    IplImage *srcOrig=cvLoadImage(imagePath,CV_LOAD_IMAGE_COLOR);
    IplImage *srcOrigTmp=cvCreateImage(cvSize(srcOrig->width*factor,srcOrig->height*factor),srcOrig->depth,srcOrig->nChannels);
    //IplImage *entityImg=cvCreateImage(cvSize(srcOrig->width*factor,srcOrig->height*factor),srcOrig->depth,srcOrig->nChannels);

    cvResize(srcGray,srcGrayTmp,CV_INTER_LINEAR);
    srcGray->width*=factor;
    srcGray->height*=factor;
    cvCopy(srcGrayTmp,srcGray);
    cvResize(srcOrig,srcOrigTmp,CV_INTER_LINEAR);
    srcOrig->width*=factor;
    srcOrig->height*=factor;
    cvCopy(srcOrigTmp,srcOrig);

    CvScalar pixel=cvGet2D(srcGray,0,0);
    //fprintf(stderr,"B=%f,G=%f,R=%f\n", pixel.val[0], pixel.val[1], pixel.val[2]);
    //fprintf(stderr,"line=%d %d %d\n",*((unsigned char *)srcGray->imageData+0*srcOrig->height+0),*((unsigned char *)srcGray->imageData+0*srcOrig->height+0+1),*((unsigned char *)srcGray->imageData+0*srcOrig->height+0+2));
    //fprintf(stderr,"line=%d %d %d\n",*((unsigned char *)srcGray->imageData+0*srcOrig->height+1),*((unsigned char *)srcGray->imageData+0*srcOrig->height+1+1),*((unsigned char *)srcGray->imageData+0*srcOrig->height+1+2));

    unsigned int threshold=otsu((unsigned char *)srcGray->imageData,srcGray->width,srcGray->height,srcGray->widthStep);
    //threshold=240;
    //LOG(LM_DEBUG,"threshold=%d\n",threshold);
    //thresholdImage((unsigned char *)srcGray->imageData,srcGray->width,srcGray->height,srcGray->widthStep,threshold);
    //cvThreshold(srcGray,srcGray,300,255,CV_THRESH_OTSU|CV_THRESH_BINARY_INV);
    /*
    cvSetZero(entityImg);
    cvCopy(srcOrig,entityImg,srcGray);
    cvNamedWindow("entityImg",CV_WINDOW_AUTOSIZE);
    cvShowImage("entityImg",entityImg);
    */
    /*
    cvNamedWindow("srcGray1",CV_WINDOW_AUTOSIZE);
    cvShowImage("srcGray1",srcGray);
    cvNamedWindow("srcOrig",CV_WINDOW_AUTOSIZE);
    cvShowImage("srcOrig",srcOrig);
    */
    int entityHist[COLOR_TABLE_SIZE];
    memset(entityHist,0,COLOR_TABLE_SIZE*sizeof(int));
    getEntityHistogram(srcOrig,(unsigned char *)srcGray->imageData,srcGray->width,srcGray->height,srcGray->widthStep,COLOR_TABLE_SIZE,entityHist, 0x00);
    typedef struct{
        int histValue;
        int histIndex;
    }__Hist;
    __Hist hist[COLOR_TABLE_SIZE];
    for(int i=0;i<COLOR_TABLE_SIZE;++i){
        hist[i].histValue=entityHist[i];
        hist[i].histIndex=i;
    }
    for(int i=0;i<COLOR_TABLE_SIZE;++i){
        int maxHistValue=hist[i].histValue;
        int maxHistIndex=i;
        for(int j=i;j<COLOR_TABLE_SIZE;++j){
            if(maxHistValue<hist[j].histValue){
                maxHistValue=hist[j].histValue;
                maxHistIndex=j;
            }
        }
        __Hist tmp=hist[i];
        hist[i]=hist[maxHistIndex];
        hist[maxHistIndex]=tmp;
    }
    /*
    for(int i=0;i<25;++i){
        fprintf(stderr,"sorted_entityHist[%d]=%d\n ",hist[i].histIndex,hist[i].histValue);
    }
    */
    for(int i=0;i<COLOR_TABLE_SIZE;++i){
        //LOG(LM_DEBUG,"[%2d][%8d][%s]\n",i,hist[i].histValue,gColorText[hist[i].histIndex]);
    }

    //fprintf(stderr,"\n");
    //findContours((unsigned char *)srcGray->imageData,srcGray->width,srcGray->height,srcGray->widthStep);
    //cvNamedWindow("srcGray",CV_WINDOW_AUTOSIZE);
    //cvShowImage("srcGray",srcGray);
    //cvWaitKey(0);
    cvReleaseImage(&srcGray);
    cvReleaseImage(&srcGrayTmp);
    cvReleaseImage(&srcOrig);
    cvReleaseImage(&srcOrigTmp);
    //cvReleaseImage(&entityImg);
}

const char *getColorText(int index)
{
    if(index>=0 && index<COLOR_TABLE_SIZE){
        return gColorText[index];
    }else{
        return "undefined";
    }
}

bool getImageColor(char *imgdata, unsigned int imgsize,int *indexCollection,int count)
{
    static int load_failed = 0;
    CvMat mat = cvMat( 100, 100, CV_8UC1, imgdata);
    IplImage* srcOrig = cvDecodeImage( &mat, CV_LOAD_IMAGE_COLOR);
    if(srcOrig == NULL)
    {
        // GIF not supported, so this may fail.
        if(++load_failed % 100 == 0)
            LOG(ERROR) << "decode image data failed." << load_failed << endl;
        return false;
    }
    IplImage* srcGray = cvDecodeImage(&mat, CV_LOAD_IMAGE_GRAYSCALE);
    if(srcGray == NULL)
    {
        cvReleaseImage(&srcOrig);
        LOG(WARNING) << "decode image data failed." << std::endl;
        return false;
    }
    return getImageColor(srcGray, srcOrig, indexCollection, count);
}

bool getImageColor(const char * const imagePath,int *indexCollection,int count)
{
    static int non_exist = 0;
    static int load_failed = 0;
    if(access(imagePath,F_OK)==-1){
        if(++non_exist % 10000 == 0)
            LOG(ERROR) << "file does not exist:" << non_exist << endl;
        return false;
    }
    if(access(imagePath,R_OK)==-1){
        LOG(ERROR) << "file cannot be read" << imagePath << endl;
        return false;
    }

    IplImage *srcGray=NULL,*srcOrig=NULL;
    if(NULL==(srcGray=cvLoadImage(imagePath,CV_LOAD_IMAGE_GRAYSCALE))){
        // GIF not supported, so this may fail.
        if(++load_failed % 100 == 0)
            LOG(ERROR) << "cvLoadImage failed 1 : " << load_failed << endl;
        return false;
    }
    if(NULL==(srcOrig=cvLoadImage(imagePath,CV_LOAD_IMAGE_COLOR))){
        cvReleaseImage(&srcGray);
        LOG(ERROR) << "cvLoadImage failed 2" << endl;
        printf("Error string %s\n", cvErrorStr(cvGetErrStatus()));
        return false;
    }
    return getImageColor(srcGray, srcOrig, indexCollection, count);
}

bool getImageColor(IplImage* srcGray, IplImage* srcOrig, int* indexCollection, int count)
{
    IplImage *srcGrayTmp=NULL,*srcOrigTmp=NULL;
    float factor=0.8;
    if(srcGray->width*factor>50&&srcGray->height*factor>50){
        if(NULL==(srcGrayTmp=cvCreateImage(cvSize(srcGray->width*factor,srcGray->height*factor),srcGray->depth,srcGray->nChannels))){
            cvReleaseImage(&srcGray);
            cvReleaseImage(&srcOrig);
            LOG(ERROR) << "createImageColor failed 3" << endl;
            return false;
        }
        if(NULL==(srcOrigTmp=cvCreateImage(cvSize(srcOrig->width*factor,srcOrig->height*factor),srcOrig->depth,srcOrig->nChannels))){
            cvReleaseImage(&srcGray);
            cvReleaseImage(&srcGrayTmp);
            cvReleaseImage(&srcOrig);
            LOG(ERROR) << "createImageColor failed 4" << endl;
            return false;
        }
        //LOG(LM_DEBUG,"TEST:width=%d height=%d width*factor=%d height*factor=%d\n",srcGray->width,srcGray->height,srcGray->width*factor,srcGray->height*factor);
        cvResize(srcGray,srcGrayTmp,CV_INTER_LINEAR);
        srcGray->width*=factor;
        srcGray->height*=factor;
        cvCopy(srcGrayTmp,srcGray);
        cvResize(srcOrig,srcOrigTmp,CV_INTER_LINEAR);
        srcOrig->width*=factor;
        srcOrig->height*=factor;
        cvCopy(srcOrigTmp,srcOrig);
    }

    unsigned int threshold=otsu((unsigned char *)srcGray->imageData,srcGray->width,srcGray->height,srcGray->widthStep);
    //LOG(LM_DEBUG,"threshold=%d\n",threshold);
    thresholdImage((unsigned char *)srcGray->imageData,srcGray->width,srcGray->height,srcGray->widthStep,threshold);
    int entityHist[COLOR_TABLE_SIZE];
    memset(entityHist,0,COLOR_TABLE_SIZE*sizeof(int));
    getEntityHistogram(srcOrig,(unsigned char *)srcGray->imageData,srcGray->width,srcGray->height,srcGray->widthStep,COLOR_TABLE_SIZE,entityHist, 0x00);

    typedef struct{
        int histValue;
        int histIndex;
    }__Hist;
    __Hist hist[COLOR_TABLE_SIZE];
    for(int i=0;i<COLOR_TABLE_SIZE;++i){
        hist[i].histValue=entityHist[i];
        hist[i].histIndex=i;
    }
    for(int i=0;i<COLOR_TABLE_SIZE;++i){
        int maxHistValue=hist[i].histValue;
        int maxHistIndex=i;
        for(int j=i;j<COLOR_TABLE_SIZE;++j){
            if(maxHistValue<hist[j].histValue){
                maxHistValue=hist[j].histValue;
                maxHistIndex=j;
            }
        }
        __Hist tmp=hist[i];
        hist[i]=hist[maxHistIndex];
        hist[maxHistIndex]=tmp;
    }
    int c;
    if(count>=COLOR_TABLE_SIZE){
        c=COLOR_TABLE_SIZE-1;
    }else{
        c=count;
    }
    for(int i=0;i<c;++i){
        *(indexCollection+i)=hist[i].histIndex;
    }

    if(srcGray){
        cvReleaseImage(&srcGray);
    }
    if(srcGrayTmp){
        cvReleaseImage(&srcGrayTmp);
    }
    if(srcOrig){
        cvReleaseImage(&srcOrig);
    }
    if(srcOrigTmp){
        cvReleaseImage(&srcOrigTmp);
    }
    return true;

}


