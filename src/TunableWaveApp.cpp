//
//  TunableWaveApp.cpp
//  TMG Shape Displays
//
//  Created by Daniel Windham on 6/26/15.
//  Copyright 2015 MIT Media Lab. All rights reserved.
//

#include "TunableWaveApp.h"


TunableWaveApp::TunableWaveApp(KinectManager *manager) : Application(manager) {
    setTuningMethod(KEY_PRESS_TUNING, true);
};

void TunableWaveApp::setTuningMethod(TuningMethod method, bool force) {
    if (tuningMethod == method && !force) {
        return;
    }

    tuningMethod = method;

    if (tuningMethod == KEY_PRESS_TUNING) {
        frequency = 0.5;
        numCrests = 4;
    }
}

void TunableWaveApp::update(float dt) {
    if (tuningMethod == KINECT_LOCATION_TUNING) {
        updateWaveParametersWithKinect();
    }

    normalizedPhase += dt * frequency;
    updateHeights();
};

void TunableWaveApp::updateHeights() {
    float phase = 2 * pi * normalizedPhase;

    // sinc math function sin(x+k)/x where:
    //   x is normalized distance from display center plus an offset
    //   k is a time-varying phase
    //   heights are arranged to produce values within the valid output range
    //   the math has been arranged to easily tune how many wave crests you get
    ofPoint center(SHAPE_DISPLAY_SIZE_X / 2, SHAPE_DISPLAY_SIZE_Y / 2);
    float maxDistance = center.distance(ofPoint(0, 0));
    float distanceScalar = pi * (numCrests * 2 - 1) / maxDistance;
    float distanceOffset = 2 * pi;
    float maxAmplitude = 0.15;
    float heightScalar = HEIGHT_RANGE / (2 * maxAmplitude);
    int heightOffset = HEIGHT_RANGE / 2 + HEIGHT_MIN;
    for (int x = 0; x < SHAPE_DISPLAY_SIZE_X; x++) {
        for (int y = 0; y < SHAPE_DISPLAY_SIZE_Y; y++) {
            float distance = center.distance(ofPoint(x, y));
            distance = distanceScalar * distance + distanceOffset;
            float height = sin(distance - phase) / distance;
            int xy = heightsForShapeDisplay.getPixelIndex(x, y);
            heightsForShapeDisplay[xy] = heightScalar * height + heightOffset;
        }
    }
};

void TunableWaveApp::updateWaveParametersWithKinect() {
    // Get Pixels from kinect
    ofPixels depthPixels;
    kinectManager->getDepthPixels(depthPixels);
    int tableMaskLine = KINECT_Y - 200;
    int closestY = -1;
    int closestX = KINECT_X / 2;

//     Mask out all pixels before y Baseline
//    for (int y = 0; y < tableMaskLine; y++) {
//        for (int x = 0; x < KINECT_X; x++) {
//            depthPixels[x + (y * KINECT_X)] = 0;
//        }
//    }
    
    // Get cloest Y
    for (int y = tableMaskLine; y >= 0; y--) {
        for (int x = 0; x < KINECT_X; x++) {
            if (depthPixels[x + (y * KINECT_X)] > 0){
                closestY = y;
                closestX = x;
                break;
            }
        }
        if (closestY > -1) {
            break;
        }
    }

    float dropoffRate = 4;
    float minFreq = 0.5;
    float maxFreq = 5.5;
    
    // Compare pixel distance from Y baseline (table), set freqeuncy
    if (closestY > -1){
        float normalized = 1.0 * (tableMaskLine - closestY) / tableMaskLine;
        float k = pow(2.8f, -dropoffRate + 2 * dropoffRate * normalized);
        frequency = minFreq + (maxFreq - minFreq) * k / (1 + k);
    } else {
        frequency = maxFreq;
    }
    
    // Get the closest pixel and use this as the control
    numCrests = 10.0 * closestX / KINECT_X;
};

void TunableWaveApp::drawGraphicsForShapeDisplay() {
    color.setHsb(fmod(normalizedPhase * 180, 180), 255, 255);
    ofSetColor(color);
    ofImage(heightsForShapeDisplay).draw(0, 0, 300, 300);
};

string TunableWaveApp::appInstructionsText() {
    string instructions = (string) "" +
        "Waves propagate from the display center. Choose between manual and\n" +
        "kinect-driven parameter control:\n" +
        "\n" +
        (tuningMethod == KEY_PRESS_TUNING ? "* " : "  ") + "'q' : manual parameter control\n" +
        (tuningMethod == KINECT_LOCATION_TUNING ? "* " : "  ") + "'w' : kinect parameter control\n" +
        "";

    if (tuningMethod == KEY_PRESS_TUNING) {
        instructions += (string) "" +
            "\n" +
            "You can adjust the waves' frequency and wavelength using the\n" +
            "'a', 's', 'd', and 'f' keys.\n" +
            "\n" +
            "frequency: " + ofToString(frequency, 2) + "\n" +
            "number of waves: " + ofToString(numCrests, 1) + "\n" +
            "";
    }

    return instructions;
};

void TunableWaveApp::keyPressed(int key) {
    if (key == 'q') {
        setTuningMethod(KEY_PRESS_TUNING);
    } else if (key == 'w') {
        setTuningMethod(KINECT_LOCATION_TUNING);
    } else if (key == 'a') {
        if (tuningMethod == KEY_PRESS_TUNING) {
            frequency /= 1.26;
        }
    } else if (key == 's') {
        if (tuningMethod == KEY_PRESS_TUNING) {
            frequency *= 1.26;
        }
    } else if (key == 'd') {
        if (tuningMethod == KEY_PRESS_TUNING) {
            numCrests -= 0.5;
        }
    } else if (key == 'f') {
        if (tuningMethod == KEY_PRESS_TUNING) {
            numCrests += 0.5;
        }
    }
};