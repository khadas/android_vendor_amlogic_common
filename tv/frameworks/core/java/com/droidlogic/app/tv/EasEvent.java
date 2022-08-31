/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: JAVA file
 */

package com.droidlogic.app.tv;

import android.util.Log;
import vendor.amlogic.hardware.tvserver.V1_0.TvHidlParcel;


public class EasEvent {
    private static final String TAG = "EasEvent";
    public int    tableId;                       //table id
    public int    extension;                     //subtable id
    public int    version;                       //version_number
    public int    currentNext;                   //current_next_indicator
    public int    sequenceNum;                   //sequence version
    public int    protocolVersion;               //protocol version
    public int    easEventId;                    //eas event id
    public int[]  easOrigCode;                   //eas event orig code
    public int    easEventCodeLen;               //eas event code len
    public int[]  easEventCode;                  //eas event code
    public int    alertMessageTimeRemaining;     //alert msg remain time
    public int    eventStartTime;                //eas event start time
    public int    eventDuration;                 //event dur
    public int    alertPriority;                 //alert priority
    public int    detailsOOBSourceID;            //details oob source id
    public int    detailsMajorChannelNumber;     //details major channel num
    public int    detailsMinorChannelNumber;     //details minor channel num
    public int    audioOOBSourceID;              //audio oob source id
    public int    locationCount;                 //location count
    public Location[] location;                  //location info
    public int    exceptionCount;                //exception count
    public ExceptionList[]   exceptionList;      //exception info
    public int    multiTextCount;                //multi_text count
    public MultiStr[]   multiText;               //nature and alert multi str information structure.
    public int    descriptorTextCount;           //descriptor text count.
    public Descriptor[]   descriptor;            //descriptor structure.

    public class Location {
        public int  stateCode;
        public int  countySubdiv;
        public int  countyCode;
    }
    public class ExceptionList {
        public int  inBandRefer;
        public int  exceptionMajorChannelNumber; //the exception major channel num
        public int  exceptionMinorChannelNumber; //the exception minor channel num
        public int  exceptionOOBSourceID;        //the exception oob source id
    }
    public class MultiStr {
        public int[]   lang;                     //the language of mlti str
        public int   type;                       //the str type, alert or nature
        public int   compressionType;            //compression type
        public int   mode;                       //mode
        public int   numberBytes;                //number bytes
        public int[]   compressedStr;            //the compressed str
    }
    public class Descriptor {
        public int  tag;                         //descriptor_tag
        public int  length;                      //descriptor_length
        public int[]  data;                      //content
    }

    public void printEasEventInfo(){
        Log.i(TAG,"[EasEventInfo]"+
            "\n alertMessageTimeRemaining = "+alertMessageTimeRemaining+
            "\n alertPriority = "+alertPriority+
            "\n detailsMajorChannelNumber = "+detailsMajorChannelNumber+
            "\n detailsMinorChannelNumber = "+detailsMinorChannelNumber);
    }

    public void readEasEvent(TvHidlParcel p) {
        Log.i(TAG,"readEasEvent");
        int i, j, k, index = 0;
        index++;//skip sectionCount

        tableId = p.bodyInt.get(index++);
        extension = p.bodyInt.get(index++);
        version = p.bodyInt.get(index++);
        currentNext = p.bodyInt.get(index++);
        sequenceNum = p.bodyInt.get(index++);
        protocolVersion = p.bodyInt.get(index++);
        easEventId = p.bodyInt.get(index++);
        easOrigCode = new int[3];
        for (j=0;j<3;j++) {
            easOrigCode[j] = p.bodyInt.get(index++);
        }
        easEventCodeLen = p.bodyInt.get(index++);
        if (easEventCodeLen != 0) {
            easEventCode = new int[easEventCodeLen];
            for (j=0;j<easEventCodeLen;j++)
                easEventCode[j] = p.bodyInt.get(index++);
        }
        alertMessageTimeRemaining = p.bodyInt.get(index++);
        eventStartTime = p.bodyInt.get(index++);
        eventDuration = p.bodyInt.get(index++);
        alertPriority = p.bodyInt.get(index++);
        detailsOOBSourceID = p.bodyInt.get(index++);
        detailsMajorChannelNumber = p.bodyInt.get(index++);
        detailsMinorChannelNumber = p.bodyInt.get(index++);
        audioOOBSourceID = p.bodyInt.get(index++);
        locationCount = p.bodyInt.get(index++);
        if (locationCount != 0) {
            location = new Location[locationCount];
            for (j=0;j<locationCount;j++) {
                location[j] = new Location();
                location[j].stateCode = p.bodyInt.get(index++);
                location[j].countySubdiv = p.bodyInt.get(index++);
                location[j].countyCode = p.bodyInt.get(index++);
            }
        }
        exceptionCount = p.bodyInt.get(index++);
        if (exceptionCount != 0) {
            exceptionList = new ExceptionList[exceptionCount];
            for (j=0;j<exceptionCount;j++) {
                exceptionList[j] = new ExceptionList();
                exceptionList[j].inBandRefer = p.bodyInt.get(index++);
                exceptionList[j].exceptionMajorChannelNumber = p.bodyInt.get(index++);
                exceptionList[j].exceptionMinorChannelNumber = p.bodyInt.get(index++);
                exceptionList[j].exceptionOOBSourceID = p.bodyInt.get(index++);
            }
        }
        multiTextCount = p.bodyInt.get(index++);
        if (multiTextCount != 0) {
            multiText = new MultiStr[multiTextCount];
            for (j=0;j<multiTextCount;j++) {
                multiText[j] = new MultiStr();
                multiText[j].lang = new int[3];
                multiText[j].lang[0] = p.bodyInt.get(index++);
                multiText[j].lang[1] = p.bodyInt.get(index++);
                multiText[j].lang[2] = p.bodyInt.get(index++);
                multiText[j].type = p.bodyInt.get(index++);
                multiText[j].compressionType = p.bodyInt.get(index++);
                multiText[j].mode = p.bodyInt.get(index++);
                multiText[j].numberBytes = p.bodyInt.get(index++);
                multiText[j].compressedStr = new int[multiText[j].numberBytes];
                for (k=0;k<multiText[j].numberBytes;k++) {
                    multiText[j].compressedStr[k] = p.bodyInt.get(index++);
                }
            }
        }
        descriptorTextCount = p.bodyInt.get(index++);
        if (descriptorTextCount != 0) {
            descriptor = new Descriptor[descriptorTextCount];
            for (j=0;j<descriptorTextCount;j++) {
                descriptor[j] = new Descriptor();
                descriptor[j].tag = p.bodyInt.get(index++);
                descriptor[j].length = p.bodyInt.get(index++);
                descriptor[j].data = new int[descriptor[j].length];
                for (k=0;k<descriptor[j].length;k++) {
                    descriptor[j].data[k] = p.bodyInt.get(index++);
                }
            }
        }
        Log.d(TAG, "TV event EAS index = "+ index);
    }
 }

