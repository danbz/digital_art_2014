#include "ofApp.h"

/*

 UNTITLED DIGITAL ART (AUGMENTED HAND SERIES)
 By Golan Levin, Chris Sugrue, and Kyle McDonald
 Repository: http://github.com/CreativeInquiry/digital_art_2014
 Contact: @golan or golan@flong.com
 
 Commissioned by the Cinekid Festival, Amsterdam, October 2014, with support from the Mondriaan Fund for visual art. Developed at the Frank-Ratchye STUDIO for Creative Inquiry at Carnegie Mellon University with additional support from the Pennsylvania Council on the Arts and the Frank-Ratchye Fund for Art @ the Frontier. Concept and software development: Golan Levin, Chris Sugrue, Kyle McDonald. Software assistance: Dan Wilcox, Bryce Summers, Erica Lazrus. Conceived 2005; developed 2013-2014.
 
 Special thanks to Paulien Dresscher, Theo Watson and Eyeo Festival for encouragement, and to Dan Wilcox, Bryce Summers, Erica Lazrus, and Zachary Rispoli for their help making this project possible. Thanks to Elliot Woods and Simon Sarginson for assistance with Leap/camera calibration, and to Adam Carlucci for his helpful tutorial on using the Accelerate Framework in openFrameworks. Additional thanks to Rick Barraza and Ben Lower of Microsoft; Christian Schaller and Hannes Hofmann of Metrilus GmbH; Dr. Roland Goecke of University of Canberra; and Doug Carmean and Chris Rojas of Intel.
 
 Developed in openFrameworks (OF), a free, open-source toolkit for arts engineering. This project also uses a number of open-source addons for openFrameworks contributed by others: ofxPuppet by Zach Lieberman, based on Ryan Schmidt's implementation of As-Rigid-As-Possible Shape Manipulation by Igarashi, Moscovich & Hughes; ofxLeapMotion by Theo Watson, with assistance from Dan Wilcox; ofxCv, ofxLibdc, and ofxTiming by Kyle McDonald; ofxCvMin and ofxRay by Elliot Woods; and the ofxButterfly mesh subdivision addon by Bryce Summers.
 
 Shoutouts from @golan @chrissugrue & @kcimc: @admsyn @bla_fasel @bwycz @cinekid @CMUSchoolofArt @creativeinquiry @danomatika @elliotwoods @eyeofestival @laurmccarthy @openframeworks @PESfilm @rickbarraza @SimonsMine @theowatson @zachlieberman @workergnome
 
*/


//=============================================================================
/*  
 // AN IMPORTANT NOTE FOR COMPILING THIS PROJECT WITH LEAP on OSX:
 // In OS X, you must have this in the Run Script Build Phase of your project.
 // where the first path ../../../addons/ofxLeapMotion/ is the path to the ofxLeapMotion addon.

 cp -f ../../../addons/ofxLeapMotion/libs/lib/osx/libLeap.dylib "$TARGET_BUILD_DIR/$PRODUCT_NAME.app/Contents/MacOS/libLeap.dylib"; install_name_tool -change ./libLeap.dylib @executable_path/libLeap.dylib "$TARGET_BUILD_DIR/$PRODUCT_NAME.app/Contents/MacOS/$PRODUCT_NAME";

 // If you don't have this, you'll see an error like this in the console:

 dyld: Library not loaded: @loader_path/libLeap.dylib

*/


//=============================================================================
//
// TODO: Enable saving of GUIs into XML files.
// TODO: Make sure cursor hides/shows properly.
// TODO: Refactor ofApp.cpp/h to have a CameraAndLeapRecorder. (Not urgent)
// TODO: Check whether computing amount of leap motion works with delayed data
// TODO: Get Bryce to amend ofxButterfly so that it does not reorder triangles.
// TODO: Create option to rotate entire display view by 180.


//=============================================================================

void ofApp::setup(){
	
	// App settings
    // ofSetFrameRate(60);
    ofSetVerticalSync(false);
	ofSetLogLevel(OF_LOG_WARNING);
	ofSetCylinderResolution (16, 1, 16);
    
    // All data will be in SharedData
    string basePath = ofToDataPath("", true);
    ofSetDataPathRoot("../../../../../SharedData/");
	
	backgroundImage.loadImage("recordings/backgroundImage.png");
    
    cameraWidth		= 1024;
    cameraHeight	= 768;
	drawW			= cameraWidth/2;
    drawH			= cameraHeight/2;
    backgroundGray  = 0;
	
	imgW			= cameraWidth;
    imgH			= cameraHeight;
	currentSceneID  = 0;
    previousDir     = -1;
	
	bWorkAtHalfScale = true; // HAS to be true, 'cuz something no longer works elsewise.
	if (bWorkAtHalfScale){
		imgW		= cameraWidth/2;
		imgH		= cameraHeight/2;
	}
	
    initializeCamera();
	initializeCameraCalibration();
    exportTimer.setPeriod(5);
    
    //--------------- Setup video saver
	bRecording = false;
	currentFrameNumber = 0;
	imageSequence.resize(300);
	
	// currentFrameImg and processFrameImg are both at the camera resolution.
    currentFrameImg.allocate (cameraWidth, cameraHeight, OF_IMAGE_COLOR);
    processFrameImg.allocate (cameraWidth, cameraHeight, OF_IMAGE_COLOR);
	for (int i = 0; i < imageSequence.size(); i++) {
		imageSequence[i].allocate (cameraWidth,cameraHeight, OF_IMAGE_COLOR);
	}
    
    //--------------- Initialize boolean state variables.
    playingFrame				= 0;
    bInPlaybackMode				= false;
	bEndRecording				= false;
    playing						= false;
    bUseVirtualProjector		= false;
    bUseFbo						= true;
    bInputMousePoints			= false;
    bShowCalibPoints			= false;
    bRecordingForCalibration	= false;
    bRecordThisCalibFrame		= false;
    bUseCorrectedCamera			= true;
    bDrawLeapWorld              = true;
    bShowText					= false;
    bDrawMiniImages             = true;
    bDrawSmallCameraView        = true;
	bDrawImageInBackground		= true;
	bDrawContourAnalyzer		= true;
    bDrawAppFaultDebugText      = false;
	bComputeAndDisplayPuppet	= true;
	bFullscreen					= true;
	bComputePixelBasedFrameDifferencing = false;
	bDoCompositeThresholdedImageWithLeapFboPixels = false;
	bDrawGradient               = true;
	bKioskMode                  = false;
    bInIdleMode                 = true;
    bInIdleModePrev             = false;
	bUseBothTypesOfScenes		= true;
	bDataSampleGrabbingEnabled	= false;
	bDrawFaultFeedback			= true;
    bEnableAppFaultManager      = true;
	
	//--------------- Setup LEAP
	leap.open();
    leap.setPolicyFlagHMD();
    leapVisualizer.setup();
	leapVisualizer.enableVoronoiRendering (imgW, imgH, bWorkAtHalfScale);
	leapVisualizer.bDrawGrid = false;
    leapRecorder.setup();
	
	bShowOffsetByNFrames		= false;
    framesBackToPlay			= 0;
	maxNPrevLeapFrames			= 10;
	prevLeapFrameRecorder.setup();
	
	cam.setOrientation(ofPoint(-55, 0, 0));

	leapColorFbo.allocate		(imgW,imgH, GL_RGBA);
    leapDiagnosticFbo.allocate	(imgW,imgH, GL_RGB);
    puppetDisplayFbo.allocate   (1024, 768, GL_RGB);

	
    folderName = ofGetTimestampString();
    lastIndexVideoPos.set(0,0,0);
    lastIndexLeapPos.set(0,0,0);
    
    //-------------------------------------------
    // OF & OpenGL settings.
    
    ofEnableAlphaBlending();
	
    glEnable  (GL_NORMALIZE);
	glDisable (GL_DEPTH);
    glDisable (GL_DEPTH_TEST);
    
    string versionDisplay = "Using openFrameworks version: " + ofToString( ofGetVersionInfo());
	cout << versionDisplay;
    
    //-------------------------------------------
    // Display settings
    initializeScreens();

	
	//-------------------------------------------
	bUseRedChannelForLuminance	= true;
	bDoMorphologicalOps			= false;
	bDoAdaptiveThresholding		= false;
	bUseGradientThreshold		= true;

	blurKernelSize				= 4.0;
	blurredStrengthWeight		= 0.07;
	thresholdValue				= 28;
	thresholdValueDelta			= 10;
	prevThresholdValue			= 0;
	prevThresholdValueDelta		= 0;
	gradientThreshPow			= 0.75;
	prevGradientThreshPow		= 1.0;
	skinColorPatchSize			= 64;
	averageSkinLuminance		= 0;
    maxPossibleSkinLuma         = 210;
    minPossibleSkinLuma         = 56;
    skinLumaPowf                = 1.5;
    maxSkinThresh               = 39;
    minSkinThresh               = 27;
	
	dataSampleGrabIntervalMillis = 20000;
	lastDataSampleGrabTimeMillis = 0;
	dataSampleImg.allocate(cameraWidth, cameraHeight, OF_IMAGE_COLOR);
	
	
	skinColorPatch.create (skinColorPatchSize, skinColorPatchSize, CV_8UC1);
	
	colorVideo.allocate			(cameraWidth, cameraHeight);
    maskedCamVidImg.create      (imgW, imgH, CV_8UC3);
	
	colorVideoHalfScale.allocate(imgW, imgH);
	leapFboPixels.allocate		(imgW, imgH, OF_IMAGE_COLOR);
	
	
	grayMat.create				(imgH, imgW, CV_8UC1);
	prevGrayMat.create			(imgH, imgW, CV_8UC1);
	diffGrayMat.create			(imgH, imgW, CV_8UC1);
	
	blurred.create				(imgH, imgW, CV_8UC1);
	thresholded.create			(imgH, imgW, CV_8UC1);
	thresholdedFinal.create		(imgH, imgW, CV_8UC1);
	adaptiveThreshImg.create	(imgH, imgW, CV_8UC1);
	thresholdConstMat.create	(imgH, imgW, CV_8UC1);
	tempGrayscaleMat1.create	(imgH, imgW, CV_8UC1);
	tempGrayscaleMat2.create	(imgH, imgW, CV_8UC1);
	gradientThreshImg.create	(imgH, imgW, CV_8UC1);
	leapArmPixelsOnlyMat.create	(imgH, imgW, CV_8UC1);
	
	videoMat.create				(imgH, imgW, CV_8UC3);
	leapDiagnosticFboMat.create	(imgH, imgW, CV_8UC3); // 3-channel
	coloredBinarizedImg.create	(imgH, imgW, CV_8UC3); // 3-channel
	thresholdedFinal8UC3.create (imgH, imgW, CV_8UC3);
	
	graySmall.create			(imgH/4, imgW/4, CV_8UC1);
	blurredSmall.create			(imgH/4, imgW/4, CV_8UC1);
	
	rgbVideoChannelMats[0].create (imgH, imgW, CV_8UC1);
	rgbVideoChannelMats[1].create (imgH, imgW, CV_8UC1);
	rgbVideoChannelMats[2].create (imgH, imgW, CV_8UC1);
	
	leapFboChannelMats[0].create (imgH, imgW, CV_8UC1);
	leapFboChannelMats[1].create (imgH, imgW, CV_8UC1);
	leapFboChannelMats[2].create (imgH, imgW, CV_8UC1);
	

	
	//-------------------------------------
	// Clear the contents of all images.
	grayMat.setTo(0);
	blurred.setTo(0);
	thresholded.setTo(0);
	thresholdedFinal.setTo(0);
	adaptiveThreshImg.setTo(0);
	thresholdConstMat.setTo(0);
	tempGrayscaleMat1.setTo(0);
	tempGrayscaleMat2.setTo(0);
	
	leapDiagnosticFboMat.setTo(0);
	coloredBinarizedImg.setTo(0);
	graySmall.setTo(0);
	blurredSmall.setTo(0);
	//------------------------
	
	
	// Get us ready to demo in a hurry
	string filePathCalib = "2015-july-13-CALIBRATION";
        // Previously:
        // "oct-8-CALIBRATION";
        // "calib_chris_corrected_4";
	calibrateFromXML(filePathCalib);
	
	string filePathPlay = "2015-april-15-recording";
    // "sep15-golan-perfect";
    // "play_chris_corrected_4";
	folderName = filePathPlay;
	loadAndPlayRecording(filePathPlay);
	bUseVirtualProjector = true;

	amountOfPixelMotion01   = 0;
	amountOfLeapMotion01    = 0;
	zHandExtent             = 0.00;
	motionAlpha             = 0.60;
	zExtentAlpha            = 0.30;
	fingerCurlAlpha         = 0.60;
	amountOfFingerCurl01    = 0;
	
	elapsedMicros = 0;
	elapsedMicrosInt = 0;
	
	int morph_size = 1;
	int morph_type = cv::MORPH_ELLIPSE;
	morphStructuringElt = getStructuringElement(morph_type,
												cv::Size( 2*morph_size + 1, 2*morph_size+1 ),
												cv::Point(  morph_size,       morph_size ) );
	
    //----------------------------------
    // CONTOUR ANALYZER AND MESH BUILDER
	myHandContourAnalyzer.setup(imgW, imgH);
	myHandMeshBuilder.initialize(imgW, imgH);
	myHandMeshBuilder.setWorkAtHalfScale(bWorkAtHalfScale);
	bSuccessfullyBuiltMesh = false;
	
	// PUPPET MANAGER & PUPPET
	myPuppetManager.setupPuppeteer (myHandMeshBuilder);
	myPuppetManager.setupPuppetGui ();
    puppetDisplayScale = 1.15;
	
	bUseTopologyModifierManager = false;
    myTopologyModifierManager.setup();
    
    // APPLICATION FAULT MANAGER
    appFaultManager.setup();
    minHandInsertionPercent = 0.29;
    maxAllowableMotion		= 15.0;
    maxAllowableFingerCurl	= 0.51;
    maxAllowableExtentZ		= 0.68;
    maxAllowableHeightZ     = 3.25;
	
    //--------------
	// MUST BE LAST IN SETUP()
	setupGui();
    keyPressed('k');
}


