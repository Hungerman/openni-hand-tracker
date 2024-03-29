
//*************************************************************************************
//INCLUDES
//*************************************************************************************
#include <openni/XnOpenNI.h>
#include <openni/XnCppWrapper.h>
#include <openni/XnHash.h>
#include <openni/XnLog.h>
#include <opencv2/opencv.hpp>
#include <nite/XnVNite.h>
#include <Eigen/Core>
#include "PointDrawer.h"
#include <GL/glut.h>
#include "Palm.h"
#include "particle.h"
#include "signal_catch.h"
//*************************************************************************************


//*************************************************************************************
//Defines di sistema
//*************************************************************************************
#define SAMPLE_XML_PATH "Sample-Tracking.xml"
//*************************************************************************************
//*************************************************************************************



//*************************************************************************************
//Parametri intrinseci del kinect
//*************************************************************************************
#define fx_rgb 5.2921508098293293e+02
#define fy_rgb 5.2556393630057437e+02
#define cx_rgb 3.2894272028759258e+02
#define cy_rgb 2.6748068171871557e+02
#define k1_rgb 2.6451622333009589e-01
#define k2_rgb -8.3990749424620825e-01
#define p1_rgb -1.9922302173693159e-03
#define p2_rgb 1.4371995932897616e-03
#define k3_rgb 9.1192465078713847e-01

#define fx_d 5.9421434211923247e+02
#define fy_d 5.9104053696870778e+02
#define cx_d 3.3930780975300314e+02
#define cy_d 2.4273913761751615e+02
#define k1_d -2.6386489753128833e-01
#define k2_d 9.9966832163729757e-01
#define p1_d -7.6275862143610667e-04
#define p2_d 5.0350940090814270e-03
#define k3_d -1.3053628089976321e+00

//Eigen::MatrixXd R(9.9984628826577793e-01, 1.2635359098409581e-03,-1.7487233004436643e-02, -1.4779096108364480e-03,9.9992385683542895e-01, -1.2251380107679535e-02,1.7470421412464927e-02, 1.2275341476520762e-02,9.9977202419716948e-01);
//Eigen::Vector3d T(0.019985242312092553,-0.00074423738761617583,-0.010916736334336222);

double T[]={0.019985242312092553, -0.00074423738761617583,-0.010916736334336222};
double R[]={0.9998462882657779,0.0012635359098409581,-0.017487233004436643,
		-0.0014779096108364480,0.99992385683542895,-0.012251380107679535,
		0.017470421412464927,0.012275341476520762,0.99977202419716948};
//*************************************************************************************
//*************************************************************************************


//*************************************************************************************
//Verifica degli errori dello status del kinect
//*************************************************************************************
#define CHECK_RC(rc, what)											\
		if (rc != XN_STATUS_OK)											\
		{																\
			printf("%s failed: %s\n", what, xnGetStatusString(rc));		\
			return rc;													\
		}

#define CHECK_ERRORS(rc, errors, what)		\
		if (rc == XN_STATUS_NO_NODE_PRESENT)	\
		{										\
			XnChar strError[1024];				\
			errors.ToString(strError, 1024);	\
			printf("%s\n", strError);			\
			return (rc);						\
		}
//*************************************************************************************
//*************************************************************************************


//*************************************************************************************
//Namespaces
//*************************************************************************************
using namespace cv;
//*************************************************************************************
#define STDIM 	200
#define	 GENER	60

particle* stormo;
particle best;
bool draw=false;



//*************************************************************************************
//Definizione variabili Globali e Define vari
//*************************************************************************************

// OpenNI objects
xn::Context g_Context;
xn::ScriptNode g_ScriptNode;
xn::DepthGenerator g_DepthGenerator;
xn::ImageGenerator g_ImageGenerator;
xn::HandsGenerator g_HandsGenerator;
xn::GestureGenerator g_GestureGenerator;
xn::DepthMetaData depthMD;
xn::ImageMetaData imageMD;


//Mano
//Hand* mano;
Palm* palmo;


// NITE objects
XnVSessionManager* g_pSessionManager;
XnVFlowRouter* g_pFlowRouter;

// the drawer
XnVPointDrawer* g_pDrawer;

//OPENCV GLOBALS
Mat colorShow = Mat::zeros(480,640, CV_8UC3 );
Mat hsvShow = Mat::zeros(480,640, CV_8UC3 );

//Font utilizzato per disegnare nelle finestre
CvFont dafont = fontQt("Times",10,Scalar(255,255,255));
float angle=0.0f;
int trackX=0;
int trackY=0;
int trackZ=0;
int prova;
int angleT=90;
int distT=40;
int dist2T=70;


int hmin=118;
int hmax=122;
int smin=100;
int smax=255;
int vmin=100;
int vmax=255;
int erosion_elem = 0;
int erosion_size = 0;

#define GL_WIN_SIZE_X 720
#define GL_WIN_SIZE_Y 480

// Draw the depth map?
XnBool g_bDrawDepthMap = true;
XnBool g_bPrintFrameID = false;
// Use smoothing?
XnFloat g_fSmoothing = 0.0f;
XnBool g_bPause = false;
XnBool g_bQuit = false;
SessionState g_SessionState = NOT_IN_SESSION;

//*************************************************************************************
//*************************************************************************************


//*************************************************************************************
//Exit routine
//*************************************************************************************

void CleanupExit()
{
	g_ScriptNode.Release();
	g_DepthGenerator.Release();
	g_HandsGenerator.Release();
	g_GestureGenerator.Release();
	g_Context.Release();

	exit (1);
}
//*************************************************************************************
//*************************************************************************************



