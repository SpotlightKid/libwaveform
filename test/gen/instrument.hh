//Set of C++ wave table function
//©Romain Michon (rmichon@ccrma.stanford.edu), 2011
//licence: STK-4.3

#include <stdio.h>
#include <stdlib.h>

#define TABLE_SIZE 1024

#ifndef __instrument_h__
#define __instrument_h__

//******************************************************
//functions for parameters lookup and interpolation
//******************************************************

class LookupTable
{
public:
	LookupTable();
	void set(double *points, int num_points);
	double getValue(double x);

protected:

	// Note: Actual array size is 2*m_nPoints;
	double *m_Points;
	int m_nPoints;
};

//********************************
//stick for modal synthesis
//********************************
/*
float readMarmstk1(int index){
	static float marmstk1[TABLE_SIZE/4] = {
		0.000579833984375, -0.003417968750000, 0.015930175781250, -0.037689208984375, 0.062866210937500,
		0.168640136718750, -0.226287841796875, -0.020233154296875, 0.017120361328125, 0.032745361328125,
		0.028198242187500, -0.065704345703125, 0.102355957031250, -0.135375976562500, -0.088378906250000,
		0.135375976562500, 0.036987304687500, 0.030181884765625, -0.023498535156250, -0.050872802734375,
		0.120574951171875, -0.223419189453125, 0.235260009765625, -0.296081542968750, 0.384582519531250,
		-0.363708496093750, 0.206207275390625, 0.076873779296875, -0.262420654296875, 0.306579589843750,
		-0.349090576171875, 0.359161376953125, -0.304809570312500, 0.156860351562500, 0.022552490234375,
		-0.063598632812500, 0.017425537109375, 0.024505615234375, -0.016296386718750, -0.056304931640625,
		0.093536376953125, -0.108825683593750, 0.215484619140625, -0.354858398437500, 0.316925048828125,
		-0.164672851562500, 0.028594970703125, 0.095001220703125, -0.165679931640625, 0.218811035156250,
		-0.239105224609375, 0.182830810546875, -0.026275634765625, -0.016601562500000, -0.042175292968750,
		0.080566406250000, -0.123352050781250, 0.071563720703125, -0.021514892578125, -0.000488281250000,
		0.080139160156250, -0.188354492187500, 0.230712890625000, -0.172271728515625, 0.033325195312500,
		0.111236572265625, -0.127532958984375, 0.118682861328125, -0.136383056640625, 0.068878173828125,
		0.041931152343750, -0.126129150390625, 0.134155273437500, -0.024902343750000, -0.094726562500000,
		0.136840820312500, -0.140930175781250, 0.123962402343750, -0.080383300781250, -0.033691406250000,
		0.167541503906250, -0.194976806640625, 0.151489257812500, -0.042388916015625, -0.028625488281250,
		0.030853271484375, -0.079559326171875, 0.071166992187500, 0.026977539062500, -0.075714111328125,
		0.110107421875000, -0.076507568359375, -0.043426513671875, 0.063110351562500, -0.099487304687500,
		0.137664794921875, -0.086181640625000, 0.047119140625000, 0.022491455078125, -0.092956542968750,
		0.070709228515625, -0.036560058593750, -0.004943847656250, 0.051208496093750, -0.042541503906250,
		0.042114257812500, -0.024414062500000, -0.039916992187500, 0.082580566406250, -0.094451904296875,
		0.039459228515625, 0.037048339843750, -0.061218261718750, 0.080810546875000, -0.070159912109375,
		0.037139892578125, 0.008789062500000, -0.078094482421875, 0.094024658203125, -0.048431396484375,
		0.009643554687500, 0.020263671875000, -0.032379150390625, 0.021820068359375, -0.021270751953125,
		-0.033203125000000, 0.102172851562500, -0.089721679687500, 0.052856445312500, -0.001495361328125,
		-0.070404052734375, 0.109436035156250, -0.104156494140625, 0.116302490234375, -0.074310302734375,
		-0.004425048828125, 0.061309814453125, -0.090698242187500, 0.056732177734375, -0.015380859375000,
		-0.010406494140625, 0.019622802734375, 0.000213623046875, -0.017272949218750, 0.065399169921875,
		-0.119842529296875, 0.105499267578125, -0.051391601562500, -0.024383544921875, 0.085968017578125,
		-0.099731445312500, 0.121948242187500, -0.098876953125000, 0.038085937500000, 0.034362792968750,
		-0.071441650390625, 0.039550781250000, -0.017272949218750, -0.001708984375000, 0.031402587890625,
		-0.027740478515625, 0.013183593750000, 0.013488769531250, -0.083831787109375, 0.103637695312500,
		-0.061645507812500, 0.026947021484375, 0.036499023437500, -0.078735351562500, 0.089294433593750,
		-0.090393066406250, 0.034820556640625, 0.019500732421875, -0.070129394531250, 0.102569580078125,
		-0.070922851562500, 0.039672851562500, 0.020507812500000, -0.078674316406250, 0.065002441406250,
		-0.045806884765625, 0.027801513671875, 0.012115478515625, -0.018829345703125, 0.015594482421875,
		-0.010772705078125, -0.042938232421875, 0.062103271484375, -0.032745361328125, 0.004791259765625,
		0.028137207031250, -0.067687988281250, 0.078094482421875, -0.063049316406250, 0.039215087890625,
		0.012359619140625, -0.052337646484375, 0.074401855468750, -0.063629150390625, 0.034362792968750,
		0.013732910156250, -0.044189453125000, 0.042419433593750, -0.047210693359375, 0.019897460937500,
		0.020538330078125, -0.039825439453125, 0.048675537109375, -0.025726318359375, -0.016998291015625,
		0.038482666015625, -0.056060791015625, 0.061584472656250, -0.014343261718750, -0.023101806640625,
		0.051849365234375, -0.069854736328125, 0.043853759765625, -0.016662597656250, 0.002380371093750,
		0.033721923828125, -0.039733886718750, 0.021148681640625, -0.010375976562500, 0.000000000000000,
		0.000000000000000, 0.000000000000000, -0.000030517578125, 0.000030517578125, 0.000000000000000,
		0.000000000000000, -0.000030517578125, -0.000030517578125, 0.000030517578125, 0.000030517578125,
		-0.000061035156250, 0.000000000000000, 0.000000000000000, 0.000000000000000, 0.000030517578125,
		-0.000030517578125, 0.000000000000000, 0.000030517578125, -0.000030517578125, 0.000000000000000,
		0.000061035156250, -0.000061035156250, 0.000030517578125, 0.000000000000000, -0.000030517578125,
		0.000000000000000, 0.000061035156250, 0.000000000000000, -0.000030517578125, 0.000000000000000,
		0.000030517578125};
	return marmstk1[index];
};
*/
#endif
