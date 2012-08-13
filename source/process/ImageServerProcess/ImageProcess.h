#ifndef __IMAGE_PROCESS_H__
#define __IMAGE_PROCESS_H__

#define COLOR_TABLE_SIZE      45
#define ENTITY_PERCENT_THRESHOLD    0.2
#define COLOR_RET_NUM  3

typedef struct S_RGB{
    int B;
    int G;
    int R;
    S_RGB(int b, int g, int r)
        :B(b), G(g), R(r)
    {
    }
}RGB,*RGBPtr;

void getHistogram(unsigned char *imageData,int width,int height,int widthStep,int blockSize,int *histogram);
//void getEntityHistogram(IplImage *imageSrc,unsigned char *contourData,int width,int height,int widthStep,int blockSize,int *contourHist,unsigned char mask=0x0);
int otsu(unsigned char *imageData, int width, int height, int widthStep);
void thresholdImage(unsigned char *imageData, int width, int height, int widthStep, unsigned threshold);
//void thresholdImage(const unsigned char * const imageData, unsigned char *newImageData,int width, int height, int widthStep, unsigned threshold)
//void notImage(IplImage *edge);
void findContours(unsigned char *imageData, int width, int height, int widthStep);
int quantizeColor(const RGB &rgb);
double rgb_distance(const RGB &rgb1,const RGB &rgb2);
void initColorTable();
void initDistTable();
const char *getColorText(int index);
bool getImageColor(const char * const imagePath,int *indexCollection,int count);
bool getImageColor(char *imgdata, unsigned int imgsize,int *indexCollection,int count);

void testGetEntityImage(const char * const imagePath);

#endif