//--------------------------------------------------------------
void ofApp::initializeScreens(){
    
    // Get screen widths and heights from Quartz Services
    // See https://developer.apple.com/library/mac/documentation/GraphicsImaging/Reference/Quartz_Services_Ref/index.html
    // See http://forum.openframeworks.cc/t/dual-monitor-full-screen/13654/10
    
    CGDisplayCount displayCount;
    CGDirectDisplayID displays[32];
    
    // Grab the active displays
    CGGetActiveDisplayList (32, displays, &displayCount);
    numActiveDisplays = (int) displayCount;
    printf ("----------------------\n");
    printf ("numActiveDisplays = %d\n", numActiveDisplays);
    for (int i=0; i<numActiveDisplays; i++){
        int displayWidth  = CGDisplayPixelsWide ( displays[i] );
        int displayHeight = CGDisplayPixelsHigh ( displays[i] );
        printf ("   Display %d: (%d, %d)\n", i, displayWidth, displayHeight);
    }
    
    // Screens are assumed to be arranged as follows:
    /*
     
     otherscreen         touchscreen
     |-------------------+----------+
     |                   |          |
     |                   | 1024x768 |
     |    1920 x 1080    |          |
     |                   +----------+
     |                   |
     +-------------------+
    
     */
    
    touchscreenW = 1024;
    touchscreenH = 768;
    otherscreenW = 0;
    otherscreenH = 0;
    if (numActiveDisplays > 1){
        // figure out which one is the touchscreen
        int touchscreenId = 1; // a guess.
        int otherScreenId = 0; // a guess.
        for (int i=0; i<numActiveDisplays; i++){
            if (CGDisplayPixelsWide ( displays[i] ) == touchscreenW){
                touchscreenId = i;
                otherScreenId = 1-i;
            }
        }
        touchscreenW = CGDisplayPixelsWide ( displays[touchscreenId] );
        touchscreenH = CGDisplayPixelsHigh ( displays[touchscreenId] );
        
        otherscreenW = CGDisplayPixelsWide ( displays[otherScreenId] );
        otherscreenH = CGDisplayPixelsHigh ( displays[otherScreenId] );
    }
    printf ("----------------------\n");
}


//--------------------------------------------------------------
void ofApp::initializeCamera(){
	
	//--------------- Setup camera grabber.
    // We are using a PointGrey color firewire camera.
	#ifdef _USE_LIBDC_GRABBER
    
		// For the ofxLibdc::PointGrey cameraLibdc;
		cout << "libdc cameras found: " << cameraLibdc.getCameraCount() << endl;
		
        ofSetVerticalSync(false);
		cameraLibdc.setImageType (OF_IMAGE_COLOR);
		cameraLibdc.setSize (cameraWidth, cameraHeight);
        cameraLibdc.setImageType (OF_IMAGE_COLOR);
        cameraLibdc.setBayerMode (DC1394_COLOR_FILTER_GRBG);
        // Why does the camera video turn grayscale???
        // There's some sort of error which initializes it incorrectly.
    
        cameraLibdc.setBlocking(true);
        cameraLibdc.setFrameRate(30);
    
        cameraLibdc.setExposure(1.0);
        cameraLibdc.setup();
    
		cameraLibdcShutterInv = 41.0;
		cameraLibdcBrightness = 0;
		cameraLibdcGain = 0.0;
		cameraLibdcGamma = 1.0;
    
        cameraLibdc.setBrightness (cameraLibdcBrightness);
        cameraLibdc.setGain (cameraLibdcGain);
        cameraLibdc.setGammaAbs (cameraLibdcGamma);
		cameraLibdc.setShutterAbs (1.0 / cameraLibdcShutterInv);

	#else
		cameraVidGrabber.setVerbose(true);
		cameraVidGrabber.initGrabber(cameraWidth,cameraHeight);
	#endif
}

//--------------------------------------------------------------
void ofApp::initializeCameraCalibration(){
	
	// Set up corrected camera calibration (un-distortion).
	#ifdef _USE_CORRECTED_CAMERA // (...which is true, BTW)
    
		// We are undistorting the camera.
        // Load the meta-undistortion settings.
		ofxCv::FileStorage settings (ofToDataPath("settingsForCameraCalibrator.yml"), ofxCv::FileStorage::READ);
		if (settings.isOpened()) {
			
			// Is this needed if we're not calibrating??
			int patternXCount = settings["patternXCount"];
			int patternYCount = settings["patternYCount"];
			myCalibration.setPatternSize(patternXCount, patternYCount);
			float squareSize = settings["squareSize"];
			myCalibration.setSquareSize(squareSize);
			
			ofxCv::CalibrationPattern patternType = ofxCv::CHESSBOARD;
			switch(settings["patternType"]) {
				default:
				case 0: patternType = ofxCv::CHESSBOARD; break;
				case 1: patternType = ofxCv::CIRCLES_GRID; break;
				case 2: patternType = ofxCv::ASYMMETRIC_CIRCLES_GRID; break;
			}
			myCalibration.setPatternType(patternType);
		}
		
        // Load the actual (calibrated) undistortion parameters.
		ofxCv::FileStorage prevCalibrationFile (ofToDataPath("calibration.yml"), ofxCv::FileStorage::READ);
		if (prevCalibrationFile.isOpened()){
			prevCalibrationFile.release();
			myCalibration.load("calibration.yml");
			myCalibration.calibrate();
			if (myCalibration.isReady()){
				cout << "calibration ready" << endl;
			}
		}
	#endif
}

//========================================================================
void ofApp::setupGui() {
	
	guiTabBar = new ofxUITabBar();
	guiTabBar->setName("GUI");
	
	//------------------------------------
	ofxUICanvas* gui0 = new ofxUICanvas();
	gui0->setName("GUI0");
	gui0->addLabel("Basics");
	
	// Display of time consumption for vision & meshing.
	gui0->addSpacer();
	gui0->addFPS();
    gui0->addIntSlider("playingFrame", 0, 1000,         &playingFrame);
    gui0->addLabelToggle("Fullscreen",					&bFullscreen);
    gui0->addLabelToggle("Do Puppet",					&bComputeAndDisplayPuppet);
	gui0->addLabelToggle("bDataSampleGrabbingEnabled",	&bDataSampleGrabbingEnabled);
    
    // Display of time consumption for puppeteering.
    gui0->addSpacer();
	gui0->addValuePlotter("Vision: Micros", 256, 0, 50000, &elapsedMicros);
	gui0->addIntSlider("Vision: Micros", 0, 50000, &elapsedMicrosInt);
	gui0->addValuePlotter("Puppet: Micros", 256, 0, 50000, &(myPuppetManager.elapsedPuppetMicros));
	gui0->addIntSlider("Puppet: Micros", 0, 50000, &(myPuppetManager.elapsedPuppetMicrosInt));
    
    // Camera controls.
    gui0->addSpacer();
    gui0->addSlider("LibDC Camera Shutter",		32.0, 100.0,	&cameraLibdcShutterInv);
    gui0->addSlider("LibDC Camera Brightness",	0.0, 1.0,		&cameraLibdcBrightness);
    gui0->addSlider("LibDC Camera Gain",		0.00, 1.0,		&cameraLibdcGain);
    gui0->addSlider("LibDC Camera Gamma",		0.00, 3.0,		&cameraLibdcGamma);
	
	gui0->autoSizeToFitWidgets();
	ofAddListener(gui0->newGUIEvent,this,&ofApp::guiEvent);
	guiTabBar->addCanvas(gui0);
	guis.push_back(gui0);

	
	//------------------------------------
	ofxUICanvas* gui1 = new ofxUICanvas();
	gui1->setName("GUI1");
	gui1->addLabel("Image Processing");
	
	gui1->loadSettings(originalAppDataPath + "HandArtworkSettings1.xml");
	
	gui1->addSpacer();
	gui1->addIntSlider("thresholdValue",		0, 128,		&thresholdValue);
	gui1->addIntSlider("thresholdValueDelta",	0, 64,		&thresholdValueDelta);
	gui1->addSlider("gradientThreshPow",		0.5, 2.0,   &gradientThreshPow);
	gui1->addLabelToggle("bUseGradientThreshold",			&bUseGradientThreshold);
	
	gui1->addValuePlotter("averageSkinLuminance", 256,	0.00, 255.0, &averageSkinLuminance, 32);
	gui1->addSlider("averageSkinLuminance",		0, 255,		&averageSkinLuminance);
    gui1->addSlider("maxPossibleSkinLuma", 0, 255, &maxPossibleSkinLuma );
    gui1->addSlider("minPossibleSkinLuma", 0, 255, &minPossibleSkinLuma );
    gui1->addSlider("skinLumaPowf",  0, 4, &skinLumaPowf );
    gui1->addSlider("maxSkinThresh", 0, 255, &maxSkinThresh );
    gui1->addSlider("minSkinThresh", 0, 255, &minSkinThresh );

	gui1->addLabelToggle("bUseRedChannelForLuminance",		&bUseRedChannelForLuminance);
	gui1->addLabelToggle("bDoAdaptiveThresholding",			&bDoAdaptiveThresholding);
	gui1->addLabelToggle("bDoMorphologicalOps",				&bDoMorphologicalOps);
	
	gui1->addSpacer();
	gui1->addSlider("HCA-thresholdValue", 0.0, 128.0,       &(myHandContourAnalyzer.thresholdValue));
	gui1->addSlider("HCA-blurKernelSize", 1, 40,            &(myHandContourAnalyzer.blurKernelSize));
	gui1->addSlider("HCA-blurredStrengthWeight", 0.0, 1.0,  &(myHandContourAnalyzer.blurredStrengthWeight));
	gui1->addSlider("HCA-lineBelongingTolerance", 0, 64,	&(myHandContourAnalyzer.lineBelongingTolerance));
	gui1->addSlider("HCA-perpendicularSearch", 0.0, 0.5,	&(myHandContourAnalyzer.perpendicularSearch));
	
	gui1->loadSettings(originalAppDataPath + "HandArtworkSettings1.xml");
	gui1->autoSizeToFitWidgets();
	ofAddListener(gui1->newGUIEvent,this,&ofApp::guiEvent);
	guiTabBar->addCanvas(gui1);
	guis.push_back(gui1);
	
	
	//------------------------------------
	ofxUICanvas* gui2 = new ofxUICanvas();
	gui2->setName("GUI2");
	gui2->addLabel("Contour Analysis");
	
	gui2->addIntSlider("smoothingOfLowpassContour", 1, 15,	&(myHandContourAnalyzer.smoothingOfLowpassContour));
	gui2->addSlider("crotchCurvaturePowf",  0.0, 2.0,		&(myHandContourAnalyzer.crotchCurvaturePowf));
	gui2->addSlider("crotchDerivativePowf", 0.0, 2.0,		&(myHandContourAnalyzer.crotchDerivativePowf));
	gui2->addIntSlider("crotchSearchRadius", 1, 32,			&(myHandContourAnalyzer.crotchSearchRadius));
	gui2->addSlider("crotchContourSearchMask", 0.0, 0.5,    &(myHandContourAnalyzer.crotchContourSearchTukeyMaskPct));
    
    gui2->addSpacer();
    gui2->addSlider("minCrotchQuality", 0.0, 0.40,			&(myHandContourAnalyzer.minCrotchQuality));
    gui2->addSlider("malorientationSuppression", 0.0, 1.0,	&(myHandContourAnalyzer.malorientationSuppression));
	
	gui2->autoSizeToFitWidgets();
	ofAddListener(gui2->newGUIEvent,this,&ofApp::guiEvent);
	guiTabBar->addCanvas(gui2);
	guis.push_back(gui2);
	
	//------------------------------------
	// GUI for Application State
	ofxUICanvas* gui3 = new ofxUICanvas();
	gui3->setName("GUI3");
	gui3->addLabel("User Metrics");
	gui3->addSlider("minHandInsertionPercent",  0.0, 1.0,	&minHandInsertionPercent);
	
	gui3->addSpacer();
	gui3->addValuePlotter("amountOfLeapMotion01", 256,	0.00, 25.0, &amountOfLeapMotion01, 32);
	gui3->addSlider("amountOfLeapMotion01",				0.00, 25.0, &amountOfLeapMotion01);
	gui3->addSlider("maxAllowableMotion",				0.00, 25.0, &maxAllowableMotion );
	gui3->addSpacer();
	gui3->addValuePlotter("amountOfFingerCurl01", 256,	0.00, 1.00, &amountOfFingerCurl01, 32);
	gui3->addSlider("amountOfFingerCurl01",				0.00, 1.00, &amountOfFingerCurl01 );
	gui3->addSlider("maxAllowableFingerCurl",			0.00, 1.00, &maxAllowableFingerCurl );
	gui3->addSpacer();
	gui3->addValuePlotter("zHandExtent",		  256,	0.00, 2.00, &zHandExtent, 32);
	gui3->addSlider("zHandExtent",						0.00, 2.00, &zHandExtent );
	gui3->addSlider("maxAllowableExtentZ",				0.00, 2.00, &maxAllowableExtentZ );

    gui3->addSpacer();
    gui3->addValuePlotter("zHandHeight",		  256,	0.00, 5.00, &zHandHeight, 32);
    gui3->addSlider("zHandHeight",						0.00, 5.00, &zHandHeight );
    gui3->addSlider("maxAllowableHeightZ",				0.00, 5.00, &maxAllowableHeightZ );
    
	gui3->autoSizeToFitWidgets();
	ofAddListener(gui3->newGUIEvent,this,&ofApp::guiEvent);
	guiTabBar->addCanvas(gui3);
	guis.push_back(gui3);
    
    //------------------------------------
    // GUI for WHAT TO DRAW & HOW TO DRAW IT
    //
    ofxUICanvas* gui4 = new ofxUICanvas();
    gui4->setName ("GUI4");
    gui4->addLabel("What to Render");
	gui4->addToggle("bUseBothTypesOfScenes",			&bUseBothTypesOfScenes);
	gui4->addToggle("useTopologyModifierManager",		&bUseTopologyModifierManager);
	gui4->addIntSlider("nTolerableTriangleIntersections", 0, 500,	&(myHandMeshBuilder.nTolerableTriangleIntersections));

    gui4->addSlider("backgroundGray", 0,255,            &backgroundGray); // slider
	gui4->addLabelToggle("bDrawImageInBackground",		&bDrawImageInBackground);
    gui4->addSlider("puppetDisplayScale", 0.5, 2.0,     &puppetDisplayScale); // slider
    
    gui4->addSpacer();
    gui4->addLabelToggle("bDrawMeshBuilderWireframe",   &bDrawMeshBuilderWireframe,     false, true);
    gui4->addLabelToggle("bShowText",                   &bShowText,                     false, true);
    gui4->addLabelToggle("bDrawAppFaultDebugText",      &bDrawAppFaultDebugText,        false, true);
    gui4->addLabelToggle("bDrawGradientOverlay",		&bDrawGradient,					false, true);
	gui4->addLabelToggle("bDrawFaultFeedback",			&bDrawFaultFeedback,			false, true);

    gui4->addSpacer();
    vector<string> vnames;
    vnames.push_back("grayMat");
    vnames.push_back("thresholded");
    vnames.push_back("gradientThreshImg");
	vnames.push_back("leapArmPixelsOnlyMat");
    vnames.push_back("thresholdedFinal");
    vnames.push_back("leapDiagnosticFboMat");
    vnames.push_back("edgeMat");
    vnames.push_back("processFrameImg");
    gui4->addLabel("Contour Analyzer Underlay", OFX_UI_FONT_SMALL);
    contourAnalyzerUnderlayRadio = gui4->addRadio("VR", vnames, OFX_UI_ORIENTATION_VERTICAL);
    contourAnalyzerUnderlayRadio->activateToggle("grayMat");

    gui4->autoSizeToFitWidgets();
    ofAddListener(gui4->newGUIEvent,this,&ofApp::guiEvent);
    guiTabBar->addCanvas(gui4);
    guis.push_back(gui4);
    
    guiTabBar->setVisible(!bKioskMode);
}


