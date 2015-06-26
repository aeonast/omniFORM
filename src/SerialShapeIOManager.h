//
//  SerialShapeIOManager.h
//  TMG Shape Displays
//
//  Created by Daniel Leithinger on 3/3/10.
//  Copyright 2010 MIT Media Lab. All rights reserved.
//

#pragma once

#include "ofMain.h"
#include "constants.h"
#include "ShapeIOManager.h"
#include "SerialShapeIO.h"


class SerialPinBoard {
public:
    unsigned char pinCoordinates[NUM_PINS_ARDUINO][2]; // what physical x and y does each pin on the board map to?
    unsigned char heights[NUM_PINS_ARDUINO]; // what height should each pin have?
    bool invertHeight; // is it mounted upside down? if so, the height is inverted
    int serialConnection; // what serial connection is it on?
};


class SerialShapeIOManager : public ShapeIOManager {
public:
    SerialShapeIOManager();
    ~SerialShapeIOManager();
    
    // send and receive height values
    void sendHeightsToShapeDisplay(unsigned char heights[SHAPE_DISPLAY_SIZE_X][SHAPE_DISPLAY_SIZE_Y]);
    void getHeightsFromShapeDisplay(unsigned char heights[SHAPE_DISPLAY_SIZE_X][SHAPE_DISPLAY_SIZE_Y]);
    void clearShapeDisplayHeights(int value=0);

    // per-pin setters for display config values
    void setGainP(float value, ofVec2f pin) {throw "not implemented";};
    void setGainI(float value, ofVec2f pin) {throw "not implemented";};
    void setMaxI(int value, ofVec2f pin) {throw "not implemented";};
    void setDeadZone(int value, ofVec2f pin) {throw "not implemented";};
    void setMaxSpeed(int value, ofVec2f pin) {throw "not implemented";};
    
    // global setters for display config values
    void setGlobalGainP(float value);
    void setGlobalGainI(float value);
    void setGlobalMaxI(int value);
    void setGlobalDeadZone(int value);
    void setGlobalMaxSpeed(int value);

    // should pins that appear stuck be turned off at regular intervals?
    bool enableStuckPinSafetyToggle = false;

    // the name of this shape display
    string shapeDisplayName = "Serial Shape Display";

protected:
    // manage the connection to the shape display
    void connectToDisplay();
    void disconnectFromDisplay(bool clearHeights=false);
    virtual void openSerialConnections(); // implement in child classes
    void closeSerialConnections();

    // setup hardware-specific board configuration
    virtual void configureBoards() = 0;
    void printBoardConfiguration();

    // pin height data processors
    void toggleStuckPins();
    void clipAllHeightValuesToBeWithinRange();
    void readyHeightDataForArduino();

    // central update manager
    void update();

    // send data to the shape display
    void sendValueToAllBoards(unsigned char termId, unsigned char value);
    void sendValuesToBoard(unsigned char termId, unsigned char boardId, unsigned char value[NUM_PINS_ARDUINO], int serialConnection);
    void sendHeightsToBoard(unsigned char boardId, unsigned char value[NUM_PINS_ARDUINO], int serialConnection);
    void sendHeightsToBoardAndRequestFeedback(unsigned char boardId, unsigned char value[NUM_PINS_ARDUINO], int serialConnection);
    void sendAllConfigValues();

    // read data received from shape display
    void readHeightsFromBoards();

    // serial communications objects
    SerialShapeIO *serialConnections[NUM_SERIAL_CONNECTIONS];
    SerialPinBoard pinBoards[NUM_ARDUINOS];

    // shape display height values (both intended and actual values)
    unsigned char heightsForShapeDisplay[SHAPE_DISPLAY_SIZE_X][SHAPE_DISPLAY_SIZE_Y];
    unsigned char heightsFromShapeDisplay[SHAPE_DISPLAY_SIZE_X][SHAPE_DISPLAY_SIZE_Y];

    // pin behavior configuration
    float gainP;
    float gainI;
    int maxI;
    int deadZone;
    int maxSpeed;

    // initialization flags
    bool boardsAreConfigured = false;
    bool isConnected = false;

    // pin behavior configuration trackers
    bool configsHaveChanged;
    unsigned long long timeOfLastConfigsRefresh;

    // properties for detecting stuck pins to toggle
    int pinDiscrepancy[SHAPE_DISPLAY_SIZE_X][SHAPE_DISPLAY_SIZE_Y];
    bool pinEnabled[SHAPE_DISPLAY_SIZE_X][SHAPE_DISPLAY_SIZE_Y];
    unsigned long long pinStuckSinceTime[SHAPE_DISPLAY_SIZE_X][SHAPE_DISPLAY_SIZE_Y];
    const int pinDiscrepancyToggleThreshold = 100;
    const int millisUntilPinToggledOff = 1000;
    const int millisUntilPinToggledOn = 3000;
};