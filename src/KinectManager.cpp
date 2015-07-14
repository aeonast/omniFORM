//
//  KinectManager.cpp
//  TMG Shape Displays
//
//  Created by Daniel Leithinger on 2/16/14.
//  Copyright 2014 MIT Media Lab. All rights reserved.
//

#include "KinectManager.h"


KinectManager::KinectManager(int nearThreshold, int farThreshold, int contourMinimumSize) {
    if (kinect.numAvailableDevices() > 0) {
        kinect.setRegistration(true); // enable depth->video image calibration
        kinect.init();
        kinect.open();
    }
    
    // print the intrinsic IR sensor values
    if (kinect.isConnected()) {
        ofLogNotice() << "sensor-emitter dist: " << kinect.getSensorEmitterDistance() << "cm";
        ofLogNotice() << "sensor-camera dist:  " << kinect.getSensorCameraDistance() << "cm";
        ofLogNotice() << "zero plane pixel size: " << kinect.getZeroPlanePixelSize() << "mm";
        ofLogNotice() << "zero plane dist: " << kinect.getZeroPlaneDistance() << "mm";
    }
    
    colorImg.allocate(kinect.width, kinect.height);
    depthImg.allocate(kinect.width, kinect.height);
    grayThreshNear.allocate(kinect.width, kinect.height);
    grayThreshFar.allocate(kinect.width, kinect.height);
    depthThreshed.allocate(kinect.width, kinect.height);
    lastDepthThreshed.allocate(kinect.width, kinect.height);
    depthThreshedDiff.allocate(kinect.width, kinect.height);

    nearThreshold = nearThreshold;
    farThreshold = farThreshold;
    contourMinimumSize = contourMinimumSize;

    loadAlphaMaskAndPrepForCvProcessing();
}

KinectManager::~KinectManager() {
    kinect.setCameraTiltAngle(0); // zero the tilt on exit
    kinect.close();
}

void KinectManager::update() {
    kinect.update();
    
    // there is a new frame and we are connected
    if (kinect.isFrameNew()) {
        colorImg.setFromPixels(kinect.getPixels(), kinect.width, kinect.height);
        depthImg.setFromPixels(kinect.getDepthPixels(), kinect.width, kinect.height);

        lastDepthThreshed.setFromPixels(depthThreshed.getPixels(), kinect.width, kinect.height);
        // always update the depth image
        depthThreshed.setFromPixels(depthImg.getPixels(), kinect.width, kinect.height);

        // subtract mask which is png alpha image called "mask.png"
        if (useMask) {
            subtractMask();
        }
        
        // threshold calcutations convery depth map into black and white images
        calculateThresholdsAndModifyImages();
        
        // find contours which are between the size of 20 pixels and 1/3 the w * h pixels.
        // if find holes is set to true, we will get interior contours as well.
        contourFinder.findContours(depthImg, contourMinimumSize, (kinect.width * kinect.height) / 2, 10, false);
        
        depthThreshedDiff.absDiff(lastDepthThreshed, depthThreshed);
    }
}

void KinectManager::flagImagesAsChanged() {
    colorImg.flagImageChanged();
    depthImg.flagImageChanged();
    depthThreshed.flagImageChanged();
}

void KinectManager::subtractMask() {
    cvAnd(depthImg.getCvImage(), maskCv.getCvImage(), depthImg.getCvImage(), NULL);
    //cvAnd(grayThreshNear.getCvImage(), grayThreshFar.getCvImage(), depthImg.getCvImage(), NULL);
}

// loads png mask and converts to cv grayscale which we need to cvAnd method
void KinectManager::loadAlphaMaskAndPrepForCvProcessing() {
    // type is OF_IMAGE_COLOR_ALPHA
    mask.loadImage("mask.png");
    
    // simple way to convert to differe image type,
    // changing the transparent areas to white
    ofImage image;
    image.setFromPixels(mask.getPixelsRef());
    image.setImageType(OF_IMAGE_COLOR);
    maskColorCv.setFromPixels(image.getPixels(), mask.getWidth(), mask.getHeight());
    maskCv = maskColorCv;
}