//--------------------------------------------------------------
void ofApp::guiEvent(ofxUIEventArgs &e) {
	string name = e.widget->getName();
	int kind = e.widget->getKind();
    string canvasParent = e.widget->getCanvasParent()->getName();
    // cout << canvasParent << endl;
}

int getSelection(ofxUIRadio* radio) {
	vector<ofxUIToggle*> toggles = radio->getToggles();
	for (int i = 0; i < toggles.size(); i++) {
		if (toggles[i]->getValue()) {
			return i;
		}
	}
	return -1;
}



//--------------------------------------------------------------
void ofApp::update(){
	
	bSuccessfullyBuiltMesh = false;
	long long computerVisionStartTime = ofGetElapsedTimeMicros();
	
	updateBufferedVideoPlaybackIfUsingStoredVideo();
	updateLiveVideoIfUsingCameraSource();
	
	updateProcessFrameImg();
	updateRecordingSystem();
	
	computeHandStatistics();
	leapVisualizer.updateHandPointVectors();
	leapVisualizer.updateVoronoiExpansion();

	renderDiagnosticLeapFboAndExtractItsPixelData();

	updateComputerVision();
    bSuccessfullyBuiltMesh = updateHandMesh();
	updateLeapHistoryRecorder();
	
	// Update the app's main state machine, including feedback to the user.
	applicationStateMachine();

	// Update our measurement of NON-PUPPET cpu consumption:
	long long computerVisionEndTime = ofGetElapsedTimeMicros();
	float elapsedMicrosThisFrame = (float)(computerVisionEndTime - computerVisionStartTime);
	elapsedMicros = 0.8*elapsedMicros + 0.2*elapsedMicrosThisFrame;
	elapsedMicrosInt = (int) elapsedMicros;
	
    bool bMeshesAreProbablyOK = true;
    if (bEnableAppFaultManager){
        bMeshesAreProbablyOK =
            (appFaultManager.doCurrentFaultsIndicateLikelihoodOfBadMeshes() == false);
    }
    
	if (bSuccessfullyBuiltMesh && bMeshesAreProbablyOK){
		
		if (bUseBothTypesOfScenes){
			// mix topo and regular.
			
            int nTopoScenes = myTopologyModifierManager.getSceneCount();
			if (currentSceneID < nTopoScenes){
				bUseTopologyModifierManager = true;
				myTopologyModifierManager.update (myHandMeshBuilder);
				myPuppetManager.updatePuppeteerDummy();
				//myHandMeshBuilder.nTolerableTriangleIntersections = 20;
			} else {
				bUseTopologyModifierManager = false;
				myPuppetManager.updatePuppeteer (bComputeAndDisplayPuppet, myHandMeshBuilder);
				//myHandMeshBuilder.nTolerableTriangleIntersections = 500;
			}
			
		} else {
			// We're selecting between topo and regular.
			// Update all aspects of the puppet geometry
			if (bUseTopologyModifierManager) {
				myTopologyModifierManager.update (myHandMeshBuilder);
			} else {
				myPuppetManager.updatePuppeteer (bComputeAndDisplayPuppet, myHandMeshBuilder);
			}
		}
	}
    
    //-----------------------------------
    // Determine and manage "idle mode" behavior.
    // If there are no hands for a while, switch to idle mode.
    
    int numberOfHandsPresent = leap.getLeapHands().size();
    bool bThereAreHandsPresent = (numberOfHandsPresent > 0);
    bool bThereAreNoHandsPresent = (numberOfHandsPresent == 0);
    
    bool bNoUserForAWhile = false;
    if (bEnableAppFaultManager){
        bNoUserForAWhile = appFaultManager.getHasFault(FAULT_NO_USER_PRESENT_LONG);
    }
    
    if (bNoUserForAWhile && bThereAreNoHandsPresent){
        bInPlaybackMode = true;
        playing = true;
        myPuppetManager.bInIdleMode = true;
        bInIdleMode = true;
        // cout << "leap hands" << leap.getLeapHands().size() << endl;
        // cout << "IDLE MODE ON" << endl;
        // set puppet draw mode and alpha
        //
    } else if (bThereAreHandsPresent && bInPlaybackMode){
        bInPlaybackMode = false;
        myPuppetManager.bInIdleMode = false;
        bInIdleMode = false;
    }
    
    if(bInIdleMode && !bInIdleModePrev) {
        backgroundImage.setFromPixels(processFrameImg.getPixelsRef());
        backgroundImage.update();
    }
    bInIdleModePrev = bInIdleMode;
}



//--------------------------------------------------------------
void ofApp::updateBufferedVideoPlaybackIfUsingStoredVideo(){
	
	//------------- Playback
	// Updates the BufferedVideo playback object, which fetches images from disk.
    if (bInPlaybackMode && !bRecording){
        video.update();
        video.setPlaying(playing);
    }
}

//--------------------------------------------------------------
void ofApp::updateLiveVideoIfUsingCameraSource(){
	
    //------------- Frame grabbing.
	// Copy fresh camera data into currentFrameImg.
    bCameraHasNewFrame = false;
	
#ifdef _USE_LIBDC_GRABBER
	
	if (cameraLibdc.grabVideo(currentFrameImg)) {
		
		cameraLibdc.setShutterAbs	(1.0 / cameraLibdcShutterInv);
		cameraLibdc.setBrightness	(cameraLibdcBrightness);
		cameraLibdc.setGain			(cameraLibdcGain);
		cameraLibdc.setGammaAbs		(cameraLibdcGamma);
		
		currentFrameImg.update();
		bCameraHasNewFrame = true;
	}
#else
	cameraVidGrabber.update();
	bCameraHasNewFrame = cameraVidGrabber.isFrameNew();
	if (bCameraHasNewFrame){
		currentFrameImg.setFromPixels(cameraVidGrabber.getPixels(), cameraWidth,cameraHeight, OF_IMAGE_COLOR);
	}
#endif

}

//--------------------------------------------------------------
void ofApp::updateProcessFrameImg(){
	
	//------------- Camera undistortion.
    // If selected, undistort the currentFrameImg into processFrameImg.
	// (Otherwise, just copy currentFrameImg into processFrameImg).
    if (useCorrectedCam()){
        if (myCalibration.size() > 0) {
            myCalibration.undistort(ofxCv::toCv(currentFrameImg), ofxCv::toCv(processFrameImg));
            processFrameImg.update();
        }
    } else{
        processFrameImg = currentFrameImg;
        processFrameImg.update();
    }
}


//--------------------------------------------------------------
void ofApp::updateRecordingSystem(){
	
	//------------- Recording (state machine).
	// Stash camera images to disk, as appropriate.
    if (bRecording && !bInPlaybackMode){
        if (!bEndRecording && currentFrameNumber >= imageSequence.size()){
            bEndRecording = true;
        }
        
        if (!bEndRecording) {
            if(bCameraHasNewFrame){
                
                // If in calibration record, mode only record some frames
                if( (bRecordingForCalibration && bRecordThisCalibFrame) || !bRecordingForCalibration){
                    leapRecorder.recordFrameXML(leap);
                    
                    ofPixels& target = imageSequence[currentFrameNumber];
                    memcpy (target.getPixels(),
                            processFrameImg.getPixels(),
                            target.getWidth() * target.getHeight() * target.getNumChannels());
					
                    currentFrameNumber++;
                    
                    if(bRecordingForCalibration){
                        bRecordThisCalibFrame = false;
                    }
                    // leap.markFrameAsOld();??????
                }
            }
            
        } else {
            finishRecording();
        }
    }
    
	
    //-------------  Leap update
	leap.markFrameAsOld();
}


//--------------------------------------------------------------
void ofApp::updateLeapHistoryRecorder(){
	//------------- Store last frame
    if (prevLeapFrameRecorder.XML.getNumTags("FRAME") > maxNPrevLeapFrames){
        prevLeapFrameRecorder.XML.removeTag("FRAME",0);
    }
    prevLeapFrameRecorder.recordFrameXML(leap);
}

//--------------------------------------------------------------
void ofApp::updateComputerVision(){
	
	// currentFrameImg comes directly from the camera, unless there's no camera, in which case it's just black & exists.
	// processedFrameImg is a copy of currentFrameImg if there's no undistortion being done; if it is, it's undistorted.
	// video is the playback buffered video

	// when live: use color img from processFrameImg
	// when playing, use color img from buffered video.
	if (bInPlaybackMode){
		extractVideoMatFromBufferedVideoFrame();		// scale down and extract grayscale.
	} else {
		extractVideoMatFromLiveVideo();
	}

	extractLuminanceChannelFromSourceVideoMat();
	computeThresholdFromSkinColor();
	computeFrameDifferencing();							// not used presently
	thresholdLuminanceImage();
	applyMorphologicalOps();
	compositeLeapArmIntoThresholdedFinal();
	compositeThresholdedImageWithLeapFboPixels();
	
	myHandContourAnalyzer.update (thresholdedFinal, leapDiagnosticFboMat, leapVisualizer);
	
}

//--------------------------------------------------------------
void ofApp::compositeLeapArmIntoThresholdedFinal(){
	
	// In some circumstances, we are losing the user's "arm":
	// -- Because of shadowing, since it is further away from the light sources, and sometimes in the shadow of the hand;
	// -- Because of sleeves, since it is October in Amsterdam and people are feeling chilly.
	// For this reason, we grab the pixels corresponding to their arm from the leap image,
	// and add them to the thresholded image.
	
	// Extract 8UC3 pixel data from leapDiagnosticFbo into leapFboPixels.
	// This part is NECESSARY for the proper functioning of the app!
	bool thisIsNotNegotiableDoNotTouchThis = true;
	if (thisIsNotNegotiableDoNotTouchThis){
		leapDiagnosticFbo.readToPixels(leapFboPixels);
		unsigned char *leapDiagnosticFboPixelData = leapFboPixels.getPixels();
		leapDiagnosticFboMat.data = leapDiagnosticFboPixelData;
		cv::flip (leapDiagnosticFboMat, leapDiagnosticFboMat, 0);
	}
	
	bool bDoAddLeapArmToThresholdedFinal = true;
	if (bDoAddLeapArmToThresholdedFinal){
		
		// Split apart the channels of the leapFboMat
		split(leapDiagnosticFboMat, leapFboChannelMats);
		leapFboChannelMats[2].copyTo (leapArmPixelsOnlyMat);
		
		// get the arm only. Those are the pixels in the blue channel with a value of 32.
		unsigned char* armPixels = leapArmPixelsOnlyMat.data;
		int nPixels = imgW * imgH;
		unsigned char val;
		for (int i=0; i<nPixels; i++){
			val = armPixels[i];
			armPixels[i] = (val == 32) ? 255 : 0;
		}
		
		// OR the arm pixels with the thresholded final.
		cv::bitwise_or(leapArmPixelsOnlyMat, thresholdedFinal, thresholdedFinal);
	}
}

//--------------------------------------------------------------
bool ofApp::updateHandMesh(){
	
	bool bRefined = myHandContourAnalyzer.refineCrotches (leapVisualizer, grayMat, thresholdedFinal, leapDiagnosticFboMat);
    ofVec3f& theHandCentroid     = myHandContourAnalyzer.handCentroidLeap;
    ofVec3f& theLeapWristPoint   = myHandContourAnalyzer.wristPosition;
	bool bSuccess = false;
    
	if (bRefined){
		ofPolyline &theHandContour  = myHandContourAnalyzer.theHandContourRefined;
		Handmark *theHandmarks      = myHandContourAnalyzer.HandmarksRefined;
		bSuccess = myHandMeshBuilder.buildMesh (theHandContour, theHandCentroid, theLeapWristPoint, theHandmarks);
		
	} else {
		ofPolyline &theHandContour  = myHandContourAnalyzer.theHandContourResampled;
		Handmark *theHandmarks      = myHandContourAnalyzer.Handmarks;
		bSuccess = myHandMeshBuilder.buildMesh (theHandContour, theHandCentroid, theLeapWristPoint, theHandmarks);
	}
	
	return bSuccess;
}