//*************************************************************************************
// Callback for when the focus is in progress
//*************************************************************************************
void XN_CALLBACK_TYPE FocusProgress(const XnChar* strFocus, const XnPoint3D& ptPosition, XnFloat fProgress, void* UserCxt)
{
	//	printf("Focus progress: %s @(%f,%f,%f): %f\n", strFocus, ptPosition.X, ptPosition.Y, ptPosition.Z, fProgress);
}
//*************************************************************************************
//*************************************************************************************


//*************************************************************************************
// callback for session start
//*************************************************************************************
void XN_CALLBACK_TYPE SessionStarting(const XnPoint3D& ptPosition, void* UserCxt)
{
	printf("Session start: (%f,%f,%f)\n", ptPosition.X, ptPosition.Y, ptPosition.Z);
	g_SessionState = IN_SESSION;
}
//*************************************************************************************
//*************************************************************************************


//*************************************************************************************
// Callback for session end
//*************************************************************************************
void XN_CALLBACK_TYPE SessionEnding(void* UserCxt)
{
	printf("Session end\n");
	g_SessionState = NOT_IN_SESSION;
}
//*************************************************************************************
//*************************************************************************************

//*************************************************************************************
//Callback for hand loss
//*************************************************************************************
void XN_CALLBACK_TYPE NoHands(void* UserCxt)
{
	if (g_SessionState != NOT_IN_SESSION)
	{
		printf("Quick refocus\n");
		g_SessionState = QUICK_REFOCUS;
	}
}
//*************************************************************************************
//*************************************************************************************


//*************************************************************************************
//Touching Callback
//*************************************************************************************
void XN_CALLBACK_TYPE TouchingCallback(xn::HandTouchingFOVEdgeCapability& generator, XnUserID id, const XnPoint3D* pPosition, XnFloat fTime, XnDirection eDir, void* pCookie)
{
	g_pDrawer->SetTouchingFOVEdge(id);
}
//*************************************************************************************
//*************************************************************************************



//PSO

float myrand()
{
	return (float)rand()/RAND_MAX;
}

void pso_init()
{

	stormo=new particle[STDIM];

	for(int i=0;i<STDIM;i++)
	{
		stormo[i].posa[partX]=myrand()*640;
		stormo[i].posa[partY]=myrand()*480;
		stormo[i].posa[partrotZ]=myrand()*360;

		printf("Particella [%d] X: %d\t Y: %d\t Z: %d\n",i,stormo[i].posa[partX],stormo[i].posa[partY],stormo[i].posa[partrotZ]);

	}



}

void pso_update()
{
	float c1=2.8;
	float c2=1.3;
	float r1=myrand();
	float r2=myrand();
	float maxSpeed=5.0f;
	float limitX=640;
	float limitY=480;


	for(int i=0;i<STDIM;i++) //ogni particella dello stormo
	{
		for(int j=0;j<partDIM;j++) //ogni elemento dello stato
		{
			if(j==partX || j==partY || j==partrotZ) //solo questi due per ora
			{
				//STORMO[i]-> i-esime particella
				//posa[j] -> j-esimo elemento

				float update_stormo=c1*r1*(best.posa[j]-stormo[i].posa[j]);
				float update_sociale=c2*r2*(stormo[i].posaBest[j]-stormo[i].posa[j]);
				float update=update_stormo+update_sociale;

				stormo[i].vposa[j]=stormo[i].vposa[j]+update;

				if(abs(stormo[i].vposa[j])>maxSpeed) //controllo se la velocità imposta è maggiore della velocità massima consentita
				{
					stormo[i].vposa[j]=(stormo[i].vposa[j]/abs(stormo[i].vposa[j]))*maxSpeed; //recupero il segno e imposta la velocità
				}

				//printf("particella %d componente %d : %d + %d = %d \n",(int)i,(int)j,(int)old,(int)update,(int)stormo[i].vposa[j]);
			}
		}
	}

	for(int i=0;i<STDIM;i++) //vedi sopra
	{
		for(int j=0;j<partDIM;j++)
		{

			if(j==partX || j==partY || j==partrotZ)
			{
				stormo[i].posa[j]=stormo[i].posa[j]+stormo[i].vposa[j];

				if(j==partX)
				{
					if(stormo[i].posa[j]<0)stormo[i].posa[j]=0;
					if(stormo[i].posa[j]>640)stormo[i].posa[j]=640;
				}
				if(j==partY)
				{
					if(stormo[i].posa[j]<0)stormo[i].posa[j]=0;
					if(stormo[i].posa[j]>480)stormo[i].posa[j]=480;
				}

			}
		}

	}


}

void pso_perturba_stormo()
{
	best.errore_posa=99999;
	float max=100;
	float min=max/2;

	for(int i=1;i<STDIM;i++)
	{
		for(int j=0;j<partDIM;j++)
		{
			if(j==partX || j==partY|| j==partrotZ)
			{
				stormo[i].posa[j]=stormo[0].posa[j]+(int)(myrand()*max-min);
			}
		}
	}
}
void pso_best()
{
	for(int i=0;i<STDIM;i++)
	{
		if(stormo[i].errore_posa<best.errore_posa)
		{
			best.errore_posa=stormo[i].errore_posa;
			for(int j=0;j<partDIM;j++)
			{
				best.posa[j]=stormo[i].posa[j];
				best.punti_esterni=stormo[i].punti_esterni;
			}
		}
	}


}

