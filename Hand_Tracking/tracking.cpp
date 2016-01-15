#include "stdafx.h"

#include <iostream>
#include <fstream>
#include <stdio.h>
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"

using namespace cv;
using namespace std;

//Contadores para validacao de gestos
int iGarraCima = 0;
int iGarraBaixo = 0;
int iGarraAbre = 0;
int iGarraFecha = 0;
int iGarraNenhum = 0;

int iCarroFrente = 0;
int iCarroTras = 0;
int iCarroEsquerda = 0;
int iCarroDireita = 0;
int iCarroNenhum = 0;

void zeraContadoresGarra() {
	iGarraCima = 0;
	iGarraBaixo = 0;
	iGarraAbre = 0;
	iGarraFecha = 0;
	iGarraNenhum = 0;
}

void zeraContadoresCarro() {
	iCarroFrente = 0;
	iCarroTras = 0;
	iCarroEsquerda = 0;
	iCarroDireita = 0;
	iCarroNenhum = 0;
}

int main( int argc, char** argv )
{
	setlocale(LC_ALL,"Portuguese");
	int AREA_MINIMA = 150, AREA_MAXIMA = 1500, SIZE_NOISE_REMOVER = 11, EROSION_TYPE = 0, TEMPO_MINIMO = 16;
    VideoCapture cap(0); //capture the video from web cam

    if (!cap.isOpened())  // if not success, exit program
    {
         cout << "Webcam não encontrada. Verifique se há uma webcam conectada." << endl;
         return -1;
    }

    cv::namedWindow("Marcador Azul", CV_WINDOW_NORMAL);
	cv::namedWindow("Marcador Vermelho", CV_WINDOW_NORMAL);
	cv::namedWindow("Area dos Marcadores", CV_WINDOW_NORMAL);

	//---------------Blue HSV (em casa com a luz acesa)
	int iBlueLowH = 83;
	int iBlueHighH = 147;

	int iBlueLowS = 77; 
	int iBlueHighS = 255;

	int iBlueLowV = 36;
	int iBlueHighV = 255;
	
	//---------------Red HSV (em casa com a luz acesa)
	int iRedLowH = 0;
	int iRedHighH = 179;

	int iRedLowS = 186; 
	int iRedHighS = 255;

	int iRedLowV = 20;
	int iRedHighV = 255;

	//Trackbars na janela "Marcador Azul" para Blue Marker
	cvCreateTrackbar("MatizMin", "Marcador Azul", &iBlueLowH, 179); //Hue (0 - 179)
	cvCreateTrackbar("MatizMax", "Marcador Azul", &iBlueHighH, 179);

	cvCreateTrackbar("SatMin", "Marcador Azul", &iBlueLowS, 255); //Saturation (0 - 255)
	cvCreateTrackbar("SatMax", "Marcador Azul", &iBlueHighS, 255);

	cvCreateTrackbar("ValorMin", "Marcador Azul", &iBlueLowV, 255);//Value (0 - 255)
	cvCreateTrackbar("ValorMax", "Marcador Azul", &iBlueHighV, 255);
	
	//Trackbars na janela "Marcador Vermelho" para Red Marker
	cvCreateTrackbar("MatizMin", "Marcador Vermelho", &iRedLowH, 179);
	cvCreateTrackbar("MatizMax", "Marcador Vermelho", &iRedHighH, 179);

	cvCreateTrackbar("SatMin", "Marcador Vermelho", &iRedLowS, 255);
	cvCreateTrackbar("SatMax", "Marcador Vermelho", &iRedHighS, 255);

	cvCreateTrackbar("ValorMin", "Marcador Vermelho", &iRedLowV, 255);
	cvCreateTrackbar("ValorMax", "Marcador Vermelho", &iRedHighV, 255);

	//cvCreateTrackbar("NoiseRemover", "Area dos Marcadores", &SIZE_NOISE_REMOVER, 100);
	//cvCreateTrackbar("ErosionType", "Area dos Marcadores", &EROSION_TYPE, 2);
	cvCreateTrackbar("AreaMin", "Area dos Marcadores", &AREA_MINIMA, 1000);
	cvCreateTrackbar("AreaMax", "Area dos Marcadores", &AREA_MAXIMA, 3000);

	string sClawGesture = "", sClawGestureArduino = "NN", sClawOld = "";
	string sCarGesture = "", sCarGestureArduino = "NN", sCarOld = "";

	//Capture a temporary image from the camera
	Mat imgTmp;
	cap.read(imgTmp); 

	//Create a black image with the size as the camera output
	Mat imgLines = Mat::zeros(imgTmp.size(), CV_8UC3);

	//Contours dos marcadores
	cv::vector<cv::vector<cv::Point> > contoursRed, contoursBlue;

	//Arquivo que contem os movimentos para o arduino
	ofstream file;
    while (true) {
        Mat imgOriginal;

        bool bSuccess = cap.read(imgOriginal); // read a new frame from video
		flip(imgOriginal, imgOriginal, 1);

        if (!bSuccess) { //if not success, break loop
             cout << "Erro ao capturar vídeo da webcam" << endl;
             break;
        }

		Mat imgHSV;
		
		cv::cvtColor(imgOriginal, imgHSV, COLOR_BGR2HSV); //Convert the captured frame from BGR to HSV
	
		Mat imgThresholdedBlue, imgThresholdedRed;

		//Verifica se os valores HSV estao entre um valor e outro (High e Low)
		cv::inRange(imgHSV, Scalar(iBlueLowH, iBlueLowS, iBlueLowV), Scalar(iBlueHighH, iBlueHighS, iBlueHighV), imgThresholdedBlue); //Threshold the image
		cv::inRange(imgHSV, Scalar(iRedLowH, iRedLowS, iRedLowV), Scalar(iRedHighH, iRedHighS, iRedHighV), imgThresholdedRed);
      
		//morphological opening (removes small objects from the foreground)
		cv::erode(imgThresholdedBlue, imgThresholdedBlue, cv::getStructuringElement(EROSION_TYPE, Size(SIZE_NOISE_REMOVER, SIZE_NOISE_REMOVER)) );
		cv::dilate( imgThresholdedBlue, imgThresholdedBlue, cv::getStructuringElement(EROSION_TYPE, Size(SIZE_NOISE_REMOVER, SIZE_NOISE_REMOVER)) ); 

		//morphological closing (removes small holes from the foreground)
		cv::dilate( imgThresholdedBlue, imgThresholdedBlue, cv::getStructuringElement(EROSION_TYPE, Size(SIZE_NOISE_REMOVER, SIZE_NOISE_REMOVER)) ); 
		cv::erode(imgThresholdedBlue, imgThresholdedBlue, cv::getStructuringElement(EROSION_TYPE, Size(SIZE_NOISE_REMOVER, SIZE_NOISE_REMOVER)) );

		cv::erode(imgThresholdedRed, imgThresholdedRed, cv::getStructuringElement(EROSION_TYPE, Size(SIZE_NOISE_REMOVER, SIZE_NOISE_REMOVER)) );
		cv::dilate( imgThresholdedRed, imgThresholdedRed, cv::getStructuringElement(EROSION_TYPE, Size(SIZE_NOISE_REMOVER, SIZE_NOISE_REMOVER)) ); 

		cv::dilate( imgThresholdedRed, imgThresholdedRed, cv::getStructuringElement(EROSION_TYPE, Size(SIZE_NOISE_REMOVER, SIZE_NOISE_REMOVER)) ); 
		cv::erode(imgThresholdedRed, imgThresholdedRed, cv::getStructuringElement(EROSION_TYPE, Size(SIZE_NOISE_REMOVER, SIZE_NOISE_REMOVER)) );

		//Find contours
		findContours(imgThresholdedBlue, contoursBlue, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE); //talvez remover o point
		findContours(imgThresholdedRed, contoursRed, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE); //talvez remover o point
		
		//Calculate the moments of the thresholded image
		Moments blueMoments = cv::moments(imgThresholdedBlue);
		Moments redMoments = cv::moments(imgThresholdedRed);
		
		/// Get the moments
		vector<Moments> momentsBlue(contoursBlue.size()), momentsRed(contoursRed.size());
		for(int i = 0; i < contoursBlue.size(); i++) {
			momentsBlue[i] = moments(contoursBlue[i]);
		}
		
		for(int i = 0; i < contoursRed.size(); i++) {
			momentsRed[i] = moments(contoursRed[i]);
		}

		/// Get the mass centers
		vector<Point2f> massCentersBlue(contoursBlue.size()), massCentersRed(contoursRed.size());
		for( int i = 0; i < contoursBlue.size(); i++ ) {
			if (momentsBlue[i].m00 > AREA_MINIMA && momentsBlue[i].m00 < AREA_MAXIMA) {
				massCentersBlue[i] = Point2f(momentsBlue[i].m10 / momentsBlue[i].m00, momentsBlue[i].m01 / momentsBlue[i].m00);
			}
		}
		
		for( int i = 0; i < contoursRed.size(); i++ ) {
			if (momentsRed[i].m00 > AREA_MINIMA && momentsRed[i].m00 < AREA_MAXIMA) {
				massCentersRed[i] = Point2f(momentsRed[i].m10 / momentsRed[i].m00, momentsRed[i].m01 / momentsRed[i].m00);
			}
		}
		
		Point2f massCenterEsquerdaBlue, massCenterEsquerdaRed, massCenterDireitaBlue, massCenterDireitaRed;
		for( int i = 0; i < massCentersBlue.size(); i++ ) {
			float x = massCentersBlue[i].x;
			if (x < (imgOriginal.cols / 2)) {
				massCenterEsquerdaBlue = massCentersBlue[i];
			} else {
				massCenterDireitaBlue = massCentersBlue[i];
			}
		}
		for( int i = 0; i < massCentersRed.size(); i++ ) {
			float x = massCentersRed[i].x;
			if (x < (imgOriginal.cols / 2)) {
				massCenterEsquerdaRed = massCentersRed[i];
			} else {
				massCenterDireitaRed = massCentersRed[i];
			}
		}

		//=======================Gestos Garra
		if (massCenterEsquerdaBlue.x > 0 && massCenterEsquerdaRed.x > 0) {
			if (massCenterEsquerdaBlue.x < massCenterEsquerdaRed.x && massCenterEsquerdaBlue.y < massCenterEsquerdaRed.y) {
				iGarraCima++;
			} else if (massCenterEsquerdaBlue.x > massCenterEsquerdaRed.x && massCenterEsquerdaBlue.y < massCenterEsquerdaRed.y) {
				iGarraFecha++;
			} else if (massCenterEsquerdaBlue.x > massCenterEsquerdaRed.x && massCenterEsquerdaBlue.y > massCenterEsquerdaRed.y) {
				iGarraAbre++;
			} else if (massCenterEsquerdaBlue.x < massCenterEsquerdaRed.x && massCenterEsquerdaBlue.y > massCenterEsquerdaRed.y) {
				iGarraBaixo++;
			} else {
				iGarraNenhum++;
			}
			cv::circle(imgOriginal, Point(massCenterEsquerdaBlue.x, massCenterEsquerdaBlue.y), 20, Scalar(0,255,0), 2);
			cv::circle(imgOriginal, Point(massCenterEsquerdaRed.x, massCenterEsquerdaRed.y), 20, Scalar(0,255,0), 2);
		} else {
			iGarraNenhum++;
		}
		//Validacao dos gestos da garra para evitar falsos positivos
		if (iGarraCima == TEMPO_MINIMO) {
			sClawGestureArduino = "GC";
			sClawGesture = "Garra para Cima";
			zeraContadoresGarra();
		} else if (iGarraFecha == TEMPO_MINIMO) {
			sClawGestureArduino = "GF";
			sClawGesture = "Fecha Garra";
			zeraContadoresGarra();
		} else if (iGarraAbre == TEMPO_MINIMO) {
			sClawGestureArduino = "GA";
			sClawGesture = "Abre Garra";
			zeraContadoresGarra();
		} else if (iGarraBaixo == TEMPO_MINIMO) {
			sClawGestureArduino = "GB";
			sClawGesture = "Garra para Baixo";
			zeraContadoresGarra();
		} else if (iGarraNenhum == TEMPO_MINIMO / 2) {
			sClawGesture = "Nenhum";
			sClawGestureArduino = "NN";
			zeraContadoresGarra();
		}
		
		//=======================Gestos Carro
		if (massCenterDireitaBlue.x > 0 && massCenterDireitaRed.x > 0) {
			if (massCenterDireitaBlue.x > massCenterDireitaRed.x && massCenterDireitaBlue.y < massCenterDireitaRed.y) {
				iCarroFrente++;
			} else if (massCenterDireitaBlue.x < massCenterDireitaRed.x && massCenterDireitaBlue.y < massCenterDireitaRed.y) {
				iCarroDireita++;
			} else if (massCenterDireitaBlue.x < massCenterDireitaRed.x && massCenterDireitaBlue.y > massCenterDireitaRed.y) {
				iCarroEsquerda++;
			} else if (massCenterDireitaBlue.x > massCenterDireitaRed.x && massCenterDireitaBlue.y > massCenterDireitaRed.y) {
				iCarroTras++;
			} else {
				iCarroNenhum++;
			}
			cv::circle(imgOriginal, Point(massCenterDireitaBlue.x, massCenterDireitaBlue.y), 20, Scalar(0,255,0), 2);
			cv::circle(imgOriginal, Point(massCenterDireitaRed.x, massCenterDireitaRed.y), 20, Scalar(0,255,0), 2);
		} else {
			iCarroNenhum++;
		}

		//Validacao dos gestos da garra para evitar falsos positivos
		if (iCarroFrente == TEMPO_MINIMO) {
			sCarGestureArduino = "CF";
			sCarGesture = "Carro para Frente";
			zeraContadoresCarro();
		} else if (iCarroDireita == TEMPO_MINIMO) {
			sCarGestureArduino = "CD";
			sCarGesture = "Carro para Direita";
			zeraContadoresCarro();
		} else if (iCarroEsquerda == TEMPO_MINIMO) {
			sCarGestureArduino = "CE";
			sCarGesture = "Carro para Esquerda";
			zeraContadoresCarro();
		} else if (iCarroTras == TEMPO_MINIMO) {
			sCarGestureArduino = "CT";
			sCarGesture = "Carro para Tras";
			zeraContadoresCarro();
		} else if (iCarroNenhum == TEMPO_MINIMO / 2) {
			sCarGestureArduino = "NN";
			sCarGesture = "Nenhum";
			zeraContadoresCarro();
		}
		
		if (sClawOld != sClawGestureArduino || sCarOld != sCarGestureArduino) {
			sClawOld = sClawGestureArduino;
			sCarOld = sCarGestureArduino;

			//Escreve no arquivo
			file.open("C:\\gestos.txt");
			file << sClawGestureArduino << sCarGestureArduino;
			file.close();
		}

		int fontFace = FONT_HERSHEY_PLAIN;
		double fontScale = 1;
		int thickness = 2;
		
		cv::putText(imgOriginal, "Gesto Garra: " + sClawGesture, Point(10, 10), fontFace, fontScale, Scalar::all(255), thickness, 8);
		cv::putText(imgOriginal, "Gesto Carro: " + sCarGesture, Point(350, 10), fontFace, fontScale, Scalar::all(255), thickness, 8);

		imshow("Imagem Filtrada", imgThresholdedBlue + imgThresholdedRed); //show the thresholded image

		cv::line(imgOriginal, Point((imgOriginal.cols / 2), 0), Point((imgOriginal.cols / 2), imgOriginal.rows), Scalar(0,255,0), 2);
		imshow("Imagem Original", imgOriginal); //show the original image with lines

        if (waitKey(30) == 27) { //wait for 'esc' key press for 30ms. If 'esc' key is pressed, break loop
            cout << "Usuario pressionou ESC para terminar." << endl;
			remove("C:\\gestos.txt");
            break; 
		}
    }

   return 0;
}