//--------------------------------------------------------------
void ofApp::extractVideoMatFromLiveVideo(){
	// processFrameImg contains the live color video from the camera.
	if (bWorkAtHalfScale){
		colorVideo.setFromPixels(processFrameImg.getPixelsRef());
		colorVideoHalfScale.scaleIntoMe (colorVideo);
		videoMat = toCv(colorVideoHalfScale);
	} else {
		videoMat = toCv(processFrameImg);
	}
}


//--------------------------------------------------------------
void ofApp::extractVideoMatFromBufferedVideoFrame(){
	
	// videoMat is a cv::Mat version of the current video frame.
	// It's an intermediary on the way to computing grayMat.
	
	//------------------------
	// Fetch the (color) video
	if (bWorkAtHalfScale){
		colorVideo.setFromPixels(video.getPixelsRef());
		colorVideoHalfScale.scaleIntoMe (colorVideo);
		videoMat = toCv(colorVideoHalfScale);
	} else {
		videoMat = toCv(video);
	}
}

//--------------------------------------------------------------
void ofApp::extractLuminanceChannelFromSourceVideoMat(){
	//------------------------
	// Extract (or compute) the grayscale luminance channel.
	// Either use just the red channel (a very good proxy for skin),
	// Or use the properly weighted components (more computationally expensive).
	if (bUseRedChannelForLuminance){
		split(videoMat, rgbVideoChannelMats);
		grayMat = rgbVideoChannelMats[0];
	} else {
		convertColor(videoMat, grayMat, CV_RGB2GRAY);
	}

}

//--------------------------------------------------------------
void ofApp::computeThresholdFromSkinColor(){

	//----------------------------
	// Obtain the pixel-coordinate of the hand centroid, from the LEAP.
	ofVec3f handCentroidLeap = leapVisualizer.getProjectedHandCentroid();
	float cx = handCentroidLeap.x/2;
	float cy = handCentroidLeap.y/2;
	
	// Create the bounds of a ROI centered on that point
	int roiL = cx - (skinColorPatchSize/2);
	int roiT = cy - (skinColorPatchSize/2);
	roiL = MAX(0, MIN( (imgW-1)-skinColorPatchSize, roiL));
	roiT = MAX(0, MIN( (imgH-1)-skinColorPatchSize, roiT));
	int roiR = roiL + skinColorPatchSize;
	int roiB = roiT + skinColorPatchSize;
	
	// Make extra sure the centroid is inside the constrained rect.
	if ((cx > roiL) && (cx < roiR) && (cy > roiT) && (cy < roiB)){
		CvRect handCentroidROI = cvRect (roiL,roiT, skinColorPatchSize, skinColorPatchSize);
		
		unsigned char* skinColorDataSrc = grayMat.data;
		unsigned char* skinColorDataDst = skinColorPatch.data;
		
		int row, indexSrc;
		int indexDst = 0;
		
		int nPatchPixels = skinColorPatchSize*skinColorPatchSize;
		for (int y=roiT; y<roiB; y++){
			row = y*imgW;
			for (int x=roiL; x<roiR; x++){
				indexSrc = row + x;
				unsigned char val = skinColorDataSrc[indexSrc];
				skinColorDataDst[indexDst] = val;
				indexDst++;
			}
		}
		
		// sort the patch by rows and cols.
		cv::sort(skinColorPatch, skinColorPatch, cv::SORT_EVERY_ROW );
		cv::sort(skinColorPatch, skinColorPatch, cv::SORT_EVERY_COLUMN );
		
		
		int avgSum = 0;
		int avgCount = 1;
		
		int index = 0;
        int q1 = 1*skinColorPatchSize/4;
        int q3 = 3*skinColorPatchSize/4;
		unsigned char* skinColorData = skinColorPatch.data;
		for (int y=q1; y<q3; y++){
			for (int x=q1; x<q3; x++){
                int index = y*skinColorPatchSize + x;
				unsigned char val = skinColorData[index];
				if (val > 0){
					avgCount ++;
					avgSum += val;
				}
			}
		}
		
		
		float average = (float)avgSum/(float)avgCount;
		float skinLumA = 0.96;
		float skinLumB = 1.0-skinLumA;
		averageSkinLuminance = skinLumA*averageSkinLuminance + skinLumB*average;
		
		// Modify thresholdValue based on average luminance.
        float maxSkinLum = maxPossibleSkinLuma; //170;
        float minSkinLum = minPossibleSkinLuma; //40;
        float maxThresh =  maxSkinThresh; //38; // light skin
        float minThresh =  minSkinThresh; //20; // dark skin
		
		float averageSkinLuminance01 = ofMap (averageSkinLuminance, minSkinLum,maxSkinLum, 0,1);
        averageSkinLuminance01 = powf(averageSkinLuminance01, skinLumaPowf); //1.25);
		
		thresholdValue		= ofMap (averageSkinLuminance01, 0,1, minThresh,maxThresh);
		thresholdValue		= ofClamp(thresholdValue, minThresh,maxThresh);
		
		thresholdValueDelta = ofMap (averageSkinLuminance, minSkinLum,maxSkinLum, 11,7);
		
		/*
		
		ofFill();
		ofSetColor(255);
		drawMat (skinColorPatch, mouseX, mouseY);
		ofDrawBitmapString ( "mean " + ofToString (average), mouseX, mouseY-40);
		*/
		
	} else {
        thresholdValue = 27;
		; // use a default threshold if the hand centroid is not inside the patch.
	}
	
}


//--------------------------------------------------------------
void ofApp::computeFrameDifferencing(){
	// This is currently disabled, because we are able to obtain
	// a lightweight measure of motion from the LEAP coordinates.
	
	bComputePixelBasedFrameDifferencing = false;
	if (bComputePixelBasedFrameDifferencing){
	
		// Compute frame-differencing in order to estimate the amount of motion in the frame.
		// When there's too much motion in the frame (i.e. hands shaking around too much),
		// then we'll use this information to decide whether or not to do the special effects.
		// By not running the special effects when there's too much motion, we hope to
		// encourage the visitor to hold still and move slowly :)
		// Uses grayMat, prevGrayMat, and diffGrayMat.
		
		//frame-differencing parameters:
		float diffThreshValue = 20;
		
		// Take the abs value of the difference between curr and prev,
		// store in diff, then swap curr into prev:
		absdiff (grayMat, prevGrayMat, diffGrayMat );
		grayMat.copyTo (prevGrayMat);
		
		// Threshold the diffGrayMat into itself
		cv::threshold (diffGrayMat, diffGrayMat, diffThreshValue, 255, cv::THRESH_BINARY);
		
		int count = countNonZero( diffGrayMat );
		float motionf = (float) count / (float)(imgW * imgH);
		amountOfPixelMotion01 = motionAlpha*amountOfPixelMotion01 + (1.0-motionAlpha)*motionf;
	}
}



//--------------------------------------------------------------
void ofApp::thresholdLuminanceImage(){
	
	if (bDoAdaptiveThresholding){ // presently false.
		
		// Copy the gray image to a very small version, which is faster to operate on;
		cv::Size blurredSmallSize = cv::Size(imgW/4, imgH/4);
		resize(grayMat, graySmall, blurredSmallSize, 0,0, INTER_NEAREST );
		
		// Blur the heck out of that very small version
		int kernelSize = ((int)(blurKernelSize)*2 + 1);
		float thresholdBlurSigma = kernelSize/2.0;
		cv::Size blurSize = cv::Size(kernelSize, kernelSize);
		GaussianBlur (graySmall, blurredSmall, blurSize, thresholdBlurSigma);
		
		// Upscale the small blurry version into a large blurred version
		cv::Size blurredSize = cv::Size(imgW,imgH);
		resize(blurredSmall, blurred, blurredSize, 0,0, INTER_LINEAR );
		
		// Fill the thresholdConstMat with the threshold value (but only if it has changed).
		if (thresholdValue != prevThresholdValue){
			thresholdConstMat.setTo( (unsigned char) ((int)thresholdValue));
		}
		prevThresholdValue = thresholdValue;
		
		// Create the adaptiveThreshImg by adding a weighted blurred + constant image.
		cv::scaleAdd (blurred, blurredStrengthWeight, thresholdConstMat, adaptiveThreshImg);
		
		// Do the actual (adaptive thresholding.
		cv::subtract (grayMat, adaptiveThreshImg, tempGrayscaleMat1);
		int thresholdMode = cv::THRESH_BINARY; // cv::THRESH_BINARY_INV
		cv::threshold (tempGrayscaleMat1, thresholded, 1, 255, thresholdMode);
		
	} else {
		
		// Recompute the gradient threshold image if it has fresh control values.
		if ((thresholdValue != prevThresholdValue) ||
			(thresholdValueDelta != prevThresholdValueDelta) ||
			(gradientThreshPow != prevGradientThreshPow)){
			computeGradientThreshImg();
		}
		prevThresholdValue = thresholdValue;
		prevThresholdValueDelta = thresholdValueDelta;
		prevGradientThreshPow = gradientThreshPow;
		
		// Do the thresholding.
		if (bUseGradientThreshold){
			// thresholded against gradient
			cv::subtract(grayMat, gradientThreshImg, thresholded);
			threshold (thresholded, thresholded, 1);
			
		} else {
			// regular threshold
			threshold (grayMat, thresholded, thresholdValue);
		}
		
	}
}

//--------------------------------------------------------------
void ofApp::computeGradientThreshImg(){

	
	unsigned char* gradientThreshImgData = gradientThreshImg.data;
	for (int x=0; x<imgW; x++){
		float val = ofMap (x, 0,imgW, 1,0);
		val = powf(val, gradientThreshPow);
		val = ofMap(val, 1,0, thresholdValue, thresholdValue-thresholdValueDelta);
		unsigned char valch = (unsigned char)((int)roundf(val));
		gradientThreshImgData[x] = valch;
	}
	for (int y=1; y<imgH; y++){
		for (int x=0; x<imgW; x++){
			int index = y*imgW + x;
			gradientThreshImgData[index] = gradientThreshImgData[x];
		}
	}
	
}

//--------------------------------------------------------------
void ofApp::applyMorphologicalOps(){
	
	if (bDoMorphologicalOps){
		
		long t0 = ofGetElapsedTimeMicros();
		
		// Apply the erosion operation
		cv::erode ( thresholded, thresholded,		morphStructuringElt );
		cv::erode ( thresholded, thresholded,		morphStructuringElt );
		cv::dilate( thresholded, thresholdedFinal,	morphStructuringElt );
		
		long t1 = ofGetElapsedTimeMicros();
		// printf("applyMorphologicalOps took: %d micros.\n", (int)(t1-t0));
		
	} else {
		
		thresholded.copyTo (thresholdedFinal);
	}
}



//--------------------------------------------------------------
void ofApp::computeHandStatistics(){
	// determine the amount of movement, the vertical extent, and the finger curledness.
	// can be used later to decide whether or not to run more advanced processing.
	if (leapToCameraCalibrator.calibrated && bUseVirtualProjector){
		
		// Update the calculated amount of hand movement (derived from leap)
		float movement	= leapVisualizer.getMotionAmountFromHandPointVectors();
		float curl		= 0.1 * leapVisualizer.getCurlFromHandPointVectors();
		
		ofVec2f zRange	= leapVisualizer.getZExtentFromHandPointVectors();
		float minZ		= 100.0 * zRange.x;
		float maxZ		= 100.0 * zRange.y;
		float zextent	= fabs(maxZ - minZ);
		
		if ((bInPlaybackMode && playing) || (!bInPlaybackMode)){
			amountOfLeapMotion01 = (motionAlpha*amountOfLeapMotion01)		+ (1.0-motionAlpha)*movement;
			zHandExtent          = (zExtentAlpha*zHandExtent)				+ (1.0-zExtentAlpha)*zextent;
			zHandHeight          = (zExtentAlpha*zHandHeight)				+ (1.0-zExtentAlpha)*(100.0 - minZ);
			amountOfFingerCurl01 = (fingerCurlAlpha*amountOfFingerCurl01)	+ (1.0-fingerCurlAlpha)*curl;
        
        } else if (bInPlaybackMode && !playing){
            zHandExtent          = (zExtentAlpha*zHandExtent)				+ (1.0-zExtentAlpha)*zextent;
            zHandHeight          = (zExtentAlpha*zHandHeight)				+ (1.0-zExtentAlpha)*(100.0 - minZ);
            amountOfFingerCurl01 = (fingerCurlAlpha*amountOfFingerCurl01)	+ (1.0-fingerCurlAlpha)*curl;
        }
        
	}
}