void KinectManager::calculateThresholdsAndModifyImages() {
    depthImg.erode_3x3();
    depthImg.dilate_3x3();
    
    // we do two thresholds - one for the far plane and one for the near plane
    // we then do a cvAnd to get the pixels which are a union of the two thresholds
    grayThreshNear = depthImg;
    grayThreshFar = depthImg;
    grayThreshNear.threshold(nearThreshold, true);
    grayThreshFar.threshold(farThreshold);
    cvAnd(grayThreshNear.getCvImage(), grayThreshFar.getCvImage(), depthImg.getCvImage(), NULL);
    
    // find depth map excluding thresholded data
    // this causes the 10 finger effect and could be related to our discussion
    // today about dynamic thresholding
    //
    // if we threshold with the near value, and the user moves the hand just past the near point
    // and thus out of range
    // their hand will be black (since black is used for out of range areas)
    // however since their hands shadow is also black this will cause the 10 finger effect.
    //
    //cvAnd(grayThreshNear.getCvImage(), depthThreshed.getCvImage(), depthThreshed.getCvImage(), NULL);
    cvAnd(grayThreshFar.getCvImage(), depthThreshed.getCvImage(), depthThreshed.getCvImage(), NULL);
    
    //    ofPixelsRef depthPixels = depthThreshed.getPixelsRef();
    //    for (int x = 0; x < depthPixels.getWidth(); x++) {
    //        for (int y = 0; y < depthPixels.getHeight(); y++) {
    //            depthPixels.setColor((depthPixels.getColor(x,y).getBrightness() + mFarThreshold) * 255.f / (mNearThreshold - mFarThreshold));
    //        }
    //    }
}


//--------------------------------------------------------------
//
// Draw various images
//
//--------------------------------------------------------------

// color image
void KinectManager::drawColorImage(int x, int y, int width, int height) {
    ofSetColor(255);
    colorImg.draw(x, y, width, height);
}

// gray image with contour blobs drawn on top
void KinectManager::drawThresholdImage(int x, int y, int width, int height){
    ofSetColor(255);
    depthImg.draw(x, y, width, height);
    contourFinder.draw(x, y, width, height);
}

// draw from the live kinect
void KinectManager::drawDepthImage(int x, int y, int width, int height){
    ofSetColor(255);
    depthImg.draw(x, y, width, height);
}

// black and white image from within threshold range
void KinectManager::drawDepthThreshedImage(int x, int y, int width, int height) {
    ofSetColor(255);
    depthThreshed.draw(x, y, width, height);
}

// black and white difference image from within threshold range
// (only data from movements)
void KinectManager::drawDepthThreshedDiff(int x, int y, int width, int height) {
    ofSetColor(255);
    depthThreshedDiff.draw(x,y, width, height);
}


//--------------------------------------------------------------
//
// Get various kinect images as pixels
//
//--------------------------------------------------------------

// standard color image from kinect video camera
unsigned char *KinectManager::getColorPixels() {
    return colorImg.getPixels();
}

// kinect depth map
// grayscale image where white = near, black = far
unsigned char *KinectManager::getDepthPixels() {
    return kinect.getDepthPixels();
}

// kinect depth map WHERE
// areas outside near and far threshold range are black.
// This is currently used by the wave animation
unsigned char *KinectManager::depthThreshedPixels() {
    return depthThreshed.getPixels();
}

// kinect depth map WHERE
// areas outside near and far threshold range are black.
// This is currently used by the wave animation
ofPixels KinectManager::depthThresholdOFPixels() {
    return depthThreshed.getPixelsRef();
}

// kinect depth map WHERE
// only black OR white (0 OR 255) is used.
// calculated using using pContourMinimumSize
unsigned char *KinectManager::grayImagePixels() {
    return depthImg.getPixels();
}


// get a reference to the contour finder
ofxCvContourFinder *KinectManager::getContourFinder() {
    return &contourFinder;
}

//--------------------------------------------------------------
//
// Kinect pass through getters
//
// Since we wrap the kinect class we no longer have access to
// its methods unless we add them to KinectManager too.
//
//--------------------------------------------------------------

int KinectManager::numAvailableDevices(){
    return kinect.numAvailableDevices();
}

bool KinectManager::isFrameNew() {
    return kinect.isFrameNew();
}

bool KinectManager::isConnected() {
    return kinect.isConnected();
}