void pso_compute_error()
{



	for(int i=0;i<STDIM;i++){

		for(int j=0;j<partDIM;j++)
		{
			palmo->posa[j]=stormo[i].posa[j];

		}
		stormo[i].punti_esterni=0;

		int errore=0;

		palmo->Update(); //dopo l'update mi ritrovo nel vettore "puntiproiettati" le coordinate dei vertici

		XnDepthPixel* pDepth = depthMD.WritableData();
		for(int k=0;k<PALM_VERTEX;k++)
		{
			int index=480*(int)palmo->puntiProiettati[k][1]+(int)palmo->puntiProiettati[k][0];

			if(index<307200)
			{
				int depth=pDepth[index];

				if(depth<=0)
				{
					errore=errore+50;
					stormo[i].punti_esterni++;
				}
			}

		}

		int distance=std::sqrt(		pow((int)g_pDrawer->ptProjective.X-(int)palmo->puntiProiettati[0][0],2)+
				pow((int)g_pDrawer->ptProjective.Y-(int)palmo->puntiProiettati[0][1],2)
		);
		errore=errore+distance/3;
		//printf("Punto(%d,%d) Palmo(%d,%d)\n",(int)palmo->puntiProiettati[0][0],(int)palmo->puntiProiettati[0][1],(int)g_pDrawer->ptProjective.X,(int)g_pDrawer->ptProjective.Y);

		stormo[i].errore_posa=errore;

		if(stormo[i].errore_posa<stormo[i].errore_posaBest)
		{
			for(int j=0;j<partDIM;j++)
			{
				stormo[i].posaBest[j]=stormo[i].posa[j];
			}
			stormo[i].errore_posaBest=stormo[i].errore_posa;

		}

	}

}

//*************************************************************************************
//Gesture Callback
//*************************************************************************************
/*
void XN_CALLBACK_TYPE MyGestureInProgress(xn::GestureGenerator& generator, const XnChar* strGesture, const XnPoint3D* pPosition, void* pCookie)
{
    printf("Gesture %s in progress\n", strGesture);
}
void XN_CALLBACK_TYPE MyGestureReady(xn::GestureGenerator& generator, const XnChar* strGesture, const XnPoint3D* pPosition, void* pCookie)
{
    printf("Gesture %s ready for next stage\n", strGesture);
}
 */
//*************************************************************************************
//*************************************************************************************

//*************************************************************************************
//Gestures callbacks (hints about wave gesture status)
//*************************************************************************************
void XN_CALLBACK_TYPE GestureIntermediateStageCompletedHandler(xn::GestureGenerator& generator, const XnChar* strGesture, const XnPoint3D* pPosition, void* pCookie)
{
	printf("Gesture %s: Intermediate stage complete (%f,%f,%f)\n", strGesture, pPosition->X, pPosition->Y, pPosition->Z);
}
void XN_CALLBACK_TYPE GestureReadyForNextIntermediateStageHandler(xn::GestureGenerator& generator, const XnChar* strGesture, const XnPoint3D* pPosition, void* pCookie)
{
	printf("Gesture %s: Ready for next intermediate stage (%f,%f,%f)\n", strGesture, pPosition->X, pPosition->Y, pPosition->Z);
}
void XN_CALLBACK_TYPE GestureProgressHandler(xn::GestureGenerator& generator, const XnChar* strGesture, const XnPoint3D* pPosition, XnFloat fProgress, void* pCookie)
{
	printf("Gesture %s progress: %f (%f,%f,%f)\n", strGesture, fProgress, pPosition->X, pPosition->Y, pPosition->Z);
}
//*************************************************************************************
//*************************************************************************************


//*************************************************************************************
//AXIS: called each frame
//*************************************************************************************
void DrawAxes(float length)
{
	glPushMatrix();
	glScalef(length, length, length);

	glLineWidth(2.f);
	glBegin(GL_LINES);

	// x red
	glColor3f(1.f, 0.f, 0.f);
	glVertex3f(0.f, 0.f, 0.f);
	glVertex3f(1.f, 0.f, 0.f);

	// y green
	glColor3f(0.f, 1.f, 0.f);
	glVertex3f(0.f, 0.f, 0.f);
	glVertex3f(0.f, 1.f, 0.f);

	// z blue
	glColor3f(0.f, 0.f, 1.f);
	glVertex3f(0.f, 0.f, 0.f);
	glVertex3f(0.f, 0.f, 1.f);

	glEnd();
	glLineWidth(1.f);

	glPopMatrix();
}



void RGBkinect2OpenCV(cv::Mat colorImage)
{
	//Puntatore alla riga di pixel
	const XnRGB24Pixel* pImageRow;
	//Puntatore al pixel
	const XnRGB24Pixel* pPixel;
	//Matrice dei punti RGB
	cv::Mat colorArr[3];

	pImageRow = imageMD.RGB24Data();


	colorArr[0] = cv::Mat(imageMD.YRes(),imageMD.XRes(),CV_8U);
	colorArr[1] = cv::Mat(imageMD.YRes(),imageMD.XRes(),CV_8U);
	colorArr[2] = cv::Mat(imageMD.YRes(),imageMD.XRes(),CV_8U);

	for (int y=0; y<imageMD.YRes(); y++)
	{
		pPixel = pImageRow;
		uchar* Bptr = colorArr[0].ptr<uchar>(y);
		uchar* Gptr = colorArr[1].ptr<uchar>(y);
		uchar* Rptr = colorArr[2].ptr<uchar>(y);
		for(int x=0;x<imageMD.XRes();++x , ++pPixel)
		{
			Bptr[x] = pPixel->nBlue;
			Gptr[x] = pPixel->nGreen;
			Rptr[x] = pPixel->nRed;
		}
		pImageRow += imageMD.XRes();
	}
	cv::merge(colorArr,3,colorImage);
}