//--------------------------------------------------------------
void ofApp::renderDiagnosticLeapFboAndExtractItsPixelData(){
	
	// This is for the purposes of computer vision -- not display:
	// Render the Leap into leapDiagnosticFbo using diagnostic colors:
	// Colors which encode the orientation and identity of the fingers, etc.
    ofPushStyle();
	{
		leapDiagnosticFbo.begin();
		ofClear(0,0,0,0);
		glEnable(GL_DEPTH_TEST);
		
		if (leapToCameraCalibrator.calibrated && bUseVirtualProjector){
			leapToCameraCalibrator.projector.beginAsCamera();
			
			leapVisualizer.bDrawDiagnosticColors = true;
			leapVisualizer.setProjector(leapToCameraCalibrator.projector);
			ofFill();
									
			if (bInPlaybackMode && !bRecording){ // CANNED LEAP
				// draw leap from pre-loaded xml recording
				int nFrameTags = leapVisualizer.myXML.getNumTags("FRAME");
				if (nFrameTags > 0){
					
					// Compute the index of which frame to draw.
					playingFrame = video.getCurrentFrameID();
					int whichFrame = playingFrame;
					if (playingFrame > 0 && bShowOffsetByNFrames){
						whichFrame = (playingFrame - framesBackToPlay + nFrameTags)%nFrameTags;
					}
					
					// "Fluff out" the hand rendering with a Voronoi-based expansion halo
					// computed through OpenGL tricks on the graphics card.
					leapVisualizer.drawVoronoiFrameFromXML (whichFrame, leapVisualizer.myXML);
					leapToCameraCalibrator.projector.endAsCamera();
					glDisable(GL_DEPTH_TEST);
					leapVisualizer.drawVoronoi();
					glEnable(GL_DEPTH_TEST);
					leapToCameraCalibrator.projector.beginAsCamera();
					
					// Render the actual CGI hand, on top of the voronoi diagram (using diagnostic colors)
					leapVisualizer.drawFrameFromXML(whichFrame);
				}
				
			} else { // LIVE LEAP
								
				// "Fluff out" the hand rendering with a Voronoi-based expansion halo
				// computed through OpenGL tricks on the graphics card.
				if (bShowOffsetByNFrames && (framesBackToPlay > 0)){
					int totalFramesSaved = prevLeapFrameRecorder.XML.getNumTags("FRAME");
					int whichFrame = totalFramesSaved - framesBackToPlay;
					if (whichFrame < 0) whichFrame = 0;
					leapVisualizer.drawVoronoiFrameFromXML (whichFrame, prevLeapFrameRecorder.XML);
				} else {
					// the usual case: no frame delay
					leapVisualizer.drawVoronoiFrame(leap);
				}
				
				leapToCameraCalibrator.projector.endAsCamera();
				glDisable(GL_DEPTH_TEST);
				leapVisualizer.drawVoronoi();
				glEnable(GL_DEPTH_TEST);
				leapToCameraCalibrator.projector.beginAsCamera();
				
				
				// draw live leap
				if (bShowOffsetByNFrames && (framesBackToPlay > 0)){
					int totalFramesSaved = prevLeapFrameRecorder.XML.getNumTags("FRAME");
					int whichFrame = totalFramesSaved - framesBackToPlay;
					if (whichFrame < 0) whichFrame = 0;
					leapVisualizer.drawFrameFromXML (whichFrame, prevLeapFrameRecorder.XML); // working
				} else {
					leapVisualizer.drawFrame(leap); // no frame delay. working
				}
				
			}
			leapToCameraCalibrator.projector.endAsCamera();
		}

		glDisable(GL_DEPTH_TEST);
		leapDiagnosticFbo.end();
	}
    ofPopStyle();
}

//--------------------------------------------------------------
void ofApp::compositeThresholdedImageWithLeapFboPixels(){
	
	/* 
	// done earlier in compositeLeapArmIntoThresholdedFinal!
	// Extract 8UC3 pixel data from leapDiagnosticFbo into leapFboPixels.
	// This part is necessary
	//
	leapDiagnosticFbo.readToPixels(leapFboPixels);
	unsigned char *leapDiagnosticFboPixelData = leapFboPixels.getPixels();
	leapDiagnosticFboMat.data = leapDiagnosticFboPixelData;
	cv::flip (leapDiagnosticFboMat, leapDiagnosticFboMat, 0);
	*/
	
	if (bDoCompositeThresholdedImageWithLeapFboPixels){ // presently false!
		// Composite the colored orientation image (in leapFboMat) against
		// the thresholdedFinal (in an RGBfied version), to produce the coloredBinarizedImg.
		
		// GL 10/7/14: As far as I can tell, the coloredBinarizedImg is not used.
		// The compositing takes ~900 microseconds on my laptop.
		// So I have disabled this block of code.
		long t0 = ofGetElapsedTimeMicros();
		
		thresholdedFinalThrice[0] = thresholdedFinal;
		thresholdedFinalThrice[1] = thresholdedFinal;
		thresholdedFinalThrice[2] = thresholdedFinal;
		cv::merge(thresholdedFinalThrice, 3, thresholdedFinal8UC3);
		cv::bitwise_and(leapDiagnosticFboMat, thresholdedFinal8UC3, coloredBinarizedImg);
		
		long t1 = ofGetElapsedTimeMicros();
		printf("CompositeThresholdedImageWithLeapFboPixels took: %d micros.\n", (int)(t1-t0));
	}
}






//--------------------------------------------------------------
void ofApp::draw(){

	ofSetFullscreen(bFullscreen);
    ofBackground(backgroundGray);
    
    //-----------------------------------
    // 1. DEBUGVIEW: DIAGNOSTICS & CONTROLS FOR DEVS
	guiTabBar->setPosition(20,20);
    int nTopoScenes = myTopologyModifierManager.getSceneCount();
	
	// set puppet or topology modifier gui visibility
    if(guiTabBar->isVisible()) {
		
		if (bUseBothTypesOfScenes){
            
			if (currentSceneID < nTopoScenes){
				bUseTopologyModifierManager = true;
				myTopologyModifierManager.setGuiVisibility(true);
				myPuppetManager.setGuiVisibility(false);
			} else {
				bUseTopologyModifierManager = false;
				myPuppetManager.setGuiVisibility(true);
				myTopologyModifierManager.setGuiVisibility(false);
			}
			
		} else {
			
			if(bUseTopologyModifierManager) {
				myTopologyModifierManager.setGuiVisibility(true);
				myPuppetManager.setGuiVisibility(false);
			} else {
				myPuppetManager.setGuiVisibility(true);
				myTopologyModifierManager.setGuiVisibility(false);
			}
		}
		
    } else {
        myPuppetManager.setGuiVisibility(false);
        myTopologyModifierManager.setGuiVisibility(false);
    }

	


    
    
    if (!bKioskMode){
        drawLeapWorld();
        drawDiagnosticMiniImages();
        drawContourAnalyzer();
        
        if (!bInPlaybackMode){
            drawLiveForRecording();
        } else {
            drawPlayback();
            
            // draw mouse cursor for calibration input
            if(bInPlaybackMode && bInputMousePoints){
                drawCrosshairMouseCursor();
            }
        }
        
        // Diagnostics
        if (bDrawMeshBuilderWireframe){
            drawMeshBuilderWireframe();
        }
        if (bShowText){
            drawText();
        }
    }
    if (!bKioskMode){
        ofShowCursor();
    } else {
        ofHideCursor();
    }
    
    
    
    
    

	//------------------------------
	// 2. COMPUTE AND DISPLAY PUPPET
	if (bComputeAndDisplayPuppet){
        
        // Determine if all systems are go.
        bool bEverythingIsAwesome = false;
        bool bCalculatedMesh = bSuccessfullyBuiltMesh; //myHandMeshBuilder.bCalculatedMesh;
        bool bMeshesAreProbablyOK = true;
        if (bEnableAppFaultManager){
            bMeshesAreProbablyOK = (appFaultManager.doCurrentFaultsIndicateLikelihoodOfBadMeshes() == false);
        }
        bEverythingIsAwesome = bCalculatedMesh && bMeshesAreProbablyOK;
        if (bKioskMode && bInIdleMode){
            // 'Hack' so that live video does not show in idle mode
            bEverythingIsAwesome = false;
        }
        
        // If it's time to export the camera, do so.
        if (bEverythingIsAwesome){
            if (bDataSampleGrabbingEnabled && !bRecording && !bInPlaybackMode){
                if (exportTimer.tick()) {
                    string filename = "continuous/" + ofGetTimestampString();
                    exportFrame(filename);
                }
            }
        }
        
        // Compute the coordinates to display the puppet.
        float puppetScale1 = puppetDisplayScale * 1.0;
        float puppetScale2 = puppetDisplayScale * 2.0;
        float puppetOffsetX =  touchscreenW - cameraWidth*puppetScale1;
        float puppetOffsetY = (touchscreenH - cameraHeight*puppetScale1)/2.0;
        
        // Capture the final display into an FBO.
        puppetDisplayFbo.begin();
            ofPushStyle();
            ofPushMatrix();
            ofClear(0,0,0,255);
            ofSetColor(255,255,255);
            drawMainPuppetView (bEverythingIsAwesome, puppetOffsetX, puppetOffsetY, puppetScale1, puppetScale2);
            if( bDrawGradient ) drawGradientOverlay();
            ofPopMatrix();
            ofPopStyle();
        puppetDisplayFbo.end();
        
        // Render the final display's FBO on the touchscreen
        ofSetColor(255,255,255);
        puppetDisplayFbo.draw(otherscreenW,0, touchscreenW,touchscreenH);
        
        // Now also draw that FBO on the "other" (projection) screen, if we're in kiosk mode.
        if (numActiveDisplays > 1){
            if (bKioskMode == true){
                ofPushMatrix();
                float ratio1080to1024 = (float)otherscreenH / (float) touchscreenW;
                ofScale (ratio1080to1024, ratio1080to1024);
                ofTranslate(otherscreenW/2.0, 0);
                ofRotate(90);
                ofTranslate(0, -touchscreenH/2.0);
                ofSetColor(255,255,255);
                puppetDisplayFbo.draw(0,0, touchscreenW,touchscreenH);
                ofPopMatrix();
            }
        }
	}

    
    
    //-----------------------------------
    // 3. DISPLAY FEEDBACK TO USER:
    // APPLICATION STATE, INSTRUCTIONS,
    // AND DIAGNOSTIC INSET VIEW
    
    // Display diagnostic inset view here:
    // Some combo of meshBuilderWireframe, Leap view, and contourAnalyzer
    // rotated for the user's perspective
    
    // Application state manager feedbacK:
    if (bEnableAppFaultManager){
        if (bDrawAppFaultDebugText){
            appFaultManager.drawDebug(ofGetWidth()-200,20);
            // shows all active faults as debug text
        }
        if (bDrawFaultFeedback){
            float px = 0;
            float py = 0;
            float ph = ofGetHeight();
            if (numActiveDisplays > 1){
                px = otherscreenW;
                py = 0;
                ph = touchscreenH;
            }
            appFaultManager.drawFaultHelpScreen(px, py, ph);
        }
    }
	

}


//--------------------------------------------------------------
void ofApp::drawMainPuppetView (bool bEverythingIsAwesome,
                                float puppetOffsetX, float puppetOffsetY,
                                float puppetScale1, float puppetScale2){
    
    ofPushMatrix();
    
    if (bEverythingIsAwesome){
        // ALL GOOD! SHOW THE PUPPET!

        // Before rendering the puppet, show an image in the background if desired.
        if (bDrawImageInBackground){
            ofPushStyle();
            ofPushMatrix();
            ofTranslate( puppetOffsetX, puppetOffsetY, 0);
            ofScale (puppetScale1, puppetScale1);
            ofSetColor(255);
            backgroundImage.draw(0,0, 1024,768);
            ofPopMatrix();
            ofPopStyle();
        }
        
        // Position the right edge of the puppet at the right edge of the window.
        // We also scale up the puppet image to the optimal display size here.
        ofPushMatrix(); {
            ofTranslate( puppetOffsetX, puppetOffsetY, 0);
            ofScale (puppetScale2,puppetScale2);
            
            // Get the texture from the camera or the stored video, depending on the playback mode.
            ofTexture &handImageTexture = (bInPlaybackMode) ?
            (video.getTextureReference()) :
            (processFrameImg.getTextureReference());
            
            if (bUseBothTypesOfScenes){
                // mix topo and regular.
                int nTopoScenes = myTopologyModifierManager.getSceneCount();
                if (currentSceneID < nTopoScenes){
                    bUseTopologyModifierManager = true;
                    myTopologyModifierManager.draw(handImageTexture);
                } else {
                    bUseTopologyModifierManager = false;
                    myPuppetManager.drawPuppet(bComputeAndDisplayPuppet, handImageTexture);
                }
                
            } else {
                // We're selecting between topo and regular.
                if (bUseTopologyModifierManager) {
                    myTopologyModifierManager.draw (handImageTexture);
                } else {
                    // Draw the puppet.
                    myPuppetManager.drawPuppet (bComputeAndDisplayPuppet, handImageTexture);
                }
            }
        }
        ofPopMatrix();
        
    } else {
        
        // THE PUPPET IS FAULTY :(
        // SO ONLY SHOW THE VIDEO/CAMERA.
        
        if (bDrawImageInBackground){
            
            // In this instance, we show the live, unmasked, unaltered camera image.
            // We show it at full (1024x768) resolution, and we include its own natural background.
            // Note: the image needs to be scaled by puppetscale, or else there is scale-flipping.
            //
            
            if (bInIdleMode && bKioskMode){
                ofPushMatrix();
                ofTranslate( puppetOffsetX, puppetOffsetY, 0);
                ofScale (puppetScale2, puppetScale2);
                drawIdleContour();
                ofPopMatrix();
                
            } else {
                ofPushMatrix();
                ofTranslate( puppetOffsetX, puppetOffsetY, 0);
                ofScale (puppetScale1, puppetScale1);
                ofSetColor(255,255,255);
                colorVideo.draw(0,0, 1024,768);
                ofPopMatrix();
            }
            
            
        } else {
            
            // We're not drawing an image in the background;
            // instead, we'll show the synthetic black background, and
            // (on top of it) the masked image of the live video hand.
            
            // We'll use the camera/video version at 512x384 resolution.
            // (Masking the 1024 version threw memory errors, and it had a crappy edge anyway.)
            // Composite the colored camera/video image (in videoMat) against
            // the thresholdedFinal (in an RGBfied version), to produce the maskedCamVidImg.
            //
            thresholdedFinalThrice[0] = thresholdedFinal;
            thresholdedFinalThrice[1] = thresholdedFinal;
            thresholdedFinalThrice[2] = thresholdedFinal;
            cv::merge(thresholdedFinalThrice, 3, thresholdedFinal8UC3);
            cv::bitwise_and(videoMat, thresholdedFinal8UC3, maskedCamVidImg);
            
            ofPushMatrix();
            ofTranslate( puppetOffsetX, puppetOffsetY, 0);
            ofScale (puppetScale2,puppetScale2);
            ofSetColor(255,255,255);
            drawMat(maskedCamVidImg, 0,0);
            ofPopMatrix();
            
        }
        
    }

    ofPopMatrix();
}




