#include "testApp.h"

/*
// OPENFRAMEWORKS + LEAP MOTION SDK 2.0 HAND SKELETON DEMO 
// By Golan Levin (@golan), http://github.com/golanlevin
// Uses ofxLeapMotion addon by Theo Watson, with assistance from Dan Wilcox
// Supported in part by the Frank-Ratchye STUDIO for Creative Inquiry at CMU
*/



/* Note on OS X, you must have this in the Run Script Build Phase of your project. 
where the first path ../../../addons/ofxLeapMotion/ is the path to the ofxLeapMotion addon. 

cp -f ../../../addons/ofxLeapMotion/libs/lib/osx/libLeap.dylib "$TARGET_BUILD_DIR/$PRODUCT_NAME.app/Contents/MacOS/libLeap.dylib"; install_name_tool -change ./libLeap.dylib @executable_path/libLeap.dylib "$TARGET_BUILD_DIR/$PRODUCT_NAME.app/Contents/MacOS/$PRODUCT_NAME";

   If you don't have this you'll see an error in the console: dyld: Library not loaded: @loader_path/libLeap.dylib
*/




//--------------------------------------------------------------
void testApp::setup(){
    
    // All data will be in SharedData
    string basePath = ofToDataPath("", true);
    ofSetDataPathRoot("../../../../../SharedData/");
    
    cameraWidth = 1024;
    cameraHeight = 768;
	drawW = 640;
    drawH = 480;
    
    
    //--------------- Setup camera grabber
    #ifdef _USE_LIBDC_GRABBER
	
    // For the ofxLibdc::PointGrey cameraLibdc;
	cout << "libdc cameras found: " << cameraLibdc.getCameraCount() << endl;

    cameraLibdc.setImageType(OF_IMAGE_COLOR);
    cameraLibdc.setSize (cameraWidth, cameraHeight);
    //cameraLibdc.setBayerMode(DC1394_COLOR_FILTER_GRBG); // why turns video grayscale???

    cameraLibdc.setup();
    cameraLibdc.setShutterAbs(1. / 31.0);
	cameraLibdc.setExposure(1.0);
	cameraLibdc.setBrightness(0);
	cameraLibdc.setGain(0);
	cameraLibdc.setGammaAbs(1);
    //cameraLibdc.setBlocking(true);
    #else
	cameraVidGrabber.setDeviceID(1);
    cameraVidGrabber.setVerbose(true);
	cameraVidGrabber.initGrabber(cameraWidth,cameraHeight);
    #endif

	
    
    
	//--------------- Setup video saver
	bRecording = false;
	currentFrameNumber = 0;
	imageSequence.resize(500);

    currentFrameImg.allocate(cameraWidth, cameraHeight, OF_IMAGE_COLOR);
    processFrameImg.allocate(cameraWidth, cameraHeight, OF_IMAGE_COLOR);
    
	for(int i = 0; i < imageSequence.size(); i++) {
		imageSequence[i].allocate(cameraWidth,cameraHeight, OF_IMAGE_COLOR);
	}
	
    
    //--------------- Set up corrected camera calibration
    #ifdef _USE_CORRECTED_CAMERA
    ofxCv::FileStorage settings (ofToDataPath("settingsForCameraCalibrator.yml"), ofxCv::FileStorage::READ);
	if(settings.isOpened()) {
        
        // needed if not calibrating??
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
    
    
    //--------------- Setup leap
	leap.open();
    leapVisualizer.setup();
    leapRecorder.setup();
    prevLeapFrameRecorder.setup();
	cam.setOrientation(ofPoint(-55, 0, 0));
    fbo.allocate(cameraWidth,cameraHeight,GL_RGBA);

    playingFrame = 0;
    bPlaying = false;
	bEndRecording = false;
    playing = false;
    bUseVirtualProjector = false;
    bUseFbo = true;
    bInputMousePoints = false;
    bShowCalibPoints = true;
    bRecordingForCalibration = false;
    bRecordThisCalibFrame = false;
    bUseCorrectedCamera = true;
    bShowText = true;
    bShowLargeCamImageOnTop = false;    // temp for quickly showing on hand video only
    bShowOffBy1Frame = false;
    framesBackToPlay = 1;
    
    folderName = ofGetTimestampString();
    lastIndexVideoPos.set(0,0,0);
    lastIndexLeapPos.set(0,0,0);
    
    
    //--------------- App settings
    // ofSetFrameRate(60);
    ofSetVerticalSync(false);
	ofSetLogLevel(OF_LOG_WARNING);
	ofSetCylinderResolution (16, 1, 16);
    
   // ofEnableAlphaBlending();
    //glEnable(GL_NORMALIZE); // needed??
	//glEnable(GL_DEPTH);
    //glEnable(GL_DEPTH_TEST); // why is this messing the render up in the projector cam??????
    
    
    string versionDisplay = "Using openFrameworks version: " + ofToString( ofGetVersionInfo());
	cout << versionDisplay;
	

}


//--------------------------------------------------------------
void testApp::update(){
    
    
    
    //------------- Playback
    if(bPlaying && !bRecording){
        video.update();
        video.setPlaying(playing);
    }
    
    
    
    //------------- Recording
    bool bCameraHasNewFrame = false;
    #ifdef _USE_LIBDC_GRABBER
        if (cameraLibdc.grabVideo(currentFrameImg)) {
            bCameraHasNewFrame = true;
            currentFrameImg.update();
        }
    #else
        cameraVidGrabber.update();
        bCameraHasNewFrame = cameraVidGrabber.isFrameNew();
        if (bCameraHasNewFrame){
            currentFrameImg.setFromPixels(cameraVidGrabber.getPixels(), cameraWidth,cameraHeight, OF_IMAGE_COLOR);
        }
    #endif
    
    
    if(useCorrectedCam()){
        if (myCalibration.size() > 0) {
            myCalibration.undistort(ofxCv::toCv(currentFrameImg), ofxCv::toCv(processFrameImg));
            processFrameImg.update();
        }
    }else{
        processFrameImg = currentFrameImg;
        processFrameImg.update();
    }
    
    if(bRecording && !bPlaying){
        
        if(!bEndRecording && currentFrameNumber >= imageSequence.size()){
            bEndRecording = true;
        }
        
        if(!bEndRecording) {
            
            if(bCameraHasNewFrame){
                
                // if in calibration record mode only record some frames
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
void testApp::draw(){

    ofBackground(0,0,0);
    
    if(!bPlaying){
        drawLiveForRecording();
    }else{
        drawPlayback();
        
        // draw mouse cursor for calibration input
        if(bPlaying && bInputMousePoints){
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
    }
	
    if(bShowText){
       drawText();
    }
    
    if(bShowLargeCamImageOnTop){
        ofSetColor(255);
        processFrameImg.draw(0,0,1024,768);
    }
    
    
    //------------- Store last frame
    if(prevLeapFrameRecorder.XML.getNumTags("FRAME") > 5){
        prevLeapFrameRecorder.XML.removeTag("FRAME",0);
    }
    prevLeapFrameRecorder.recordFrameXML(leap);
    
}

//--------------------------------------------------------------
void testApp::drawLiveForRecording(){
    
    // draw live image
    ofSetColor(ofColor::white);
#ifdef _USE_LIBDC_GRABBER
    processFrameImg.draw(drawW,0,drawW,drawH);
#else
    processFrameImg.draw(drawW,0,drawW,drawH);
#endif
            
    
    // draw leap
    drawLeapWorld();
    
}

//--------------------------------------------------------------
void testApp::drawPlayback(){
    
    drawLeapWorld();
    
    // if calibrated and want to see view from projector
    if (leapCameraCalibrator.calibrated && bShowCalibPoints && bUseVirtualProjector){
        ofPushMatrix();
        ofScale(drawW/(float)cameraWidth, drawH/(float)cameraHeight);
        leapCameraCalibrator.drawImagePoints();
        ofPopMatrix();
    }
    
    
    if(bPlaying && !bRecording){
        ofPushStyle();
        ofSetColor(255);
        video.draw(drawW, 0,drawW,drawH);
        
        if(bInputMousePoints){
            indexRecorder.drawPointHistory(video.getCurrentFrameID() );
        }
        ofPopStyle();
    }
}

void testApp::drawLeapWorld(){
    
    // draw video underneath calibration to see alignment
    glDisable(GL_DEPTH_TEST);
    if(leapCameraCalibrator.calibrated && bUseVirtualProjector){
        ofSetColor(255);
        if(bPlaying ){
            video.draw(0, 0, drawW,drawH);
        }else{
            #ifdef _USE_LIBDC_GRABBER
                processFrameImg.draw(0,0,drawW,drawH);
            #else
                processFrameImg.draw(0,0,drawW,drawH);
            #endif
        }
    }
    
    // start fbo
    if (bUseFbo){
        fbo.begin();
        ofClear(0,0,0,0);
    }
    
    // start camera (either calibrated projection or easy cam)
    if (leapCameraCalibrator.calibrated && bUseVirtualProjector){
        // glEnable(GL_DEPTH_TEST); // why is this messing the render up in the projector cam??????
        leapCameraCalibrator.projector.beginAsCamera();
		
	
		
    }else {
        glEnable(GL_DEPTH_TEST);
        cam.begin();
    }
    
    // draw grid
    ofSetColor(255);
    leapVisualizer.drawGrid();
    
    // draw world points
    if(leapCameraCalibrator.calibrated && bShowCalibPoints){
        leapCameraCalibrator.drawWorldPoints();
    }
    
    // draw leap from xml
    if (bPlaying && !bRecording){
        int nFrameTags = leapVisualizer.XML.getNumTags("FRAME");
        if (nFrameTags > 0){
            ofFill();
            playingFrame = video.getCurrentFrameID();
            if(playingFrame > 0 && bShowOffBy1Frame){
                int whichFrame = playingFrame - framesBackToPlay;
                if(whichFrame <0) whichFrame = 0;
                leapVisualizer.drawFrameFromXML(whichFrame);
            }
            else{ leapVisualizer.drawFrameFromXML(playingFrame); }

            if( lastIndexVideoPos.x > 0 && lastIndexVideoPos.y > 0){
                ofSetColor(ofColor::red);
                ofDrawSphere(lastIndexLeapPos, 5.0f);
            }
        }
    }else{
        // draw live leap
        
        if(bShowOffBy1Frame){
            int totalFramesSaved = prevLeapFrameRecorder.XML.getNumTags("FRAME");
            int whichFrame = totalFramesSaved-framesBackToPlay;
            if(whichFrame<0) whichFrame = 0;
            leapVisualizer.drawFrameFromXML(whichFrame,prevLeapFrameRecorder.XML);

        }else{
            leapVisualizer.drawFrame(leap);
        }
    }
    
    // end camera
	if (leapCameraCalibrator.calibrated && bUseVirtualProjector){
        leapCameraCalibrator.projector.endAsCamera();
    }else {
        
        // draw calibration projector for debugging
        if (leapCameraCalibrator.calibrated){
            leapCameraCalibrator.projector.draw();
        }
        cam.end();
        glDisable(GL_DEPTH_TEST);
    }
    
    // end fbo
    if (bUseFbo){
        fbo.end();
    }
    
    
    // if we are calibrated draw fbo with transparency to see overlay better
    if (leapCameraCalibrator.calibrated) ofSetColor(255,255,255,200);
    else ofSetColor(255,255,255,255);
    
    
    // draw the fbo
    ofPushMatrix();
   if(bUseVirtualProjector){
        ofScale(1,-1,1);    // flip fbo
        fbo.draw(0,-drawH,drawW,drawH);
    }else{
        fbo.draw(0,0,drawW,drawH);
   }
    ofPopMatrix();
    

}

//--------------------------------------------------------------
void testApp::drawText(){
	
	float textY = 500;
	
	ofSetColor(ofColor::white);
	//ofDrawBitmapString("Display, record & playback Leap 2.0 Controller data.", 20, textY); textY+=15;
	//ofDrawBitmapString("Built in openFrameworks by Golan Levin, golan@flong.com", 20, textY); textY+=15;
	//textY+=15;

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
	ofDrawBitmapString("Press 'g' to toggle grid", 20, textY); textY+=15;
    ofDrawBitmapString("Press 'v' to toggle virtual projector", 20, textY); textY+=15;
    ofDrawBitmapString("Press 'w' to toggle calibration points draw", 20, textY); textY+=15;
    ofDrawBitmapString("Press 'C' to load current playback folder's calibration", 20, textY); textY+=15;
    ofDrawBitmapString("Press 'm' to allow mouse input points", 20, textY); textY+=15;
    ofDrawBitmapString("Press 'left/right' to advance frame by frame", 20, textY); textY+=15;
    ofDrawBitmapString("Press '' (space) to pause/play", 20, textY); textY+=15;
    
    string usePrev = bShowOffBy1Frame ? "on" : "off";
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
	if (leapVisualizer.XML.getNumTags("FRAME") > 0){
		ofDrawBitmapString("Press 'p' to pause PLAYBACK",  textX, textY); textY+=15;
	}
	
	if (bPlaying){
		ofSetColor(ofColor::green);
		ofDrawBitmapString("PLAYING! " + ofToString(playingFrame), textX, textY); textY+=15;
	} else if (bRecording){
		ofSetColor(ofColor::red);
		ofDrawBitmapString("RECORDING! " + ofToString(leapRecorder.recordingFrameCount), textX, textY); textY+=15;
	}
    
    textY+=15;
    ofSetColor(ofColor::white);
    ofDrawBitmapString("Playback folder: "+folderName,  textX, textY); textY+=15;
    ofDrawBitmapString("Calibration file: "+leapCameraCalibrator.dirNameLoaded,  textX, textY); textY+=15;

	
}

//--------------------------------------------------------------
void testApp::loadPlaybackFromDialogForCalibration(){
    
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
void testApp::loadPlaybackFromDialog(){
    
    //Open the Open File Dialog
    ofFileDialogResult openFileResult= ofSystemLoadDialog("Choose a recording folder:",true);
    
    if (openFileResult.bSuccess){
        
        string filePath = openFileResult.getName();
        folderName = filePath;
        loadAndPlayRecording(filePath);
        
    }
}

//--------------------------------------------------------------
void testApp::loadCalibrationFromDialog(){
    //Open the Open File Dialog
    ofFileDialogResult openFileResult= ofSystemLoadDialog("Choose a recording folder:",true);
    
    if (openFileResult.bSuccess){
        string filePath = openFileResult.getName();
        calibrateFromXML(filePath);
    }
}

//--------------------------------------------------------------
void testApp::finishRecording(){
    
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
void testApp::loadAndPlayRecording(string folder){
    
    leapVisualizer.loadXmlFile("recordings/"+folder+"/leap/leap.xml");
    video.load("recordings/"+folder+"/camera");
    
    indexRecorder.setup("recordings/"+folder+"/calib","fingerCalibPts.xml");
    indexRecorder.setDrawOffsets(drawW,0,cameraWidth/drawW,cameraHeight/drawH);
    
    // open calibration if exists?
    
    bPlaying = true;
    playing = true;
    currentFrameNumber = 0;
    
}

//--------------------------------------------------------------
void testApp::calibrateFromXML( string calibFolderName ){
    
    leapCameraCalibrator.setup(cameraWidth, cameraHeight);
    leapCameraCalibrator.loadFingerTipPoints("recordings/"+calibFolderName+"/calib/fingerCalibPts.xml");
    
    if(useCorrectedCam()){
        leapCameraCalibrator.correctCameraPNP(myCalibration);
    }else{
        leapCameraCalibrator.correctCamera();

    }
}

bool testApp::useCorrectedCam(){

#ifdef _USE_CORRECTED_CAMERA
    if(bUseCorrectedCamera) return true;
    else return false;
#else
    return false;
#endif

}

//--------------------------------------------------------------
void testApp::keyPressed(int key){

	if ((key == 'r') || (key == 'R')){
		if (leap.isConnected()){
			// Toggle Recording.
			//reset so we don't store extra tags
			if(bRecording){
                bEndRecording = true;
            }else{
                bRecording = !bRecording;
                folderName = ofGetTimestampString();
				leapRecorder.startRecording();
                leapVisualizer.XML.clear();
                bPlaying = false;
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
        case 'c':
            cam.reset();
            break;
        case 'C':
            calibrateFromXML(folderName);
            break;
        case 'F':
            bUseFbo = !bUseFbo;
            break;
        case 'g':
            leapVisualizer.bDrawGrid = !leapVisualizer.bDrawGrid;
            break;
        case 'l':
            if(bPlaying) bPlaying = false;
            break;
        case 'm':
            bInputMousePoints = !bInputMousePoints;
            break;
        case 'p':
            if(bPlaying) playing = !playing;
            break;
        case 's':
            leapVisualizer.bDrawSimple = !leapVisualizer.bDrawSimple;
            break;
        case 'v':
            bUseVirtualProjector = !bUseVirtualProjector;
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
            if(bPlaying) playing = !playing;
            else if(bRecordingForCalibration) bRecordThisCalibFrame = true;
            break;
        case 'u':
            bUseCorrectedCamera = !bUseCorrectedCamera;
            break;
        case '0':
            bShowLargeCamImageOnTop = !bShowLargeCamImageOnTop;
            break;
        case 'f':
            ofToggleFullscreen();
            break;
        case '9':
            if(drawW == 640){
                drawW = 1024;
                drawH = 768;
                bShowText = false;
            }else{
                drawW = 640;
                drawH = 480;
                bShowText = true;
            }
            break;
        case 'o':
            bShowOffBy1Frame= !bShowOffBy1Frame;
            break;
        case '{':
            framesBackToPlay--;
            if(framesBackToPlay <0) framesBackToPlay = 5;
        case '}':
            framesBackToPlay++;
            if(framesBackToPlay > 5) framesBackToPlay = 1;
            break;
    }
    
}

//--------------------------------------------------------------
void testApp::keyReleased(int key){
}

//--------------------------------------------------------------
void testApp::mouseMoved(int x, int y ){
    
    
    
}

//--------------------------------------------------------------
void testApp::mouseDragged(int x, int y, int button){
}

//--------------------------------------------------------------
void testApp::mousePressed(int x, int y, int button){
    
    if(bPlaying && bInputMousePoints){
        if(x > indexRecorder.xOffset && y > indexRecorder.yOffset &&
           x < indexRecorder.xOffset+drawW && y < drawH){
            indexRecorder.recordPosition(x, y-20, leapVisualizer.getIndexFingertipFromXML(video.getCurrentFrameID()),video.getCurrentFrameID());
            lastIndexVideoPos.set(x,y-20);
            lastIndexLeapPos = leapVisualizer.getIndexFingertipFromXML(video.getCurrentFrameID());
        }
    }
}

//--------------------------------------------------------------
void testApp::mouseReleased(int x, int y, int button){
}

//--------------------------------------------------------------
void testApp::windowResized(int w, int h){
}

//--------------------------------------------------------------
void testApp::gotMessage(ofMessage msg){
}

//--------------------------------------------------------------
void testApp::dragEvent(ofDragInfo dragInfo){
}

//--------------------------------------------------------------
void testApp::exit(){
    // let's close down Leap and kill the controller
    leap.close();
}



/// makeTimeStampFbo();
//fboTimeStamp.readToPixels(processFrameImg.getPixelsRef());

/*
void testApp::makeTimeStampFbo(){
 
    fboTimeStamp.begin();
    ofClear(0,255);
    ofSetColor(255);
    processFrameImg.draw(0,0);
    
    int cameraNowTime = cameraLibdc.getTimestamp()-cameraRecordTimestampStart;
    cout << cameraNowTime<< " camera time" << endl << endl;;
    ofDrawBitmapString( ofToString(cameraNowTime), 10, cameraHeight-30);
    
    fboTimeStamp.end();
}

*/