/*
void init_mano(float *state)
{
	for(int i=0;i<HAND_DIM;i++)
	{
		state[i]=myrand()*50;
		//tmp=(myrand()*( mano->Constraints[i][1]-mano->Constraints[i][0] ) )+mano->Constraints[i][0];
	}
	state[18]=state[19]=state[20]=state[15]=state[16]=state[17]=0;
	state[15]=-90;
}
 */

void hand_found(cv::Mat depthshow,XnDepthPixel* pDepth)
{

	cvtColor(colorShow, hsvShow, CV_BGR2HSV);


	createTrackbar( "H min", "HSV", &hmin, 255,  NULL);//OK tested
	createTrackbar( "H max", "HSV", &hmax, 255,  NULL);//OK tested
	createTrackbar( "S min", "HSV", &smin, 255,  NULL);//OK tested
	createTrackbar( "S max", "HSV", &smax, 255,  NULL);//OK tested
	createTrackbar( "V min", "HSV", &vmin, 255,  NULL);//OK tested
	createTrackbar( "V max", "HSV", &vmax, 255,  NULL);//OK tested
	createTrackbar( "Erosion Elem", "HSV", &erosion_elem, 2,  NULL);//OK tested
	createTrackbar( "Erosion Size", "HSV", &erosion_size, 10,  NULL);//OK tested

	Mat hsvBLUE = Mat::zeros(480,640, CV_8UC3 );
	Mat hsvRED = Mat::zeros(480,640, CV_8UC3 );

	inRange(hsvShow,Scalar(hmin, smin,vmin),Scalar(hmax, smax, vmax), hsvBLUE);
	inRange(hsvShow,Scalar(65, smin,vmin),Scalar(72, smax, vmax), hsvRED);

	//************************************************************************************
	//EROSIONE
	//************************************************************************************
	int erosion_type;
	if( erosion_elem == 0 ){ erosion_type = MORPH_RECT; }
	else if( erosion_elem == 1 ){ erosion_type = MORPH_CROSS; }
	else if( erosion_elem == 2) { erosion_type = MORPH_ELLIPSE; }


	Mat element = getStructuringElement( erosion_type,
			Size( 2*erosion_size + 1, 2*erosion_size+1 ),
			Point( erosion_size, erosion_size ) );

	erode( hsvBLUE, hsvBLUE, element );
	erode( hsvRED, hsvRED, element );
	//************************************************************************************

	SimpleBlobDetector::Params parametri;
	parametri.filterByArea=1;
	parametri.minArea=2;
	parametri.maxArea=32500;
	parametri.filterByColor=1;


	SimpleBlobDetector * blob_detector;
	blob_detector = new SimpleBlobDetector(parametri);
	blob_detector->create("SimpleBlobDetector");

	vector<KeyPoint> keypoints;
	blob_detector->detect(hsvBLUE, keypoints);
	if(keypoints.size()>0)
		cv::circle(colorShow, keypoints[0].pt, 5, Scalar(255,255,255), 5, 8, 0);

	blob_detector->detect(hsvRED, keypoints);

	if(keypoints.size()>0)
		cv::circle(colorShow, keypoints[0].pt, 5, Scalar(50,255,100), 5, 8, 0);





	imshow("HSV",hsvRED);

	//Nome della finestra da creare
	namedWindow( "Processing", CV_WINDOW_AUTOSIZE|CV_WINDOW_OPENGL );
	//createTrackbar( "Angle", "Processing", &angleT, 360,  NULL);//OK tested
	//createTrackbar( "Dist min", "Processing", &distT, 200,  NULL);//OK tested
	//createTrackbar( "Dist max", "Processing", &dist2T, 200,  NULL);//OK tested
	//namedWindow( "Repro", CV_WINDOW_AUTOSIZE );

	//Matrice dei punti di depth riproiettati
	cv::Mat reprojected=cv::Mat::zeros(480,640,CV_8UC3);

	const XnRGB24Pixel* pImageRow;
	const XnRGB24Pixel* pPixel;
	pImageRow = imageMD.RGB24Data();

	//Ciclo ogni elemento del depth buffer in modo da eliminare (porre a zero più precisamente)
	//tutti i punti che si trovando al di fuori di un offset dalla Z del punto in cui è stata
	//individuata la mano

	for (XnUInt y = 0; y < depthMD.YRes(); ++y)
	{

		pPixel = pImageRow;

		for (XnUInt x = 0; x < depthMD.XRes(); ++x, ++pDepth,++pPixel)
		{

			if (*pDepth>g_pDrawer->pt3D.Z+50 || *pDepth<g_pDrawer->pt3D.Z-50)
			{
				*pDepth=0;
			}
			else
			{
				Point3_<uchar>* p = reprojected.ptr<Point3_<uchar> >(y,x);
				*pDepth=1000;
				p->x=pPixel->nBlue;
				p->y=pPixel->nGreen;
				p->z=pPixel->nRed;
			}

		}

		pImageRow+=640;

	}

	//	//REPROJECTION
	//	XnDepthPixel* pDepthIterator = depthMD.WritableData();
	//	int depth;
	//	int ppX;
	//	int ppY;
	//	int ppZ;
	//
	//	int ppXp;
	//	int ppYp;
	//	int ppZp;
	//
	//	Eigen::Vector3d p;
	//	Eigen::Vector3d pprimo;
	//
	//	int P2D_rgbX;
	//	int P2D_rgbY;
	//
	//	const XnRGB24Pixel* pImageRow;
	//	const XnRGB24Pixel* pPixel;
	//	pImageRow = imageMD.RGB24Data();
	//
	//
	//	for(int i=0;i<480;i++)
	//	{
	//		pPixel = pImageRow;
	//
	//		for(int j=0;j<640;j++,++pDepthIterator,++pPixel)
	//		{
	//
	//			depth=*pDepthIterator;
	//			if(depth!=0 || 1)
	//			{
	//				ppX = (j - cx_d)*depth/fx_d;	//x
	//				ppY = (i - cy_d)*depth/fy_d;	//y
	//				ppZ = depth; 					//z
	//
	//				//pprimo=R.dot(p):
	//
	//				ppXp = (ppX*R[0]+ppY*R[1]+ppZ*R[2]) + T[0];
	//				ppYp = (ppX*R[3]+ppY*R[4]+ppZ*R[5]) + T[1];
	//				ppZp = (ppX*R[6]+ppY*R[7]+ppZ*R[8]) + T[2];
	//
	//				P2D_rgbX = (ppXp * fx_rgb / ppZp) + cx_rgb;
	//				P2D_rgbY = (ppYp * fy_rgb / ppZp) + cy_rgb;
	//
	//				//if(P2D_rgbY<480 && P2D_rgbX<640 && P2D_rgbY!=0 && P2D_rgbX!=0 && P2D_rgbY>0 && P2D_rgbX>0)
	//				if(ppYp<480 && ppXp<640 && ppYp!=0 && ppXp!=0 && ppYp>0 && ppXp>0)
	//				{
	//					//nerd way
	//					//reprojected.ptr<uchar>(P2D_rgbY)[P2D_rgbX]=255;
	//					//easy way
	//					//reprojected.at<uchar>(P2D_rgbY,P2D_rgbX)=255;
	//					//Vec3b v = reprojected.at<uchar>(P2D_rgbY,P2D_rgbX);
	//
	//					Point3_<uchar>* p = reprojected.ptr<Point3_<uchar> >(ppYp,ppXp);
	//					p->x=depth;
	//					p->y=depth;
	//					p->z=depth;
	//				}
	//
	//			}
	//
	//
	//		}
	//		pImageRow +=640;
	//	}


	//Istanzio il punto2D nella finestra in cui ho individuato la mano
	cv::Point handPos(g_pDrawer->ptProjective.X, g_pDrawer->ptProjective.Y);
	//Disegno il cerchio nel punto in cui ho individuato la mano
	cv::circle(depthshow, handPos, 5, cv::Scalar(0xffff), 5, 8, 0);
	cv::circle(colorShow, handPos, 5, cv::Scalar(0xffff), 5, 8, 0);







	/*
	Mat src_gray;
	//Fattore di soglia
	int thresh = 10;
	//Colore random
	RNG rng(12345);
	//Effettuo una sfocatura dell'immagine di depth per diminuire il rumore
	blur( depthshow, src_gray, Size(6,6) );

	//Destinazione dell'operatore di soglia
	Mat threshold_output;
	//Contorni
	vector<vector<Point> > contours;
	//Gerarchia dei contorni
	vector<Vec4i> hierarchy;
	//Effettuo la soglia
	threshold( src_gray, threshold_output, thresh, 255, THRESH_BINARY );
	//Trovo i contorni
	findContours( threshold_output, contours, hierarchy, CV_RETR_TREE, CV_CHAIN_APPROX_SIMPLE, Point(0, 0) );

	//Oggetti in cui salvare l'hull
	vector<vector<int> > hullsI (contours.size()); 		//di int
	vector<vector<Point> >	hullsP (contours.size() );	//di Point
	vector<vector<Vec4i> > 	defects (contours.size() );	//Defezioni (punti in cui mi aspetto di trovare le dita)

	//per ogni contorno rilevato, esamino il guscio (l'hull)
	//e cerco i difetti di convessità
	for( int i = 0; i < (int)contours.size(); i++ )
	{
		convexHull(contours[i], hullsI[i], false, false);
		convexHull(contours[i], hullsP[i], false, true);
		if (contours[i].size() >3 )
		{
			convexityDefects(contours[i], hullsI[i], defects[i]);
		}
	}

	//Matrice di destinazione
	Mat drawing = Mat::zeros( threshold_output.size(), CV_8UC3 );
	//vettore di quattro elementi di tipi Vec4i
	vector <Vec4i> biggest	(4);
	Vec4i initVec(0, 0, 0, 0);
	//Inizializzo ogni vettore contenuti in "biggest" al vettore nullo initVec(0,0,0,0)
	biggest[0] = initVec;
	biggest[1] = initVec;
	biggest[2] = initVec;
	biggest[3] = initVec;

	//std::cout << "{";
	for( int i = 0; i < (int)defects.size(); i++)			//TUTTI I DIFETTI
	{
		//std::cout << "[";
		for(int j = 0; j < (int)defects[i].size(); j++)	//TUTTI I DIFETTI PER QUEL CONTORNO
		{
			//std::cout << "(" << defects[i][j][0] << " " << defects[i][j][1] << " " << defects[i][j][2] << " " << defects[i][j][3] << " " << std::endl;
			Vec4i defect(defects[i][j]);			//Inizializzo un vettore al i-j-esimo difetto
			Point start(contours[i][defect[0]]);	//Punto di inizio del difetto
			Point end(contours[i][defect[1]]);		//Punto di fine del difetto
			Point far(contours[i][defect[2]]);		//Punto di massima distanza dal contorno
			int tmp = defect[3]; 					//distanza minima necessaria per rimuovere punti troppo vicini
			if(tmp > 1700)							//tmp=defects[3] contiene la distanza del difetto dal bordo
			{
				Point v1 = start - handPos;
				Point v2 = far - handPos;
				Point v = start - far;
				float angle = acos( (v1.x*v2.x + v1.y*v2.y) / (norm(v1) * norm(v2)) );
				float dist2 = sqrt(v2.x*v2.x + v2.y*v2.y);
				float dist = sqrt(v.x*v.x + v.y*v.y);
				//std::cout << "Dist: " << dist << v.x << v.y << std::endl;

				//line(drawing, start, handPos, Scalar( 255, 255, 0 ), 1);
				//line(drawing, far, handPos, Scalar( 255, 255, 0 ), 1);
				//line(drawing, far, start, Scalar( 255, 0, 255 ), 1);
				//circle(drawing, far, 2, Scalar( 0, 0, 255 ), 5, 8, 0);		//difetto


				if(angle*180/3.14 < angleT && dist2 < dist2T && dist > distT)
				{

					circle(drawing, start, 2, Scalar( 0, 255, 255 ), 5, 8, 0);		//fingertip
					char sentence[10];
					sprintf(sentence, "P: %d", j);
					addText(drawing,sentence, start+Point(10,-10),dafont);
					//circle(drawing, end, 2, Scalar( 0, 0, 255 ), 5, 8, 0);		//inutile
				}
			}
		}
		//std::cout << "]" << std::endl;
	}



	//std::cout << "}" << std::endl;

	//Disegno i contorni della mano che ho elaborato precedentemente
	for( int i = 0; i < (int)contours.size(); i++ )
	{
		drawContours( drawing, contours, i, Scalar( 0, 255, 0 ), 1, 8, vector<Vec4i>(), 0, Point() );
		//drawContours( drawing, hullsP, i, Scalar( 255, 0, 0 ), 1, 8, vector<Vec4i>(), 0, Point() );
	}

	//Istanzio e mostro la finestra di in cui ho disegnato i contorni e i difetti
	//putText(drawing, "Fingertips", Point(10,20), FONT_HERSHEY_SIMPLEX, 0.5f, Scalar(255,255,255)); //perchè non è una 8bit a 3 canali

	//DISEGNO I PUNTI DEI VERTICI DEL BEST PALM

	for(int i=0;i<PALM_VERTEX;i++)
	{
		cv::Point vertex(palmo->puntiProiettati[i][0],palmo->puntiProiettati[i][1]);
		circle(drawing, vertex, 1, Scalar( 255, 0, 255 ), 1, 8, 0);
	}

	imshow( "Processing", drawing );
	 */

	//PSO HERE
	/*
	for(int i=0;i<GENER;i++)
	{
		pso_compute_error();
		pso_best();
		pso_update();
	}

	for(int i=0;i<partDIM;i++)
	{
		palmo->posa[i]=(float)best.posa[i];
	}
	palmo->Update();
	printf("X: %d\t Y: %d\t Ext: %d\t Err: %d\t\n",best.posa[0],best.posa[1],best.punti_esterni, best.errore_posa);
	best.errore_posa=99999;
	pso_perturba_stormo();
	 */

	//cvtColor( reprojected, reprojected, CV_RGB2GRAY );
	//blur( reprojected, reprojected, Size(3,3) );
	//threshold( reprojected, reprojected, thresh, 255, THRESH_BINARY );
	//putText(reprojected, "RGB Image + Blur pass + Threshold", Point(10,20), FONT_HERSHEY_SIMPLEX, 0.5f, Scalar(255,255,255)); //perchè non è una 8bit a 3 canali
	//imshow( "Repro", reprojected );

}