//--------------------------------------------------------------
void ofApp::drawCrosshairMouseCursor(){
    ofPushStyle();
    ofNoFill();
    ofSetColor(0,255,0);
    ofEllipse(mouseX, mouseY-20, 16, 16);
    ofLine(mouseX,mouseY,mouseX,mouseY-12);
    ofLine(mouseX,mouseY-28,mouseX,mouseY-40);
    ofLine(mouseX-8,mouseY-20,mouseX-20,mouseY-20);
    ofLine(mouseX+8,mouseY-20,mouseX+20,mouseY-20);
    ofPopStyle();
}


//--------------------------------------------------------------
void ofApp::drawDiagnosticMiniImages(){
    
    ofPushMatrix();
    float miniScale = 0.35;
    ofTranslate(0, ofGetHeight() - (miniScale * imgH));
    ofScale(miniScale, miniScale);
    
    int xItem = 0;
    ofSetColor(ofColor::white);
    
    drawMat(grayMat,                        imgW * xItem, 0); xItem++;
    drawMat(thresholded,                    imgW * xItem, 0); xItem++;
    drawMat(thresholdedFinal,               imgW * xItem, 0); xItem++;
    drawMat(myHandContourAnalyzer.edgeMat,	imgW * xItem, 0); xItem++;
    drawMat(leapDiagnosticFboMat,           imgW * xItem, 0); xItem++;
    
    string imageName[] = {"grayMat", "thresholded", "thresholdedFinal", "edgeMat", "leapDiagnosticFboMat"};
    
    ofSetColor(ofColor::orange);
    for (int i=0; i<xItem; i++){
        ofDrawBitmapString( ofToString(i+1) + " " + ofToString(imageName[i]), imgW*i+10, 15/miniScale);
    }
    
    ofPopMatrix();
}


//--------------------------------------------------------------
void ofApp::drawContourAnalyzer(){
    
    // Draw the contour analyzer and associated CV images.

    float contourAnalyzerRenderScale = 2.0;
    float sW = (numActiveDisplays > 1) ? otherscreenW : ofGetWidth();
    float sH = (numActiveDisplays > 1) ? otherscreenH : ofGetHeight();
    float insetX = sW - contourAnalyzerRenderScale*imgW;
    float insetY = sH - contourAnalyzerRenderScale*imgH;
    
    ofPushMatrix();
    ofPushStyle();
    ofTranslate (insetX, insetY);
    ofScale (contourAnalyzerRenderScale, contourAnalyzerRenderScale);
    
    int whichCAU = getSelection (contourAnalyzerUnderlayRadio);
    
    ofSetColor(255);
    switch (whichCAU){
        default:
        case 0:		drawMat(grayMat,						0,0, imgW,imgH);	break;
        case 1:		drawMat(thresholded,					0,0, imgW,imgH);	break;
        case 2:		drawMat(gradientThreshImg,				0,0, imgW,imgH);	break;
		case 3:		drawMat(leapArmPixelsOnlyMat,			0,0, imgW,imgH);	break;
        case 4:		drawMat(thresholdedFinal,				0,0, imgW,imgH);	break;
        case 5:		drawMat(leapDiagnosticFboMat,			0,0, imgW,imgH);	break;
        case 6:     drawMat(myHandContourAnalyzer.edgeMat,	0,0, imgW,imgH);	break;
        case 7:		if (bInPlaybackMode ){
                        video.draw(0, 0, imgW,imgH);
                    } else {
                        processFrameImg.draw(0,0,imgW,imgH);
                    }
                    break;
    }
    
    myHandContourAnalyzer.draw();
	

	
	
	
	/*
	ofNoFill();
	ofSetColor(0,255,0);
	ofRect (roiL,roiT,skinColorPatchSize,skinColorPatchSize);
    ofFill();
   
	*/
	
	
    //-----------------------------------
	// Does not currently render properly when CA is scaled. 
    bool bDrawMouseOrientationProbe = false;
    if (bDrawMouseOrientationProbe){
        int px = mouseX - insetX;
        int py = mouseY - insetY;
        if ((px > 0) && (px < imgW) &&
            (py > 0) && (py < imgH)){

            int index1 = py * imgW + px;
            int index3 = index1 * 3;

            unsigned char *pixels = leapDiagnosticFboMat.data;
            float pr = (float) pixels[index3+0];
            float pg = (float) pixels[index3+1];
            float pb = (float) pixels[index3+2];

            float orientation = leapVisualizer.getDiagnosticOrientationFromColor(pr,pg,pb);

            float angX = 60.0 * cos(orientation);
            float angY = 60.0 * sin(orientation);

            ofSetColor(255);
            ofLine (mouseX, mouseY, mouseX+angX, mouseY+angY);
            ofDrawBitmapString( ofToString( RAD_TO_DEG*orientation), mouseX, mouseY-10);
        }
    }
    
    ofPopStyle();
    ofPopMatrix();
}

//--------------------------------------------------------------
void ofApp::drawMeshBuilderWireframe(){
    
    ofPushMatrix();
    ofTranslate(drawW, 0);
    //myHandMeshBuilder.drawRefinedMesh();
    myHandMeshBuilder.drawMeshWireframe();
    ofPopMatrix();
}

//--------------------------------------------------------------
void ofApp::applicationStateMachine(){
	
    float dt = ofGetElapsedTimef()-prevTime;
    
	bool bHandJustInserted	= false;
	bool bHandJustDeparted	= false;
	long currentHandStartTime = 0;
	
	//--------------------------------------// What the system should do when:
	int		STATE_SHOWING_SCREENSAVER;		// Nobody has been around for a while: show a screensaver
	int		STATE_SHOWING_AUGMENTATION;		// Ideal operating conditions: show the processed hand
	int		STATE_SHOWING_PLAIN_VIDEO;		// Momentarily faulty conditions: pass through the video
	int		STATE_SHOWING_ERROR;			// Sustained faulty conditions: show the video, plus a message
	
	/*
     int		FAULT_NOTHING_WRONG;			// Everything appears to be working normally
     int		FAULT_NO_USER_PRESENT_BRIEF;	// Hand may have been momentarily withdrawn, not present for ~1 second
     int		FAULT_NO_USER_PRESENT_LONG;		// Nobody is present in the camera nor leap, for more than ~10 seconds
     int		FAULT_LEAP_DROPPED_FRAME;		// There was a hand in the camera (as recently as a second ago), but there's a leap dropout.
     int		FAULT_NO_LEAP_HAND_TOO_SMALL;	// There's a handlike object in the camera, but it may be too small for the leap to work
     int		FAULT_NO_LEAP_OBJECT_PRESENT;	// Some bastard put something in the box
     int		FAULT_TOO_MANY_HANDS;			// There's more than one hand in view
     int		FAULT_HAND_TOO_FAST;			// The hand is moving too quickly
     int		FAULT_HAND_TOO_HIGH;			// The hand is physically too close to the cameras
     int		FAULT_HAND_TOO_CURLED;			// The hand is a fist or curled up, or has a curled finger
     int		FAULT_HAND_TOO_VERTICAL;		// The hand is turned away from the camera
     int		FAULT_HAND_NOT_DEEP_ENOUGH;		// THe hand needs to be inserted deeper into the box.
     */
	
	ApplicationFault theApplicationFault = FAULT_NOTHING_WRONG;
	
	float	insertionPercentage		= myHandContourAnalyzer.getInsertionPercentageOfHand(); // gets percentage of left side of blob.
	float	distanceOfBlobFromEntry	= myHandContourAnalyzer.getDistanceOfBlobFromEntry();	// gets percentage of right side of blob.
	int		nBlobsInScene			= myHandContourAnalyzer.getNumberOfBlobs();
	bool	bLeapIsConnected		= leap.isConnected();
	int		nLeapHandsInScene		= leapVisualizer.nLeapHandsInScene;
	
	
	
    //------- update time each fault has occurred
    
    // no user present
    if (bEnableAppFaultManager){
        if (nBlobsInScene == 0 || (bInPlaybackMode && bKioskMode) ){
            appFaultManager.updateHasFault(FAULT_NO_USER_PRESENT_BRIEF,dt);
            appFaultManager.updateHasFault(FAULT_NO_USER_PRESENT_LONG,dt);
        } else {
            appFaultManager.updateResetFault(FAULT_NO_USER_PRESENT_BRIEF);
            appFaultManager.updateResetFault(FAULT_NO_USER_PRESENT_LONG);
        }
    
        // things we need to know:
        // high values of distanceOfBlobFromEntry indicate either (A) bad thresholding or (B) object in scene.
        // An object is in the scene:
        // -- does not move (is stationary)
        // -- no hand in leap
        // -- area is larger than minRecognizeableObject area
        // -- nBlobsInScene is > 0
        
        
        // TODO: put this distanceOfBlobFromEntry threshold in gui
        if (nBlobsInScene > 0 &&
           nLeapHandsInScene == 0 &&
           distanceOfBlobFromEntry > 100 &&
           amountOfLeapMotion01 < .5*maxAllowableMotion ){
            
            appFaultManager.updateHasFault(FAULT_NO_LEAP_OBJECT_PRESENT,dt);
        } else {
            appFaultManager.updateResetFault(FAULT_NO_LEAP_OBJECT_PRESENT);
        }

        
        // no leap hands
        if (nLeapHandsInScene == 0 && nBlobsInScene > 0){
            appFaultManager.updateHasFault (FAULT_LEAP_DROPPED_FRAME, dt);
        } else {
            appFaultManager.updateResetFault (FAULT_LEAP_DROPPED_FRAME);
        }
    
    
        // too many hands
        if (nLeapHandsInScene > 1){
            appFaultManager.updateHasFault (FAULT_TOO_MANY_HANDS, dt);
        } else {
            appFaultManager.updateResetFault(FAULT_TOO_MANY_HANDS);
        }
    
        
        // hands too fast
        if (amountOfLeapMotion01 > maxAllowableMotion){
            appFaultManager.updateHasFault (FAULT_HAND_TOO_FAST, dt);
        } else {
            appFaultManager.updateResetFault (FAULT_HAND_TOO_FAST);
        }
    
        
        // hand too curled
        if (amountOfFingerCurl01 > maxAllowableFingerCurl){
            appFaultManager.updateHasFault (FAULT_HAND_TOO_CURLED, dt);
        } else {
            appFaultManager.updateResetFault (FAULT_HAND_TOO_CURLED);
        }
    
        
        // hand too vertical, e.g.: thumb above pinky
        if (zHandExtent > maxAllowableExtentZ){
            appFaultManager.updateHasFault (FAULT_HAND_TOO_VERTICAL, dt);
        } else {
            appFaultManager.updateResetFault (FAULT_HAND_TOO_VERTICAL);
        }
    
        
        // hand too high, i.e. too close to camera
        float zHandHeightReportedWhenNoHandsPresent = 100.0;
        float zCeil = zHandHeightReportedWhenNoHandsPresent/2.0;
        if ((zHandHeight > maxAllowableHeightZ) && (zHandHeight < zCeil)){
            appFaultManager.updateHasFault (FAULT_HAND_TOO_HIGH, dt);
        } else {
            appFaultManager.updateResetFault (FAULT_HAND_TOO_HIGH);
        }
        
        
        // not deep enough
        if (insertionPercentage < minHandInsertionPercent && nBlobsInScene > 0){
            appFaultManager.updateHasFault (FAULT_HAND_NOT_DEEP_ENOUGH, dt);
        } else {
            appFaultManager.updateResetFault (FAULT_HAND_NOT_DEEP_ENOUGH);
        }
        
        
        // scene on too long
        if( ofGetElapsedTimef() - myPuppetManager.sceneStartTime > 50 ){
            appFaultManager.updateHasFault (FAULT_SAME_SCENE_TOO_LONG, dt);
        } else {
            appFaultManager.updateResetFault(FAULT_SAME_SCENE_TOO_LONG);
        }
	
        
        // hand too small
        float theHandContourArea = myHandContourAnalyzer.theHandContourArea;
        float theHandContourArea01 = theHandContourArea / (float)(imgW*imgH);
        float curlAreaRatio = (amountOfFingerCurl01 / (0.001 + theHandContourArea01));
        if ((theHandContourArea01 < 0.09) &&
            (insertionPercentage > 0.25) &&
            (curlAreaRatio > 4.1)){
            appFaultManager.updateHasFault (FAULT_NO_LEAP_HAND_TOO_SMALL, dt);
        } else {
            appFaultManager.updateResetFault (FAULT_NO_LEAP_HAND_TOO_SMALL);
        }
        // printf("theHandContourArea01 = %f\n", theHandContourArea01);
        
        
        
        // get all detected faults
        vector<ApplicationFault> faults = appFaultManager.getAllFaults();
        
        // get longest fault detected
        ApplicationFault longFault = appFaultManager.getLongestFault();
    }
    
	prevTime = ofGetElapsedTimef();
}

//--------------------------------------------------------------
void ofApp::drawLiveForRecording(){
    
    // Draw the live (or pre-recorded) camera image underneath...
    ofSetColor(ofColor::white);
	#ifdef _USE_LIBDC_GRABBER
		processFrameImg.draw(drawW,0,drawW,drawH);
	#else
		processFrameImg.draw(drawW,0,drawW,drawH);
	#endif
}

//--------------------------------------------------------------
void ofApp::drawPlayback(){
    
    // if calibrated and want to see view from projector
    if (leapToCameraCalibrator.calibrated && bShowCalibPoints && bUseVirtualProjector){
        ofPushMatrix();
        ofScale(drawW/(float)cameraWidth, drawH/(float)cameraHeight);
        leapToCameraCalibrator.drawImagePoints();
        ofPopMatrix();
    }
    
    if(bInPlaybackMode && !bRecording){
        ofPushStyle();
        ofSetColor(255);
        video.draw(drawW, 0,drawW,drawH);
        
        if (bInputMousePoints){
            indexRecorder.drawPointHistory(video.getCurrentFrameID() );
        }

        ofPopStyle();
    }
}

