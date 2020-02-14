#ifdef OPENCV

#include "stdio.h"
#include "stdlib.h"
#include "opencv2/opencv.hpp"
#include "image.h"
#include <pylonc/PylonC.h> // for Neon-J

using namespace cv;

unsigned char* imgBuf;        /* Buffer used for grabbing. */
unsigned char* imgBuf_BGR;    /* Buffer used for RGB2BGR convert */
PYLON_DEVICE_HANDLE hDev;     /* Handle for the pylon device. */
int32_t  payloadSize;         /* Size of an image frame in bytes. */
Mat Mat_Img;

extern "C" {

IplImage *image_to_ipl(image im)
{
    int x,y,c;
    IplImage *disp = cvCreateImage(cvSize(im.w,im.h), IPL_DEPTH_8U, im.c);
    int step = disp->widthStep;
    for(y = 0; y < im.h; ++y){
        for(x = 0; x < im.w; ++x){
            for(c= 0; c < im.c; ++c){
                float val = im.data[c*im.h*im.w + y*im.w + x];
                disp->imageData[y*step + x*im.c + c] = (unsigned char)(val*255);
            }
        }
    }
    return disp;
}

image ipl_to_image(IplImage* src)
{
    int h = src->height;
    int w = src->width;
    int c = src->nChannels;
    image im = make_image(w, h, c);
    unsigned char *data = (unsigned char *)src->imageData;
    int step = src->widthStep;
    int i, j, k;

    for(i = 0; i < h; ++i){
        for(k= 0; k < c; ++k){
            for(j = 0; j < w; ++j){
                im.data[k*w*h + i*w + j] = data[i*step + j*c + k]/255.;
            }
        }
    }
    return im;
}

Mat image_to_mat(image im)
{
    image copy = copy_image(im);
    constrain_image(copy);
    if(im.c == 3) rgbgr_image(copy);

    IplImage *ipl = image_to_ipl(copy);
    Mat m = cvarrToMat(ipl, true);
    cvReleaseImage(&ipl);
    free_image(copy);
    return m;
}

image mat_to_image(Mat m)
{
    IplImage ipl = m;
    image im = ipl_to_image(&ipl);
    rgbgr_image(im);
    return im;
}

void *open_video_stream(const char *f, int c, int w, int h, int fps)
{
    VideoCapture *cap;
    if(f) cap = new VideoCapture(f);
    else cap = new VideoCapture(c);
    if(!cap->isOpened()) return 0;
    if(w) cap->set(CV_CAP_PROP_FRAME_WIDTH, w);
    if(h) cap->set(CV_CAP_PROP_FRAME_HEIGHT, w);
    if(fps) cap->set(CV_CAP_PROP_FPS, w);
    return (void *) cap;
}

image get_image_from_stream(void *p)
{
    VideoCapture *cap = (VideoCapture *)p;
    Mat m;
    *cap >> m;
    if(m.empty()) return make_empty_image(0,0,0);
    return mat_to_image(m);
}

image load_image_cv(char *filename, int channels)
{
    int flag = -1;
    if (channels == 0) flag = -1;
    else if (channels == 1) flag = 0;
    else if (channels == 3) flag = 1;
    else {
        fprintf(stderr, "OpenCV can't force load with %d channels\n", channels);
    }
    Mat m;
    m = imread(filename, flag);
    if(!m.data){
        fprintf(stderr, "Cannot load image \"%s\"\n", filename);
        char buff[256];
        sprintf(buff, "echo %s >> bad.list", filename);
        system(buff);
        return make_image(10,10,3);
        //exit(0);
    }
    image im = mat_to_image(m);
    return im;
}

int show_image_cv(image im, const char* name, int ms)
{
    Mat m = image_to_mat(im);
    imshow(name, m);
    int c = waitKey(ms);
    if (c != -1) c = c%256;
    return c;
}

void make_window(char *name, int w, int h, int fullscreen)
{
    namedWindow(name, WINDOW_NORMAL); 
    if (fullscreen) {
        setWindowProperty(name, CV_WND_PROP_FULLSCREEN, CV_WINDOW_FULLSCREEN);
    } else {
        resizeWindow(name, w, h);
        if(strcmp(name, "Demo") == 0) moveWindow(name, 0, 0);
    }
}

int Neon_Basler_Init(void)
{
    GENAPIC_RESULT          res;           /* Return value of pylon methods. */
    size_t                  numDevices;    /* Number of available devices. */
    _Bool                    isAvail;

    /* Before using any pylon methods, the pylon runtime must be initialized. */
    PylonInitialize();

    /* Enumerate all camera devices. You must call
    PylonEnumerateDevices() before creating a device! */
    res = PylonEnumerateDevices( &numDevices );
    if ( 0 == numDevices )
    {
        /* Before exiting a program, PylonTerminate() should be called to release
           all pylon related resources. */
        PylonTerminate();
        return -1;
    }

    /* Get a handle for the first device found.  */
    res = PylonCreateDeviceByIndex( 0, &hDev );
    if (res != GENAPI_E_OK) return -1;

    /* Before using the device, it must be opened. Open it for configuring
    parameters and for grabbing images. */
    res = PylonDeviceOpen( hDev, PYLONC_ACCESS_MODE_CONTROL | PYLONC_ACCESS_MODE_STREAM );
    if (res != GENAPI_E_OK) return -1;

    /* Print out the name of the camera we are using. */
    {
        char buf[256];
        size_t siz = sizeof(buf);
        _Bool isReadable;

        isReadable = PylonDeviceFeatureIsReadable(hDev, "DeviceModelName");
        if ( isReadable )
        {
            res = PylonDeviceFeatureToString(hDev, "DeviceModelName", buf, &siz );
            if (res != GENAPI_E_OK) return -1;
            printf("Neon-J: Using camera %s\n", buf);
        }
    }

   isAvail = PylonDeviceFeatureIsAvailable( hDev, "EnumEntry_TriggerSelector_FrameStart");
    if (isAvail)
    {
        res = PylonDeviceFeatureFromString( hDev, "TriggerSelector", "FrameStart");
        if (res != GENAPI_E_OK) return -1;
        res = PylonDeviceFeatureFromString( hDev, "TriggerMode", "Off");
        if (res != GENAPI_E_OK) return -1;
        printf("Neon-J: Turn TriggerSelector_FrameSatart off\n");
    }

    /* Set pixel format to RGB if available */
    isAvail = PylonDeviceFeatureIsAvailable( hDev, "EnumEntry_PixelFormat_RGB8");
    if (isAvail)
    {
        res = PylonDeviceFeatureFromString( hDev, "PixelFormat", "RGB8");
        if (res != GENAPI_E_OK) return -1;
        printf("Neon-J: Set PixelFormat to RGB\n");
    }

    /* Set color space to sRGB if available */
    isAvail = PylonDeviceFeatureIsAvailable( hDev, "EnumEntry_BslColorSpaceMode_sRGB");
    if (isAvail)
    {
        res = PylonDeviceFeatureFromString( hDev, "BslColorSpaceMode", "sRGB");
        if (res != GENAPI_E_OK) return -1;
        printf("Neon-J: Set ColorSpaceMode to sRGB\n");
    }

    /* Determine the required size of the grab buffer. */
    if ( PylonDeviceFeatureIsReadable(hDev, "PayloadSize") )
    {
        res = PylonDeviceGetIntegerFeatureInt32( hDev, "PayloadSize", &payloadSize );
        if (res != GENAPI_E_OK) return -1;
        printf("Neon-J: Payload size is %d\n", payloadSize);
    }

    int32_t ImgWidth, ImgHeight;
    if ( PylonDeviceFeatureIsReadable(hDev, "Width") )
    {
        res = PylonDeviceGetIntegerFeatureInt32( hDev, "Width", &ImgWidth );
        if (res != GENAPI_E_OK) return -1;
    }
    if ( PylonDeviceFeatureIsReadable(hDev, "Height") )
    {
        res = PylonDeviceGetIntegerFeatureInt32( hDev, "Height", &ImgHeight );
        if (res != GENAPI_E_OK) return -1;
    }
    printf("Neon-J: Image width x height is %d x %d\n", ImgWidth, ImgHeight);
    
    /* Allocate memory for grabbing. */
    imgBuf = (unsigned char*) malloc( payloadSize );
    imgBuf_BGR = (unsigned char*) malloc( payloadSize );
    if ( NULL == imgBuf )
    {
        printf("Neon-J: Out of memory.\n" );
        Neon_Basler_Terminate();
        return -1;
    }

    Mat_Img.create(ImgHeight, ImgWidth, CV_8UC3);

    return 0;
}

image Neon_Basler_Get_Image(void)
{
    GENAPIC_RESULT res;           /* Return value of pylon methods. */
    PylonGrabResult_t grabResult;
    _Bool bufferReady;

       for ( int i=0; i<10; i++ ) {
        res = PylonDeviceGrabSingleFrame(hDev, 0, imgBuf, payloadSize, &grabResult, &bufferReady, 500);
        //printf("Neon-J: PylonDeviceGrabSingleFrame(), times:%d\n", i+1);
        if ( grabResult.Status == Grabbed ) {
            //printf("Neon-J: image grabbed successfully.\n");
            break;
        }
        else
            printf("Neon-J: image wasn't grabbed successfully(times:%d).  Error code = 0x%08X\n", i+1, 		grabResult.ErrorCode );
    }

    if ( GENAPI_E_OK == res && !bufferReady )
    {
        /* Timeout occurred. */
        printf("Neon-J: get image timeout\n");
        return make_empty_image(0,0,0);
    }

    /* Check to see if the image was grabbed successfully. */
    if ( grabResult.Status == Grabbed )
    {
        // Convert color space from RGB to BGR
        int w, h;
        for (h=0; h<grabResult.SizeY; h++)
            for (w=0; w<grabResult.SizeX; w++)
            {
                imgBuf_BGR[3*grabResult.SizeX*h + 3*w]     = imgBuf[3*grabResult.SizeX*h + 3*w + 2];
                imgBuf_BGR[3*grabResult.SizeX*h + 3*w + 1] = imgBuf[3*grabResult.SizeX*h + 3*w + 1];
                imgBuf_BGR[3*grabResult.SizeX*h + 3*w + 2] = imgBuf[3*grabResult.SizeX*h + 3*w];
            }
        memcpy(Mat_Img.ptr(), imgBuf_BGR, payloadSize);
        return mat_to_image(Mat_Img);
    }
    else //if ( grabResult.Status == Failed )
    {
        printf( "Neon-J: image wasn't grabbed successfully.  Error code = 0x%08X\n", grabResult.ErrorCode );
        return make_empty_image(0,0,0);
    }
}

void Neon_Basler_Terminate(void)
{
    PylonDeviceClose( hDev );
    PylonDestroyDevice ( hDev );

    /* Free memory for grabbing. */
    if (imgBuf)
        free( imgBuf );
    if (imgBuf_BGR)
        free( imgBuf_BGR );

    /* Shut down the pylon runtime system. Don't call any pylon method after
       calling PylonTerminate(). */
    PylonTerminate();
}


}

#endif