void glutDisplay (void*)
{
	glEnable(GL_DEPTH_TEST);
	glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0,640,-480,0,0,500);

	glPushMatrix();


	glTranslatef(0,0,-100);
	/*
	glTranslatef(320,-240,0);
	glRotatef(trackX,1,0,0);
	glRotatef(trackY,0,1,0);
	glRotatef(trackZ,0,0,1);
	glTranslatef(-320,240,0);
	 */

	Palm prova;
	prova.posa[0]=palmo->posa[0];
	prova.posa[1]=-palmo->posa[1];
	prova.posa[2]=palmo->posa[2];
	prova.posa[3]=palmo->posa[3];
	prova.posa[4]=palmo->posa[4];
	prova.posa[5]=palmo->posa[5];
	prova.Update();

	//palmo->posa[PALMy]=-palmo->posa[PALMy];
	//palmo->Update();
	prova.Draw();

	glBegin(GL_LINES);
	glVertex3f(prova.posa[PALMx],prova.posa[PALMy],prova.posa[PALMz]);
	glVertex3f(g_pDrawer->ptProjective.X,-g_pDrawer->ptProjective.Y,palmo->posa[PALMz]);
	glEnd();
	/*
	palmo->Update();
	palmo->posa[PALMroty]=prova*3.14/180;
	palmo->posa[PALMx]=320;
	palmo->posa[PALMy]=240;
	palmo->posa[PALMy]=-palmo->posa[PALMy];
	 */

	glPopMatrix();

	glPushMatrix();
	glTranslatef(50,-430,-50);
	glRotatef(trackX,1,0,0);
	glRotatef(trackY,0,1,0);
	glRotatef(trackZ,0,0,1);
	DrawAxes(150.0f);

	glPopMatrix();
	//init_mano(stato_mano);
	//mano->setState(stato_mano);

	glFlush();

}