//--------------------------------------------------------------
void ofApp::drawLeapWorld(){
    ofPushStyle();
    
    // draw video underneath calibration to see alignment
    if (leapToCameraCalibrator.calibrated && bUseVirtualProjector){
        ofSetColor(255);
        if (bInPlaybackMode){
            video.draw(0, 0, drawW,drawH);
        } else{
            #ifdef _USE_LIBDC_GRABBER
                processFrameImg.draw(0,0,drawW,drawH);
            #else
                processFrameImg.draw(0,0,drawW,drawH);
            #endif
        }
    }
    
    // start leapColorFbo
	leapVisualizer.bDrawDiagnosticColors = false;
    if (bUseFbo){
        leapColorFbo.begin();
        ofClear(0,0,0,0);
    }
    
    // start camera (either calibrated projection or easy cam)
    if (leapToCameraCalibrator.calibrated && bUseVirtualProjector){
        leapToCameraCalibrator.projector.beginAsCamera();
		
		//glEnable(GL_DEPTH);
		glEnable(GL_DEPTH_TEST);
		
    } else {
		glEnable(GL_DEPTH);
		glEnable(GL_DEPTH_TEST);
        cam.begin();
    }
    
    // draw grid
    ofSetColor(255);
    leapVisualizer.drawGrid();
    
    // draw world points
    if (leapToCameraCalibrator.calibrated && bShowCalibPoints){
        leapToCameraCalibrator.drawWorldPoints();
    }
    
    // draw leap from xml
    if (bInPlaybackMode && !bRecording){
        int nFrameTags = leapVisualizer.myXML.getNumTags("FRAME");
        if (nFrameTags > 0){
			
            playingFrame = video.getCurrentFrameID();
			int whichFrame = playingFrame;
			if (playingFrame > 0 && bShowOffsetByNFrames){
				whichFrame = (playingFrame - framesBackToPlay + nFrameTags)%nFrameTags;
			}
			
			ofFill();
            leapVisualizer.drawFrameFromXML(whichFrame);
			
            if( lastIndexVideoPos.x > 0 && lastIndexVideoPos.y > 0){
                ofSetColor(ofColor::red);
                ofDrawSphere(lastIndexLeapPos, 5.0f);
            }
        }
    } else{
        // draw live leap
		
		if (bShowOffsetByNFrames && (framesBackToPlay > 0)){
            int totalFramesSaved = prevLeapFrameRecorder.XML.getNumTags("FRAME");
            int whichFrame = totalFramesSaved - framesBackToPlay;
            if (whichFrame < 0) whichFrame = 0;
            leapVisualizer.drawFrameFromXML (whichFrame, prevLeapFrameRecorder.XML);
        } else {
			// Just show the live leap, without any frame delay
            leapVisualizer.drawFrame(leap);
        }

    }
    
    // end camera
	if (leapToCameraCalibrator.calibrated && bUseVirtualProjector){
        leapToCameraCalibrator.projector.endAsCamera();
		glDisable(GL_DEPTH_TEST);
    } else {
        // draw calibration projector for debugging
        if (leapToCameraCalibrator.calibrated){
            leapToCameraCalibrator.projector.draw();
        }
        cam.end();
		glDisable(GL_DEPTH_TEST);
		//glDisable(GL_DEPTH);
    }
    
    // end fbo
    if (bUseFbo){
        leapColorFbo.end();
    }
    
    //-------------------------- Display semitransparent leapColorFbo
    // if we are calibrated draw leapFbo with transparency to see overlay better
    if (leapToCameraCalibrator.calibrated) {
		ofSetColor(255,255,255,200);
	} else {
		ofSetColor(255,255,255,255);
	}
    
	// draw the fbo
	ofPushMatrix();
	if (bUseVirtualProjector){
		ofScale(1,-1,1);    // flip fbo
		leapColorFbo.draw(0,-drawH,drawW,drawH);
	} else {
		leapColorFbo.draw(0,0,drawW,drawH);
	}
	ofPopMatrix();

    ofPopStyle();
}


//--------------------------------------------------------------
void ofApp::drawText2(){
    float textY = 500;
    ofSetColor(ofColor::white);
    ofDrawBitmapString("YO KYLE & CHRIS",               20, textY+=15);
    ofDrawBitmapString("Press 'p' to show/hide puppet", 20, textY+=15);
    ofDrawBitmapString("Press 'g' to show/hide GUI",    20, textY+=15);
    ofDrawBitmapString("(You may need to press 'g' twice to see puppet gui)",    20, textY+=15);
    ofDrawBitmapString("Press '2' to load calibration", 20, textY+=15);
    ofDrawBitmapString("Press '1' to load sequence",    20, textY+=15);
    ofDrawBitmapString("TODO's at top of ofApp.cpp",    20, textY+=15);
}

//--------------------------------------------------------------
void ofApp::drawText(){
	
	float textY = 500;
	
	ofSetColor(ofColor::white);

	ofSetColor( (leap.isConnected()) ?  ofColor::green : ofColor(255,51,51)) ;
	ofDrawBitmapString( (leap.isConnected() ? "Leap is Connected!" : "Leap is NOT CONNECTED!"), 20, textY);
	ofDrawBitmapString( (leap.isConnected() ? "Leap is Connected!" : "Leap is NOT CONNECTED!"), 21, textY); textY+=15;
	textY+=15;
	
	ofSetColor(ofColor::white);
	ofDrawBitmapString("Press '1' to open playback recording from a directory", 20, textY); textY+=15;
    ofDrawBitmapString("Press '2' to apply calibration from a directory", 20, textY); textY+=15;
    ofDrawBitmapString("Press '3' to open calibration frames for calibration input", 20, textY); textY+=15;
    textY+=15;
    ofDrawBitmapString("Press 's' to toggle hand skeleton/cylinders", 20, textY); textY+=15;
	ofDrawBitmapString("Press 'c' to restore easy-cam orientation", 20, textY); textY+=15;
	ofDrawBitmapString("Press 'g' to toggle GUI", 20, textY); textY+=15;
    ofDrawBitmapString("Press 'v' to toggle virtual projector", 20, textY); textY+=15;
    ofDrawBitmapString("Press 'w' to toggle calibration points draw", 20, textY); textY+=15;
    ofDrawBitmapString("Press 'C' to load current playback folder's calibration", 20, textY); textY+=15;
    ofDrawBitmapString("Press 'm' to allow mouse input points", 20, textY); textY+=15;
    ofDrawBitmapString("Press 'left/right' to advance frame by frame", 20, textY); textY+=15;
    ofDrawBitmapString("Press '' (space) to pause/play", 20, textY); textY+=15;
	
	string usePrev = bShowOffsetByNFrames ? "on" : "off";
    ofDrawBitmapString("Press 'o' toggle use prev "+usePrev+" {} frames behind: "+ofToString(framesBackToPlay), 20, textY); textY+=15;

	textY+=15;
	
    textY = 500;
    int textX = 610;
    
    ofDrawBitmapString("All folders must be in SharedData/recordings", textX, textY); textY+=15;
    textY+=15;
    
    ofDrawBitmapString("Press 'l' to return to live mode", textX, textY); textY+=15;
    
    if (leap.isConnected()){
		ofDrawBitmapString("Press 'r' to toggle RECORDING", textX, textY); textY+=15;
        ofDrawBitmapString("Press 'R' to toggle RECORDING for CALIBRATION", textX, textY); textY+=15;
        ofDrawBitmapString("Press ' ' to record CALIBRATION frame", textX, textY); textY+=15;

	}
	if (leapVisualizer.myXML.getNumTags("FRAME") > 0){
		ofDrawBitmapString("Press ' ' to pause PLAYBACK",  textX, textY); textY+=15;
	}
	
	if (bInPlaybackMode){
		ofSetColor(ofColor::green);
		ofDrawBitmapString("PLAYING! " + ofToString(playingFrame), textX, textY); textY+=15;
	} else if (bRecording){
		ofSetColor(ofColor::red);
		ofDrawBitmapString("RECORDING! " + ofToString(leapRecorder.recordingFrameCount), textX, textY); textY+=15;
	}
    
    textY+=15;
    ofSetColor(ofColor::white);
    ofDrawBitmapString("Playback folder: "+folderName,  textX, textY); textY+=15;
    ofDrawBitmapString("Calibration file: "+leapToCameraCalibrator.dirNameLoaded,  textX, textY); textY+=15;

	
}

//--------------------------------------------------------------
void ofApp::drawGradientOverlay(){
    
    /* // old, pre-fbo
    // Defaults here assume ofGetWidth() == 1024.
    // This is not so if we have two screens,
    // in which case ofGetWidth() is probably 1920+1024.
    //
    int offSet = 50;
    int boxSize = (ofGetWidth()*0.25)+offSet;
    if (numActiveDisplays > 1){
        boxSize = (1024*0.25)+offSet;
    }
    
    ofFill();
    ofSetColor(0,255);
    ofRect(ofGetWidth()-offSet,0,offSet,ofGetHeight());
    
    glBegin(GL_QUADS);
    glColor4f(0,0,0,0);
    glVertex2f(ofGetWidth()-boxSize,0);
    glColor4f(0,0,0,1);
    glVertex2f(ofGetWidth()-offSet,0);
    glVertex2f(ofGetWidth()-offSet,ofGetHeight());
    glColor4f(0,0,0,0);
    glVertex2f(ofGetWidth()-boxSize,ofGetHeight());
    glEnd();
    */
    

    
    int offSet = 50;
    int boxSize = (int)(touchscreenW*0.25)+offSet;
    
    ofFill();
    ofSetColor(0,255);
    ofRect(touchscreenW-offSet,0,offSet,touchscreenH);
    
    glBegin(GL_QUADS);
    glColor4f(0,0,0,0);
    glVertex2f(touchscreenW-boxSize,0);
    glColor4f(0,0,0,1);
    glVertex2f(touchscreenW-offSet,0);
    glVertex2f(touchscreenW-offSet,touchscreenH);
    glColor4f(0,0,0,0);
    glVertex2f(touchscreenW-boxSize,touchscreenH);
    glEnd();
    
    
}

//--------------------------------------------------------------
void ofApp::drawIdleContour(){
    
    // get contours from handContourAnalyzer
    ofPushStyle();
    
    ofSetColor(255,255,255);
    backgroundImage.draw(0,0, imgW, imgH);

    ofSetLineWidth(4);
    ofSetColor(25,200,255,200);
    ofPolyline smoothContour = myHandContourAnalyzer.theHandContourVerySmooth;
    
//        for(int i = 0; i < smoothContour.size();i+=2){
//            ofEllipse(smoothContour[i].x,smoothContour[i].y,2,2);
//        }
	smoothContour.draw();
    ofPopStyle();
}

//--------------------------------------------------------------
void ofApp::loadPlaybackFromDialogForCalibration(){
    
    //Open the Open File Dialog
    ofFileDialogResult openFileResult= ofSystemLoadDialog("Choose a recording folder:",true);
    
    if (openFileResult.bSuccess){
        
        string filePath = openFileResult.getName();
        folderName = filePath;
        loadAndPlayRecording(filePath);
        video.setPlaying(true);
        video.update();
        playing = false;
        indexRecorder.setup("recordings/"+folderName+"/calib","fingerCalibPts.xml");
        indexRecorder.setDrawOffsets(drawW,0,cameraWidth/drawW,cameraHeight/drawH);    }
}

//--------------------------------------------------------------
void ofApp::loadPlaybackFromDialog(){
    
    //Open the Open File Dialog
    ofFileDialogResult openFileResult= ofSystemLoadDialog("Choose a recording folder:",true);
    
    if (openFileResult.bSuccess){
        
        string filePath = openFileResult.getName();
        folderName = filePath;
        loadAndPlayRecording(filePath);
        
    }
}

//--------------------------------------------------------------
void ofApp::loadCalibrationFromDialog(){
    //Open the Open File Dialog
    ofFileDialogResult openFileResult= ofSystemLoadDialog("Choose a recording folder:",true);
    
    if (openFileResult.bSuccess){
        string filePath = openFileResult.getName();
        calibrateFromXML(filePath);
    }
}

//--------------------------------------------------------------
void ofApp::finishRecording(){
    
    bRecording = false;
    bEndRecording = false;
    
    ofFileDialogResult openFileResult= ofSystemSaveDialog(folderName,"Make a folder in SharedData/recordings:");
    
    if (openFileResult.bSuccess){
        folderName = openFileResult.getName();
        
        int totalImage = MIN(currentFrameNumber,imageSequence.size());
        for(int i = 0; i < totalImage; i++) {
            if(imageSequence[i].getWidth() == 0) break;
            ofSaveImage(imageSequence[i], "recordings/"+folderName+"/camera/"+ofToString(i, 3, '0') + ".jpg");
        }
        
        ofDirectory dir;
        dir.open("recordings/"+folderName+"/leap");
        if(!dir.exists())dir.createDirectory("recordings/"+folderName+"/leap");
        leapRecorder.endRecording("recordings/"+folderName+"/leap/leap.xml");
        
        loadAndPlayRecording(folderName);

        if(bRecordingForCalibration){
            playing = false;
        }

    }
    
    imageSequence.clear();
    imageSequence.resize(300);
	for(int i = 0; i < imageSequence.size(); i++) {
		imageSequence[i].allocate(cameraWidth,cameraHeight, OF_IMAGE_COLOR);
	}
    currentFrameImg.clear();
    currentFrameImg.allocate(cameraWidth,cameraHeight, OF_IMAGE_COLOR);
}

//--------------------------------------------------------------
void ofApp::loadAndPlayRecording(string folder){
    
    leapVisualizer.loadXmlFile("recordings/"+folder+"/leap/leap.xml");
    video.load("recordings/"+folder+"/camera");
    
    indexRecorder.setup("recordings/"+folder+"/calib","fingerCalibPts.xml");
    indexRecorder.setDrawOffsets(drawW,0,cameraWidth/drawW,cameraHeight/drawH);
    
    // open calibration if exists?
    
    bInPlaybackMode = true;
    playing = true;
    currentFrameNumber = 0;
}