//*************************************************************************************
//IDLE FUNC
//*************************************************************************************
void glutIdle (void)
{
	if (g_bQuit) {
		CleanupExit();
	}

	// Display the frame, do nothing
	glutPostRedisplay();
}
//*************************************************************************************
//*************************************************************************************


//*************************************************************************************
//User input
//*************************************************************************************
void glutKeyboard (unsigned char key, int x, int y)
{
	switch (key)
	{
	case 27:
		// Exit
		CleanupExit();
		break;
	case'p':
		// Toggle pause
		g_bPause = !g_bPause;
		break;
	case 'd':
		// Toggle drawing of the depth map
		g_bDrawDepthMap = !g_bDrawDepthMap;
		g_pDrawer->SetDepthMap(g_bDrawDepthMap);
		break;
	case 'f':
		g_bPrintFrameID = !g_bPrintFrameID;
		g_pDrawer->SetFrameID(g_bPrintFrameID);
		break;
	case 's':
		// Toggle smoothing
		if (g_fSmoothing == 0)
			g_fSmoothing = 0.1;
		else
			g_fSmoothing = 0;
		g_HandsGenerator.SetSmoothing(g_fSmoothing);
		break;
	case 'e':
		// end current session
		g_pSessionManager->EndSession();
		break;
	}
}
//*************************************************************************************
//*************************************************************************************