//--------------------------------------------------------------
void ofApp::calibrateFromXML( string calibFolderName ){
    
    leapToCameraCalibrator.setup (cameraWidth, cameraHeight);
    leapToCameraCalibrator.loadFingerTipPoints("recordings/"+calibFolderName+"/calib/fingerCalibPts.xml");
	
    if (useCorrectedCam()){
        leapToCameraCalibrator.correctCameraPNP(myCalibration);
    } else{
        leapToCameraCalibrator.correctCamera();
    }
}

//--------------------------------------------------------------
bool ofApp::useCorrectedCam(){
	#ifdef _USE_CORRECTED_CAMERA
		if(bUseCorrectedCamera) return true;
		else return false;
	#else
		return false;
	#endif
}

void ofApp::exportFrame(string filename) {
    myHandMeshBuilder.getMesh().save(filename + ".ply");
    leapRecorder.startRecording();
    leapRecorder.recordFrameXML(leap);
    leapRecorder.endRecording(filename + ".xml");
    Mat colorVideoMat = toCv(colorVideo);
    ofxCv::saveImage(colorVideoMat, filename + ".jpg");
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){

	if ((key == 'r') || (key == 'R')){
		if (leap.isConnected()){
			// Toggle Recording.
			//reset so we don't store extra tags
			if (bRecording){
                bEndRecording = true;
            } else{
                bRecording = !bRecording;
                folderName = ofGetTimestampString();
				leapRecorder.startRecording();
                leapVisualizer.myXML.clear();
                bInPlaybackMode = false;
                currentFrameNumber = 0;
                if( key == 'R') bRecordingForCalibration = true;
                else bRecordingForCalibration = false;
			}

		}
	
	}
    
    switch(key){
        case OF_KEY_LEFT:
            if(video.isLoaded()) video.goToPrevious();
            break;
        case OF_KEY_RIGHT:
             if(video.isLoaded()) video.goToNext();
            break;
			
		case '<':
        case ',':
			prevScene();
			break;
		case '>':
        case '.':
			nextScene();
			break;
			
        case 'c':
            cam.reset();
            break;
        case 'C':
            calibrateFromXML(folderName);
            break;

        
			
        case 'l':
            bInPlaybackMode = !bInPlaybackMode;
            break;
        case 'm':
            bInputMousePoints = !bInputMousePoints;
            break;
            
        case 'E': // export ply
            exportFrame("handmesh-" + ofToString(playingFrame));
            break;
            
		case 'P':
        case 'p':
            bComputeAndDisplayPuppet = !bComputeAndDisplayPuppet;
            break;
        case 'S':
            
            break;
        
        case '1':
            loadPlaybackFromDialog();
            break;
        case '2':
            loadCalibrationFromDialog();
            bUseVirtualProjector = true;
            break;
        case '3':
            loadPlaybackFromDialogForCalibration();
            break;
        case 'w':
            bShowCalibPoints = !bShowCalibPoints;
            break;
        case ' ':
            if(bInPlaybackMode) playing = !playing;
            else if(bRecordingForCalibration) bRecordThisCalibFrame = true;
            break;
        
		//case 'F':
        case 'f':
            bFullscreen = !bFullscreen;
            break;
		
		case 'o':
            bShowOffsetByNFrames= !bShowOffsetByNFrames;
            break;
        case '{':
            framesBackToPlay = (framesBackToPlay+maxNPrevLeapFrames -1)%maxNPrevLeapFrames;
			break;
        case '}':
			framesBackToPlay = (framesBackToPlay+maxNPrevLeapFrames +1)%maxNPrevLeapFrames;
            break;
			
		case 'k':
		case 'K':
            bKioskMode = !bKioskMode;
            guiTabBar->setVisible(!bKioskMode);
            myPuppetManager.setGuiVisibility(!bKioskMode);
                                             
            if(!bKioskMode){
                myPuppetManager.bInIdleMode = false;
                bInIdleMode = false;
            }
            break;
        
            
            
            
            
            
            
            
		/*
		 // Deprecated key commands: keypress commands that are on the way out.
		 //
		case 'T':
			// Captures meshes from Topo manager. Works, but disabled.
			if (bUseTopologyModifierManager && bComputeAndDisplayPuppet){
				// myTopologyModifierManager.saveMeshes();
			}
			break;
		case 'v':
            bUseVirtualProjector = !bUseVirtualProjector;
            break;
		 //
		*/
    }
    
}

//--------------------------------------------------------------
void ofApp::keyReleased(int key){
}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y ){
    
}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button){
}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button){
    
    if(bInPlaybackMode && bInputMousePoints){
        if(x > indexRecorder.xOffset && y > indexRecorder.yOffset &&
           x < indexRecorder.xOffset+drawW && y < drawH){
            indexRecorder.recordPosition(x, y-20, leapVisualizer.getIndexFingertipFromXML(video.getCurrentFrameID()),video.getCurrentFrameID());
            lastIndexVideoPos.set(x,y-20);
            lastIndexLeapPos = leapVisualizer.getIndexFingertipFromXML(video.getCurrentFrameID());
        }
    }
    
    if (bKioskMode){
        int dir = previousDir;
        dir = +1;
        changeScene(dir);
        previousDir = dir;
        
        // older code doesn't use multi-screens yet, but allows forward//backward navigation.
        /*
        int dir = previousDir;
        if(y < ofGetWidth() / 3) {
            dir = -1;
        } else if(y > 2 * ofGetWidth() / 3) {
            dir = +1;
        }
        changeScene(dir);
        previousDir = dir;
         */
    }
}

//--------------------------------------------------------------
void ofApp::mouseReleased (int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::nextScene(){ changeScene ( 1); }
void ofApp::prevScene(){ changeScene (-1); }
void ofApp::changeScene (int dir){
	
	if (dir >= 0){
		dir  = 1;
	} else {
		dir = -1;
	}
	
	if (bUseBothTypesOfScenes){
		
        int nTopoScenes = myTopologyModifierManager.getSceneCount();
		int nPuppScenes = myPuppetManager.scenes.size();
		int nTotalScenes = nTopoScenes + nPuppScenes;
        //printf("nTopoScenes = %d\n", nTopoScenes);
        //printf("nPuppScenes = %d\n", nPuppScenes);
		
		currentSceneID = (currentSceneID + dir + nTotalScenes)%nTotalScenes;
		
		// inform puppeteers
		if (currentSceneID < nTopoScenes){
			myTopologyModifierManager.setScene (currentSceneID);
			bUseTopologyModifierManager = true;
			
		} else {
			int whichPuppScene = currentSceneID - nTopoScenes;
			myPuppetManager.animateSceneChangeToGivenScene (whichPuppScene, dir);
			bUseTopologyModifierManager = false;
		}
		
		myPuppetManager.sceneStartTime = ofGetElapsedTimef();
		
	} else {
		
		myPuppetManager.animateSceneChange(dir);
	}

}


//--------------------------------------------------------------
void ofApp::windowResized(int w, int h){
}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg){
}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo){
}

//--------------------------------------------------------------
void ofApp::exit(){
    // let's close down Leap and kill the controller
    leap.close();
	/* delete gui; */
	
	for(vector<ofxUICanvas *>::iterator it = guis.begin(); it != guis.end(); ++it){
        ofxUICanvas *g = *it;
        delete g;
    }
    delete guiTabBar;
}

//--------------------------------------------------------------
void ofApp::huntForBlendFunc(int period, int defaultSid, int defaultDid){
	// sets all possible combinations of blend functions,
	// changing modes every [period] milliseconds.
    
    // All checked out, works well.
	
	int sfact[] = {
		GL_ZERO,
		GL_ONE,
		GL_DST_COLOR,
		GL_ONE_MINUS_DST_COLOR,
		GL_SRC_ALPHA,
		GL_ONE_MINUS_SRC_ALPHA,
		GL_DST_ALPHA,
		GL_ONE_MINUS_DST_ALPHA,
		GL_SRC_ALPHA_SATURATE
	};
	
	int dfact[] = {
		GL_ZERO,
		GL_ONE,
		GL_SRC_COLOR,
		GL_ONE_MINUS_SRC_COLOR,
		GL_SRC_ALPHA,
		GL_ONE_MINUS_SRC_ALPHA,
		GL_DST_ALPHA,
		GL_ONE_MINUS_DST_ALPHA
	};
	
	
	
	glEnable(GL_BLEND);
	
	if ((defaultSid == -1) && (defaultDid == -1)) {
		
		int sid =  (ofGetElapsedTimeMillis()/(8*period))%9;
		int did =  (ofGetElapsedTimeMillis()/period)%8;
		glBlendFunc(sfact[sid], dfact[did]);
		// ofLog(OF_LOG_NOTICE, "SRC %d	DST %d\n", sid, did);
		printf("SRC %d	DST %d\n", sid, did);
		
	} else if (defaultDid == -1){
		
		int did =  (ofGetElapsedTimeMillis()/period)%8;
		glBlendFunc(sfact[defaultSid], dfact[did]);
		// ofLog(OF_LOG_NOTICE, "SRC %d	DST %d\n", defaultSid, did);
		printf("SRC %d	DST %d\n", defaultSid, did);
		
	} else if (defaultSid == -1){
		
		int sid =  (ofGetElapsedTimeMillis()/(8*period))%9;
		glBlendFunc(sfact[sid], dfact[defaultDid]);
		// ofLog(OF_LOG_NOTICE, "SRC %d	DST %d\n", sid, defaultDid);
		printf("SRC %d	DST %d\n", sid, defaultDid);
		
	} else {
		
		glBlendFunc(sfact[defaultSid], dfact[defaultDid]);
		
	}
	
}


/*
 // HTML Acknowlegments
 
 
 <b>UNTITLED DIGITAL ART (AUGMENTED HAND SERIES)</b>
 By <a href="http://flong.com" rel="nofollow">Golan Levin</a>, <a href="http://csugrue.com/" rel="nofollow">Chris Sugrue</a>, and <a href="http://kylemcdonald.net/" rel="nofollow">Kyle McDonald</a>
 Repository: <a href="https://github.com/CreativeInquiry/digital_art_2014" rel="nofollow">github.com/CreativeInquiry/digital_art_2014</a>
 Contact: <a href="https://twitter.com/golan" rel="nofollow">@golan</a> or golan@flong.com
 
 Photos � by <a href="http://www.fotogeus.nl/" rel="nofollow">Gerlinde de Geus</a>, courtesy Cinekid Festival.
 
 Commissioned by the <a href="http://www.cinekid.nl/" rel="nofollow">Cinekid Festival</a>, Amsterdam, October 2014, with support from the Mondriaan Fund for visual art. Developed at the <a href="http://studioforcreativeinquiry.org" rel="nofollow">Frank-Ratchye STUDIO for Creative Inquiry</a> at Carnegie Mellon University with additional support from the <a href="http://www.arts.pa.gov/" rel="nofollow">Pennsylvania Council on the Arts</a> and the <a href="http://studioforcreativeinquiry.org/grants" rel="nofollow">Frank-Ratchye Fund for Art @ the Frontier</a>. Concept and software development: Golan Levin, Chris Sugrue, Kyle McDonald. Software assistance: Dan Wilcox, Bryce Summers, Erica Lazrus. Conceived 2005; developed 2013-2014.
 
 Special thanks to Paulien Dresscher, Theo Watson and Eyeo Festival for encouragement, and to Dan Wilcox, Bryce Summers, and Erica Lazrus for their help making this project possible. Thanks to Elliot Woods and Simon Sarginson for assistance with Leap/camera calibration, and to Adam Carlucci for his <a href="http://forum.openframeworks.cc/t/a-guide-to-speeding-up-your-of-app-with-accelerate-osx-ios/10560" rel="nofollow">helpful tutorial</a> on using the Accelerate Framework in openFrameworks. Additional thanks to Rick Barraza and Ben Lower of Microsoft; Christian Schaller and Hannes Hofmann of Metrilus GmbH; Dr. Roland Goecke of University of Canberra; and Doug Carmean and Chris Rojas of Intel.
 
 Developed in <a href="http://openframeworks.cc/" rel="nofollow">openFrameworks</a> (OF), a free, open-source toolkit for arts engineering. This project also uses a number of open-source addons for openFrameworks contributed by others: <a href="https://github.com/ofZach/ofxPuppet" rel="nofollow">ofxPuppet</a> by Zach Lieberman, based on Ryan Schmidt's <a href="http://www.dgp.toronto.edu/~rms/software/Deform2D/index.html" rel="nofollow">implementation</a> of <a href="http://www-ui.is.s.u-tokyo.ac.jp/~takeo/research/rigid/index.html" rel="nofollow">As-Rigid-As-Possible Shape Manipulation</a> by Igarashi, Moscovich & Hughes; <a href="https://github.com/ofTheo/ofxLeapMotion" rel="nofollow">ofxLeapMotion</a> by Theo Watson, with assistance from Dan Wilcox; <a href="https://github.com/kylemcdonald/ofxCv" rel="nofollow">ofxCv</a>, <a href="https://github.com/kylemcdonald/ofxLibdc" rel="nofollow">ofxLibdc</a>, and <a href="https://github.com/kylemcdonald/ofxTiming" rel="nofollow">ofxTiming</a> by Kyle McDonald; <a href="https://github.com/elliotwoods/ofxCvMin" rel="nofollow">ofxCvMin</a> and <a href="https://github.com/elliotwoods/ofxRay" rel="nofollow">ofxRay</a> by Elliot Woods; and the <a href="https://github.com/Bryce-Summers/ofxButterfly" rel="nofollow">ofxButterfly</a> mesh subdivision addon by Bryce Summers.
 
 <i>Shoutouts from @golan @chrissugrue & @kcimc: @admsyn @bla_fasel @bwycz @cinekid @CMUSchoolofArt @creativeinquiry @danomatika @elliotwoods @eyeofestival @laurmccarthy @openframeworks @PESfilm @rickbarraza @SimonsMine @theowatson @zachlieberman</i>

*/