//*************************************************************************************
//GL init parameters (non utilizzata)
//*************************************************************************************
//void glInit (int * pargc, char ** argv)
//{
//	glutInit(pargc, argv);
//	glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
//	glutInitWindowSize(GL_WIN_SIZE_X, GL_WIN_SIZE_Y);
//	glutCreateWindow ("PrimeSense Nite Point Viewer");
//	glutFullScreen();
//	glutSetCursor(GLUT_CURSOR_NONE);
//
//	glutKeyboardFunc(glutKeyboard);
//	glutDisplayFunc(glutDisplay);
//	glutIdleFunc(glutIdle);
//
//	glDisable(GL_DEPTH_TEST);
//	glEnable(GL_TEXTURE_2D);
//
//	glEnableClientState(GL_VERTEX_ARRAY);
//	glDisableClientState(GL_COLOR_ARRAY);
//}
//*************************************************************************************
//*************************************************************************************

//CALLBACK PER IL MOUSE
void onMouse( int event, int x, int y, int, void* )
{
	if( event == CV_EVENT_LBUTTONDOWN )

	{
		printf("CLICK AT (%d, %d)\n",x,y);
		int blue=0;//olorShow.ptr<uchar>(y)[3*x+0];
		int green=0;//colorShow.ptr<uchar>(y)[3*x+1];
		int red=0;//colorShow.ptr<uchar>(y)[3*x+2];

		int averagingfactor=0;

		for(int yy=y-1;yy<=y+1;yy++)
		{
			for(int xx=x-1;xx<=x+2;xx++)
			{
				blue=blue+(int)colorShow.ptr<uchar>(yy)[3*xx+0];
				green=green+(int)colorShow.ptr<uchar>(yy)[3*xx+1];
				red=red+(int)colorShow.ptr<uchar>(yy)[3*xx+2];
				averagingfactor++;
			}
		}

		blue=blue/averagingfactor;
		green=green/averagingfactor;
		red=red/averagingfactor;

		printf("Averaging is: %d %d %d \n",red,green,blue);


		rectangle(colorShow,Point(x-4,y-4),Point(x+4,y+4),Scalar(0,0,255),-1);

	}



}


//*************************************************************************************
//MAIN FUNC
//*************************************************************************************
int main(int argc, char ** argv)
{
	//init_mano(stato_mano);
	//mano = new Hand(stato_mano);

	palmo=new Palm();

	printf("PSO INIT\n");
	pso_init();
	printf("PSO OK\n");

	//Variabile di stato per il kinect
	XnStatus rc = XN_STATUS_OK;
	//Enumeratore per gli errori
	xn::EnumerationErrors errors;

	// Inizializzazione OPENNI
	rc = g_Context.InitFromXmlFile(SAMPLE_XML_PATH, g_ScriptNode, &errors);
	CHECK_ERRORS(rc, errors, "InitFromXmlFile");
	CHECK_RC(rc, "InitFromXmlFile");
	//=============================================================================

	//Cerco i nodi definiti nell'xml (fondamentali detph e image)
	rc = g_Context.FindExistingNode(XN_NODE_TYPE_IMAGE, g_ImageGenerator);
	CHECK_RC(rc, "Find image generator");
	rc = g_Context.FindExistingNode(XN_NODE_TYPE_DEPTH, g_DepthGenerator);
	CHECK_RC(rc, "Find depth generator");
	rc = g_Context.FindExistingNode(XN_NODE_TYPE_HANDS, g_HandsGenerator);
	CHECK_RC(rc, "Find hands generator");
	rc = g_Context.FindExistingNode(XN_NODE_TYPE_GESTURE, g_GestureGenerator);
	CHECK_RC(rc, "Find gesture generator");
	//=============================================================================

	//Inizio la registrazione delle callbacks
	XnCallbackHandle h;
	if (g_HandsGenerator.IsCapabilitySupported(XN_CAPABILITY_HAND_TOUCHING_FOV_EDGE))
	{
		g_HandsGenerator.GetHandTouchingFOVEdgeCap().RegisterToHandTouchingFOVEdge(TouchingCallback, NULL, h);
	}

	XnCallbackHandle hGestureIntermediateStageCompleted, hGestureProgress, hGestureReadyForNextIntermediateStage;
	g_GestureGenerator.RegisterToGestureIntermediateStageCompleted(GestureIntermediateStageCompletedHandler, NULL, hGestureIntermediateStageCompleted);
	g_GestureGenerator.RegisterToGestureReadyForNextIntermediateStage(GestureReadyForNextIntermediateStageHandler, NULL, hGestureReadyForNextIntermediateStage);
	g_GestureGenerator.RegisterGestureCallbacks(NULL, GestureProgressHandler, NULL, hGestureProgress);
	//=============================================================================

	// reate NITE objects
	g_pSessionManager = new XnVSessionManager;
	rc = g_pSessionManager->Initialize(&g_Context, "Click,Wave", "RaiseHand");
	CHECK_RC(rc, "SessionManager::Initialize");
	g_pSessionManager->RegisterSession(NULL, SessionStarting, SessionEnding, FocusProgress);
	g_pDrawer = new XnVPointDrawer(20, g_DepthGenerator);
	g_pFlowRouter = new XnVFlowRouter;
	g_pFlowRouter->SetActive(g_pDrawer);
	g_pSessionManager->AddListener(g_pFlowRouter);
	g_pDrawer->RegisterNoPoints(NULL, NoHands);
	g_pDrawer->SetDepthMap(g_bDrawDepthMap);
	//=============================================================================

	// Initialization done. Start generating
	rc = g_Context.StartGeneratingAll();
	CHECK_RC(rc, "StartGenerating");
	//=============================================================================

	//Mat opencv in cui disegnare la depth
	cv::Mat depthshow;
	//Tasto premuto
	int key_pre = 0 ;

	//Riproietto la depth image sulla rgbimage
	g_DepthGenerator.GetAlternativeViewPointCap().SetViewPoint(g_ImageGenerator);


	//<><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><>
	//<><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><>
	//								Main loop del programma
	//<><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><>
	//<><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><>

	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGB|GLUT_DOUBLE|GLUT_DEPTH);

	glClearColor(0,0,0,0);
	glClearDepth(1.0f);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);

	// OpenGL init
	//		glutInit(&argc, argv);
	//		glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
	//		glutInitWindowSize(GL_WIN_SIZE_X, GL_WIN_SIZE_Y);
	//		glutCreateWindow ("GLUT Window");
	//glutFullScreen();
	//glutSetCursor(GLUT_CURSOR_NONE);
	//glutKeyboardFunc(glutKeyboard);
	//glutDisplayFunc(glutDisplay);
	//glutIdleFunc(glutIdle);

	//		glEnable(GL_DEPTH_TEST);
	//		glEnable(GL_TEXTURE_2D);
	// Per frame code is in glutDisplay
	//glutMainLoop();

	const string windowName = "OpenGL Sample";

	/*
	namedWindow(windowName,CV_WINDOW_OPENGL | CV_WINDOW_AUTOSIZE);
	createTrackbar( "X", windowName, &trackX, 360,  NULL);//OK tested
	createTrackbar( "Y", windowName, &trackY, 360,  NULL);//OK tested
	createTrackbar( "Z", windowName, &trackZ, 360,  NULL);//OK tested
	createTrackbar( "rotY", windowName, &prova, 360,  NULL);//OK tested
	resizeWindow(windowName, 640, 480);
	setOpenGlDrawCallback(windowName, glutDisplay);
	 */

	//FINESTRA RGB, REGISTRO LA CALLBACK
	namedWindow("RGB");
	setMouseCallback("RGB",	onMouse,0);


	for( ; ; )
	{
		updateWindow(windowName);

		//Terminare il ciclo se ho premuto esc precedentemente ESC
		if (key_pre == 27) break ;

		//Configuro la modalità dell'output
		XnMapOutputMode mode;
		g_DepthGenerator.GetMapOutputMode(mode);

		//Aggiorno tutti i contenti del kinect (RGB+depth)
		g_Context.WaitAnyUpdateAll();

		//Acquisisco i metadati
		// depthMD -> info sulla depth
		// imageMD -> info sugli RGB
		g_DepthGenerator.GetMetaData(depthMD);
		g_ImageGenerator.GetMetaData(imageMD);

		//Aggiorno il tree di NITE
		g_pSessionManager->Update(&g_Context);

		//puntatore al buffer scrivibile delle depth
		XnDepthPixel* pDepth = depthMD.WritableData();
		//Mano trovata, chiamo la routine apposita
		if(g_pDrawer->handFound) hand_found(depthshow,pDepth);

		//Matrice dei punti di depth
		cv::Mat depth16bit(480,640,CV_16SC1,(unsigned short*)depthMD.WritableData());
		depth16bit.convertTo(depthshow, CV_8UC1, 0.025);





		//COLORED IMAGE BGR
		//Matrice dei punti pronta per essere disegnata
		//colorShow è global



		imshow("RGB", colorShow);

		RGBkinect2OpenCV(colorShow);


		//=========================================
		//Disegno le finestre
		//=========================================

		//RGB


		//addText(colorShow,"RGB Image", Point(10,20),dafont);


		//Depth
		//namedWindow("Depth");

		//putText(depthshow, "Depth Image", Point(10,20), FONT_HERSHEY_SIMPLEX, 0.5f, Scalar(255,255,255)); //perchè non è una 8bit a 3 canali, non è saggio mostrare questa label
		//imshow("Depth", depthshow);

		//Mat fake=cv::Mat::zeros(480,640,CV_8UC3);



		//=========================================
		//Verifica input utente
		//=========================================
		key_pre = cv::waitKey(33);


	}

	CleanupExit();
	return 0;
}
